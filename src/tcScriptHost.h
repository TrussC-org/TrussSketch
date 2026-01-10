#pragma once

#include <TrussC.h>
#include <string>
#include <functional>
#include <vector>
#include <angelscript.h>

using namespace std;
using namespace tc;

class tcScriptHost {
public:
    tcScriptHost();
    ~tcScriptHost();

    // Load and execute script code (single file mode)
    bool loadScript(const string& code);

    // Multi-file support
    void clearScriptFiles();
    void addScriptFile(const string& name, const string& code);
    bool buildScriptFiles();

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

    // Multi-file storage (preserves order)
    vector<pair<string, string>> scriptFiles_;

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
