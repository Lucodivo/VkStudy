#pragma once

struct CStringRingBuffer {
  u32 first;
  u32 count;
  u32 cStringMaxSize;
  u32 cStringMaxCount;
  char* buffer;
};

CStringRingBuffer createCStringRingBuffer(u32 stringsCount, u32 stringSize) {
  CStringRingBuffer ringBuffer;
  ringBuffer.first = 0;
  ringBuffer.count = 0;
  ringBuffer.cStringMaxSize = stringSize;
  ringBuffer.cStringMaxCount = stringsCount;
  u64 bufferSize = stringSize * stringsCount;
  ringBuffer.buffer = new char[bufferSize];
  memset(ringBuffer.buffer, '\0', bufferSize);
  return ringBuffer;
}

// adds a string to the last available spot, if no spot is available, it replaces the first
// cstring and moves the first index forward
void addCString(CStringRingBuffer& ringBuffer, const char* cStr) {
  u32 bufferOffset;

  if(ringBuffer.count == ringBuffer.cStringMaxCount) {
    bufferOffset = ringBuffer.first;
    ringBuffer.first = (ringBuffer.first + 1) % ringBuffer.cStringMaxCount;
  } else {
    bufferOffset = (ringBuffer.first + ringBuffer.count);
    ++ringBuffer.count;
  }

  char* cStrBuffer = ringBuffer.buffer + (bufferOffset * ringBuffer.cStringMaxSize);

  u32 strLength = (u32)strlen(cStr);
  u32 copyCount = Min(ringBuffer.cStringMaxSize - 1, strLength);
  strncpy_s(cStrBuffer, ringBuffer.cStringMaxSize, cStr, copyCount);
}

void addCString(CStringRingBuffer& ringBuffer, const char* cStr, u32 strLength) {
  u32 bufferOffset;

  if(ringBuffer.count == ringBuffer.cStringMaxCount) {
    bufferOffset = ringBuffer.first;
    ringBuffer.first = (ringBuffer.first + 1) % ringBuffer.cStringMaxCount;
  } else {
    bufferOffset = ringBuffer.count++;
  }

  char* cStrBuffer = ringBuffer.buffer + (bufferOffset * ringBuffer.cStringMaxSize);

  u32 copyCount = Min(ringBuffer.cStringMaxSize - 1, strLength);
  strncpy_s(cStrBuffer, ringBuffer.cStringMaxSize, cStr, copyCount);
}

void addCStringF(CStringRingBuffer& ringBuffer, const char* format, ...) {
  va_list args;
  va_start (args, format);

  // _vscprintf tells you how big the buffer needs to be
  int strLength = _vscprintf(format, args);
  if (strLength == -1) {
    return;
  }
  strLength++; // add room for null token

  u32 bufferOffset;
  if(ringBuffer.count == ringBuffer.cStringMaxCount) {
    bufferOffset = ringBuffer.first++;
    ringBuffer.first %= ringBuffer.cStringMaxCount;
  } else {
    bufferOffset = ringBuffer.count++;
  }

  char* cStrBuffer = ringBuffer.buffer + (bufferOffset * ringBuffer.cStringMaxSize);

  u32 copyCount = Min(ringBuffer.cStringMaxSize - 1, (u32)strLength);
  vsnprintf(cStrBuffer, copyCount, format, args);

  va_end(args);
}

void forEachString(const CStringRingBuffer& ringBuffer, std::function<void(const char* str)> func) {
  u32 firstPortionIter = ringBuffer.first;
  while(firstPortionIter < ringBuffer.count) {
    u32 strBufferOffset = ringBuffer.cStringMaxSize * firstPortionIter;
    func(ringBuffer.buffer + strBufferOffset);
    firstPortionIter++;
  }

  u32 secondPortionIter = 0;
  while(secondPortionIter != ringBuffer.first) {
    u32 strBufferOffset = ringBuffer.cStringMaxSize * secondPortionIter;
    func(ringBuffer.buffer + strBufferOffset);
    secondPortionIter++;
  }
}

void deleteCStringRingBuffer(CStringRingBuffer& ringBuffer) {
  delete[] ringBuffer.buffer;
  ringBuffer = {}; // zero out buffer
}