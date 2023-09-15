#include<X11/X.h>
#include<imgui/imgui.h>
#include<imgui/imgui_internal.h>
#include<stdlib.h>
#include<memory.h>
#include<X11/Xlib.h>
#include<X11/XKBlib.h>
#include<X11/keysym.h>
#include<GL/glew.h>
#include<GL/glx.h>
#include<iostream>
#define _USE_MATH_DEFINES
#include <cmath>

#include "Viewer.h"
#include "RenderLauncher.h"

#include "FrameBufferHost.h"
#include "FrameBufferGL.h"
#include "StringUtils.h"
#include <nanovdb/util/IO.h> // for NanoVDB file import

#define NANOVDB_USE_IMGUI

#if defined(NANOVDB_USE_OPENVDB)
#include <nanovdb/util/OpenToNanoVDB.h>
#endif

static uint64_t clockOffset = 0; 
static const clockid_t currentclock = CLOCK_REALTIME;
static const uint64_t frequency = 1000000000;

uint64_t getTimerValue() {
	struct timespec ts;
	clock_gettime(currentclock, &ts);
	return (uint64_t)ts.tv_sec * frequency + (uint64_t)ts.tv_nsec;
}

Viewer::Viewer(const RendererParams& params)
    : RendererBase(params)
{
#if defined(NANOVDB_USE_OPENVDB)
    openvdb::initialize();
#endif

    setRenderPlatform(0);
    mFps = 0;
    setSceneFrame(params.mFrameStart);
}

Viewer::~Viewer()
{
    if(glXGetCurrentContext() == this->glxcontext) {
		glXMakeCurrent(this->display, 0, 0);
	}

	if(this->glxcontext) {
		glXDestroyContext(this->display, this->glxcontext);
	}

	if(this->window) {
		XDestroyWindow(this->display, this->window);
	}

	if(this->colorMap) {
		XFreeColormap(this->display, this->colorMap);
	}

	if(this->visualInfo) {
		XFree(this->visualInfo);
		this->visualInfo = NULL;
	}

	if(this->display) {
		XCloseDisplay(this->display);
		this->display = NULL;
	}
}

