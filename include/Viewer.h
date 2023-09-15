#pragma once
#include "Renderer.h"
#include <X11/X.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<GL/glx.h>
#include<iostream>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_x11.h>

enum MOUSE {
	MOUSE_BUTTON_PRESS,
	MOUSE_BUTTON_RELEASE,
	MOUSE_BUTTON_MOVE,
    MOUSE_WHEEL_UP,
    MOUSE_WHEEL_DOWN
};

class Viewer;
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

    void OnKey(int key);
    void OnMouse(int button, int action, int x, int y);

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

    Display* display;
	XVisualInfo* visualInfo;
	Colormap colorMap;
	GLXFBConfig fbConfig;
	Window window;
	GLXContext glxcontext;
	bool closed;
	bool fullscreen;
	bool guiSupport;
	struct windowsize_t {
		float width;
		float height;
	} windowSize;

	void setGUISupport(bool gui);
	void processEvents(void);
	bool isClosed(void);
	windowsize_t getSize(void);
	void swapBuffers(void);

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
