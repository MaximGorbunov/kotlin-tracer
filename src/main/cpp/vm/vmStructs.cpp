#include "vmStructs.h"

namespace kotlin_tracer {
char *gHotSpotVMStructs;
uint64_t gHotSpotVMStructEntryTypeNameOffset;
uint64_t gHotSpotVMStructEntryFieldNameOffset;
uint64_t gHotSpotVMStructEntryTypeStringOffset;
uint64_t gHotSpotVMStructEntryIsStaticOffset;
uint64_t gHotSpotVMStructEntryOffsetOffset;
uint64_t gHotSpotVMStructEntryAddressOffset;
uint64_t gHotSpotVMStructEntryArrayStride;

// VM Types
char *gHotSpotVMTypes;
uint64_t gHotSpotVMTypeEntryTypeNameOffset;
uint64_t gHotSpotVMTypeEntrySuperclassNameOffset;
uint64_t gHotSpotVMTypeEntryIsOopTypeOffset;
uint64_t gHotSpotVMTypeEntryIsIntegerTypeOffset;
uint64_t gHotSpotVMTypeEntryIsUnsignedOffset;
uint64_t gHotSpotVMTypeEntrySizeOffset;
uint64_t gHotSpotVMTypeEntryArrayStride;

// VM Int constants
char *gHotSpotVMIntConstants;
uint64_t gHotSpotVMIntConstantEntryNameOffset;
uint64_t gHotSpotVMIntConstantEntryValueOffset;
uint64_t gHotSpotVMIntConstantEntryArrayStride;

// VM Long constants
char *gHotSpotVMLongConstants;
uint64_t gHotSpotVMLongConstantEntryNameOffset;
uint64_t gHotSpotVMLongConstantEntryValueOffset;
uint64_t gHotSpotVMLongConstantEntryArrayStride;

#define get_address(var_name) \
auto addr_##var_name = reinterpret_cast<char**>(addr_2_symbol->getVariableAddress(#var_name)); \
if (addr_##var_name == nullptr ) { throw std::runtime_error(std::string(#var_name) + " not found!"); } \
var_name = *addr_##var_name;

#define get_offset(var_name) \
auto addr_##var_name = reinterpret_cast<uint64_t*>(addr_2_symbol->getVariableAddress(#var_name)); \
if (addr_##var_name == nullptr ) { throw std::runtime_error(std::string(#var_name) + " not found!"); } \
var_name = *addr_##var_name;

void init(addr2Symbol::Addr2Symbol *addr_2_symbol) {
  get_address(gHotSpotVMStructs);
  get_address(gHotSpotVMTypes);
  get_address(gHotSpotVMIntConstants);
  get_address(gHotSpotVMLongConstants);
  get_offset(gHotSpotVMStructEntryTypeNameOffset);
  get_offset(gHotSpotVMStructEntryFieldNameOffset);
  get_offset(gHotSpotVMStructEntryTypeStringOffset);
  get_offset(gHotSpotVMStructEntryIsStaticOffset);
  get_offset(gHotSpotVMStructEntryOffsetOffset);
  get_offset(gHotSpotVMStructEntryAddressOffset);
  get_offset(gHotSpotVMStructEntryArrayStride);
  get_offset(gHotSpotVMTypeEntryTypeNameOffset);
  get_offset(gHotSpotVMTypeEntrySuperclassNameOffset);
  get_offset(gHotSpotVMTypeEntryIsOopTypeOffset);
  get_offset(gHotSpotVMTypeEntryIsIntegerTypeOffset);
  get_offset(gHotSpotVMTypeEntryIsUnsignedOffset);
  get_offset(gHotSpotVMTypeEntrySizeOffset);
  get_offset(gHotSpotVMTypeEntryArrayStride);
  get_offset(gHotSpotVMIntConstantEntryNameOffset);
  get_offset(gHotSpotVMIntConstantEntryValueOffset);
  get_offset(gHotSpotVMIntConstantEntryArrayStride);
  get_offset(gHotSpotVMLongConstantEntryNameOffset);
  get_offset(gHotSpotVMLongConstantEntryValueOffset);
  get_offset(gHotSpotVMLongConstantEntryArrayStride);
}
}  // namespace kotlin_tracer
