void StartTimer(Timer& timer) {
  timer.prev = std::chrono::high_resolution_clock::now();
  timer.delta = 0.0;
}

f64 StopTimer(Timer& timer) {
  std::chrono::steady_clock::time_point prevPrev = timer.prev;
  timer.prev = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> dur = timer.prev - prevPrev;
  timer.delta = dur.count();
  return timer.delta;
}