void Viewer::close()
{
    XEvent ev;
	memset(&ev, 0, sizeof (ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = this->window;
	ev.xclient.message_type = XInternAtom(this->display, "WM_PROTOCOLS", true);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(this->display, "WM_DELETE_WINDOW", false);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(this->display, this->window, False, NoEventMask, &ev);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplX11_Shutdown();
    ImGui::DestroyContext();
}

double Viewer::getTime()
{
    return (double)(getTimerValue() - clockOffset) / frequency;
}

Viewer::windowsize_t Viewer::getSize(void) {
	return this->windowSize;
}

void Viewer::open()
{
    static int frameBufferAttrib[] = {
		GLX_DOUBLEBUFFER, True,
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_STENCIL_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		None 
	};

	XSetWindowAttributes winAttribs;
	GLXFBConfig *pGlxFBConfig;
	XVisualInfo *pTempVisInfo = NULL;
	
	int numFBConfigs;
	int defaultScreen;
	int styleMask;

	this->display = XOpenDisplay(NULL);
	defaultScreen = XDefaultScreen(this->display);
	pGlxFBConfig = glXChooseFBConfig(this->display, defaultScreen, frameBufferAttrib, &numFBConfigs);
	int bestFrameBufferConfig = -1, bestNumOfSamples = -1;
	for(int i = 0; i < numFBConfigs; i++) {
		pTempVisInfo = glXGetVisualFromFBConfig(this->display, pGlxFBConfig[i]);
		if(pTempVisInfo != NULL) {
			int samplesBuffer, samples;
			glXGetFBConfigAttrib(this->display, pGlxFBConfig[i], GLX_SAMPLE_BUFFERS, &samplesBuffer);
			glXGetFBConfigAttrib(this->display, pGlxFBConfig[i], GLX_SAMPLES, &samples);
			if(bestFrameBufferConfig < 0 || samplesBuffer && samples > bestNumOfSamples) {
				bestFrameBufferConfig = i;
				bestNumOfSamples = samples;
			}
		}
		XFree(pTempVisInfo);
	}

	this->fbConfig = pGlxFBConfig[bestFrameBufferConfig];
	XFree(pGlxFBConfig);

	this->visualInfo = glXGetVisualFromFBConfig(this->display, this->fbConfig);
	winAttribs.border_pixel = 0;
	winAttribs.background_pixmap = 0;
	winAttribs.colormap = XCreateColormap(this->display, RootWindow(this->display, this->visualInfo->screen), this->visualInfo->visual, AllocNone);
	this->colorMap = winAttribs.colormap;
	winAttribs.background_pixel = BlackPixel(this->display, defaultScreen);
	winAttribs.event_mask = ExposureMask | VisibilityChangeMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | ButtonMotionMask | StructureNotifyMask;
	styleMask = CWBorderPixel | CWBackPixel | CWEventMask | CWColormap;

	this->window = XCreateWindow(this->display, RootWindow(this->display, this->visualInfo->screen), 0, 0, mParams.mWidth, mParams.mHeight, 0, this->visualInfo->depth, InputOutput, this->visualInfo->visual, styleMask, &winAttribs);

	Atom windowManagerDelete = XInternAtom(this->display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(this->display, this->window, &windowManagerDelete, 1);

	XMapWindow(this->display, this->window);

	closed = false;

	typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
	glXCreateContextAttribsARBProc glXCreateContextAttribARB = NULL;
	glXCreateContextAttribARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
	const int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 460 / 100,
		GLX_CONTEXT_MINOR_VERSION_ARB, 460 % 100 / 10,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | GLX_CONTEXT_DEBUG_BIT_ARB,
		None
	};

	this->glxcontext =  glXCreateContextAttribARB(this->display, this->fbConfig, 0, True, attribs);
	glXMakeCurrent(this->display, this->window, this->glxcontext);
	glewInit();

    std::cout << "GL_VERSION  : " << glGetString(GL_VERSION) << "\n";
    std::cout << "GL_RENDERER : " << glGetString(GL_RENDERER) << "\n";
    std::cout << "GL_VENDOR   : " << glGetString(GL_VENDOR) << "\n";

    mFrameBuffer.reset(new FrameBufferGL(this->glxcontext, this->display));
    this->resizeFrameBuffer(mParams.mWidth, mParams.mHeight);

    this->setGUISupport(true);
    NANOVDB_GL_CHECKERRORS();
}

void Viewer::processEvents(void) {
	XEvent event,next_event;
	KeySym keysym;

	static unsigned currentButton;

	while(XPending(this->display)) {
		XNextEvent(this->display, &event);
		switch(event.type) {
		case KeyPress:
			keysym = XkbKeycodeToKeysym(this->display, event.xkey.keycode, 0, 0);
                this->OnKey(keysym);
			break;
		case ButtonPress:
			currentButton = event.xbutton.button;
            OnMouse(event.xbutton.button, MOUSE_BUTTON_PRESS, event.xbutton.x, event.xbutton.y);
			break;
		case ButtonRelease:
			currentButton = 0;
            OnMouse(event.xbutton.button, MOUSE_BUTTON_RELEASE, event.xbutton.x, event.xbutton.y);
			break;
		case MotionNotify:
            OnMouse(currentButton, MOUSE_BUTTON_MOVE, event.xmotion.x, event.xmotion.y);
			break;
		case ConfigureNotify:
			windowSize.width = event.xconfigure.width;
			windowSize.height = event.xconfigure.height;
            this->resizeFrameBuffer(windowSize.width, windowSize.height);
			break;
		case ClientMessage:
			closed = true;
			break;
		default:
			break;
		}
		if(guiSupport)
			ImGui_ImplX11_EventHandler(event,nullptr);
	}
}

void Viewer::run()
{
    mFpsFrame = 0;
    mTime = this->getTime();

    while (!this->isClosed()) {

        //process events
        this->processEvents();

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glDepthFunc(GL_LESS);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        updateAnimationControl();

        mIsDrawingPendingGlyph = false;
        if (mSelectedSceneNodeIndex >= 0) {
            updateNodeAttachmentRequests(mSceneNodes[mSelectedSceneNodeIndex], false, mIsDumpingLog, &mIsDrawingPendingGlyph);
        }

        updateScene();

        render(getSceneFrame());
        renderViewOverlay();

        // workaround for bad GL state in ImGui...
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        NANOVDB_GL_CHECKERRORS();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplX11_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        drawMenuBar();
        drawSceneGraph();
        drawRenderOptionsDialog();
        drawRenderStatsOverlay();
        drawAboutDialog();
        drawHelpDialog();
        drawEventLog();
        drawAssets();
        drawPendingGlyph();

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::EndFrame();

        // eval fps...
        ++mFpsFrame;
        double elapsed = this->getTime() - mTime;
        if (elapsed > 1.0) {
            mTime = this->getTime();
            mFps = (int)(double(mFpsFrame) / elapsed);
            //updateWindowTitle();
            mFpsFrame = 0;
        }
        this->swapBuffers();
    }
}

void Viewer::setGUISupport(bool gui){
	this->guiSupport = gui;
	// Initialize IMGUI Support
	if(gui){
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		ImGui_ImplOpenGL3_Init();
		ImGui_ImplX11_Init(this->display, &this->window);
	}
}

void Viewer::swapBuffers() {
	glXSwapBuffers(this->display, this->window);
}

void Viewer::renderViewOverlay()
{}

void Viewer::resizeFrameBuffer(int width, int height)
{
    auto fb = static_cast<FrameBufferGL*>(mFrameBuffer.get());
    fb->setupGL(width, height, nullptr, GL_RGBA32F, GL_DYNAMIC_DRAW);
    resetAccumulationBuffer();
}

bool Viewer::render(int frame)
{
    if (display == nullptr)
        return false;

    if (RendererBase::render(frame) == false) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return true;
    } else {
        mFrameBuffer->render(0, 0, mFrameBuffer->width(), mFrameBuffer->height());
        return true;
    }
}

bool Viewer::isClosed(void) {
	return closed;
}

void Viewer::printHelp(std::ostream& s) const
{
    auto platforms = mRenderLauncher.getPlatformNames();

    assert(platforms.size() > 0);
    std::stringstream platformList;
    platformList << "[" << platforms[0];
    for (size_t i = 1; i < platforms.size(); ++i)
        platformList << "," << platforms[i];
    platformList << "]";

    s << "-------------------------------------\n";
    s << "Hot Keys:\n";
    s << "-------------------------------------\n";
    s << "\n";
    s << "- Show Hot-Keys                [H]\n";
    s << "- Renderer-platform            [1 - 9] = (" << (mParams.mRenderLauncherType) << " / " << platformList.str() << ")\n";
    s << "\n";
    s << "Scene options ------------------------\n";
    s << "- Select Next/Previous Node    [+ / -] = (" << ((mSelectedSceneNodeIndex >= 0) ? mSceneNodes[mSelectedSceneNodeIndex]->mName : "") << ")\n";
    s << "\n";
    s << "View options ------------------------\n";
    s << "- Frame Selected               [F]\n";
    s << "- Screenshot                   [CTRL + P (or PRINTSCREEN)]\n";
    s << "\n";
    s << "Animation options ------------------------\n";
    s << "- Toggle Play/Stop             [ENTER]\n";
    s << "- Play From Start              [CTRL + ENTER]\n";
    s << "- Previous/Next Frame          [< / >]\n";
    s << "- Goto Start                   [CTRL + <]\n";
    s << "- Goto End                     [CTRL + >]\n";
    s << "- Scrub                        [TAB + MOUSE_LEFT]\n";
    s << "\n";
    s << "Render options ----------------------\n";
    s << "- Toggle Render Progressive    [P] = (" << (mParams.mUseAccumulation ? "ON" : "OFF") << ")\n";
    s << "- Toggle Render Lighting       [L] = (" << (mParams.mSceneParameters.useLighting ? "ON" : "OFF") << ")\n";
    s << "- Toggle Render Background     [B] = (" << (mParams.mSceneParameters.useBackground ? "ON" : "OFF") << ")\n";
    s << "- Toggle Render Shadows        [S] = (" << (mParams.mSceneParameters.useShadows ? "ON" : "OFF") << ")\n";
    s << "- Toggle Render Ground-plane   [G] = (" << (mParams.mSceneParameters.useGround ? "ON" : "OFF") << ")\n";
    s << "-------------------------------------\n";
    s << "\n";
}

void Viewer::updateAnimationControl()
{
    if (mPlaybackState == PlaybackState::PLAY) {
        float t = (float)getTime();
        if ((t - mPlaybackLastTime) * mPlaybackRate > 1.0f) {
            mPlaybackTime += (t - mPlaybackLastTime) * mPlaybackRate;
            mPlaybackLastTime = t;
        }
        RendererBase::setSceneFrame((int)mPlaybackTime);
    }
}

void Viewer::setSceneFrame(int frame)
{
    RendererBase::setSceneFrame(frame);

    mPlaybackLastTime = (float)getTime();
    mPlaybackTime = (float)(frame - mParams.mFrameStart);
    mPlaybackState = PlaybackState::STOP;
}

bool Viewer::updateCamera()
{
    int  sceneFrame = getSceneFrame();
    bool isChanged = false;

    if (mCurrentCameraState->mFrame != (float)sceneFrame) {
        isChanged = true;
        mCurrentCameraState->mFrame = (float)sceneFrame;
    }

    if (mPlaybackState == PlaybackState::PLAY && mParams.mUseTurntable) {
        int count = (mParams.mFrameEnd <= mParams.mFrameStart) ? 100 : (mParams.mFrameEnd - mParams.mFrameStart + 1);
        mCurrentCameraState->mCameraRotation[1] = ((sceneFrame * 2.0f * 3.14159265f) / count) / std::max(mParams.mTurntableRate, 1.0f);
        mCurrentCameraState->mIsViewChanged = true;
    } else {
    }

    isChanged |= mCurrentCameraState->update();
    return isChanged;
}

void Viewer::OnKey(int key)
{
    switch(key){
        
        case XK_Escape:
            this->close();
        break;
        default:
        break;
    }

    if(key >= XK_1 && key <= XK_9){
        setRenderPlatform(key - XK_1);
        mFps = 0;
    }
}

void Viewer::OnMouse(int button, int action, int x, int y)
{
    const float zoomSpeed = mCurrentCameraState->mCameraDistance * 0.1f;
    static int pos = mWheelPos;
    if (button == Button1) {
        if (action == MOUSE_BUTTON_PRESS) {
            mMouseDown = true;
            mIsFirstMouseMove = true;
        }
    } else if (button == Button2) {
        if (action == MOUSE_BUTTON_PRESS) {
            mMouseDown = true;
            mIsMouseRightDown = true;
            mIsFirstMouseMove = true;
        }
    }else if(button == Button4){
        if (action == MOUSE_BUTTON_PRESS) {
            pos += 1;            
        }
    }else if(button == Button5){
        if (action == MOUSE_BUTTON_PRESS) {
            pos -= 1;
        }
    }

    if (action == MOUSE_BUTTON_RELEASE) {
        mMouseDown = false;
        mIsMouseRightDown = false;
    }

    mCurrentCameraState->mIsViewChanged = true;
    if (mMouseDown) {
            mPendingSceneFrame = mParams.mFrameStart + (int)((float(x) / windowSize.width) * (mParams.mFrameEnd - mParams.mFrameStart + 1));
            if (mPendingSceneFrame > mParams.mFrameEnd)
                mPendingSceneFrame = mParams.mFrameEnd;
            else if (mPendingSceneFrame < mParams.mFrameStart)
                mPendingSceneFrame = mParams.mFrameStart;
            mPlaybackState = PlaybackState::STOP;
            mCurrentCameraState->mIsViewChanged = true;
    }

    const float orbitSpeed = 0.01f;
    const float strafeSpeed = 0.005f * mCurrentCameraState->mCameraDistance * tanf(mCurrentCameraState->mFovY * 0.5f * (3.142f / 180.f));

    if (mIsFirstMouseMove) {
        mMouseX = float(x);
        mMouseY = float(y);
        mIsFirstMouseMove = false;
    }

    float dx = float(x) - mMouseX;
    float dy = float(y) - mMouseY;

    if (mMouseDown) {
        if (!mIsMouseRightDown) {
            mCurrentCameraState->mCameraRotation[1] -= dx * orbitSpeed;
            mCurrentCameraState->mCameraRotation[0] += dy * orbitSpeed;

            mCurrentCameraState->mIsViewChanged = true;
        } else {
            mCurrentCameraState->mCameraLookAt = mCurrentCameraState->mCameraLookAt + (dy * mCurrentCameraState->V() + dx * mCurrentCameraState->U()) * strafeSpeed;

            mCurrentCameraState->mIsViewChanged = true;
        }
    }

    mMouseX = float(x);
    mMouseY = float(y);

    int speed = abs(mWheelPos - pos);

    if(mWheelPos >= pos){
        mCurrentCameraState->mCameraDistance += float(speed) * zoomSpeed;
    }
    else{
        mCurrentCameraState->mCameraDistance -= float(speed) * zoomSpeed;
        mCurrentCameraState->mCameraDistance = std::max(0.001f, mCurrentCameraState->mCameraDistance);
    }

    mWheelPos = pos;
    mCurrentCameraState->mIsViewChanged = true;
    mIsFirstMouseMove = false;
}

#if defined(NANOVDB_USE_IMGUI)
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
#endif

void Viewer::drawPendingGlyph()
{
#if defined(NANOVDB_USE_IMGUI)
    if (!mIsDrawingPendingGlyph)
        return;
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("##PendingMessage", &mIsDrawingPendingGlyph, windowFlags)) {
        ImGui::TextUnformatted("Loading...");
    }
    ImGui::End();
#endif
}

