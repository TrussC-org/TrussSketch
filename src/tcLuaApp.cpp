#include "tcLuaApp.h"

tcLuaApp* g_app = nullptr;

void tcLuaApp::setup() {
    g_app = this;
    host_ = make_unique<tcLuaScriptHost>();
    loadDefaultSketch();
}

void tcLuaApp::loadDefaultSketch() {
    std::ifstream f(getDataPath("sketch.lua"));
    if (!f) return;                       // web build w/o bundled data, etc.
    std::stringstream ss;
    ss << f.rdbuf();
    clearScriptFiles();
    addScriptFile("sketch.lua", ss.str());
    if (!buildScriptFiles()) {
        logError("Lua load error: " + getLastError());
    }
}

void tcLuaApp::update() {
    // Frame boundary: the previous frame (including any ASYNCIFY-suspended
    // draw on a replaced state) has fully completed — safe to destroy
    // states retired by buildScriptFiles().
    if (host_) host_->collectRetired();
    if (paused_) return;
    if (scriptLoaded_) {
        // Advance cooperative tasks (spawn/wait/forever) before user update().
        host_->callTick(getElapsedTime());
        host_->callUpdate();
    }
}

void tcLuaApp::draw() {
    if (!scriptLoaded_) {
        clear(0.12f);
        drawBitmapString("Waiting for Lua script...", 20, 30);
        return;
    }
    host_->callDraw();
}

void tcLuaApp::keyPressed(int key)            { if (scriptLoaded_) host_->callKeyPressed(key); }
void tcLuaApp::keyReleased(int key)           { if (scriptLoaded_) host_->callKeyReleased(key); }
void tcLuaApp::mousePressed(Vec2 p, int b)    { if (scriptLoaded_) host_->callMousePressed(p.x, p.y, b); }
void tcLuaApp::mouseReleased(Vec2 p, int b)   { if (scriptLoaded_) host_->callMouseReleased(p.x, p.y, b); }
void tcLuaApp::mouseMoved(Vec2 p)             { if (scriptLoaded_) host_->callMouseMoved(p.x, p.y); }
void tcLuaApp::mouseDragged(Vec2 p, int b)    { if (scriptLoaded_) host_->callMouseDragged(p.x, p.y, b); }
void tcLuaApp::windowResized(int w, int h)    { if (scriptLoaded_) host_->callWindowResized(w, h); }

void tcLuaApp::loadScript(const string& code) {
    clearScriptFiles();
    addScriptFile("main.tcs", code);
    buildScriptFiles();
}

void tcLuaApp::clearScriptFiles() {
    if (host_) host_->clearScriptFiles();
}

void tcLuaApp::addScriptFile(const string& name, const string& code) {
    if (host_) host_->addScriptFile(name, code);
}

bool tcLuaApp::buildScriptFiles() {
    if (!host_) return false;
    bool ok = host_->buildScriptFiles();
    if (ok) {
        host_->callSetup();   // run the freshly-loaded setup() once
        scriptLoaded_ = true;
    }
    return ok; // on failure, keep the previous sketch state; error is in getLastError()
}
