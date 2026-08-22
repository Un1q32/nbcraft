#include <stdarg.h>
#include <stdio.h>

#include "thirdparty/SDL/SDL.h"

#include "client/app/App.hpp"

#include "AppPlatform_vita.hpp"

typedef AppPlatform_vita UsedAppPlatform;

#include "client/app/NinecraftApp.hpp"
#include "client/player/input/Multitouch.hpp"

#ifndef MCE_GFX_API_NULL
#define MCE_GFX_API_OGL 1
#include "renderer/platform/ogl/Extensions.hpp"
#endif

// Video Mode Flags
#define VIDEO_FLAGS (SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN)

// The Vita's native resolution
#define VITA_SCREEN_WIDTH 960
#define VITA_SCREEN_HEIGHT 544

NinecraftApp *g_pApp;

SDL_Window *window = NULL;
SDL_GLContext glContext = NULL;

static UsedAppPlatform* getPlatform()
{
	return static_cast<UsedAppPlatform*>(AppPlatform::singleton());
}

static void preInitGraphics()
{
#if MCE_GFX_API_OGL
	// Configure OpenGL ES Context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

	// Buffer Sizes
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

	// Double-Buffering
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif
}

static void initGraphics()
{
#if MCE_GFX_API_OGL
	// Create OpenGL ES Context
	glContext = SDL_GL_CreateContext(window);
	if (!glContext)
	{
		LOG_E("Unable to create OpenGL context: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// Vsync is controlled through the AppPlatform,
	// default to no vsync here, let platform set it when needed
	if (SDL_GL_SetSwapInterval(0) == -1)
	{
		LOG_W("Setting the swap interval is not supported on this platform!");
	}

	if (!mce::Platform::OGL::InitBindings())
	{
		LOG_E(mce::Platform::OGL::ERROR_MSG);
		exit(EXIT_FAILURE);
	}
#endif
}

static void teardownGraphics()
{
#if MCE_GFX_API_OGL
	SDL_GL_DeleteContext(glContext);
#endif
}

static void teardown()
{
	if (window != NULL)
	{
		teardownGraphics();
		SDL_DestroyWindow(window);
		SDL_Quit();
		window = NULL;
	}
}

// Touch
#define TOUCH_IDS_SIZE (MAX_TOUCHES - 1) // ID 0 Is Reserved For The Mouse
struct touch_id_data {
	bool active;
	SDL_TouchID device;
	SDL_FingerID finger;

	touch_id_data()
	{
		active = false;
	}
};
static touch_id_data touch_ids[TOUCH_IDS_SIZE];
static char get_touch_id(SDL_TouchID device, SDL_FingerID finger) {
	for (int i = 0; i < TOUCH_IDS_SIZE; i++) {
		touch_id_data &data = touch_ids[i];
		if (data.active && data.device == device && data.finger == finger) {
			return i + 1;
		}
	}
	// Not Found
	for (int i = 0; i < TOUCH_IDS_SIZE; i++) {
		// Find First Inactive ID, And Activate It
		touch_id_data &data = touch_ids[i];
		if (!data.active) {
			data.active = true;
			data.device = device;
			data.finger = finger;
			return i + 1;
		}
	}
	// Fail
	return 0;
}
static void drop_touch_id(int id) {
	touch_ids[id - 1].active = false;
}
static void handle_touch(int x, int y, int type, char id) {
	if (id == 0) {
		return;
	}
	switch (type) {
		case SDL_FINGERDOWN:
		case SDL_FINGERUP: {
			bool data = type == SDL_FINGERUP ? 0 : 1;
			Mouse::feed(MOUSE_BUTTON_LEFT, data, x, y);
			Multitouch::feed(MOUSE_BUTTON_LEFT, data, x, y, id);
			if (type == SDL_FINGERUP) {
				drop_touch_id(id);
			}
			break;
		}
		case SDL_FINGERMOTION: {
			Mouse::feed(MOUSE_BUTTON_NONE, 0, x, y);
			Multitouch::feed(MOUSE_BUTTON_NONE, 0, x, y, id);
			break;
		}
	}
}

// Handle Events
static void handle_events()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			{
				// Hate this hack
				if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START && event.cbutton.state == SDL_PRESSED)
				{
					g_pApp->pauseGame() || g_pApp->resumeGame();
				}
				getPlatform()->handleControllerButtonEvent(event.cbutton.which, event.cbutton.button, event.cbutton.state);
				break;
			}
			case SDL_CONTROLLERAXISMOTION:
				getPlatform()->handleControllerAxisEvent(event.caxis.which, event.caxis.axis, event.caxis.value);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if (event.button.which != SDL_TOUCH_MOUSEID)
				{
					MouseButtonType type = AppPlatform_sdl::GetMouseButtonType(event.button.button);
					bool state = AppPlatform_sdl::GetMouseButtonState(event);
					float x = event.button.x;
					float y = event.button.y;
					Mouse::feed(type, state, x, y);
					if (getPlatform()->isTouchscreen())
						Multitouch::feed(type, state, x, y, 0);
				}
				break;
			}
			case SDL_FINGERDOWN:
			case SDL_FINGERUP:
			case SDL_FINGERMOTION:
			{
				float x = event.tfinger.x * Minecraft::width;
				float y = event.tfinger.y * Minecraft::height;
				handle_touch(x, y, event.type, get_touch_id(event.tfinger.touchId, event.tfinger.fingerId));
				break;
			}
			case SDL_TEXTINPUT:
			{
				if (g_pApp != nullptr)
				{
					size_t length = strlen(event.text.text);
					for (size_t i = 0; i < length; i++)
					{
						char x = event.text.text[i];
						g_pApp->handleCharInput(x);
					}
				}
				break;
			}
			case SDL_CONTROLLERDEVICEADDED:
				getPlatform()->gameControllerAdded(event.cdevice.which);
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				getPlatform()->gameControllerRemoved(event.cdevice.which);
				break;
			case SDL_QUIT:
			{
				g_pApp->quit();
				break;
			}
		}
	}
}

