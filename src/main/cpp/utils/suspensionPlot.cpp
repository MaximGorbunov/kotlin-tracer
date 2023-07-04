#include "suspensionPlot.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <list>

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

static inline std::unique_ptr<std::string> printSuspension(
    const shared_ptr<SuspensionInfo> &suspension,
    std::ofstream &file,
    const std::string &parent
) {
  std::stringstream stack_trace_stream;
  auto suspendTime = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::nanoseconds(suspension->end - suspension->start));
  for (auto &frame : *suspension->suspension_stack_trace) {
    stack_trace_stream << *frame << "<br>";
  }
  auto stack_trace = std::make_unique<string>(stack_trace_stream.str());
  file << "data[0].labels.push('" + *stack_trace + "');\n";
  file << "data[0].parents.push('" + parent + "');\n";
  if (suspendTime.count() >= 0) {
    file << "data[0].values.push(" + std::to_string(suspendTime.count()) + ");\n";
  } else {
    file << "data[0].values.push(-1);\n";
  }
  return stack_trace;
}

static void printSuspensions(const string &parent,
                             jlong coroutine_id,
                             std::ofstream &file,
                             const TraceStorage &storage) {
  auto coroutine_info = storage.getCoroutineInfo(coroutine_id);
  auto const coroutineName = string("coroutine#" + std::to_string(coroutine_id));
  auto running_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::nanoseconds(coroutine_info->running_time));

  file << "data[0].labels.push('" + coroutineName + "');\n";
  file << "data[0].parents.push('" + parent + "');\n";
  file << "data[0].values.push(0);\n";

  file << "data[0].labels.push('running');\n";
  file << "data[0].parents.push('" + coroutineName + "');\n";
  file << "data[0].values.push(" + std::to_string(running_time.count()) + ");\n";

  logInfo("Suspensions: " + std::to_string(coroutine_info->suspensions_list->size()));
  std::list<std::shared_ptr<SuspensionInfo>> chain;
  shared_ptr<SuspensionInfo> lastSuspensionInfo;
  std::function<void(shared_ptr<SuspensionInfo>)>
      suspensionPrint =
      [&file, &coroutineName, &chain, &lastSuspensionInfo](const shared_ptr<SuspensionInfo> &suspension) {
        if (lastSuspensionInfo != nullptr && suspension->start < lastSuspensionInfo->end) {
          chain.push_front(suspension);
        } else {
          if (!chain.empty()) {
            auto chain_parent = std::make_unique<string>(coroutineName);
            while (!chain.empty()) {
              auto current = chain.front();
              chain.pop_front();
              chain_parent = printSuspension(current, file, *chain_parent);
            }
          }
          printSuspension(suspension, file, coroutineName);
        }
        lastSuspensionInfo = suspension;
      };
  coroutine_info->suspensions_list->forEach(suspensionPrint);
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
    auto suspensions = storage.getCoroutineInfo(trace_info.coroutine_id);
    printSuspensions("", trace_info.coroutine_id, file, storage);
    file << *bottom_half;
  } else {
    throw std::runtime_error("Failed to open file" + path);
  }
}

}  // namespace kotlin_tracer
