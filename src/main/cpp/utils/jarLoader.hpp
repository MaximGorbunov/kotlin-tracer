#ifndef KOTLIN_TRACER_JARLOADER_H
#define KOTLIN_TRACER_JARLOADER_H

#include <string>
#include <memory>

#include "../vm/jvm.hpp"

namespace kotlin_tracer {
class JarLoader {
 public:
  static std::unique_ptr<InstrumentationMetadata> load(std::unique_ptr<std::string> t_jar_path,
                                                       const std::string &t_name,
                                                       const std::shared_ptr<JVM>& t_jvm);
};
}
#endif //KOTLIN_TRACER_JARLOADER_H
