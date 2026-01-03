# tcScriptEngine

ChaiScript-based scripting engine for [TrussC](https://github.com/TrussC-org/trussc) Web Playground.

## Overview

tcScriptEngine powers [tcScript](https://trussc.org/tcscript/), a browser-based creative coding playground. Write C++-like code and see it run instantly in WebAssembly.

```javascript
global hue = 0.0

def setup() {
    logNotice("Hello from tcScript!")
}

def update() {
    hue = hue + 0.005
    if (hue > 1.0) { hue = 0.0 }
}

def draw() {
    clear(1.0)

    for (var i = 0; i < 8; ++i) {
        var angle = TAU * i / 8.0 + getElapsedTimef()
        var x = getWindowWidth() / 2.0 + cos(angle) * 120.0
        var y = getWindowHeight() / 2.0 + sin(angle) * 120.0

        setColorHSB(fmod(hue + i * 0.125, 1.0) * TAU, 0.7, 0.9)
        drawCircle(x, y, 25.0)
    }
}

def mousePressed(x, y, button) {
    logNotice("Click at " + to_string(x) + ", " + to_string(y))
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
- Emscripten SDK
- TrussC library

### Build for Web (WASM)

```bash
mkdir build && cd build
emcmake cmake .. -DTRUSSC_DIR=/path/to/trussc
cmake --build .
```

Output files will be in `bin/`:
- `tcScriptEngine.js`
- `tcScriptEngine.wasm`

### Deploy to R2

After building, deploy both files to Cloudflare R2:

```bash
cd bin
wrangler r2 object put trussc-wasm/tcscript/tcScriptEngine.wasm --file tcScriptEngine.wasm --remote
wrangler r2 object put trussc-wasm/tcscript/tcScriptEngine.js --file tcScriptEngine.js --remote
```

### Build for macOS (Development)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## API Reference

See [REFERENCE.md](REFERENCE.md) for the complete API documentation.

Online reference: [trussc.cc/tcscript/reference/](https://trussc.cc/tcscript/reference/)

## Architecture

```
tcScriptEngine/
├── src/
│   ├── main.cpp           # Entry point, Emscripten exports
│   ├── tcApp.cpp/h        # TrussC app with script lifecycle
│   ├── tcScriptHost.cpp/h # ChaiScript wrapper with TrussC bindings
│   └── libs/
│       └── chaiscript/    # ChaiScript headers
├── CMakeLists.txt
├── REFERENCE.md           # Auto-generated API reference
├── ROADMAP.md             # Planned features
└── README.md
```

### API Documentation Generation

`REFERENCE.md` is auto-generated from `tc_v0.0.1/docs/api-definition.yaml`:

```bash
cd ../tc_v0.0.1/docs/scripts
node generate-docs.js
```

This also generates `tcscript-api.js` for the web playground's autocomplete and reference page.

## License

MIT License - see TrussC for details.

## Links

- [tcScript Playground](https://trussc.cc/tcscript/)
- [tcScript API Reference](https://trussc.cc/tcscript/reference/)
- [TrussC Framework](https://github.com/TrussC-org/TrussC)
- [ChaiScript](https://chaiscript.com/)
