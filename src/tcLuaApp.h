#pragma once
// tcLuaApp — TrussC App that drives a Lua script through tcLuaScriptHost.
// Mirrors the method surface of the AngelScript tcApp so main.cpp's C exports
// (updateScriptCode / addScriptFile / buildScriptFiles / ...) work unchanged.

#include <TrussC.h>
#include "tcLuaScriptHost.h"

using namespace std;
using namespace tc;

class tcLuaApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mousePressed(Vec2 pos, int button) override;
    void mouseReleased(Vec2 pos, int button) override;
    void mouseMoved(Vec2 pos) override;
    void mouseDragged(Vec2 pos, int button) override;
    void windowResized(int width, int height) override;

    // Script management (called from the JS C-exports in main.cpp).
    void loadScript(const string& code);
    void clearScriptFiles();
    void addScriptFile(const string& name, const string& code);
    bool buildScriptFiles();
    string getLastError() const { return host_ ? host_->getLastError() : initError_; }
    string getRuntimeError() const { return host_ ? host_->getRuntimeError() : ""; }

    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }

private:
    // Native/standalone convenience: load bundled data/sketch.lua at startup.
    // On the web build the editor's C-exports replace it; if absent, no-op.
    void loadDefaultSketch();

    unique_ptr<tcLuaScriptHost> host_;
    string initError_;
    bool scriptLoaded_ = false;
    bool paused_ = false;
};

// Global pointer for Emscripten interop (same role as the AngelScript build).
extern tcLuaApp* g_app;
