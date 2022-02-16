void imguiTextWindow(const char* title, const CStringRingBuffer& ringBuffer, bool& show) {
  // debug text window
  if(show) {
    ImGui::Begin(title, &show, ImGuiWindowFlags_None);
    {
      ImGui::BeginChild("Scrolling");
      {
        u32 firstRoundIter = ringBuffer.first;
        while(firstRoundIter < ringBuffer.count) {
          ImGui::Text(ringBuffer.buffer + (firstRoundIter * ringBuffer.cStringMaxSize));
          firstRoundIter++;
        }

        u32 secondRoundIter = 0;
        while(secondRoundIter != ringBuffer.first) {
          ImGui::Text(ringBuffer.buffer + (secondRoundIter * ringBuffer.cStringMaxSize));
          secondRoundIter++;
        }
      }
      ImGui::EndChild();
    }
    ImGui::End();
  }
}