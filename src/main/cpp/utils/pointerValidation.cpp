#include "pointerValidation.h"

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>

bool is_valid(uint64_t address_ptr) {
  if (address_ptr == 0) return false;
  vm_map_t task = mach_task_self();
  auto address = static_cast<mach_vm_address_t>(address_ptr);
  mach_vm_size_t size = 0;
  vm_region_basic_info_data_64_t info;
  mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
  mach_port_t object_name;
  kern_return_t ret = mach_vm_region(
      task,
      &address,
      &size,
      VM_REGION_BASIC_INFO_64,
      (vm_region_info_t)&info,
      &count,
      &object_name);
  if (ret != KERN_SUCCESS) return false;
  return (static_cast<mach_vm_address_t>(address_ptr)) >= address && ((info.protection & VM_PROT_READ) == VM_PROT_READ);
}
#elif defined(__linux__)
#include <sys/mman.h>
#include <cstddef>
#include <csignal>

bool is_valid(uint64_t address_ptr) {
  /* get the page size */
  size_t page_size = sysconf(_SC_PAGESIZE);
  /* find the address of the page that contains p */
  void *base = reinterpret_cast<void *>(((static_cast<size_t>(address_ptr)) / page_size) * page_size);
  /* call msync, if it returns non-zero, return false */
  return msync(base, page_size, MS_ASYNC) == 0;
}
#endif
