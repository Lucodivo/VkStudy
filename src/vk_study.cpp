#include "vk_study.h"

#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080

struct AppState {
  Input input;
};

void update(AppState& state, WindowManager& windowManager);

int main(int argc, char* argv[]) {
  AppState state = {};

  WindowManager& windowManager = WindowManager::getInstance(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, false);

  VulkanEngine vkEngine;
  vkEngine.init();

  while(!state.input.quit) {
    update(state, windowManager);
    vkEngine.update(state.input);
    vkEngine.draw();
  }

  vkEngine.cleanup();

  return 0;
}

void update (AppState& state, WindowManager& windowManager) {
  state.input = windowManager.processInput();

  while(state.input.windowMinimized) {
    windowManager.waitForRestore();
    state.input = windowManager.processInput();
  }

  if(state.input.toggleFullScreen) {
    windowManager.toggleFullScreen();
  }
}