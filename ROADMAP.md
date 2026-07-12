# TrussSketch Roadmap

*Last rewritten: 2026-07. The previous version of this file was the
ChaiScript-era plan; its Priority 1–8 items (Vec2/Color/Path/Tween/Image/…)
are all covered by the generated tcxLua bindings today. See [VISION.md](VISION.md)
for the why; this file is the what/when.*

## Done (the current baseline)

- **Engine**: Lua 5.4 via tcxLua/sol2, WASM (emscripten, -Os, ASYNCIFY,
  `-sGROWABLE_ARRAYBUFFERS=0`). ~13MB raw / ~2MB gzip.
- **Bindings**: generated from `reference-data.json` — ~420 functions,
  150+ usertypes, enums, colors, constants. Verified by `bindcheck`.
- **Editor**: CodeMirror 6 behind a Monaco-compat shim (Chromebook-fast),
  Lua highlighting, API completions from `trusssketch-api.js`.
- **Safe hot reload**: candidate-state build — lint/syntax/load failure keeps
  the previous sketch running. Auto-run on by default.
- **Undefined-global lint**: bytecode-level static check (tcxLuaLint in the
  TrussC repo); typo'd variables are build errors with line markers.
- **Runtime error surfacing**: per-frame errors (draw/update/tasks) polled and
  shown with markers + traceback, deduped.
- **Number-drag slider**: tap a number literal → floating slider → live
  rebuild at ~150ms throttle. Range auto-derived (0–1 floats → [0,1]/0.01).
- **Cooperative tasks**: `spawn(fn)` / `wait(sec)` / `forever(fn)` prelude —
  sequential time without state machines. Errors flow into the runtime-error
  pipeline. (Names deliberately chosen to be mirrored by a future C++
  `tc::Task` / `co_await`.)
- **Versioned CDN + embed loader**: `cdn.trussc.org/sketch@<ver>.js` loader
  (`TrussSketch.mount('#canvas', code)`), engine at
  `cdn.trussc.org/<ver>/TrussSketch.{js,wasm,data}`, `@latest` alias for
  testing only. Deploy via `tools/deploy_cdn.sh <version>|latest`.
  Engine versions are aligned with TrussC release tags (first: 0.6.5).

## Next up (decided, ordered)

### 1. Measure the WASM, then split builds
- Run `twiggy`/`bloaty` on the wasm; publish the byte breakdown.
- Ship two blessed bundles per version: **full** (everything) and **lite**
  (no sound/video) — no per-addon combinatorics, no dynamic linking.
- Check/enable brotli on the CDN. Optimize *time-to-first-frame*
  (streaming instantiation, load splash), not raw bytes.
- Target feel: first visit 1–2s on school Wi-Fi, repeat visits 0s
  (immutable cache). p5-sized payloads are a non-goal.

### 2. Errors as conversation
- **Mechanical layer first**: did-you-mean on the undefined-global lint
  (edit distance against bound globals + user locals), kid-readable JP/EN
  messages, hide tracebacks in kid mode.
- **`ask` button (future)**: error panel is designed as
  `{message, location, actions[]}` so an LLM action can be added later.
  Reuses the existing trussc-docs-ai RAG endpoint (api.trussc.org).
  Cost controls: normalize+cache identical errors (kid errors repeat
  heavily), per-session rate limit. No auto-fix button — a wrong fix a kid
  can't question is worse than no fix.

### 3. Sprite layer (tables, honestly)
- `addSprite(image)` → a Lua table wrapping a TrussC Node; per-object
  callbacks (`function cat:clicked()`), combined with spawn/wait for
  behavior. Teaches blueprint-vs-instance without class ceremony.
- Design the API names together with the future C++ `tc::Task`/co_await
  counterpart (Lua ships first; C++ is a separate TrussC-core project).

### 4. Sharing & remix (no accounts)
- Phase 1 (free): `?remixOf=<id>` param on the existing URL sharing.
- Phase 2: content-addressed storage on Workers KV — file blobs keyed by
  hash, a project = manifest {name→hash, parent, engineVersion}; short URL =
  manifest hash prefix (uniform length). Shared library files dedupe to one
  blob no matter how many remixes carry them.
- Phase 3: curated (hand-picked) gallery — no open UGC listing, no
  moderation burden. Remix graph view (manifest parent chain) for
  classrooms.
- Accounts only if cross-device "my works" demand becomes real; local-first
  until then. Anonymity is a feature for schools.

### 5. Version pinning UX
- Every shared sketch/manifest records its engine version; opening loads
  that exact version from the CDN (versions are kept forever).
- New sketches pin the current latest *at creation time* (never "latest").
- Remix inherits the parent's version; editor shows a "made with v0.6.5"
  badge + one-click **upgrade to latest** (re-pin, rebuild — the lint turns
  removed APIs into visible line errors; one-click revert).
- Lua-side deprecation aliases mirror TrussC's `[[deprecated]]` policy so
  most drift is soft. Before each release, sweep saved/gallery sketches
  against the new engine (bindcheck-style) and publish a compat report.

### 6. Embed bridge (minimal, by design)
- JS→Lua: send named events/params; Lua→JS: one callback channel.
  postMessage-shaped so it works identically for loader and iframe embeds.
- No DOM manipulation from Lua. Ever.

## Later / parked

- **C++ `tc::Task`** (C++20 coroutines, `co_await tc::wait(1.0)`) in TrussC
  core — same names as the Lua tasks; needs real design (lifetime,
  cancellation, loop integration).
- **Addon bundles** (e.g. Box2D as the "fun" candidate) — parked until the
  size measurements and lite/full split land. ImGui's original motivation
  (live number tweaking) was obsoleted by the editor's number slider.
- **Infinite-loop watchdog** for user scripts (a non-yielding `while true`
  still freezes the tab; `forever` has an implicit yield, raw loops don't).
- **Pet-name aliases** for content-addressed share URLs.
- **Paint/sound micro-editors** (Scratch-style asset creation) — only after
  the sprite layer proves out.
- 4 internal:: types, hybrid generation for the 13 hand-written usertypes,
  `TC_LUA_BIND` for `Event<T>`/`ThreadChannel<T>`, bindcheck in CI
  (tracked on the TrussC side).

## Operational notes

- Cloudflare edge caches page JS (max-age=14400): after deploying
  trussc.org or `@latest`, expect mixed old/new assets until TTL expiry.
  Versioned paths are immune (immutable).
- The sketch CDN is the `trussc-sketch-cdn` bucket ROOT (cdn.trussc.org).
  `trussc-wasm` is the examples web player — nothing sketch-related goes
  there.
