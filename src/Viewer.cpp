#include <imgui/imgui_impl_x11.h>
#define NANOVDB_USE_IMGUI

#include <cinttypes> // for PRIu64
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>


#include "../include/Viewer.h"
#include "../include/RenderLauncher.h"

#include "../include/FrameBufferHost.h"
#include "../include/FrameBufferGL.h"
#include "../include/StringUtils.h"

#include <nanovdb/util/IO.h> // for NanoVDB file import

#if defined(NANOVDB_USE_OPENVDB)
#include <nanovdb/util/OpenToNanoVDB.h>
#endif

void keyboard(glwindow* window, int key) {

}

void mouse(glwindow* window, int button, int action, int x, int y) {
	if(button == Button1 && action == MOUSE_BUTTON_PRESS) {
	}

    if(button == Button1 && action == MOUSE_BUTTON_RELEASE) {
	}
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
{}

void Viewer::close()
{

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplX11_Shutdown();
    ImGui::DestroyContext();

    delete window;
}

void Viewer::updateWindowTitle()
{
    if (!mWindow)
        return;

    std::ostringstream ss;
    ss << "NanoVDB Viewer: " << mRenderLauncher.name() << " @ " << mFps << " fps";
    if (mSelectedSceneNodeIndex >= 0) {
        ss << " - ";
        ss << mSceneNodes[mSelectedSceneNodeIndex]->mName;
        ss << '[' << mSceneNodes[mSelectedSceneNodeIndex]->mAttachments[0]->mAssetUrl.fullname() << ']';
    }
    //glfwSetWindowTitle((GLFWwindow*)mWindow, ss.str().c_str());
}

double Viewer::getTime()
{
    return window->getTime();
}

void Viewer::open()
{
    window = new glwindow("HPP Demo",0,0,mParams.mWidth,mParams.mHeight,460);

    window->setFullscreen(false);
    window->setKeyboardFunc(keyboard);
    window->setMouseFunc(mouse);

    mFrameBuffer.reset(new FrameBufferGL(window->getDisplay(), window->getWindow()));
    resizeFrameBuffer(mParams.mWidth, mParams.mHeight);
    window->setGUISupport(true);

    NANOVDB_GL_CHECKERRORS();
}

void Viewer::mainLoop(void* userData)
{
    auto viewerPtr = reinterpret_cast<Viewer*>(userData);

    bool rc = viewerPtr->runLoop();
    if (rc == false) {
    }
}

bool Viewer::runLoop()
{
    // updateAnimationControl();

    // mIsDrawingPendingGlyph = false;
    // if (mSelectedSceneNodeIndex >= 0) {
    //     updateNodeAttachmentRequests(mSceneNodes[mSelectedSceneNodeIndex], false, mIsDumpingLog, &mIsDrawingPendingGlyph);
    // }

    // updateScene();

    // render(getSceneFrame());
    // renderViewOverlay();

    // // workaround for bad GL state in ImGui...
    // glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    // glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    // NANOVDB_GL_CHECKERRORS();

    // ImGui_ImplOpenGL3_NewFrame();
    // ImGui_ImplGlfw_NewFrame();
    // ImGui::NewFrame();
    // ImGuiIO& io = ImGui::GetIO();

    // drawMenuBar();
    // drawSceneGraph();
    // drawRenderOptionsDialog();
    // drawRenderStatsOverlay();
    // drawAboutDialog();
    // drawHelpDialog();
    // drawEventLog();
    // drawAssets();
    // drawPendingGlyph();

    // ImGui::Render();

    // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // ImGui::EndFrame();

    // // eval fps...
    // ++mFpsFrame;
    // double elapsed = glfwGetTime() - mTime;
    // if (elapsed > 1.0) {
    //     mTime = glfwGetTime();
    //     mFps = (int)(double(mFpsFrame) / elapsed);
    //     updateWindowTitle();
    //     mFpsFrame = 0;
    // }

    // window->swapBuffers();
}

void Viewer::run()
{
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glDepthFunc(GL_LESS);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mFpsFrame = 0;
    mTime = window->getTime();

    while (!window->isClosed()) {
        
        window->processEvents();

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
        double elapsed = window->getTime() - mTime;
        if (elapsed > 1.0) {
            mTime = window->getTime();
            mFps = (int)(double(mFpsFrame) / elapsed);
            updateWindowTitle();
            mFpsFrame = 0;
        }
        window->swapBuffers();
    }
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
    if (mWindow == nullptr)
        return false;

    if (RendererBase::render(frame) == false) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return true;
    } else {
        mFrameBuffer->render(0, 0, mFrameBuffer->width(), mFrameBuffer->height());
        return true;
    }
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

    // if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_D) == GLFW_PRESS) {
    //     // move the light source to the camera direction.
    //     mParams.mSceneParameters.sunDirection = -mCurrentCameraState->W().normalize();
    //     isChanged = true;
    // }

    return isChanged;
}

