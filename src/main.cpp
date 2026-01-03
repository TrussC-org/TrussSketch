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

// Pause the main loop (for power saving when stopped)
EMSCRIPTEN_KEEPALIVE
void pauseEngine() {
    emscripten_pause_main_loop();
}

// Resume the main loop
EMSCRIPTEN_KEEPALIVE
void resumeEngine() {
    emscripten_resume_main_loop();
}

} // extern "C"
#endif

int main() {
    tc::WindowSettings settings;
    settings.setSize(600, 600);
    settings.setTitle("tcScript - TrussC Playground");

    return tc::runApp<tcApp>(settings);
}
