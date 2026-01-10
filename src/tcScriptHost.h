#pragma once

#include <TrussC.h>
#include <string>
#include <functional>
#include <angelscript.h>

using namespace std;
using namespace tc;

class tcScriptHost {
public:
    tcScriptHost();
    ~tcScriptHost();

    // Load and execute script code
    bool loadScript(const string& code);

    // Get last error message
    string getLastError() const { return lastError_; }

    // Lifecycle calls (call from tcApp)
    void callSetup();
    void callUpdate();
    void callDraw();

    // Event calls
    void callMousePressed(float x, float y, int button);
    void callMouseReleased(float x, float y, int button);
    void callMouseMoved(float x, float y);
    void callMouseDragged(float x, float y, int button);
    void callKeyPressed(int key);
    void callKeyReleased(int key);
    void callWindowResized(int width, int height);

private:
    void registerTrussCFunctions();
    void messageCallback(const asSMessageInfo* msg);

    asIScriptEngine* engine_ = nullptr;
    asIScriptModule* module_ = nullptr;
    asIScriptContext* ctx_ = nullptr;
    string lastError_;

    // Cached function pointers
    asIScriptFunction* setupFunc_ = nullptr;
    asIScriptFunction* updateFunc_ = nullptr;
    asIScriptFunction* drawFunc_ = nullptr;
    asIScriptFunction* mousePressedFunc_ = nullptr;
    asIScriptFunction* mouseReleasedFunc_ = nullptr;
    asIScriptFunction* mouseMovedFunc_ = nullptr;
    asIScriptFunction* mouseDraggedFunc_ = nullptr;
    asIScriptFunction* keyPressedFunc_ = nullptr;
    asIScriptFunction* keyReleasedFunc_ = nullptr;
    asIScriptFunction* windowResizedFunc_ = nullptr;
};