int main(int argc, char *argv[])
{
#ifdef DEBUG_LOG_TO_FILE
	// Redirect stdout/stderr to files, since there is no console on the Vita
	freopen("ux0:/data/" C_STORAGE_DIR "/stdout.log", "w", stdout);
	setvbuf(stdout, NULL, _IONBF, 0);
#endif

	// Setup Logging
	Logger::setSingleton(new Logger);

	// Screen Size
	Minecraft::width = VITA_SCREEN_WIDTH;
	Minecraft::height = VITA_SCREEN_HEIGHT;

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		LOG_E("Unable To Initialize SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	preInitGraphics();

	// Create Window
	window = SDL_CreateWindow(C_GAME_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		VITA_SCREEN_WIDTH, VITA_SCREEN_HEIGHT, VIDEO_FLAGS);
	if (!window)
	{
		LOG_E("Unable to create SDL window: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	initGraphics();

	// Setup Teardown
	atexit(teardown);

	// Storage Directory
	std::string storagePath = "ux0:/data/" C_STORAGE_DIR "/";

	createFolderIfNotExists(storagePath.c_str());

	// Start MCPE
	AppPlatform_vita* appPlatform = new AppPlatform_vita(storagePath, window);
	appPlatform->m_externalStorageDir = storagePath;
	appPlatform->setVSyncEnabled(true);
	g_pApp = new NinecraftApp;
	g_pApp->init();

	// Loop
	while (true)
	{
		// Handle Events
		handle_events();

		// Update MCPE
		g_pApp->update();

#ifdef DEBUG_LOG_TO_FILE
		{
			// Watchdog: log GL errors, they often precede vitaGL GPU faults (green screen)
			GLenum glErr = glGetError();
			if (glErr != GL_NO_ERROR)
			{
				static int nErrors = 0;
				if (nErrors++ < 50)
					LOG_E("glGetError: 0x%X", glErr);
			}
		}
#endif

		// Swap Buffers
		SDL_GL_SwapWindow(window);

		if (g_pApp->wantToQuit())
		{
			g_pApp->saveOptions();
			delete g_pApp;
			delete getPlatform();
			teardown();
			break;
		}
	}

	return 0;
}
