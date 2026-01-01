# tcScriptEngine

ChaiScript-based scripting engine for [TrussC](https://github.com/TrussC-org/trussc) Web Playground.

## Overview

tcScriptEngine powers [tcScript](https://trussc.org/tcscript/), a browser-based creative coding playground. Write C++-like code and see it run instantly in WebAssembly.

```javascript
global hue = 0.0

def setup() {
    print("Hello from tcScript!")
}

def update() {
    hue = hue + 0.005
    if (hue > 1.0) { hue = 0.0 }
}

def draw() {
    clear(1.0)

    for (var i = 0; i < 8; ++i) {
        var angle = TAU * i / 8.0 + getElapsedTime()
        var x = getWindowWidth() / 2.0 + cos(angle) * 120.0
        var y = getWindowHeight() / 2.0 + sin(angle) * 120.0

        setColor(fmod(hue + i * 0.125, 1.0), 0.7, 0.9)
        drawCircle(x, y, 25.0)
    }
}

def mousePressed(x, y, button) {
    print("Click at " + to_string(x) + ", " + to_string(y))
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

### Build for macOS (Development)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## API Reference

### Lifecycle Functions

```javascript
def setup() { }      // Called once at start
def update() { }     // Called every frame
def draw() { }       // Called every frame after update
```

### Event Functions

```javascript
def mousePressed(x, y, button) { }
def mouseReleased(x, y, button) { }
def mouseMoved(x, y) { }
def mouseDragged(x, y, button) { }
def keyPressed(key) { }
def keyReleased(key) { }
def windowResized(width, height) { }
```

### Graphics

```javascript
clear(gray)
clear(r, g, b)
setColor(gray)
setColor(r, g, b)
setColor(r, g, b, a)

drawRect(x, y, w, h)
drawCircle(x, y, radius)
drawEllipse(x, y, w, h)
drawLine(x1, y1, x2, y2)
drawTriangle(x1, y1, x2, y2, x3, y3)
drawText(text, x, y)

fill() / noFill()
stroke() / noStroke()
setStrokeWeight(weight)
```

### Transform

```javascript
translate(x, y)
rotate(radians)
rotateDeg(degrees)
scale(s)
scale(sx, sy)
pushMatrix() / popMatrix()
```

### Input

```javascript
getMouseX()
getMouseY()
isMousePressed()
```

### Time

```javascript
getElapsedTime()   // Seconds since start
getDeltaTime()     // Seconds since last frame
getFrameRate()     // Current FPS
getFrameCount()    // Total frames rendered
```

### Math

```javascript
random()              // 0.0 - 1.0
random(max)           // 0.0 - max
random(min, max)      // min - max
noise(x)              // Perlin noise
noise(x, y)
noise(x, y, z)
lerp(a, b, t)
clamp(v, min, max)
map(v, inMin, inMax, outMin, outMax)
sin, cos, tan, abs, sqrt, pow, min, max, floor, ceil, fmod
radians(deg), degrees(rad)
```

### Constants

```javascript
TAU          // 6.28318... (2 * PI)
HALF_TAU     // 3.14159... (PI)
QUARTER_TAU  // 1.57079... (PI / 2)
PI           // 3.14159...
```

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
├── ROADMAP.md             # Planned features
└── README.md
```

## License

MIT License - see TrussC for details.

## Links

- [tcScript Playground](https://trussc.org/tcscript/)
- [TrussC Framework](https://github.com/TrussC-org/trussc)
- [ChaiScript](https://chaiscript.com/)
