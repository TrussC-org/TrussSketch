# TrussSketch Vision

*Last rewritten: 2026-07 (Lua/CodeMirror era — replaces the original ChaiScript-era spec).*

## One sentence

**A toy that grows up**: a sketch written by a kid on a school Chromebook can be
shared by URL, embedded in any website, and one day rebuilt as a native
desktop app — without ever changing mental models.

## The two goals

TrussSketch is one product aimed at two gaps that no existing tool covers:

### 1. The escape hatch from p5.js

p5.js owns "creative coding in the browser", but it hits walls: canvas-bound
performance, no real 3D/shader/audio-DSP pipeline, and no path off the web.
TrussSketch is the place p5 users land when they outgrow it:

- **Performance ceiling**: the engine is TrussC (C++/sokol) compiled to WASM —
  100k particles, real shaders, FBOs, real-time audio.
- **The native exit**: the same Lua sketch (and later, the same concepts in
  C++) runs as a real app on macOS/Windows/Linux. p5 cannot leave the browser;
  Scratch cannot grow up. TrussSketch does both.
- **Embeddability**: one `<script>` tag + `TrussSketch.mount()` puts a sketch
  on any page, pinned to an immutable engine version on the CDN.

We do not fight p5's ecosystem head-on (tutorials, galleries, community take
years). We win the users for whom p5 is no longer enough.

### 2. The graduation path from Scratch

In Japan especially there is a well-known cliff: GIGA-school Chromebooks run
Scratch until 6th grade, and then… a black Python terminal. TrussSketch is
positioned as **"where you go after Scratch"** — not a Scratch clone.

We do NOT copy blocks. We import the three things Scratch actually got right,
in text form:

- **No silent failure**: Scratch makes syntax errors impossible; we make
  errors *conversational* instead — build-time undefined-variable lint with
  line markers, did-you-mean suggestions, kid-readable messages, no raw
  tracebacks in kid mode.
- **Sequential time**: Scratch scripts read top-to-bottom through time
  ("move, wait, meow"). Our `spawn` / `wait` / `forever` coroutine tasks give
  the same power in Lua — no state machines, no `if now - t0 > 1`.
- **Tinkerability**: safe hot reload (broken edits never kill the running
  sketch), tap-a-number sliders for live tweaking, and eventually a remix
  culture where every shared sketch's code is one tap away.

Classes are taught honestly, not hidden: Lua tables (`Cat = {}`,
`function Cat:update()`) are the gentlest possible introduction to
"blueprint vs instance", and the future sprite layer builds on them.

## The growth staircase

The same concepts transfer at every step — that is the product:

```
Scratch kid  →  TrussSketch Lua (spawn/wait, sprites, tables)
             →  TrussC C++ (same API names, co_await tc::wait() later)
             →  shipping native apps
```

## Non-goals

- Block-based editing (that is Scratch's identity, not ours).
- Deep DOM integration from Lua (a minimal JS↔Lua message bridge only).
- Matching p5's 200KB payload (physically impossible with a real engine;
  we optimize time-to-first-frame and immutable-cache hits instead).
- User accounts as a prerequisite for anything (anonymity is a feature for
  schools; accounts come last, if ever).

## Principles

- **Zero server load**: everything runs client-side in WASM; sharing is
  URL-encoded or content-addressed blobs on a KV store — no app servers.
- **Chromebook first**: if it lags on a school Chromebook, it's broken
  (this is why Monaco was replaced by CodeMirror 6).
- **Immutable versions**: every published engine version lives forever at
  `cdn.trussc.org/<version>/`; sketches pin the version they were written
  with and never silently break.
- **Verified, not hoped**: builds are witnessed (native + web), bindings are
  bindcheck'd, and features ship only after live browser verification.
