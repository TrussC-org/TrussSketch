
# Project Specification: TrussC Web Playground (tcScript)

## Overview
Develop a client-side web playground for the **TrussC** creative coding framework.
Instead of server-side compilation, this project will use an embedded script engine (**ChaiScript**) running within a WebAssembly (Wasm) container. This allows users to write C++-like code in the browser, which is parsed and executed at runtime by the compiled TrussC binary.

## Key Goals
1.  **Zero Server Load:** No server-side compilation. All parsing and execution happen in the client's browser via Wasm.
2.  **Chromebook Compatible:** Must run efficiently on low-spec devices.
3.  **Instant Feedback:** "Hot reload" experience (edit code -> immediate run).
4.  **C++ Syntax Similarity:** The scripting language should resemble C++ as much as possible to serve as a stepping stone to native TrussC.

## Technology Stack
* **Core:** TrussC (C++20, Sokol-based framework)
* **Script Engine:** [ChaiScript](https://chaiscript.com/) (Header-only C++ scripting language, highly compatible with C++ syntax)
* **Build System:** CMake + Emscripten (targeting Wasm)
* **Frontend:** HTML5, JavaScript, Monaco Editor (for code editing)

---

## Architecture

### 1. The C++ Runtime (Wasm)
The Wasm binary acts as the host application.
* **Initialize:** Sets up the TrussC window and initializes the ChaiScript engine instance.
* **Binding:** Exposes TrussC core functions (`tc::drawRect`, `tc::setColor`, etc.) to ChaiScript.
* **Lifecycle Bridge:**
    * `App::setup()` -> calls script `setup()`
    * `App::update()` -> calls script `update()`
    * `App::draw()` -> calls script `draw()`
* **Event Bridge:**
    * `App::mousePressed(x, y)` -> calls script `mousePressed(x, y)`
    * Also needs: `mouseReleased`, `mouseMoved`, `keyPressed`, `keyReleased`, `windowResized`.

### 2. The Script Interface (tcScript)
The user writes code that looks like this (ChaiScript syntax):

```cpp
// User Code Example
global rectX = 100.0;
global rectY = 100.0;

def setup() {
  setWindowTitle("My Script");
  setWindowSize(800, 600);
}

def update() {
  // Simple logic
  rectX = rectX + 1.0;
}

def draw() {
  clear(50); // Gray background
  setColor(255, 0, 0);
  drawRect(rectX, rectY, 50, 50);
}

def mousePressed(x, y, button) {
  rectX = x;
  rectY = y;
}

```

### 3. Frontend UI

* **Editor:** Monaco Editor (VS Code style) embedded in the page.
* **Canvas:** The WebGL canvas where Wasm renders.
* **Controls:**
* [Run/Restart]: Sends the string from Editor to Wasm to re-evaluate.
* [Stop]: Pauses the Wasm loop.


* **Storage:**
* [Save]: Downloads the code as a `.tc` text file (or saves to localStorage).
* [Load]: Uploads a text file to the editor.
* *Note: No server database. All persistence is client-side.*



---

## Implementation Details for AI Developer

### Step 1: Dependencies & CMake

1. Add `chaiscript` headers to the `TrussC/libs` or fetch via CMake `FetchContent`.
2. Update `CMakeLists.txt` to include ChaiScript and link `dl` (needed for *nix/wasm).
3. Ensure Exception Handling is enabled in Emscripten flags (`-fexceptions`).

### Step 2: C++ Binding Implementation (`tcScriptHost`)

Create a class `tcScriptHost` that wraps the ChaiScript instance.

```cpp
// Pseudo-code for Binding
void bindTrussC(chaiscript::ChaiScript& chai) {
    using namespace chaiscript;
    using namespace tc;

    // 1. Bind Basic Types
    // (ChaiScript handles int, double, string automatically)
    
    // 2. Bind Core Functions
    chai.add(fun(&tc::clear), "clear");
    chai.add(fun(&tc::setColor), "setColor");
    chai.add(fun(&tc::drawRect), "drawRect");
    chai.add(fun(&tc::drawCircle), "drawCircle");
    
    // 3. Bind Math/Utility
    chai.add(fun(&tc::random), "random");
    chai.add(fun(&tc::getMouseX), "getMouseX");
    chai.add(fun(&tc::getMouseY), "getMouseY");
    
    // 4. Bind Constants
    chai.add_global_const(const_var(colors::red), "RED");
}

```

### Step 3: Lifecycle Management

In the main `tcApp`:

```cpp
void tcApp::update() {
    // Check if new code arrived from JS
    std::string newCode = getCodeFromJS(); 
    if (!newCode.empty()) {
        reloadScript(newCode);
    }

    // Execute script update
    try {
        if (scriptHasUpdate) scriptUpdateFunc();
    } catch (const std::exception& e) {
        logErrorToConsole(e.what());
    }
}

```

### Step 4: Input Injection

Map TrussC events to Script function calls.

```cpp
void tcApp::mousePressed(float x, float y, int button) {
    try {
        // e.g., call "mousePressed(x, y, button)" in ChaiScript
        auto func = chai.eval<std::function<void(float,float,int)>>("mousePressed");
        func(x, y, button);
    } catch (...) { /* Ignore if function not defined in script */ }
}

```

### Step 5: Web Integration (Emscripten)

* Expose a C function `extern "C" void updateScriptCode(const char* code)` that JS can call.
* Implement `std::cout` redirection to an HTML `div` so script `print()` or errors are visible to the user.

## Limitations & Considerations

* **Performance:** Script execution is slower than native C++. Avoid heavy computation in loops inside the script.
* **Syntax:** ChaiScript is close to C++, but variables are dynamically typed (`var` or `auto`). Instruct users on these minor differences.

```

***

### 使い方

このMarkdownテキストをコピーして、コーディングを担当するAI（CursorのComposer機能や、Clineなど）に貼り付け、以下のように指示してください。

> 「この要件定義書に基づいて、TrussCプロジェクトに `tcScript` 機能（Wasmでのスクリプト実行環境）を追加する実装を行ってください。まずは、ChaiScriptをCMakeに組み込み、簡単な `drawRect` がスクリプトから呼べる状態を目指してください。」

```