void Viewer::drawHelpDialog()
{
#if defined(NANOVDB_USE_IMGUI)

    if (!mIsDrawingHelpDialog)
        return;

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Appearing);
    ImGui::Begin("Help", &mIsDrawingHelpDialog, ImGuiWindowFlags_None);
    std::ostringstream ss;
    printHelp(ss);
    ImGui::TextWrapped("%s", ss.str().c_str());
    ImGui::End();
#endif
}

void Viewer::drawRenderOptionsDialog()
{
#if defined(NANOVDB_USE_IMGUI)

    bool isChanged = false;

    if (!mIsDrawingRenderOptions)
        return;

    ImGui::Begin("Render Options", &mIsDrawingRenderOptions, ImGuiWindowFlags_NoSavedSettings);

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("Render-options", tab_bar_flags)) {
        if (ImGui::BeginTabItem("Common")) 
        {
            drawRenderPlatformWidget("Platform");
            ImGui::SameLine();
            HelpMarker("The rendering platform.");

            ImGui::Separator();

            // if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     StringMap smap;

            //     static std::vector<std::string> sFileFormats{"auto", "png", "jpg", "tga", "hdr", "pfm"};
            //     smap["format"] = (mParams.mOutputExtension.empty()) ? "auto" : mParams.mOutputExtension;
            //     int fileFormat = smap.getEnum("format", sFileFormats, fileFormat);
            //     if (ImGui::Combo(
            //             "File Format", (int*)&fileFormat, [](void* data, int i, const char** outText) {
            //                 auto& v = *static_cast<std::vector<std::string>*>(data);
            //                 if (i < 0 || i >= static_cast<int>(v.size())) {
            //                     return false;
            //                 }
            //                 *outText = v[i].c_str();
            //                 return true;
            //             },
            //             static_cast<void*>(&sFileFormats),
            //             (int)sFileFormats.size())) {
            //         mParams.mOutputExtension = sFileFormats[fileFormat];
            //         if (mParams.mOutputExtension == "auto")
            //             mParams.mOutputExtension = "";
            //         isChanged |= true;
            //     }
            //     ImGui::SameLine();
            //     HelpMarker("The output file-format. Use \"auto\" to decide based on the file path extension.");

            //     static char outputStr[512] = "";
            //     if (ImGui::InputTextWithHint("File Path", mParams.mOutputFilePath.c_str(), outputStr, IM_ARRAYSIZE(outputStr))) {
            //         mParams.mOutputFilePath.assign(outputStr);
            //     }

            //     ImGui::SameLine();
            //     HelpMarker("The file path for the output file. C-style printf formatting can be used for the frame integer. e.g. \"./images/output.%04d.png\"");
            // }

            // if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     isChanged |= ImGui::InputInt("Frame start", &mParams.mFrameStart, 1);
            //     ImGui::SameLine();
            //     HelpMarker("The frame to start rendering from.");
            //     isChanged |= ImGui::InputInt("Frame end", &mParams.mFrameEnd, 1);
            //     ImGui::SameLine();
            //     HelpMarker("The inclusive frame to end rendering.");
            //     isChanged |= ImGui::DragFloat("Frame Rate (frames per second)", &mPlaybackRate, 0.1f, 0.1f, 120.0f, "%.1f");
            //     ImGui::SameLine();
            //     HelpMarker("The frame-rate for playblasting in real-time.");
            // }

            // if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     isChanged |= ImGui::Checkbox("Use Lighting", (bool*)&mParams.mSceneParameters.useLighting);
            //     ImGui::SameLine();
            //     HelpMarker("Render with a key light.");
            //     isChanged |= ImGui::Checkbox("Use Shadows", (bool*)&mParams.mSceneParameters.useShadows);
            //     ImGui::SameLine();
            //     HelpMarker("Render key light shadows.");
            // }

            // if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     isChanged |= ImGui::Combo("Lens", (int*)&mParams.mSceneParameters.camera.lensType(), kCameraLensTypeStrings, (int)Camera::LensType::kNumTypes);
            //     ImGui::SameLine();
            //     HelpMarker("The camera lens type.");
            //     if (mParams.mSceneParameters.camera.lensType() == Camera::LensType::kODS) {
            //         isChanged |= ImGui::DragFloat("IPD", &mParams.mSceneParameters.camera.ipd(), 0.1f, 0.f, 50.f, "%.2f");
            //         ImGui::SameLine();
            //         HelpMarker("The eye separation distance.");
            //     }
            //     if (mParams.mSceneParameters.camera.lensType() == Camera::LensType::kPinHole) {
            //         isChanged |= ImGui::DragFloat("Field of View", &mCurrentCameraState->mFovY, 1.0f, 1, 120, "%.0f");
            //         ImGui::SameLine();
            //         HelpMarker("The vertical field of view in degrees.");
            //     }

            //     isChanged |= ImGui::DragInt("Samples", &mParams.mSceneParameters.samplesPerPixel, 0.1f, 1, 32);
            //     ImGui::SameLine();
            //     HelpMarker("The number of camera samples per ray.");
            //     isChanged |= ImGui::Checkbox("Render Environment", (bool*)&mParams.mSceneParameters.useBackground);
            //     ImGui::SameLine();
            //     HelpMarker("Render the background environment.");
            //     isChanged |= ImGui::Checkbox("Render Ground-plane", (bool*)&mParams.mSceneParameters.useGround);
            //     ImGui::SameLine();
            //     HelpMarker("Render the ground plane.");
            //     isChanged |= ImGui::Checkbox("Render Ground-reflections", (bool*)&mParams.mSceneParameters.useGroundReflections);
            //     ImGui::SameLine();
            //     HelpMarker("Render ground reflections (work in progress).");
            //     isChanged |= ImGui::Checkbox("Turntable Camera", &mParams.mUseTurntable);
            //     ImGui::SameLine();
            //     HelpMarker("Spin the camera around the pivot each frame step.");
            //     isChanged |= ImGui::DragFloat("Turntable Inverse Rate", &mParams.mTurntableRate, 0.1f, 1.0f, 100.0f, "%.1f");
            //     ImGui::SameLine();
            //     HelpMarker("The number of frame-sequences per revolution.");
            // }

            // if (ImGui::CollapsingHeader("Tonemapping", ImGuiTreeNodeFlags_DefaultOpen)) {
            //     isChanged |= ImGui::Checkbox("Use Tonemapping", (bool*)&mParams.mSceneParameters.useTonemapping);
            //     ImGui::SameLine();
            //     HelpMarker("Use simple Reinhard tonemapping.");
            //     isChanged |= ImGui::DragFloat("WhitePoint", &mParams.mSceneParameters.tonemapWhitePoint, 0.01f, 1.0f, 20.0f);
            //     ImGui::SameLine();
            //     HelpMarker("The Reinhard tonemapping whitepoint.");
            // }

            // ImGui::Separator();

            // isChanged |= ImGui::Checkbox("Progressive", &mParams.mUseAccumulation);
            // ImGui::SameLine();
            // HelpMarker("do a progressive accumulation of the frame.");
            // isChanged |= ImGui::DragInt("Max Progressive Iterations", &mParams.mMaxProgressiveSamples, 1.f, 1, 256);
            // ImGui::SameLine();
            // HelpMarker("The maximum progressive iterations (used in batch rendering).");

            // ImGui::Separator();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    if (isChanged)
        resetAccumulationBuffer();

    ImGui::End();
#endif
}

void Viewer::drawAboutDialog()
{
#if defined(NANOVDB_USE_IMGUI)

    if (!mIsDrawingAboutDialog)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2   window_pos = ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2);
    ImVec2   window_pos_pivot = ImVec2(0.5f, 0.5f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    //ImGui::SetNextWindowBgAlpha(0.35f);

    if (!ImGui::Begin("About", &mIsDrawingAboutDialog, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Viewer\n");

    ImGui::Separator();

    ImGui::Text("Build options:\n");
    {
        ImGui::BeginChild("##build", ImVec2(0, 60), true);
#ifdef NANOVDB_USE_BLOSC
        ImGui::BulletText("BLOSC");
#endif
#ifdef NANOVDB_USE_OPENVDB
        ImGui::BulletText("OpenVDB");
#endif
#ifdef NANOVDB_USE_ZIP
        ImGui::BulletText("ZLIB");
#endif
#ifdef NANOVDB_USE_TBB
        ImGui::BulletText("Intel TBB");
#endif
        ImGui::BulletText("NVIDIA CUDA");
#ifdef NANOVDB_USE_OPENCL
        ImGui::BulletText("OpenCL");
#endif
#ifdef NANOVDB_USE_NFD
        ImGui::BulletText("Native File Dialog");
#endif
#ifdef NANOVDB_USE_CURL
        ImGui::BulletText("libCURL");
#endif
        ImGui::EndChild();
    }

    ImGui::Separator();

    ImGui::Text("Supported render-platforms:\n");
    {
        ImGui::BeginChild("##platforms", ImVec2(0, 60), true);
        for (auto& it : mRenderLauncher.getPlatformNames())
            ImGui::BulletText("%s\n", it.c_str());
        ImGui::EndChild();
    }

    ImGui::End();
#else
    (void)mIsDrawingAboutDialog;
#endif
}

void Viewer::drawRenderPlatformWidget(const char* label)
{
#if defined(NANOVDB_USE_IMGUI)
    auto platforms = mRenderLauncher.getPlatformNames();

    static char comboStr[1024];
    char*       comboStrPtr = comboStr;
    for (auto& it : platforms) {
        strncpy(comboStrPtr, it.c_str(), it.length());
        comboStrPtr += it.length();
        *(comboStrPtr++) = 0;
    }
    *(comboStrPtr++) = 0;

    if (ImGui::Combo(label, &mParams.mRenderLauncherType, comboStr))
        setRenderPlatform(mParams.mRenderLauncherType);
#else
    (void)label;
#endif
}

bool Viewer::drawMaterialGridAttachment(SceneNode::Ptr node, int attachmentIndex)
{
#if defined(NANOVDB_USE_IMGUI)
    bool isChanged = false;
    char buf[1024];
    auto attachment = node->mAttachments[attachmentIndex];
    auto assetUrl = attachment->mAssetUrl.fullname();

    std::memcpy(buf, assetUrl.c_str(), assetUrl.length());
    buf[assetUrl.length()] = 0;

    ImGui::PushID(attachmentIndex);
    if (ImGui::InputText("##grid-value", buf, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
        setSceneNodeGridAttachment(node->mName, attachmentIndex, GridAssetUrl(buf));
        isChanged = true;
    }

    if (ImGui::BeginPopupContextItem("context")) {
        if (ImGui::Button("Clear")) {
            setSceneNodeGridAttachment(node->mName, attachmentIndex, GridAssetUrl());
            isChanged = true;
        }
        ImGui::EndPopup();
    }

    ImGui::PopID();

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GRIDASSETURL");
        if (payload) {
            std::string s((const char*)(payload->Data), payload->DataSize);
            setSceneNodeGridAttachment(node->mName, attachmentIndex, GridAssetUrl(s.c_str()));
            isChanged = true;
        }
        ImGui::EndDragDropTarget();
    }

    return isChanged;
#else
    (void)node;
    (void)attachmentIndex;
    return false;
#endif
}

bool Viewer::drawPointRenderOptionsWidget(SceneNode::Ptr node, int attachmentIndex)
{
#if defined(NANOVDB_USE_IMGUI)
    bool isChanged = false;

    static std::vector<std::string> semanticNames;
    if (semanticNames.empty()) {
        for (int i = 1; i < (int)nanovdb::GridBlindDataSemantic::End; ++i)
            semanticNames.push_back(getStringForBlindDataSemantic(nanovdb::GridBlindDataSemantic(i)));
    }

    auto attachment = node->mAttachments[attachmentIndex];

    auto* gridHdl = std::get<1>(mGridManager.getGrid(attachment->mFrameUrl, attachment->mAssetUrl.gridName())).get();
    if (!gridHdl) {
        ImGui::TextUnformatted("ERROR: Grid not resident.");
    } else if (gridHdl->gridMetaData()->isPointData() == false) {
        ImGui::TextUnformatted("ERROR: Grid class must be PointData or PointIndex.");
    } else {
        auto grid = gridHdl->grid<uint32_t>();
        assert(grid);

        std::vector<std::string> attributeNames;
        int                      n = grid->blindDataCount();

        std::vector<nanovdb::GridBlindMetaData> attributeMeta;
        attributeMeta.push_back(nanovdb::GridBlindMetaData{});
        attributeNames.push_back("None");
        for (int i = 0; i < n; ++i) {
            auto meta = grid->blindMetaData(i);
            attributeMeta.push_back(meta);
            attributeNames.push_back(meta.mName + std::string(" (") + nanovdb::toStr(meta.mDataType) + std::string(")"));
        }

        static auto vector_getter = [](void* vec, int idx, const char** out_text) {
            auto& vector = *static_cast<std::vector<std::string>*>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size())) {
                return false;
            }
            *out_text = vector.at(idx).c_str();
            return true;
        };

        int w = ImGui::GetColumnWidth(1);
        ImGui::BeginChild("left pane", ImVec2((float)w, 100.f), false, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f));
        ImGui::Columns(2);

        ImGui::AlignTextToFramePadding();

        int        attributeIndex = 0;
        static int semanticIndex = 0;
        // Left
        {
            for (int i = 0; i < semanticNames.size(); i++) {
                if (ImGui::Selectable(semanticNames[i].c_str(), semanticIndex == i))
                    semanticIndex = i;
            }
        }
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);

        // Right
        {
            attributeIndex = attachment->attributeSemanticMap[semanticIndex + 1].attribute + 1; // add one as item[0] is "None"
            isChanged |= ImGui::Combo("Attribute", &attributeIndex, vector_getter, static_cast<void*>(&attributeNames), (int)attributeNames.size());
            isChanged |= ImGui::DragFloat("Offset", &attachment->attributeSemanticMap[semanticIndex + 1].offset, 0.01f, 0.0f, 1.0f);
            isChanged |= ImGui::DragFloat("Gain", &attachment->attributeSemanticMap[semanticIndex + 1].gain, 0.01f, 0.0f, 1.0f);
        }

        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
        ImGui::EndChild();

        if (isChanged) {
            attachment->attributeSemanticMap[semanticIndex + 1].attribute = attributeIndex - 1; // minus one as item[0] is "None"
        }
    }
    return isChanged;
