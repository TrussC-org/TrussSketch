#pragma once
// =============================================================================
// tcLuaScriptHost — Lua (tcxLua / Sol2) backend for the TrussSketch playground.
//
// PoC counterpart of the AngelScript `tcScriptHost`. It keeps the SAME contract
// the browser/editor already speaks to:
//   clearScriptFiles() -> addScriptFile(name, code)* -> buildScriptFiles() -> ""|error
//   then per-frame callUpdate()/callDraw() + event callbacks.
//
// Differences from the AngelScript host (by design):
//  * A FRESH sol::state is built on every buildScriptFiles(). Recreating the
//    state drops all user globals and GC-collects every Lua-owned userdata
//    (Texture/Mesh/EasyCam/...), so we don't need the manual resource
//    containers the AngelScript host kept.
//  * Errors are reformatted to "name (line, col) : message" so the editor's
//    existing parseErrorLocation() can place markers (Lua emits "name:line: msg").
//
// NOTE: coverage of the exposed API == whatever tcxLua currently binds
// (generated free functions + hand-written usertypes). That's intentionally
// not the concern of this PoC.
// =============================================================================

#include <tcxLua.h>   // brings in sol/sol.hpp and TrussC.h
#include <tcxLuaLint.h>

class tcLuaScriptHost {
public:
    tcLuaScriptHost() { resetState(); }

    // ---- single-file (parity with updateScriptCode) ----
    bool loadScript(const std::string& code) {
        clearScriptFiles();
        addScriptFile("main.tcs", code);
        return buildScriptFiles();
    }

    // ---- multi-file ----
    void clearScriptFiles() { files_.clear(); }
    void addScriptFile(const std::string& name, const std::string& code) {
        files_.emplace_back(name, code);
    }

    // Rebuild from scratch — into a CANDIDATE state that only replaces the
    // live one on full success. On any failure (lint, syntax, top-level
    // runtime error) the previous sketch keeps running untouched, which is
    // what makes hot reload safe. getLastError() holds editor-parseable
    // messages (one per line) and ready_ keeps its previous value.
    bool buildScriptFiles() {
        lastError_.clear();
        runtimeError_.clear();

        std::shared_ptr<sol::state> cand = tcx_.getLuaState();

        // Load the cooperative-task prelude (spawn/wait/forever/__tcTick) BEFORE
        // the lint runs, so these names count as defined globals and user chunks
        // that call them are not reported as undefined-global reads.
        if (!loadPrelude(*cand)) return false;  // internal error; lastError_ set

        // Static lint: reading a global that is never assigned and not bound
        // is a build error (it would only nil-crash mid-frame otherwise).
        auto issues = tcxLuaLintUndefinedGlobals(cand->lua_state(), files_);
        if (!issues.empty()) {
            for (const auto& i : issues) {
                if (!lastError_.empty()) lastError_ += "\n";
                lastError_ += i.chunk + " (" + std::to_string(i.line) +
                              ", 1) : undefined variable '" + i.name + "'";
                // did-you-mean hint (editor translates the English wording)
                if (!i.suggestion.empty())
                    lastError_ += " (did you mean '" + i.suggestion + "'?)";
            }
            return false;
        }

        for (const auto& f : files_) {
            if (!runChunk(*cand, f.first, f.second)) return false; // lastError_ set
        }
        // Commit — but DO NOT destroy the old state here. On the web build
        // (ASYNCIFY) this call arrives via ccall while the engine may be
        // suspended INSIDE the old state's draw(); lua_close on a state that
        // is mid-pcall is undefined behavior and corrupts the heap (observed
        // as geometry silently disappearing for the rest of the session).
        // Park it instead; the app destroys it at the next frame boundary.
        retired_.push_back(std::move(lua_));
        lua_ = std::move(cand);
        ready_ = true;
        return true;
    }

    // Destroy retired states. Call from the app at a point where no call
    // into them can still be in flight (top of update(), i.e. the previous
    // frame — including a suspended draw — has fully completed).
    void collectRetired() { retired_.clear(); }

