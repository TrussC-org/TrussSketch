// =============================================================================
// main.cpp - Entry point for tcScript (TrussC Web Playground)
// =============================================================================

#include "tcApp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

extern "C" {

// Called from JavaScript to update the script code
EMSCRIPTEN_KEEPALIVE
void updateScriptCode(const char* code) {
    if (g_app && code) {
        g_app->loadScript(string(code));
    }
}

// Called from JavaScript to get the last error message
EMSCRIPTEN_KEEPALIVE
const char* getScriptError() {
    static string errorStr;
    if (g_app) {
        errorStr = g_app->getLastError();
        return errorStr.c_str();
    }
    return "";
}

// Pause the app (skip update/draw for power saving)
EMSCRIPTEN_KEEPALIVE
void pauseEngine() {
    if (g_app) {
        g_app->setPaused(true);
    }
}

// Resume the app
EMSCRIPTEN_KEEPALIVE
void resumeEngine() {
    if (g_app) {
        g_app->setPaused(false);
    }
}

} // extern "C"
#endif

int main() {
    tc::WindowSettings settings;
    settings.setSize(600, 600);
    settings.setTitle("tcScript - TrussC Playground");

    return tc::runApp<tcApp>(settings);
}
