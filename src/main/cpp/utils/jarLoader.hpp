#ifndef KOTLIN_TRACER_JARLOADER_H
#define KOTLIN_TRACER_JARLOADER_H

#include <string>
#include <memory>

#include "../vm/jvm.hpp"

namespace kotlin_tracer {
std::unique_ptr<InstrumentationMetadata> load(std::unique_ptr<std::string> jar_path,
                                              const std::string &name,
                                              const std::shared_ptr<JVM> &jvm);
}
#endif //KOTLIN_TRACER_JARLOADER_H