    const std::string& getLastError() const { return lastError_; }
    // Last error raised inside a per-frame callback (draw/update/events).
    // Separate from getLastError() so build results stay unambiguous.
    const std::string& getRuntimeError() const { return runtimeError_; }
    bool isReady() const { return ready_; }

    // ---- lifecycle / event dispatch (no-op if the Lua fn isn't defined) ----
    void callSetup()                              { callFn("setup"); }
    void callUpdate()                             { callFn("update"); }
    void callDraw()                               { callFn("draw"); }
    void callMousePressed(float x, float y, int b)  { callFn("mousePressed", x, y, b); }
    void callMouseReleased(float x, float y, int b) { callFn("mouseReleased", x, y, b); }
    void callMouseMoved(float x, float y)           { callFn("mouseMoved", x, y); }
    void callMouseDragged(float x, float y, int b)  { callFn("mouseDragged", x, y, b); }
    void callKeyPressed(int key)                    { callFn("keyPressed", key); }
    void callKeyReleased(int key)                   { callFn("keyReleased", key); }
    void callWindowResized(int w, int h)            { callFn("windowResized", w, h); }

    // Advance cooperative tasks (spawn/wait/forever) one frame. `now` is the
    // elapsed time in seconds; a task's error surfaces via getRuntimeError().
    void callTick(double now)                       { callFn("__tcTick", now); }

private:
    // Scratch-inspired cooperative task API, injected into every fresh state.
    //   spawn(fn)    — start a coroutine task; runs until fn returns.
    //   wait(sec)    — inside a task, suspend for sec seconds (wait()/wait(0) =
    //                  resume next frame); negatives clamp to 0.
    //   forever(fn)  — task that calls fn() forever, with an implicit wait(0)
    //                  after each call so a non-yielding fn cannot freeze the page.
    //   __tcTick(now)— resumes due tasks in spawn order (called by the host).
    static constexpr const char* kTaskPrelude = R"LUA(
-- Default no-op lifecycle callbacks. User chunks overwrite the ones they
-- define. This guarantees every state always has setup/update/draw defined:
-- on the web (WebGPU + ASYNCIFY) build, swapping in a sketch that LACKS
-- setup() reproducibly kills all geometry rendering for the rest of the
-- session (root cause under investigation) — with these defaults the
-- condition can no longer occur, and it also spares per-frame nil lookups.
function setup() end
function update() end
function draw() end

-- Cooperative task scheduler (Scratch-inspired). Private task list kept as an
-- upvalue so it is not visible as a global; each fresh state gets its own.
local __tasks = {}  -- array of { co = coroutine, wakeAt = seconds }

function spawn(fn)
    local t = { co = coroutine.create(fn), wakeAt = 0 }
    __tasks[#__tasks + 1] = t
    return t
end

function wait(sec)
    if sec == nil or sec < 0 then sec = 0 end  -- wait()/wait(0) = next frame
    coroutine.yield(sec)
end

function forever(fn)
    return spawn(function()
        while true do
            fn()
            wait(0)  -- implicit yield so a non-yielding fn cannot freeze the frame
        end
    end)
end

function __tcTick(now)
    -- Snapshot the count so tasks spawned during this tick wait until the next.
    local n = #__tasks
    -- Forward walk preserves spawn-order execution; table.remove shifts the
    -- tail left so removing a finished/dead task in place stays safe (we simply
    -- do not advance the index when a slot is removed).
    local i = 1
    local processed = 0
    while processed < n do
        local t = __tasks[i]
        if t.wakeAt <= now then
            local ok, res = coroutine.resume(t.co)
            if not ok then
                -- Error: drop the task, then re-raise so the host's protected
                -- call routes it into the runtime-error pipeline. Any remaining
                -- due tasks simply run next frame -- acceptable.
                table.remove(__tasks, i)
                error(res, 0)
            elseif coroutine.status(t.co) == "dead" then
                table.remove(__tasks, i)  -- finished; do not advance i
            else
                t.wakeAt = now + (tonumber(res) or 0)
                i = i + 1
            end
        else
            i = i + 1
        end
        processed = processed + 1
    end
end
)LUA";

