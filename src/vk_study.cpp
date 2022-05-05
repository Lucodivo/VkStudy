#include "vk_study.h"

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080

struct ImguiState {
  bool showGeneralDebugText;
  bool showQuickDebug;
  bool showMainMenu;
  bool showFPS;
  CStringRingBuffer generalDebugTextRingBuffer;

  ImguiState() {
    generalDebugTextRingBuffer = createCStringRingBuffer(50, 256);
  }

  ~ImguiState() {
    deleteCStringRingBuffer(generalDebugTextRingBuffer);
  }
};

struct AppState {
  Input input;
  ImguiState imguiState;
  Camera camera;
  bool editMode;
};

void update(AppState& state, WindowManager& windowManager);

int main(int argc, char* argv[]) {
  AppState state = {};

  state.camera = {};
  state.camera.pos = {0.f, -50.f, 20.f};
  state.camera.setForward({0.f, 1.f, 0.f});

  state.imguiState.showQuickDebug = false;
  state.imguiState.showFPS = true;
  state.imguiState.showGeneralDebugText = false;
  state.imguiState.showMainMenu = true;

  state.editMode = true; // NOTE: We assume edit mode is enabled by default

  WindowManager& windowManager = WindowManager::getInstance(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, false);

  VulkanEngine vkEngine;
  vkEngine.init();

  while(!state.input.quit) {
    vkEngine.initFrame(state.imguiState.showQuickDebug);
    update(state, windowManager);
    vkEngine.draw(state.camera);
  }

  vkEngine.cleanup();

  return 0;
}

void update (AppState& state, WindowManager& windowManager) {
  local_access f32 cameraMoveSpeed = 0.5f;
  local_access f32 cameraTurnSpeed = 0.5f;

  vec3 cameraDelta = {};

  state.input = windowManager.processInput();

  while(state.input.windowMinimized) {
    windowManager.waitForRestore();
    state.input = windowManager.processInput();
  }

  if(state.input.toggleFullScreen) {
    windowManager.toggleFullScreen();
  }

  if(state.editMode != state.input.switch1) {
    state.editMode = state.input.switch1;
    windowManager.setMouseMode(state.editMode);
  }

  // Used to keep camera at constant speed regardless of direction (avoidance of moving over larger distances in diagonal directions.
  // Ex: When moving left, we should move at 1.0x speed in the left direction
  // Ex: When moving left and forward, we should be moving (sqrt(2)/2)x speed in the left and (sqrt(2)/2)x speed in the forward direction,
  // to maintain a total velocity of 1.0x speed.
  const vec4 invSqrt = {
          0.0f,
          1.0f,
          0.7071067f,
          0.5773502f
  };
  u32 invSqrtIndex = 0;

  if(state.input.left ^ state.input.right) {
    invSqrtIndex++;
    if(state.input.left) {
      cameraDelta.x = -1.0f;
    } else {
      cameraDelta.x = 1.0f;
    }
  }

  if(state.input.back ^ state.input.forward) {
    invSqrtIndex++;
    if(state.input.back) {
      cameraDelta.y = -1.0f;
    } else {
      cameraDelta.y = 1.0f;
    }
  }

  if(state.input.down ^ state.input.up) {
    invSqrtIndex++;
    if(state.input.down) {
      cameraDelta.z = -1.0f;
    } else {
      cameraDelta.z = 1.0f;
    }
  }

  if(!state.editMode) {
    state.camera.turn(state.input.mouseDeltaX * cameraTurnSpeed,
                      state.input.mouseDeltaY * cameraTurnSpeed);
  }

  vkImguiQuickDebugFloat(state.imguiState.showQuickDebug, "move speed", &cameraMoveSpeed, 0.1f, 1.0f, "%.2f");

  state.camera.move(cameraDelta * (invSqrt.val[invSqrtIndex] * cameraMoveSpeed));

  if(state.imguiState.showMainMenu) {
    if(ImGui::BeginMainMenuBar()) {
      if(ImGui::BeginMenu("Windows")) {
        ImGui::MenuItem("Quick Debug Log", NULL, &state.imguiState.showQuickDebug);
        ImGui::MenuItem("General Debug Text", NULL, &state.imguiState.showGeneralDebugText);
        ImGui::MenuItem("Main Debug Menu", NULL, &state.imguiState.showMainMenu);
        ImGui::MenuItem("FPS", NULL, &state.imguiState.showFPS);
        ImGui::EndMenu();
      }ImGui::EndMainMenuBar();
    }
  }

  vkImguiTextWindow("General Debug", state.imguiState.generalDebugTextRingBuffer, state.imguiState.showGeneralDebugText);

  local_access Timer frameTimer;
  f64 frameTimeMs = StopTimer(frameTimer); // for last frame
  f64 frameFPS = 1000.0 / frameTimeMs;
  if(state.imguiState.showFPS) {
    const ImGuiWindowFlags textNoFrills = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
    if(ImGui::Begin("FPS", &state.imguiState.showFPS, textNoFrills)) {
      ImGui::Text("%5.2f ms | %3.1f fps", frameTimeMs, frameFPS);
    }ImGui::End();
  }
  StartTimer(frameTimer); // for current frame
}