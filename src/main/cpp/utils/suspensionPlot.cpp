#include "suspensionPlot.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include "log.h"

namespace kotlin_tracer {
using std::string, std::unique_ptr, std::shared_ptr;
static unique_ptr<string> top_half = std::make_unique<string>(
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "    <script src='https://cdn.plot.ly/plotly-2.24.1.min.js'></script>\n"
    "</head>\n"
    "<body>\n"
    "<div id='graph'></div>\n"
    "</body>\n"
    "<script>\n"
    "  var gd = document.getElementById('myDiv');\n"
    "  var data = ["
    "    {"
    "      name: 'Suspensions',"
    "      type: 'icicle',"
    "      labels: [],"
    "      parents: [],"
    "      values: [],"
    "      tiling: {"
    "        orientation: 'v'"
    "      },"
    "      root: {"
    "        color: 'green'"
    "      },"
    "      sort: false,"
    "      marker: {"
    "        colorbar: {"
    "          ticksuffix: 'ms'"
    "        }"
    "      }"
    "    }"
    "  ];\n"
    "  var layout = {"
    "    title: {"
    "      text: 'Suspensions'"
    "    },"
    "    margin: {l: 150},"
    "    showlegend: true"
    "  };\n"
);
static unique_ptr<string> bottom_half = std::make_unique<string>(
    "    Plotly.newPlot('graph', data, layout);"
    "</script>\n"
    "</html>\n"
);

static void printSuspensions(const string &parent,
                             jlong coroutine_id,
                             std::ofstream &file,
                             const TraceStorage &storage) {
  auto suspensions = storage.getSuspensions(coroutine_id);
  auto coroutineName = string("coroutine#" + std::to_string(coroutine_id));
  if (suspensions != nullptr) {
    file << "data[0].labels.push('" + coroutineName + "');\n";
    file << "data[0].parents.push('" + parent + "');\n";
    file << "data[0].values.push(0);\n";
    logInfo("Suspensions: " + std::to_string(suspensions->size()));
    std::function<void(shared_ptr<SuspensionInfo>)>
        suspensionPrint = [&file, &coroutineName](const shared_ptr<SuspensionInfo> &suspension) {
      auto suspendTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(
          suspension->end - suspension->start));
      std::stringstream stackTrace;
      for (auto &frame : *suspension->suspension_stack_trace) {
        stackTrace << *frame << "<br>";
      }
      file << "data[0].labels.push('" + stackTrace.str() + "');\n";
      file << "data[0].parents.push('" + coroutineName + "');\n";
      if (suspendTime.count() >= 0) {
        file << "data[0].values.push(" + std::to_string(suspendTime.count()) + ");\n";
      } else {
        file << "data[0].values.push(-1);\n";
      }
    };
    suspensions->forEach(suspensionPrint);
  }
  if (storage.containsChildCoroutineStorage(coroutine_id)) {
    std::function<void(jlong)> childCoroutinePrint = [&file, &coroutineName, &storage](jlong childCoroutine) {
      printSuspensions(coroutineName, childCoroutine, file, storage);
    };
    auto children = storage.getChildCoroutines(coroutine_id);
    children->forEach(childCoroutinePrint);
  }
}

void plot(const string &path, const TraceInfo &trace_info, const TraceStorage &storage) {
  std::ofstream file;
  file.open(path);
  if (file.is_open()) {
    file << *top_half;
    auto suspensions = storage.getSuspensions(trace_info.coroutine_id);
    printSuspensions("", trace_info.coroutine_id, file, storage);
    file << *bottom_half;
  } else {
    throw std::runtime_error("Failed to open file" + path);
  }
}

}  // namespace kotlin_tracer
