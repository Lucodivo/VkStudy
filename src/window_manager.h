#pragma once

struct Input {
  bool up, down, forward, back, left, right;
  bool button1, button2, button3;
  bool switch1, switch2;
  bool quit, toggleFullScreen;
  bool windowSizeChanged, windowMinimized;
  f32 mouseDeltaX, mouseDeltaY; // normalized to screen height
};

// Currently just a Singleton wrapper around SDL
// Not that it also sends SDL events to Imgui
class WindowManager
{
public:

  static WindowManager& getInstance(s32 requestedWidth = 1920, s32 requestedHeight = 1080, s32 requestFullScreen = false) {
    static WindowManager instance(requestedWidth, requestedHeight, requestFullScreen);
    return instance;
  }

  void createSurface(VkInstance vkInstance, VkSurfaceKHR* outSurface, u32* outWidth, u32* outHeight) {
    s32 width, height;
    SDL_Vulkan_CreateSurface(window, vkInstance, outSurface);
    SDL_GetWindowSize(window, &width, &height);
    Assert(width >= 0 && height >= 0);
    *outWidth = width;
    *outHeight = height;
  }

  void getExtent(u32* outWidth, u32* outHeight) {
    // Get extent every time. Not worth the risk of providing outdated sizes.
    s32 windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    Assert(windowWidth >= 0 && windowHeight >= 0);
    *outWidth = windowWidth;
    *outHeight = windowHeight;
  }

//  void getPosition(u32* outUpperLeftX, u32* ourUpperLeftY) {
//    s32 windowUpperLeftX = 0, windowUpperLeftY = 0;
//    SDL_GetWindowPosition(window, &windowUpperLeftX, &windowUpperLeftY);
//    *outUpperLeftX = windowUpperLeftX >= 0 ? windowUpperLeftX : 0;
//    *ourUpperLeftY = windowUpperLeftY >= 0 ? windowUpperLeftY : 0;
//  }

  void toggleFullScreen() {
    fullScreen = !fullScreen;
    if(fullScreen) {
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
      SDL_GLContext sdlContext = SDL_GL_CreateContext(window);
      SDL_GL_MakeCurrent(window, sdlContext);
    } else {
      SDL_SetWindowFullscreen(window, 0);
      SDL_GLContext sdlContext = SDL_GL_CreateContext(window);
      SDL_GL_MakeCurrent(window, sdlContext);
    }
  }

  void setMouseMode(bool on) {
    SDL_SetRelativeMouseMode(on ? SDL_FALSE : SDL_TRUE);
  }

