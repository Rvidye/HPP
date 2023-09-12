#pragma once
#include "Renderer.h"
#include "../include/windowing.h"

class Viewer : public RendererBase
{
public:
    Viewer(const RendererParams& params);
    virtual ~Viewer();

    void   open() override;
    void   close() override;
    bool   render(int frame) override;
    void   resizeFrameBuffer(int width, int height) override;
    void   run() override;
    void   renderViewOverlay() override;
    double getTime() override;
    void   printHelp(std::ostream& s) const override;
    bool   updateCamera() override;
    void   updateAnimationControl();
    void   setSceneFrame(int frame);

    void OnKey(int key, int action);
    void OnMouse(int button, int action, int x, int y);

    static void mainLoop(void* userData);
    bool        runLoop();

protected:
    void updateWindowTitle();

private:
    // gui.
    bool         mIsDrawingRenderStats = true;
    bool         mIsDrawingAboutDialog = false;
    bool         mIsDrawingHelpDialog = false;
    bool         mIsDrawingSceneGraph = false;
    bool         mIsDrawingAssets = false;
    bool         mIsDrawingRenderOptions = false;
    bool         mIsDrawingEventLog = false;
    bool         mLogAutoScroll = true;
    bool         mIsDrawingPendingGlyph = false;

    bool drawPointRenderOptionsWidget(SceneNode::Ptr node, int attachmentIndex);
    void drawRenderStatsOverlay();
    void drawAboutDialog();
    void drawRenderPlatformWidget(const char* label);
    void drawMenuBar();
    void drawSceneGraph();
    void drawAssets();
    void drawSceneGraphNodes();
    void drawHelpDialog();
    void drawRenderOptionsDialog();
    void drawEventLog();
    void drawGridInfo(const std::string& url, const std::string& gridName);
    bool drawMaterialParameters(SceneNode::Ptr node, MaterialClass mat);
    bool drawMaterialGridAttachment(SceneNode::Ptr node, int attachmentIndex);
    void drawPendingGlyph();

    std::string getScreenShotFilename(int iteration) const;

    void*  mWindow = nullptr;
    glwindow* window = nullptr;
   
    int    mWindowWidth = 0;
    int    mWindowHeight = 0;
    int    mFps = 0;
    size_t mFpsFrame = 0;
    double mTime = 0;

    enum class PlaybackState { STOP = 0,
                               PLAY = 1 
                             };

    PlaybackState mPlaybackState = PlaybackState::STOP;
    float         mPlaybackTime = 0;
    float         mPlaybackLastTime = 0;
    float         mPlaybackRate = 30;

    // mouse state.
    bool  mMouseDown = false;
    bool  mIsFirstMouseMove = false;
    bool  mIsMouseRightDown = false;
    float mMouseX = 0;
    float mMouseY = 0;
    int   mWheelPos = 0;
};
