#ifndef KOTLIN_TRACER_SRC_MAIN_CPP_VM_VMSTRUCTS_H_
#define KOTLIN_TRACER_SRC_MAIN_CPP_VM_VMSTRUCTS_H_
#include <string>
#include <list>
#include <unordered_map>
#include <memory>

#include "addr2Symbol.hpp"

namespace kotlin_tracer {
void init(addr2Symbol::Addr2Symbol *addr_2_symbol);

//VM Structs
extern char *gHotSpotVMStructs;
extern uint64_t gHotSpotVMStructEntryTypeNameOffset;
extern uint64_t gHotSpotVMStructEntryFieldNameOffset;
extern uint64_t gHotSpotVMStructEntryTypeStringOffset;
extern uint64_t gHotSpotVMStructEntryIsStaticOffset;
extern uint64_t gHotSpotVMStructEntryOffsetOffset;
extern uint64_t gHotSpotVMStructEntryAddressOffset;
extern uint64_t gHotSpotVMStructEntryArrayStride;

//VM Types
extern char *gHotSpotVMTypes;
extern uint64_t gHotSpotVMTypeEntryTypeNameOffset;
extern uint64_t gHotSpotVMTypeEntrySuperclassNameOffset;
extern uint64_t gHotSpotVMTypeEntryIsOopTypeOffset;
extern uint64_t gHotSpotVMTypeEntryIsIntegerTypeOffset;
extern uint64_t gHotSpotVMTypeEntryIsUnsignedOffset;
extern uint64_t gHotSpotVMTypeEntrySizeOffset;
extern uint64_t gHotSpotVMTypeEntryArrayStride;

//VM Int constants
extern char *gHotSpotVMIntConstants;
extern uint64_t gHotSpotVMIntConstantEntryNameOffset;
extern uint64_t gHotSpotVMIntConstantEntryValueOffset;
extern uint64_t gHotSpotVMIntConstantEntryArrayStride;

//VM Long constants
extern char *gHotSpotVMLongConstants;
extern uint64_t gHotSpotVMLongConstantEntryNameOffset;
extern uint64_t gHotSpotVMLongConstantEntryValueOffset;
extern uint64_t gHotSpotVMLongConstantEntryArrayStride;

typedef struct {
  const char *fieldName;
  const char *typeString;
  uint64_t offset;
  int32_t isStatic;
} Field;

typedef struct {
  const char *typeName;            // Type name (example: "Method")
  const char *superclassName;      // Superclass name, or null if none (example: "oopDesc")
  int32_t isOopType;               // Does this type represent an oop typedef? (i.e., "Method*" or
  // "KlassStub*", but NOT "Method")
  int32_t isIntegerType;           // Does this type represent an integer type (of arbitrary size)?
  int32_t isUnsigned;              // If so, is it unsigned?
  uint64_t size;                   // Size, in bytes, of the type
  std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<Field>>> fields;
} VMTypeEntry;
} // kotlin_tracer
#endif //KOTLIN_TRACER_SRC_MAIN_CPP_VM_VMSTRUCTS_H_