  const Input& processInput() {
    local_access bool altDown = false;
    bool altWasDown = altDown;

    // reset input
    input.toggleFullScreen = false;
    input.windowSizeChanged = false;
    s32 mouseDeltaPixelsX = 0;
    s32 mouseDeltaPixelsY = 0;

    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
      switch (e.type) {
        case SDL_QUIT: input.quit = true; break;
        case SDL_KEYDOWN:
          switch (e.key.keysym.sym)
          {
            case SDLK_ESCAPE: input.quit = true; break;
            case SDLK_SPACE: break;
            case SDLK_LSHIFT: input.button1 = true; break;
            case SDLK_a: case SDLK_LEFT: input.left = true; break;
            case SDLK_d: case SDLK_RIGHT: input.right = true; break;
            case SDLK_w: case SDLK_UP: input.forward = true; break;
            case SDLK_s: case SDLK_DOWN: input.back = true; break;
            case SDLK_q: input.down = true; break;
            case SDLK_e: input.up = true; break;
            case SDLK_LALT: case SDLK_RALT: altDown = true; break;
          }
          break;
        case SDL_KEYUP:
          switch (e.key.keysym.sym)
          {
            case SDLK_SPACE: input.switch1 = !input.switch1; break;
            case SDLK_TAB: input.switch2 = !input.switch2; break;
            case SDLK_a: case SDLK_LEFT: input.left = false; break;
            case SDLK_d: case SDLK_RIGHT: input.right = false; break;
            case SDLK_w: case SDLK_UP: input.forward = false; break;
            case SDLK_s: case SDLK_DOWN: input.back = false; break;
            case SDLK_q: input.down = false; break;
            case SDLK_e: input.up = false; break;
            case SDLK_LALT: case SDLK_RALT: altDown = false; break;
            case SDLK_RETURN: if (altWasDown) { input.toggleFullScreen = true; } break;
          }
          break;
        case SDL_MOUSEMOTION:
        {
          mouseDeltaPixelsX += e.motion.xrel;
          mouseDeltaPixelsY += e.motion.yrel;
          break;
        }
        case SDL_WINDOWEVENT:
        {
          switch (e.window.event) {
            case SDL_WINDOWEVENT_SHOWN: break; // has been shown
            case SDL_WINDOWEVENT_HIDDEN: break; // has been hidden
            case SDL_WINDOWEVENT_EXPOSED: break; // has been exposed and should be redrawn
            case SDL_WINDOWEVENT_MOVED: {
//              s32 windowUpperLeftX = e.window.data1;
//              s32 windowUpperLeftY = e.window.data2;
            } break; // window has been moved to x,y
            case SDL_WINDOWEVENT_RESIZED: {
//              s32 windowWidth = e.window.data1;
//              s32 windowHeight = e.window.data2;
            } break; // window has been resized by user or window manager, preceded by SDL_WINDOWEVENT_SIZE_CHANGED
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
//              s32 windowWidth = e.window.data1;
//              s32 windowHeight = e.window.data2;
              input.windowSizeChanged = true;
            } break; // window size has changed
            case SDL_WINDOWEVENT_MINIMIZED: input.windowMinimized = true; break; // has been minimized
            case SDL_WINDOWEVENT_MAXIMIZED: break; // has been maximized
            case SDL_WINDOWEVENT_RESTORED: input.windowMinimized = false; break; // has been restored to size & position (from minimize/maximize)
            case SDL_WINDOWEVENT_ENTER: break; // has gained mouse focus
            case SDL_WINDOWEVENT_LEAVE: break; // has lost mouse focus
            case SDL_WINDOWEVENT_FOCUS_GAINED: break; // has gained keyboard focus
            case SDL_WINDOWEVENT_FOCUS_LOST: break; // has lost keyboard focus
            case SDL_WINDOWEVENT_CLOSE: break; // window manager requests that the window be closed
#if SDL_VERSION_ATLEAST(2, 0, 5)
            case SDL_WINDOWEVENT_TAKE_FOCUS: break; // being offered focus, potentially call SDL_SetWindowInputFocus()
            case SDL_WINDOWEVENT_HIT_TEST: break; // window had a hit test that wasn't SDL_HITTEST_NORMAL
#endif
            default: break; // ???
          }
        }
        default: break;
      }

      // also send input to ImGui
      ImGui_ImplSDL2_ProcessEvent(&e);
    }

    // massage final data
    f32 normalizeFactor = 2.0f / (f32)displayHeight;
    input.mouseDeltaX = (f32)mouseDeltaPixelsX * normalizeFactor;
    input.mouseDeltaY = (f32)mouseDeltaPixelsY * normalizeFactor;

    return input;
  }

  void waitForRestore() {
    SDL_Event e;
    while(SDL_WaitEvent(&e)) {
      if(e.window.event == SDL_WINDOWEVENT_RESTORED) {
        input.windowMinimized = false;
        break;
      }
    }
  }

  void initImguiForWindow() {
    ImGui_ImplSDL2_InitForVulkan(window);
  }

  // Make sure these methods are inaccessible (especially from outside),
  // otherwise, you may accidentally get copies of singleton.
  WindowManager(WindowManager const&) = delete;
  void operator=(WindowManager const&) = delete;

private:
  SDL_Window* window = nullptr;
  Input input = {};
  s32 displayWidth, displayHeight;
  bool fullScreen = false;

  WindowManager(s32 width, s32 height, s32 fullScreen) {
    this->fullScreen = fullScreen;

    // needed for SDL to counteract the scaling Windows can do for windows
    SetProcessDPIAware();
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN |
                                                     SDL_WINDOW_RESIZABLE |
                                                     SDL_WINDOW_ALLOW_HIGHDPI |
                                                     (this->fullScreen ? SDL_WINDOW_FULLSCREEN : 0));
    window = SDL_CreateWindow(
            "Vulkan Engine",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            window_flags
    );
    SDL_CaptureMouse(SDL_TRUE);
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);

    SDL_DisplayMode displayMode;
    if (SDL_GetDesktopDisplayMode(0, &displayMode) != 0)
    {
      std::cout << "SDL_GetDesktopDisplayMode failed: " << SDL_GetError() << std::endl;
      displayWidth = 1920;
      displayHeight = 1080;
    } else {
      displayWidth = displayMode.w;
      displayHeight = displayMode.h;
    }
  }

  ~WindowManager() {
    SDL_DestroyWindow(window);
  }
};