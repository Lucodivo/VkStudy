#include "vk_study.h"

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080

struct ImguiState {
  union {
    bool showWindows[4];
    struct {
      bool generalDebugText;
      bool quickDebug;
      bool mainMenu;
      bool fps;
    } show;
  };
  CStringRingBuffer generalDebugTextRingBuffer;

  ImguiState() {
    generalDebugTextRingBuffer = createCStringRingBuffer(50, 256);
  }

  ~ImguiState() {
    deleteCStringRingBuffer(generalDebugTextRingBuffer);
  }
};

const char* saveFileName = "vkstudy.ini";
const char* saveImguiStateHeader = "ImguiState";
const char* saveImguiShowBools = "ShowBools";
void saveConfig(const ImguiState& imguiState) {
  FILE* saveFile = fopen(saveFileName, "w");

  fputc('[', saveFile);
  fputs(saveImguiStateHeader, saveFile);
  fputs("]\n", saveFile);

  fputs(saveImguiShowBools, saveFile);
  fputs("=", saveFile);
  for(int i = 0; i < ArrayCount(ImguiState::showWindows); ++i) {
    bool showBool = imguiState.showWindows[i];
    char boolCharValue = showBool ? '1' : '0';
    fputc(boolCharValue, saveFile);
  }
  fputs("\n\n", saveFile);

  fclose(saveFile);
}

static int saveFileParserCallback(void* user, const char* section, const char* name, const char* value) {
  ImguiState* imguiState = (ImguiState*)user;

  #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
  if (MATCH(saveImguiStateHeader, saveImguiShowBools)) {
    u32 iter = 0;
    while(iter < ArrayCount(ImguiState::showWindows) && value[iter] != '\0') {
      imguiState->showWindows[iter++] = value[iter] != '0';
    }
  } else {
    return 0;  /* unknown section/name, error */
  }
  return 1;
}

void loadConfig(ImguiState* imguiState) {
  if (ini_parse(saveFileName, saveFileParserCallback, imguiState) < 0) {
    printf("Can't load 'test.ini'\n");
  }
}

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

  state.imguiState.show.quickDebug = false;
  state.imguiState.show.fps = true;
  state.imguiState.show.generalDebugText = false;
  state.imguiState.show.mainMenu = true;

  loadConfig(&state.imguiState);

  state.editMode = true; // NOTE: We assume edit mode is enabled by default

  WindowManager& windowManager = WindowManager::getInstance(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, false);

  VulkanEngine vkEngine;
  vkEngine.init();

  while(!state.input.quit) {
    vkEngine.initFrame(state.imguiState.show.quickDebug);
    update(state, windowManager);
    vkEngine.draw(state.camera);
  }

  vkEngine.cleanup();
  saveConfig(state.imguiState);

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

  vkImguiQuickDebugFloat(state.imguiState.show.quickDebug, "move speed", &cameraMoveSpeed, 0.1f, 1.0f, "%.2f");

  state.camera.move(cameraDelta * (invSqrt.val[invSqrtIndex] * cameraMoveSpeed));

  if(state.imguiState.show.mainMenu) {
    if(ImGui::BeginMainMenuBar()) {
      if(ImGui::BeginMenu("Windows")) {
        ImGui::MenuItem("Quick Debug Log", NULL, &state.imguiState.show.quickDebug);
        ImGui::MenuItem("General Debug Text", NULL, &state.imguiState.show.generalDebugText);
        ImGui::MenuItem("Main Debug Menu", NULL, &state.imguiState.show.mainMenu);
        ImGui::MenuItem("FPS", NULL, &state.imguiState.show.fps);
        ImGui::EndMenu();
      }ImGui::EndMainMenuBar();
    }
  }

  vkImguiTextWindow("General Debug", state.imguiState.generalDebugTextRingBuffer, state.imguiState.show.generalDebugText);

  local_access Timer frameTimer;
  f64 frameTimeMs = StopTimer(frameTimer); // for last frame
  f64 frameFPS = 1000.0 / frameTimeMs;
  if(state.imguiState.show.fps) {
    const ImGuiWindowFlags textNoFrills = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize;
    if(ImGui::Begin("FPS", &state.imguiState.show.fps, textNoFrills)) {
      ImGui::Text("%5.2f ms | %3.1f fps", frameTimeMs, frameFPS);
    }ImGui::End();
  }
  StartTimer(frameTimer); // for current frame
}