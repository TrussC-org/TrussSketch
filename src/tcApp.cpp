#include "tcApp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Global pointer for Emscripten interop
tcApp* g_app = nullptr;

// Helper to log errors to JS console (works in Emscripten)
static void logToJSConsole(const string& msg) {
#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.error("TrussSketch: " + UTF8ToString($0));
    }, msg.c_str());
#endif
    logError("tcApp") << msg;
}

void tcApp::setup() {
    g_app = this;
    try {
        scriptHost_ = make_unique<tcScriptHost>();
        // Success - no need to log
    } catch (const std::exception& e) {
        initError_ = string("Fatal error: ") + e.what();
        logToJSConsole(initError_);
    } catch (...) {
        initError_ = "Unknown fatal error in script host init";
        logToJSConsole(initError_);
    }
    // No default script - wait for JS to send code
}

void tcApp::update() {
    // Skip update when paused
    if (paused_) return;

    // Check for pending code from JS
    if (hasPendingCode_) {
        loadScript(pendingCode_);
        hasPendingCode_ = false;
        pendingCode_.clear();
    }

    if (scriptLoaded_ && scriptHost_) {
        scriptHost_->callUpdate();
    }
}

void tcApp::draw() {
    // Skip draw when paused (keep last frame visible)
    if (paused_) return;

    // Show initialization error if any
    if (!initError_.empty()) {
        clear(0.2f, 0.1f, 0.1f);
        setColor(1.0f, 0.3f, 0.3f);
        drawBitmapString("Script Engine Error:", 20, 30);
        setColor(1.0f, 1.0f, 1.0f);
        drawBitmapString(initError_, 20, 50);
        return;
    }

    // Default background if no script
    if (!scriptLoaded_) {
        clear(0.12f);
        setColor(0.5f, 0.5f, 0.5f);
        drawBitmapString("Waiting for script...", 20, 30);
        return;
    }

    if (scriptHost_) {
        scriptHost_->callDraw();
    }
}

void tcApp::loadScript(const string& code) {
    if (scriptHost_) {
        scriptLoaded_ = scriptHost_->loadScript(code);
        if (scriptLoaded_) {
            scriptHost_->callSetup();
            logNotice("tcApp") << "Script loaded successfully";
        } else {
            logError("tcApp") << "Failed to load script: " << scriptHost_->getLastError();
        }
    }
}

string tcApp::getLastError() const {
    return scriptHost_ ? scriptHost_->getLastError() : "";
}

void tcApp::keyPressed(int key) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callKeyPressed(key);
    }
}

void tcApp::keyReleased(int key) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callKeyReleased(key);
    }
}

void tcApp::mousePressed(Vec2 pos, int button) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callMousePressed(pos.x, pos.y, button);
    }
}

void tcApp::mouseReleased(Vec2 pos, int button) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callMouseReleased(pos.x, pos.y, button);
    }
}

void tcApp::mouseMoved(Vec2 pos) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callMouseMoved(pos.x, pos.y);
    }
}

void tcApp::mouseDragged(Vec2 pos, int button) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callMouseDragged(pos.x, pos.y, button);
    }
}

void tcApp::mouseScrolled(Vec2 delta) {
    // Not forwarded to script for now
}

void tcApp::windowResized(int width, int height) {
    if (scriptHost_ && scriptLoaded_) {
        scriptHost_->callWindowResized(width, height);
    }
}

void tcApp::filesDropped(const vector<string>& files) {
    // Could load .tc files here
}

void tcApp::exit() {
    g_app = nullptr;
}
