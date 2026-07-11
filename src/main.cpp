// =============================================================================
// main.cpp — Entry point for the Lua TrussSketch PoC.
// Same C-export surface as the AngelScript build, so the existing browser/editor
// JS (Module.ccall('addScriptFile' / 'buildScriptFiles' / ...)) works unchanged.
// =============================================================================

#include "tcLuaApp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

extern "C" {

EMSCRIPTEN_KEEPALIVE
void updateScriptCode(const char* code) {
    if (g_app && code) g_app->loadScript(string(code));
}

EMSCRIPTEN_KEEPALIVE
void clearScriptFiles() {
    if (g_app) g_app->clearScriptFiles();
}

EMSCRIPTEN_KEEPALIVE
void addScriptFile(const char* name, const char* code) {
    if (g_app && name && code) g_app->addScriptFile(string(name), string(code));
}

EMSCRIPTEN_KEEPALIVE
const char* buildScriptFiles() {
    static string errorStr;
    if (g_app) {
        bool success = g_app->buildScriptFiles();
        if (!success) {
            errorStr = g_app->getLastError();
            return errorStr.c_str();
        }
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
const char* getScriptError() {
    static string errorStr;
    if (g_app) {
        errorStr = g_app->getLastError();
        return errorStr.c_str();
    }
    return "";
}

EMSCRIPTEN_KEEPALIVE
void pauseEngine() {
    if (g_app) g_app->setPaused(true);
}

EMSCRIPTEN_KEEPALIVE
void resumeEngine() {
    if (g_app) g_app->setPaused(false);
}

EMSCRIPTEN_KEEPALIVE
int isAppReady() {
    return g_app != nullptr ? 1 : 0;
}

} // extern "C"
#endif

int main() {
    tc::WindowSettings settings;
    settings.setSize(600, 600);
    settings.setTitle("tcScript (Lua) - TrussC Playground");
    return tc::runApp<tcLuaApp>(settings);
}