void Viewer::onKey(int key, int action)
{
#if defined(NANOVDB_USE_IMGUI)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return;
#endif

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE || key == 'Q') {
            glfwSetWindowShouldClose((GLFWwindow*)mWindow, true);
        } else if (key == GLFW_KEY_PRINT_SCREEN || (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_LEFT_CONTROL) && key == 'P')) {
            //saveFrameBuffer(getSceneFrame(), getScreenShotFilename(mScreenShotIteration), "png");
        } else if (key == 'H') {
            mIsDrawingHelpDialog = !mIsDrawingHelpDialog;
#if !defined(NANOVDB_USE_IMGUI)
            printHelp(std::cout);
#endif
        } else if (key == 'F') {
            bool isFramingSceneNode = (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_LEFT_CONTROL) > 0);
            resetCamera(isFramingSceneNode);
            resetAccumulationBuffer();
        } else if (key == '`') {
            mIsDrawingSceneGraph = !mIsDrawingSceneGraph;
        } else if (key == 'B') {
            mParams.mSceneParameters.useBackground = (mParams.mSceneParameters.useBackground > 0) ? 0 : 1;
            resetAccumulationBuffer();
        } else if (key == 'S') {
            mParams.mSceneParameters.useShadows = (mParams.mSceneParameters.useShadows > 0) ? 0 : 1;
            resetAccumulationBuffer();
        } else if (key == 'G') {
            mParams.mSceneParameters.useGround = (mParams.mSceneParameters.useGround > 0) ? 0 : 1;
            resetAccumulationBuffer();
        } else if (key == 'L') {
            mParams.mSceneParameters.useLighting = (mParams.mSceneParameters.useLighting > 0) ? 0 : 1;
            resetAccumulationBuffer();
        } else if (key == 'P') {
            mParams.mUseAccumulation = !mParams.mUseAccumulation;
            resetAccumulationBuffer();
        } else if (key == GLFW_KEY_COMMA) {
            if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_LEFT_CONTROL)) {
                setSceneFrame(mParams.mFrameStart);
            } else {
                setSceneFrame(mLastSceneFrame - 1);
            }
        } else if (key == GLFW_KEY_PERIOD) {
            if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_LEFT_CONTROL)) {
                setSceneFrame(mParams.mFrameEnd);
            } else {
                setSceneFrame(mLastSceneFrame + 1);
            }
        } else if (key == GLFW_KEY_ENTER) {
            if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_LEFT_CONTROL)) {
                setSceneFrame(mParams.mFrameStart);
                mPlaybackState = PlaybackState::PLAY;
            } else {
                if (mPlaybackState == PlaybackState::PLAY) {
                    mPlaybackState = PlaybackState::STOP;
                } else {
                    mPlaybackLastTime = (float)getTime();
                    mPlaybackState = PlaybackState::PLAY;
                }
            }
        } else if (key == 'T') {
            mParams.mUseTurntable = !mParams.mUseTurntable;
            resetAccumulationBuffer();
        } else if (key == GLFW_KEY_MINUS) {
            selectSceneNodeByIndex(mSelectedSceneNodeIndex - 1);
            updateWindowTitle();
            mFps = 0;
        } else if (key == GLFW_KEY_EQUAL) {
            selectSceneNodeByIndex(mSelectedSceneNodeIndex + 1);
            updateWindowTitle();
            mFps = 0;
        } else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            setRenderPlatform((key - GLFW_KEY_1));
            updateWindowTitle();
            mFps = 0;
        }
    }
}

