#include <X11/X.h>
#include<stdlib.h>
#include<memory.h>
#include<X11/Xlib.h>
#include<X11/XKBlib.h>
#include<X11/keysym.h>
#include<GL/glew.h>
#include<GL/glx.h>
#include<iostream>

#include"windowing.h"

using namespace std;

static uint64_t clockOffset = 0; 
static const clockid_t currentclock = CLOCK_REALTIME;
static const uint64_t frequency = 1000000000;

uint64_t getTimerValue() {
	struct timespec ts;
	clock_gettime(currentclock, &ts);
	return (uint64_t)ts.tv_sec * frequency + (uint64_t)ts.tv_nsec;
}

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
	if(severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message);
	}
}

glwindow::glwindow(string name, int x, int y, int width, int height, int version) {
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

	this->keyboardFunc = NULL;

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

	this->window = XCreateWindow(this->display, RootWindow(this->display, this->visualInfo->screen), x, y, width, height, 0, this->visualInfo->depth, InputOutput, this->visualInfo->visual, styleMask, &winAttribs);
	XStoreName(this->display, this->window, name.c_str());

	Atom windowManagerDelete = XInternAtom(this->display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(this->display, this->window, &windowManagerDelete, 1);

	XMapWindow(this->display, this->window);

	closed = false;

	typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
	glXCreateContextAttribsARBProc glXCreateContextAttribARB = NULL;
	glXCreateContextAttribARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
	const int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, version / 100,
		GLX_CONTEXT_MINOR_VERSION_ARB, version % 100 / 10,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | GLX_CONTEXT_DEBUG_BIT_ARB,
		None
	};

	this->glxcontext =  glXCreateContextAttribARB(this->display, this->fbConfig, 0, True, attribs);
	glXMakeCurrent(this->display, this->window, this->glxcontext);
	glewInit();

#ifdef DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
#endif
}

void glwindow::processEvents(void) {
	XEvent event,next_event;
	KeySym keysym;

	static unsigned currentButton;

	while(XPending(this->display)) {
		XNextEvent(this->display, &event);
		switch(event.type) {
		case KeyPress:
			keysym = XkbKeycodeToKeysym(this->display, event.xkey.keycode, 0, 0);
			if(this->keyboardFunc) {
				this->keyboardFunc(this, keysym);
			}
			break;
		case ButtonPress:
			currentButton = event.xbutton.button;
			if(this->mouseFunc) {
				this->mouseFunc(this, event.xbutton.button, MOUSE_BUTTON_PRESS, event.xbutton.x, event.xbutton.y);
			}
			break;
		case ButtonRelease:
			currentButton = 0;
			if(this->mouseFunc) {
				this->mouseFunc(this, event.xbutton.button, MOUSE_BUTTON_RELEASE, event.xbutton.x, event.xbutton.y);
			}
			break;
		case MotionNotify:
			if(this->mouseFunc) {
				this->mouseFunc(this, currentButton, MOUSE_BUTTON_MOVE, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ConfigureNotify:
			windowSize.width = event.xconfigure.width;
			windowSize.height = event.xconfigure.height;
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

bool glwindow::isClosed(void) {
	return closed;
}

void glwindow::swapBuffers() {
	glXSwapBuffers(this->display, this->window);
}

void glwindow::setFullscreen(bool fullscreen) {
	XEvent xev = {0};
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = this->window;
	xev.xclient.message_type = XInternAtom(this->display, "_NET_WM_STATE", False);;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = fullscreen ? 1 : 0;
	xev.xclient.data.l[1] = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", False);
	XSendEvent(this->display, RootWindow(this->display, this->visualInfo->screen), False, StructureNotifyMask, &xev);
	this->fullscreen = fullscreen;
}

void glwindow::setGUISupport(bool gui){
	this->guiSupport = gui;
	// Initialize IMGUI Support
	if(gui){
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		ImGui_ImplOpenGL3_Init();
		ImGui_ImplX11_Init(this->getDisplay(), this->getWindow());
	}
}

void glwindow::toggleFullscreen(void) {
	this->fullscreen = !this->fullscreen;
	setFullscreen(this->fullscreen);
}

glwindow::windowsize_t glwindow::getSize(void) {
	return this->windowSize;
}

void glwindow::close(void) {
	XEvent ev;
	memset(&ev, 0, sizeof (ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = this->window;
	ev.xclient.message_type = XInternAtom(this->display, "WM_PROTOCOLS", true);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(this->display, "WM_DELETE_WINDOW", false);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(this->display, this->window, False, NoEventMask, &ev);			
}

void glwindow::setKeyboardFunc(glwindowkeyboardfunc keyboardcallback) {
	this->keyboardFunc = keyboardcallback;
}

void glwindow::setMouseFunc(glwindowmousefunc mousecallback) {
	this->mouseFunc = mousecallback;
}

double glwindow::getTime() {
	return (double)(getTimerValue() - clockOffset) / frequency;
}

glwindow::~glwindow(void) {
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

Display* glwindow::getDisplay(){
	return this->display;
}

Window* glwindow::getWindow(){
	return &this->window;
}

