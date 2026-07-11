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

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <regex>

#include <tcxLua.h>   // brings in sol/sol.hpp and TrussC.h

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

    // Rebuild from scratch. Returns true on success; on failure getLastError()
    // holds an editor-parseable message and ready_ stays false.
    bool buildScriptFiles() {
        resetState();
        lastError_.clear();
        for (const auto& f : files_) {
            if (!runChunk(f.first, f.second)) return false; // lastError_ set
        }
        ready_ = true;
        return true;
    }

    const std::string& getLastError() const { return lastError_; }
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

private:
    void resetState() {
        // New state == clean slate; old state's userdata is GC'd on shared_ptr drop.
        lua_ = tcx_.getLuaState();
        ready_ = false;
    }

    // Load + run one chunk; name becomes the Lua chunk name (for error locations).
    bool runChunk(const std::string& name, const std::string& code) {
        sol::load_result chunk = lua_->load(code, name);
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
        sol::protected_function fn = (*lua_)[name];
        if (!fn.valid()) return;              // callback simply not defined
        sol::protected_function_result r = fn(std::forward<Args>(args)...);
        if (!r.valid()) {
            sol::error e = r;
            lastError_ = formatLuaError(e.what());
        }
    }

    // "name:LINE: msg" / "[string \"name\"]:LINE: msg" -> "name (LINE, 1) : msg"
    static std::string formatLuaError(const std::string& raw) {
        static const std::regex re(R"(^(?:\[string \")?([^":\]]+)\"?\]?:(\d+):\s*(.*)$)");
        std::smatch m;
        if (std::regex_match(raw, m, re)) {
            return m[1].str() + " (" + m[2].str() + ", 1) : " + m[3].str();
        }
        return raw;
    }

    tcxLua tcx_;
    std::shared_ptr<sol::state> lua_;
    std::vector<std::pair<std::string, std::string>> files_;
    std::string lastError_;
    bool ready_ = false;
};