void Viewer::onMouseButton(int button, int action)
{
#if defined(NANOVDB_USE_IMGUI)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
#endif

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mMouseDown = true;
            mIsFirstMouseMove = true;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            mMouseDown = true;
            mIsMouseRightDown = true;
            mIsFirstMouseMove = true;
        }
    }

    if (action == GLFW_RELEASE) {
        mMouseDown = false;
        mIsMouseRightDown = false;
    }

    mCurrentCameraState->mIsViewChanged = true;

    if (mMouseDown) {
        if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_TAB) == GLFW_PRESS) {
            double xpos, ypos;
            glfwGetCursorPos((GLFWwindow*)mWindow, &xpos, &ypos);
            int x = int(xpos);
            //int y = int(ypos);
            mPendingSceneFrame = mParams.mFrameStart + (int)((float(x) / mWindowWidth) * (mParams.mFrameEnd - mParams.mFrameStart + 1));
            if (mPendingSceneFrame > mParams.mFrameEnd)
                mPendingSceneFrame = mParams.mFrameEnd;
            else if (mPendingSceneFrame < mParams.mFrameStart)
                mPendingSceneFrame = mParams.mFrameStart;
            mPlaybackState = PlaybackState::STOP;
            mCurrentCameraState->mIsViewChanged = true;
        }
    }
}

void Viewer::onMouseMove(int x, int y)
{
#if defined(NANOVDB_USE_IMGUI)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
#endif

    if (mMouseDown) {
        if (glfwGetKey((GLFWwindow*)mWindow, GLFW_KEY_TAB) == GLFW_PRESS) {
            mPendingSceneFrame = mParams.mFrameStart + (int)((float(x) / mWindowWidth) * (mParams.mFrameEnd - mParams.mFrameStart + 1));
            if (mPendingSceneFrame > mParams.mFrameEnd)
                mPendingSceneFrame = mParams.mFrameEnd;
            else if (mPendingSceneFrame < mParams.mFrameStart)
                mPendingSceneFrame = mParams.mFrameStart;
            mPlaybackState = PlaybackState::STOP;
            mCurrentCameraState->mIsViewChanged = true;
            return;
        }
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
            // orbit...
            mCurrentCameraState->mCameraRotation[1] -= dx * orbitSpeed;
            mCurrentCameraState->mCameraRotation[0] += dy * orbitSpeed;

            mCurrentCameraState->mIsViewChanged = true;
        } else {
            // strafe...
            mCurrentCameraState->mCameraLookAt = mCurrentCameraState->mCameraLookAt + (dy * mCurrentCameraState->V() + dx * mCurrentCameraState->U()) * strafeSpeed;

            mCurrentCameraState->mIsViewChanged = true;
        }
    }

    mMouseX = float(x);
    mMouseY = float(y);

    //printf("mouse(%f, %f)\n", mMouseX, mMouseY);
}

void Viewer::onMouseWheel(int pos)
{
#if defined(NANOVDB_USE_IMGUI)
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;
#endif

    const float zoomSpeed = mCurrentCameraState->mCameraDistance * 0.1f;
    pos += mWheelPos;

    int speed = abs(mWheelPos - pos);

    if (mWheelPos >= pos) {
        mCurrentCameraState->mCameraDistance += float(speed) * zoomSpeed;
    } else {
        mCurrentCameraState->mCameraDistance -= float(speed) * zoomSpeed;
        mCurrentCameraState->mCameraDistance = std::max(0.001f, mCurrentCameraState->mCameraDistance);
    }

    mWheelPos = pos;
    mCurrentCameraState->mIsViewChanged = true;
    mIsFirstMouseMove = false;
}

