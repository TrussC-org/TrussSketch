#pragma once

#include <TrussC.h>
#include "tcScriptHost.h"
using namespace std;
using namespace tc;

class tcApp : public App {
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
    void mouseScrolled(Vec2 delta) override;

    void windowResized(int width, int height) override;
    void filesDropped(const vector<string>& files) override;
    void exit() override;

    // Script management (single-file mode)
    void loadScript(const string& code);

    // Script management (multi-file mode)
    void clearScriptFiles();
    void addScriptFile(const string& name, const string& code);
    bool buildScriptFiles();

    string getLastError() const;

    // Pause control (for power saving)
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }

private:
    unique_ptr<tcScriptHost> scriptHost_;
    string pendingCode_;
    string initError_;  // Error during script engine initialization
    bool hasPendingCode_ = false;
    bool scriptLoaded_ = false;
    bool paused_ = false;
};

// Global pointer for Emscripten interop
extern tcApp* g_app;
