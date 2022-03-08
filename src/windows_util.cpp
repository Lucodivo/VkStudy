struct VirtualMemoryInfo {
  bool page_execute,
      page_execute_read,
      page_execute_readwrite,
      page_execute_writecopy,
      page_noaccess,
      page_readonly,
      page_readwrite,
      page_writecopy,
      page_targets_invalid,
      page_targets_no_update,
      page_guard,
      page_nocache,
      page_write_combine,
      page_enclave_thread_control,
      page_enclave_unvalidated;
};

VirtualMemoryInfo VirtualQuery(void* ptr) {
  VirtualMemoryInfo outInfo{};

  _MEMORY_BASIC_INFORMATION basicInformation{};
  VirtualQuery(ptr, &basicInformation, sizeof(basicInformation));

  outInfo.page_execute = basicInformation.AllocationProtect & PAGE_EXECUTE;
  outInfo.page_execute_read = basicInformation.AllocationProtect & PAGE_EXECUTE_READ;
  outInfo.page_execute_readwrite = basicInformation.AllocationProtect & PAGE_EXECUTE_READWRITE;
  outInfo.page_execute_writecopy = basicInformation.AllocationProtect & PAGE_EXECUTE_WRITECOPY;
  outInfo.page_noaccess = basicInformation.AllocationProtect & PAGE_NOACCESS;
  outInfo.page_readonly = basicInformation.AllocationProtect & PAGE_READONLY;
  outInfo.page_readwrite = basicInformation.AllocationProtect & PAGE_READWRITE;
  outInfo.page_writecopy = basicInformation.AllocationProtect & PAGE_WRITECOPY;
  outInfo.page_targets_invalid = basicInformation.AllocationProtect & PAGE_TARGETS_INVALID;
  outInfo.page_targets_no_update = basicInformation.AllocationProtect & PAGE_TARGETS_NO_UPDATE;
  outInfo.page_guard = basicInformation.AllocationProtect & PAGE_GUARD;
  outInfo.page_nocache = basicInformation.AllocationProtect & PAGE_NOCACHE;
  outInfo.page_write_combine = basicInformation.AllocationProtect & PAGE_WRITECOMBINE;
  outInfo.page_enclave_thread_control = basicInformation.AllocationProtect & PAGE_ENCLAVE_THREAD_CONTROL;
  outInfo.page_enclave_unvalidated = basicInformation.AllocationProtect & PAGE_ENCLAVE_UNVALIDATED;

  return outInfo;
}