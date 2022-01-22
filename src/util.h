struct Timer {
  std::chrono::steady_clock::time_point prev;
  f64 delta;
};
void StartTimer(Timer& timer);
f64 StopTimer(Timer& timer);
void readFile(const char* filePath, std::vector<char>& fileBytes);