#else
    (void)node;
    (void)attachmentIndex;
    return false;
#endif
}

bool Viewer::drawMaterialParameters(SceneNode::Ptr node, MaterialClass mat)
{
#if defined(NANOVDB_USE_IMGUI)
    bool isChanged = false;

    ImGui::AlignTextToFramePadding();

    auto& params = node->mMaterialParameters;
    if (mat == MaterialClass::kAuto) {
        ImGui::BulletText("Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\nFormat is <scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

    } else if (mat == MaterialClass::kGrid) {
        ImGui::BulletText("Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

    } else if (mat == MaterialClass::kVoxels) {
        ImGui::BulletText("Voxels");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

        ImGui::BulletText("Density");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##density-value", &params.volumeDensityScale, 0.01f);
        ImGui::NextColumn();

        ImGui::BulletText("Voxel Geometry");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##voxelGeometry", (int*)&params.voxelGeometry, "Cube\0Sphere\0\0");
        ImGui::NextColumn();

    } else if (mat == MaterialClass::kLevelSetFast) {
        ImGui::BulletText("LevelSet Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();
        /*
        // TODO:
        ImGui::BulletText("Grid Interpolation Order");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##interpolationOrder-value", (int*)&params.interpolationOrder, "Nearest\0Linear\0\0");
        ImGui::NextColumn();
        */
        /*
        // TODO:
        ImGui::BulletText("Use Occlusion");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##useOcclusion-value", &params.useOcclusion, 0.01f, 0, 1);
        ImGui::NextColumn();*/

    } else if (mat == MaterialClass::kPointsFast) {
        ImGui::BulletText("Point Index Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

        ImGui::BulletText("Density");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##density-value", &params.volumeDensityScale, 0.01f);
        ImGui::NextColumn();

        ImGui::BulletText("Radius");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##radius", &node->mAttachments[0]->attributeSemanticMap[(int)nanovdb::GridBlindDataSemantic::PointRadius].offset, 0.01f);
        ImGui::NextColumn();

        ImGui::BulletText("Attributes");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawPointRenderOptionsWidget(node, 0);
        ImGui::NextColumn();
    } else if (mat == MaterialClass::kFogVolumePathTracer || mat == MaterialClass::kBlackBodyVolumePathTracer) {
        ImGui::BulletText("Density Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

        ImGui::BulletText("Grid Interpolation Order");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##interpolationOrder-value", (int*)&params.interpolationOrder, "Nearest\0Linear\0\0");
        ImGui::NextColumn();

        ImGui::BulletText("Density Scale");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##volumeDensityScale-value", &params.volumeDensityScale, 0.01f, 0, 10);
        ImGui::NextColumn();

        ImGui::BulletText("Albedo");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##volumeAlbedo-value", &params.volumeAlbedo, 0.01f, 0.0f, 1.0f);
        ImGui::NextColumn();
        /*
        // TODO: outstanding CUDA hang with non-zero phase.
        // presumably invalid floating-point numbers or divide-by-zero.
        ImGui::BulletText("Phase");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##phase-value", &params.phase, 0.01f, -1, 1);
        ImGui::NextColumn();
        */
        ImGui::BulletText("Transmittance Method");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##transmittancemethod-value", (int*)&params.transmittanceMethod, "ReimannSum\0DeltaTracking\0RatioTracking\0ResidualRatioTracking\0\0");
        ImGui::NextColumn();

    } else if (mat == MaterialClass::kFogVolumeFast) {
        ImGui::BulletText("Density Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 0);
        ImGui::NextColumn();

        ImGui::BulletText("Grid Interpolation Order");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##interpolationOrder-value", (int*)&params.interpolationOrder, "Nearest\0Linear\0\0");
        ImGui::NextColumn();

        ImGui::BulletText("Extinction");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##extinction-value", &params.volumeDensityScale, 0.01f, 0, 10);
        ImGui::NextColumn();

        ImGui::BulletText("Transmittance Method");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::Combo("##transmittancemethod-value", (int*)&params.transmittanceMethod, "ReimannSum\0DeltaTracking\0RatioTracking\0ResidualRatioTracking\0\0");
        ImGui::NextColumn();
    }

    if (mat == MaterialClass::kBlackBodyVolumePathTracer) {
        ImGui::BulletText("Temperature Grid");
        ImGui::SameLine();
        HelpMarker("The grid URL.\n(<scheme>://<path>#<gridName><sequence>)\nwhere optional <sequence> is [<start>-<end>]\ne.g. file://explosion.%d.vdb#density[0-100]");

        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= drawMaterialGridAttachment(node, 1);
        ImGui::NextColumn();

        ImGui::BulletText("Temperature Scale");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::DragFloat("##tempscale-value", &params.volumeTemperatureScale, 0.01f, 0, 10);
        ImGui::NextColumn();
    }

    if (mat != MaterialClass::kAuto) {
        /*
        ImGui::BulletText("maxPathDepth");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1);
        isChanged |= ImGui::InputInt("##maxPathDepth", &params.maxPathDepth, 1, 10);
        ImGui::NextColumn();*/
    }

    return isChanged;
#else
    (void)node;
    (void)mat;
    return false;
#endif
}

void Viewer::drawSceneGraphNodes()
{
#if defined(NANOVDB_USE_IMGUI)
    bool             isChanged = false;
    std::vector<int> deleteRequests;
    bool             openCreateNewNodePopup = false;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

    if (ImGui::Button("Add...")) {
        openCreateNewNodePopup = true;
    }
    ImGui::SameLine();
    HelpMarker("Add an empty node");

    ImGui::Separator();

    ImGui::Columns(2);

    for (int i = 0; i < mSceneNodes.size(); i++) {
        auto node = mSceneNodes[i];

        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();

        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
        if (node->mIndex == mSelectedSceneNodeIndex)
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

        std::stringstream label;
        label << node->mName;

        bool nodeOpen = ImGui::TreeNodeEx(label.str().c_str(), treeNodeFlags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            selectSceneNodeByIndex(i);
        }

        if (ImGui::BeginPopupContextItem("node context")) {
            if (ImGui::Button("Delete...")) {
                deleteRequests.push_back(i);
            }
            ImGui::EndPopup();
        }

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        isChanged |= ImGui::Combo("##Material", (int*)&node->mMaterialClass, kMaterialClassTypeStrings, (int)MaterialClass::kNumTypes);
        ImGui::NextColumn();

        if (nodeOpen) {
            auto attachment = node->mAttachments[0];
            auto materialClass = node->mMaterialClass;
            if (materialClass == MaterialClass::kAuto) {
                if (attachment->mGridClassOverride == nanovdb::GridClass::FogVolume)
                    materialClass = MaterialClass::kFogVolumePathTracer;
                else if (attachment->mGridClassOverride == nanovdb::GridClass::LevelSet)
                    materialClass = MaterialClass::kLevelSetFast;
                else if (attachment->mGridClassOverride == nanovdb::GridClass::PointData)
                    materialClass = MaterialClass::kPointsFast;
                else if (attachment->mGridClassOverride == nanovdb::GridClass::PointIndex)
                    materialClass = MaterialClass::kPointsFast;
                else if (attachment->mGridClassOverride == nanovdb::GridClass::VoxelVolume)
                    materialClass = MaterialClass::kVoxels;
                else
                    materialClass = MaterialClass::kGrid;
            }

            isChanged |= drawMaterialParameters(node, materialClass);
            ImGui::Separator();
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::PopStyleVar();

    removeSceneNodes(deleteRequests);

    if (isChanged) {
        resetAccumulationBuffer();
    }

    if (openCreateNewNodePopup) {
        ImGui::OpenPopup("Create New Node");
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Create New Node", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char nameBuf[256] = {0};

        ImGui::LabelText("##Name", "Name");
        ImGui::SameLine();
        ImGui::InputText("##NameText", nameBuf, 256);
        ImGui::Separator();
        if (ImGui::Button("Ok")) {
            auto nodeId = addSceneNode(std::string(nameBuf));
            selectSceneNodeByIndex(findNode(nodeId)->mIndex);

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
#else
    return;
#endif
}

void Viewer::drawSceneGraph()
{
    if (!mIsDrawingSceneGraph)
        return;

#if defined(NANOVDB_USE_IMGUI)
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene", &mIsDrawingSceneGraph, window_flags)) {
        drawSceneGraphNodes();
    }
    ImGui::End();
#endif
}

void Viewer::drawGridInfo(const std::string& url, const std::string& gridName)
{
#if defined(NANOVDB_USE_IMGUI)
    nanovdb::GridHandle<>* gridHdl = std::get<1>(mGridManager.getGrid(url, gridName)).get();
    if (gridHdl) {
        auto meta = gridHdl->gridMetaData();
        auto bbox = meta->worldBBox();
        auto bboxMin = bbox.min();
        auto bboxMax = bbox.max();
        auto iBBox = meta->indexBBox();
        auto effectiveSize = iBBox.max() - iBBox.min() + nanovdb::Coord(1);

        ImGui::Text("Effective res:");
        ImGui::SameLine(150);
        ImGui::Text("%dx%dx%d", effectiveSize[0], effectiveSize[1], effectiveSize[2]);
        ImGui::Text("BBox-min:");
        ImGui::SameLine(150);
        ImGui::Text("(%.2f,%.2f,%.2f)", bboxMin[0], bboxMin[1], bboxMin[2]);
        ImGui::Text("BBox-max:");
        ImGui::SameLine(150);
        ImGui::Text("(%.2f,%.2f,%.2f)", bboxMax[0], bboxMax[1], bboxMax[2]);
        ImGui::Text("Class(Type):");
        ImGui::SameLine(150);
        ImGui::Text("%s(%s)", nanovdb::toStr(meta->gridClass()), nanovdb::toStr(meta->gridType()));
        if (meta->gridClass() == nanovdb::GridClass::PointData || meta->gridClass() == nanovdb::GridClass::PointIndex) {
            ImGui::Text("Point count:");
            ImGui::SameLine(150);
            ImGui::Text("%" PRIu64, (meta->blindDataCount() > 0) ? meta->blindMetaData(0).mElementCount : 0);
        } else {
            ImGui::Text("Voxel count:");
            ImGui::SameLine(150);
            ImGui::Text("%" PRIu64, meta->activeVoxelCount());
        }
        ImGui::Text("Size:");
        ImGui::SameLine(150);
        ImGui::Text("%" PRIu64 " MB", meta->gridSize());
    } else {
        ImGui::TextUnformatted("Loading...");
    }
#else
    (void)url;
    (void)gridName;
    return;
#endif
}

void Viewer::drawAssets()
{
    if (!mIsDrawingAssets)
        return;

#if defined(NANOVDB_USE_IMGUI)
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Assets", &mIsDrawingAssets, window_flags)) {
        // get the resident assets and grids.
        auto residentAssetMap = mGridManager.getGridNameStatusInfo();

        bool showErroredAssets = false;

        for (auto& assetInfo : residentAssetMap) {
            auto assetUrl = assetInfo.first;
            auto statusAndGridInfos = assetInfo.second;
            auto assetHasError = statusAndGridInfos.first;
            if (!showErroredAssets && assetHasError) {
                continue;
            }

            std::stringstream itemName;
            itemName << assetUrl;
            if (assetHasError)
                itemName << " (error)";

            ImGui::PushID(assetUrl.c_str());

            if (ImGui::TreeNodeEx(itemName.str().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& assetGridInfo : statusAndGridInfos.second) {
                    auto assetGridName = assetGridInfo.first;
                    auto assetGridStatus = assetGridInfo.second;

                    std::ostringstream ss;
                    ss << assetGridName;
                    if (assetGridStatus == GridManager::AssetGridStatus::kPending)
                        ss << " (pending)";
                    else if (assetGridStatus == GridManager::AssetGridStatus::kError)
                        ss << " (error)";
                    else if (assetGridStatus == GridManager::AssetGridStatus::kLoaded)
                        ss << " (loaded)";

                    auto gridAssetUrl = assetUrl + "#" + assetGridName;

                    ImGui::PushID(gridAssetUrl.c_str());

                    ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_OpenOnArrow;
                    bool               nodeOpen = ImGui::TreeNodeEx(ss.str().c_str(), treeNodeFlags);

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload("GRIDASSETURL", gridAssetUrl.c_str(), gridAssetUrl.length(), ImGuiCond_Once);
                        ImGui::TextUnformatted(gridAssetUrl.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (nodeOpen) {
                        drawGridInfo(assetUrl, assetGridName);
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        }
        ImGui::End();
    }
#else
    return;
#endif
}

void Viewer::drawEventLog()
{
    if (!mIsDrawingEventLog)
        return;

#if defined(NANOVDB_USE_IMGUI)

    ImGui::SetNextWindowSize(ImVec2(800, 100), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Log", &mIsDrawingEventLog, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    bool doClear = ImGui::Button("Clear");
    ImGui::SameLine();
    bool doCopy = ImGui::Button("Copy");

    ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (doClear)
        mEventMessages.clear();
    if (doCopy)
        ImGui::LogToClipboard();

    for (auto& eventMsg : mEventMessages) {
        if (eventMsg.mType == GridManager::EventMessage::Type::kError)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "[ERR]");
        else if (eventMsg.mType == GridManager::EventMessage::Type::kWarning)
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[WRN]");
        else if (eventMsg.mType == GridManager::EventMessage::Type::kDebug)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "[DBG]");
        else
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "[INF]");
        ImGui::SameLine();

        ImGui::TextUnformatted(eventMsg.mMessage.c_str());
    }
    if (mLogAutoScroll) {
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
#else
    (void)mLogAutoScroll;
    return;
#endif
}

void Viewer::drawRenderStatsOverlay()
{
    if (!mIsDrawingRenderStats)
        return;

#if defined(NANOVDB_USE_IMGUI)
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    auto viewPos = ImGui::GetWindowPos();
    auto viewSize = ImGui::GetWindowSize();
#endif
    ImVec2 center = ImVec2(viewPos.x + viewSize.x / 2, viewPos.y + viewSize.y / 2);

#if 1
    const float DISTANCE = 10.0f;
    ImGui::SetNextWindowPos(ImVec2(viewPos.x + viewSize.x - DISTANCE, viewPos.y + DISTANCE + 16), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
#endif

    ImGui::SetNextWindowSize(ImVec2(200, 0), ImGuiCond_Always);
    if (ImGui::Begin("Render Stats:", &mIsDrawingRenderStats, window_flags)) {
        ImGui::Text("Frame (%d - %d): %d", mParams.mFrameStart, mParams.mFrameEnd, mLastSceneFrame);
        ImGui::Text("FPS: %d (%s)", mFps, mRenderLauncher.name().c_str());
    }
    ImGui::End();
}

std::string Viewer::getScreenShotFilename(int iteration) const
{
    std::string screenShotFilename = "screenshot-" + std::to_string((int)time(NULL)) + "-" + std::to_string(iteration) + ".png";
    return screenShotFilename;
}

void Viewer::drawMenuBar()
{
#if defined(NANOVDB_USE_IMGUI)

    bool openLoadURL = false;

    if (ImGui::BeginMainMenuBar()) {
        
        if (ImGui::BeginMenu("View")) 
        {
            if (ImGui::MenuItem("Show Log", NULL, mIsDrawingEventLog, true)) {
                mIsDrawingEventLog = !mIsDrawingEventLog;
            }
            if (ImGui::MenuItem("Show Scene Graph", NULL, mIsDrawingSceneGraph, true)) {
                mIsDrawingSceneGraph = !mIsDrawingSceneGraph;
            }
            if (ImGui::MenuItem("Show Assets", NULL, mIsDrawingAssets, true)) {
                mIsDrawingAssets = !mIsDrawingAssets;
            }
            if (ImGui::MenuItem("Show Render Stats", NULL, mIsDrawingRenderStats, true)) {
                mIsDrawingRenderStats = !mIsDrawingRenderStats;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Scene")) 
        {
            if (ImGui::MenuItem("Add Empty Node")) {
                auto nodeId = addSceneNode();
                selectSceneNodeByIndex(findNode(nodeId)->mIndex);
            }

            if (ImGui::BeginMenu("Add Primitive Node")) {
                static StringMap urls;
                urls["fog_sphere"] = "internal://#fog_sphere_100";
                urls["fog_torus"] = "internal://#fog_torus_100";
                urls["fog_box"] = "internal://#fog_box_100";
                urls["fog_octahedron"] = "internal://#fog_octahedron_100";

                for (auto& it : urls) {
                    if (ImGui::MenuItem(it.first.c_str())) {
                        auto nodeId = addSceneNode(it.first);
                        setSceneNodeGridAttachment(nodeId, 0, it.second);
                        selectSceneNodeByIndex(findNode(nodeId)->mIndex);
                        addGridAsset(it.second);
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Play", "Enter", false, mPlaybackState == PlaybackState::STOP)) {
                mPlaybackState = PlaybackState::PLAY;
            }
            if (ImGui::MenuItem("Stop", "Enter", false, mPlaybackState == PlaybackState::PLAY)) {
                mPlaybackState = PlaybackState::STOP;
            }
            if (ImGui::MenuItem("Play from start", "Ctrl(Enter)", false)) {
                mPendingSceneFrame = mParams.mFrameStart;
                mPlaybackState = PlaybackState::PLAY;
                mPlaybackLastTime = (float)getTime();
                mPlaybackTime = 0;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Render")) 
        {
            if (ImGui::MenuItem("Options...")) {
                mIsDrawingRenderOptions = true;
            }
            ImGui::EndMenu();
        }

        // if (ImGui::BeginMenu("Help")) {
        //     if (ImGui::MenuItem("View Help"))
        //         mIsDrawingHelpDialog = true;
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("About"))
        //         mIsDrawingAboutDialog = true;
        //     ImGui::EndMenu();
        // }

        ImGui::EndMainMenuBar();
    }
#else
    return;
#endif
}