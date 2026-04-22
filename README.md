# TrussSketch

AngelScript-based scripting engine for [TrussC](https://github.com/TrussC-org/TrussC) Web Playground.

## Overview

TrussSketch powers [tcScript](https://trussc.org/tcscript/), a browser-based creative coding playground. Write C++-like script code and see it run instantly in WebAssembly.

```cpp
float hue = 0.0f;

void setup() {
    logNotice("Hello from TrussSketch!");
}

void update() {
    hue += 0.005f;
    if (hue > 1.0f) hue = 0.0f;
}

void draw() {
    clear(1.0f);

    for (int i = 0; i < 8; i++) {
        float angle = TAU * float(i) / 8.0f + getElapsedTimef();
        float x = getWindowWidth() / 2.0f + cos(angle) * 120.0f;
        float y = getWindowHeight() / 2.0f + sin(angle) * 120.0f;

        setColorHSB(fmod(hue + float(i) * 0.125f, 1.0f), 0.7f, 0.9f);
        drawCircle(x, y, 25.0f);
    }
}

void mousePressed(float x, float y, int button) {
    logNotice("Click at " + x + ", " + y);
}
```

## Features

- **Instant feedback**: Edit code and click Run to see changes immediately
- **Zero server load**: Everything runs client-side in WebAssembly
- **Chromebook friendly**: Works on low-spec devices
- **Share your creations**: Generate URLs to share your sketches

## Building

### Requirements

- CMake 3.20+
- Emscripten SDK (for Web builds)
- TrussC library

### Build for Web (WASM)

```bash
./build-web.command
```

Or manually:

```bash
mkdir build-web && cd build-web
emcmake cmake ..
cmake --build .
```

Output files land in `bin/`:
- `TrussSketch.js`
- `TrussSketch.wasm`

### Deploy to R2

After building, deploy both files to Cloudflare R2:

```bash
cd bin
wrangler r2 object put trussc-wasm/sketch/TrussSketch.wasm --file TrussSketch.wasm --remote
wrangler r2 object put trussc-wasm/sketch/TrussSketch.js --file TrussSketch.js --remote
```

### Build for macOS (Development)

```bash
mkdir build-macos && cd build-macos
cmake ..
cmake --build .
```

## API Reference

See [REFERENCE.md](REFERENCE.md) for the complete API documentation.

Online reference: [trussc.org/tcscript/reference/](https://trussc.org/tcscript/reference/)

## Architecture

```
TrussSketch/
├── src/
│   ├── main.cpp           # Entry point, Emscripten exports
│   ├── tcApp.cpp/h        # TrussC app with script lifecycle
│   └── tcScriptHost.cpp/h # AngelScript wrapper with TrussC bindings
├── testScript/            # Sample .tcs scripts used for local testing
├── CMakeLists.txt
├── REFERENCE.md           # Auto-generated API reference
├── ROADMAP.md             # Planned features
└── README.md
```

AngelScript itself is pulled in via CMake `FetchContent` (see `CMakeLists.txt`), so there is no vendored script engine in the source tree.

### API Documentation Generation

`REFERENCE.md` is auto-generated from `TrussC/docs/api-definition.yaml`:

```bash
cd ../docs/scripts
node generate-docs.js
```

This also generates `tcscript-api.js` for the web playground's autocomplete and reference page.

## License

MIT License - see TrussC for details.

## Links

- [tcScript Playground](https://trussc.org/tcscript/)
- [tcScript API Reference](https://trussc.org/tcscript/reference/)
- [TrussC Framework](https://github.com/TrussC-org/TrussC)
- [AngelScript](https://www.angelcode.com/angelscript/)
