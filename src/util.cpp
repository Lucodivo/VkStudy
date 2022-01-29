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

void readFile(const char* filePath, std::vector<char>& fileBytes)
{
	//open the file. With cursor at the end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	Assert(file.is_open());
	if (!file.is_open()) {
		std::cout << "Could not open file: " << filePath << std::endl;
	}

	//find what the size of the file is by looking up the location of the cursor
	u64 fileSize = (u64)file.tellg();

	fileBytes.clear();
	fileBytes.resize(fileSize);

	//put file cursor at beginning
	file.seekg(0);
	file.read((char*)fileBytes.data(), fileSize);
	file.close();
}