    // Load the task prelude into a state. A failure here is an internal error
    // (the embedded chunk should always compile/run), not a user script error.
    bool loadPrelude(sol::state& lua) {
        sol::load_result chunk = lua.load(kTaskPrelude, "=tc_prelude");
        if (!chunk.valid()) {
            sol::error e = chunk;
            lastError_ = std::string("internal: task prelude failed to load: ") + e.what();
            return false;
        }
        sol::protected_function_result r = chunk();
        if (!r.valid()) {
            sol::error e = r;
            lastError_ = std::string("internal: task prelude failed to run: ") + e.what();
            return false;
        }
        return true;
    }

    void resetState() {
        // New state == clean slate; old state's userdata is GC'd on shared_ptr drop.
        lua_ = tcx_.getLuaState();
        ready_ = false;
    }

    // Load + run one chunk on the given state; name becomes the Lua chunk
    // name (for error locations).
    bool runChunk(sol::state& lua, const std::string& name, const std::string& code) {
        sol::load_result chunk = lua.load(code, name);
        if (!chunk.valid()) {                 // syntax / compile error
            sol::error e = chunk;
            lastError_ = formatLuaError(e.what());
            return false;
        }
        sol::protected_function_result r = chunk(); // run top-level (defines fns/globals)
        if (!r.valid()) {                     // runtime error during load
            sol::error e = r;
            lastError_ = formatLuaError(e.what());
            return false;
        }
        return true;
    }

    // Call a global Lua function by name if it exists; capture runtime errors.
    template <typename... Args>
    void callFn(const char* name, Args&&... args) {
        if (!ready_ || !lua_) return;
        // Type-check BEFORE wrapping: constructing a sol::protected_function
        // around a nil global and invoking it corrupts state on the web build
        // (observed as all geometry silently vanishing for the rest of the
        // session when a sketch omitted setup()/update()).
        sol::object obj = (*lua_)[name];
        if (obj.get_type() != sol::type::function) return;  // callback not defined
        sol::protected_function fn = obj;
        sol::protected_function_result r = fn(std::forward<Args>(args)...);
        if (!r.valid()) {
            sol::error e = r;
            runtimeError_ = formatLuaError(e.what());
        }
    }

    // "name:LINE: msg" / "[string \"name\"]:LINE: msg" -> "name (LINE, 1) : msg"
    // Runtime errors carry a multi-line stack traceback: reformat only the
    // first line (what the editor parses for markers) and keep the rest.
    static std::string formatLuaError(const std::string& raw) {
        size_t nl = raw.find('\n');
        std::string s    = (nl == std::string::npos) ? raw : raw.substr(0, nl);
        std::string rest = (nl == std::string::npos) ? ""  : raw.substr(nl);

        std::string name;
        if (s.rfind("[string \"", 0) == 0) {        // [string "name"]:LINE: msg
            size_t q = s.find('"', 9);
            size_t b = (q == std::string::npos) ? std::string::npos : s.find("]:", q);
            if (b == std::string::npos) return raw;
            name = s.substr(9, q - 9);
            s = s.substr(b + 2);
        } else {                                    // name:LINE: msg
            size_t c = s.find(':');
            if (c == std::string::npos) return raw;
            name = s.substr(0, c);
            s = s.substr(c + 1);
        }
        size_t d = 0;
        while (d < s.size() && s[d] >= '0' && s[d] <= '9') ++d;
        if (d == 0 || d >= s.size() || s[d] != ':') return raw;
        std::string line = s.substr(0, d);
        std::string msg = s.substr(d + 1);
        if (!msg.empty() && msg[0] == ' ') msg.erase(0, 1);
        return name + " (" + line + ", 1) : " + msg + rest;
    }

    tcxLua tcx_;
    std::shared_ptr<sol::state> lua_;
    std::vector<std::pair<std::string, std::string>> files_;
    std::vector<std::shared_ptr<sol::state>> retired_;  // see collectRetired()
    std::string lastError_;
    std::string runtimeError_;
    bool ready_ = false;
};
