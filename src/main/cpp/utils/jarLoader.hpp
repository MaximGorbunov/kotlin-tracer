#ifndef KOTLIN_TRACER_JARLOADER_H
#define KOTLIN_TRACER_JARLOADER_H

#include <string>
#include <memory>

#include "../vm/jvm.hpp"

namespace kotlinTracer {
class JarLoader {
 public:
  static std::unique_ptr<InstrumentationMetadata> load(const std::string &t_name, std::shared_ptr<JVM> t_jvm);
};
}
#endif //KOTLIN_TRACER_JARLOADER_H