void Viewer::onResize(int width, int height)
{
    mWindowWidth = width;
    mWindowHeight = height;

    resizeFrameBuffer(width, height);
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
        if (ImGui::BeginTabItem("Common")) {
            drawRenderPlatformWidget("Platform");
            ImGui::SameLine();
            HelpMarker("The rendering platform.");

            ImGui::Separator();

            if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen)) {
                StringMap smap;

                static std::vector<std::string> sFileFormats{"auto", "png", "jpg", "tga", "hdr", "pfm"};
                smap["format"] = (mParams.mOutputExtension.empty()) ? "auto" : mParams.mOutputExtension;
                int fileFormat = smap.getEnum("format", sFileFormats, fileFormat);
                if (ImGui::Combo(
                        "File Format", (int*)&fileFormat, [](void* data, int i, const char** outText) {
                            auto& v = *static_cast<std::vector<std::string>*>(data);
                            if (i < 0 || i >= static_cast<int>(v.size())) {
                                return false;
                            }
                            *outText = v[i].c_str();
                            return true;
                        },
                        static_cast<void*>(&sFileFormats),
                        (int)sFileFormats.size())) {
                    mParams.mOutputExtension = sFileFormats[fileFormat];
                    if (mParams.mOutputExtension == "auto")
                        mParams.mOutputExtension = "";
                    isChanged |= true;
                }
                ImGui::SameLine();
                HelpMarker("The output file-format. Use \"auto\" to decide based on the file path extension.");

                static char outputStr[512] = "";
                if (ImGui::InputTextWithHint("File Path", mParams.mOutputFilePath.c_str(), outputStr, IM_ARRAYSIZE(outputStr))) {
                    mParams.mOutputFilePath.assign(outputStr);
                }

                ImGui::SameLine();
                HelpMarker("The file path for the output file. C-style printf formatting can be used for the frame integer. e.g. \"./images/output.%04d.png\"");
            }

            if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
                isChanged |= ImGui::InputInt("Frame start", &mParams.mFrameStart, 1);
                ImGui::SameLine();
                HelpMarker("The frame to start rendering from.");
                isChanged |= ImGui::InputInt("Frame end", &mParams.mFrameEnd, 1);
                ImGui::SameLine();
                HelpMarker("The inclusive frame to end rendering.");
                isChanged |= ImGui::DragFloat("Frame Rate (frames per second)", &mPlaybackRate, 0.1f, 0.1f, 120.0f, "%.1f");
                ImGui::SameLine();
                HelpMarker("The frame-rate for playblasting in real-time.");
            }

            if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
                isChanged |= ImGui::Checkbox("Use Lighting", (bool*)&mParams.mSceneParameters.useLighting);
                ImGui::SameLine();
                HelpMarker("Render with a key light.");
                isChanged |= ImGui::Checkbox("Use Shadows", (bool*)&mParams.mSceneParameters.useShadows);
                ImGui::SameLine();
                HelpMarker("Render key light shadows.");
            }

            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
                isChanged |= ImGui::Combo("Lens", (int*)&mParams.mSceneParameters.camera.lensType(), kCameraLensTypeStrings, (int)Camera::LensType::kNumTypes);
                ImGui::SameLine();
                HelpMarker("The camera lens type.");
                if (mParams.mSceneParameters.camera.lensType() == Camera::LensType::kODS) {
                    isChanged |= ImGui::DragFloat("IPD", &mParams.mSceneParameters.camera.ipd(), 0.1f, 0.f, 50.f, "%.2f");
                    ImGui::SameLine();
                    HelpMarker("The eye separation distance.");
                }
                if (mParams.mSceneParameters.camera.lensType() == Camera::LensType::kPinHole) {
                    isChanged |= ImGui::DragFloat("Field of View", &mCurrentCameraState->mFovY, 1.0f, 1, 120, "%.0f");
                    ImGui::SameLine();
                    HelpMarker("The vertical field of view in degrees.");
                }

                isChanged |= ImGui::DragInt("Samples", &mParams.mSceneParameters.samplesPerPixel, 0.1f, 1, 32);
                ImGui::SameLine();
                HelpMarker("The number of camera samples per ray.");
                isChanged |= ImGui::Checkbox("Render Environment", (bool*)&mParams.mSceneParameters.useBackground);
                ImGui::SameLine();
                HelpMarker("Render the background environment.");
                isChanged |= ImGui::Checkbox("Render Ground-plane", (bool*)&mParams.mSceneParameters.useGround);
                ImGui::SameLine();
                HelpMarker("Render the ground plane.");
                isChanged |= ImGui::Checkbox("Render Ground-reflections", (bool*)&mParams.mSceneParameters.useGroundReflections);
                ImGui::SameLine();
                HelpMarker("Render ground reflections (work in progress).");
                isChanged |= ImGui::Checkbox("Turntable Camera", &mParams.mUseTurntable);
                ImGui::SameLine();
                HelpMarker("Spin the camera around the pivot each frame step.");
                isChanged |= ImGui::DragFloat("Turntable Inverse Rate", &mParams.mTurntableRate, 0.1f, 1.0f, 100.0f, "%.1f");
                ImGui::SameLine();
                HelpMarker("The number of frame-sequences per revolution.");
            }

            if (ImGui::CollapsingHeader("Tonemapping", ImGuiTreeNodeFlags_DefaultOpen)) {
                isChanged |= ImGui::Checkbox("Use Tonemapping", (bool*)&mParams.mSceneParameters.useTonemapping);
                ImGui::SameLine();
                HelpMarker("Use simple Reinhard tonemapping.");
                isChanged |= ImGui::DragFloat("WhitePoint", &mParams.mSceneParameters.tonemapWhitePoint, 0.01f, 1.0f, 20.0f);
                ImGui::SameLine();
                HelpMarker("The Reinhard tonemapping whitepoint.");
            }

            ImGui::Separator();

            isChanged |= ImGui::Checkbox("Progressive", &mParams.mUseAccumulation);
            ImGui::SameLine();
            HelpMarker("do a progressive accumulation of the frame.");
            isChanged |= ImGui::DragInt("Max Progressive Iterations", &mParams.mMaxProgressiveSamples, 1.f, 1, 256);
            ImGui::SameLine();
            HelpMarker("The maximum progressive iterations (used in batch rendering).");

            ImGui::Separator();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    if (isChanged)
        resetAccumulationBuffer();

    ImGui::End();
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

    ImGui::Text("NanoVDB Viewer\n");

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
#ifdef NANOVDB_USE_CUDA
        ImGui::BulletText("NVIDIA CUDA");
