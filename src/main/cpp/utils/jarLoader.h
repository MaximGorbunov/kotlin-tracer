#ifndef KOTLIN_TRACER_JARLOADER_H
#define KOTLIN_TRACER_JARLOADER_H

#include <string>

class JarLoader {
 public:
  static void load(const std::string &name);
};

#endif //KOTLIN_TRACER_JARLOADER_H