#endif
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

    ImGui::SetNextWindowPos(ImGui::GetWindowViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
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
#if defined(NANOVDB_USE_IMGUI_DOCKING)
    auto viewPos = ImGui::GetMainViewport()->Pos;
    auto viewSize = ImGui::GetMainViewport()->Size;
#else
    
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
#else
    return;
#endif
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
        if (ImGui::BeginMenu("File")) {
#if defined(NANOVDB_USE_NFD)
            if (ImGui::MenuItem("Load from file...", nullptr)) {
                std::string filePath;
                if (openFileDialog(filePath)) {
                    try {
                        int sceneNodeIndex = addGridAssetsAndNodes("default", {filePath});
                        selectSceneNodeByIndex(sceneNodeIndex);
                    }
                    catch (const std::exception& e) {
                        std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
                    }
                }
            }
#else
            ImGui::MenuItem("Load from file...", "(Please build with NFD support)", false, false);
#endif
            if (ImGui::MenuItem("Load from URL...")) {
                openLoadURL = true;
            }

            ImGui::Separator();

#if defined(NANOVDB_USE_NFD)
            if (ImGui::MenuItem("Save to file...", nullptr)) {
                std::string filePath;
                if (openSaveDialog(filePath)) {
                    try {
                        std::cout << filePath << "\n";
                    }
                    catch (const std::exception& e) {
                        std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
                    }
                }
            }
#else
            ImGui::MenuItem("Save to file...", "(Please build with NFD support)", false, false);
#endif
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Q")) {
                glfwSetWindowShouldClose((GLFWwindow*)mWindow, 1);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
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

        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Add Empty Node")) {
                auto nodeId = addSceneNode();
                selectSceneNodeByIndex(findNode(nodeId)->mIndex);
            }

            if (ImGui::BeginMenu("Add Primitive Node")) {
                static StringMap urls;
                urls["ls_sphere"] = "internal://#ls_sphere_100";
                urls["ls_torus"] = "internal://#ls_torus_100";
                urls["ls_box"] = "internal://#ls_box_100";
                urls["ls_octahedron"] = "internal://#ls_octahedron_100";
                urls["ls_bbox"] = "internal://#ls_bbox_100";
                urls["fog_sphere"] = "internal://#fog_sphere_100";
                urls["fog_torus"] = "internal://#fog_torus_100";
                urls["fog_box"] = "internal://#fog_box_100";
                urls["fog_octahedron"] = "internal://#fog_octahedron_100";
                urls["points_sphere"] = "internal://#points_sphere_100";
                urls["points_torus"] = "internal://#points_torus_100";
                urls["points_box"] = "internal://#points_box_100";
                urls["fog_mandelbulb"] = "internal://#fog_mandelbulb_100";

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

        if (ImGui::BeginMenu("Render")) {
            {
                bool areAttachmentsReady = true;
                if (mSelectedSceneNodeIndex >= 0) {
                    areAttachmentsReady = updateNodeAttachmentRequests(mSceneNodes[mSelectedSceneNodeIndex], true, mIsDumpingLog);
                }

                auto screenShotFilename = getScreenShotFilename(mScreenShotIteration);

                if (ImGui::MenuItem("Save Screenshot", screenShotFilename.c_str(), false, areAttachmentsReady)) {
                    //saveFrameBuffer(getSceneFrame(), screenShotFilename, "png");
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Options...")) {
                mIsDrawingRenderOptions = true;
            }

            ImGui::Separator();

            if (mParams.mOutputFilePath.empty()) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            if (ImGui::Button("Batch Render")) {
                renderSequence();
            }

            if (mParams.mOutputFilePath.empty()) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("View Help"))
                mIsDrawingHelpDialog = true;
            ImGui::Separator();
            if (ImGui::MenuItem("About"))
                mIsDrawingAboutDialog = true;
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (openLoadURL)
        ImGui::OpenPopup("Load URL");

#if defined(NANOVDB_USE_IMGUI_DOCKING)
    auto viewPos = ImGui::GetMainViewport()->Pos;
    auto viewSize = ImGui::GetMainViewport()->Size;
#else
    auto viewPos = ImGui::GetWindowPos();
    auto viewSize = ImGui::GetWindowSize();
#endif
    ImVec2 center = ImVec2(viewPos.x + viewSize.x / 2, viewPos.y + viewSize.y / 2);

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Load URL", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char urlBuf[1024] = {0};
        static char nameBuf[256] = {0};

        ImGui::InputText("Node Name", nameBuf, 1024);
        ImGui::InputText("Asset URL", urlBuf, 1024);

        ImGui::Separator();
        if (ImGui::Button("Load")) {
            auto nodeId = addSceneNode(std::string(nameBuf));
            setSceneNodeGridAttachment(nodeId, 0, urlBuf);
            addGridAsset(urlBuf);
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