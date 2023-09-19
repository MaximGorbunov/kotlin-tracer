#include "suspensionPlot.hpp"
#include <string>
#include <fstream>
#include <list>
#include <atomic>
#include <sstream>
#include <utility>

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
      std::chrono::nanoseconds((suspension->end - suspension->start).get()));
  for (auto &frame : *suspension->suspension_stack_trace) {
    stack_trace_stream << *frame->frame << "<br>";
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

struct TraceCount;
struct Level {
  std::list<TraceCount> traces;
};

struct TraceCount {
  int id;
  std::unique_ptr<std::string> method;
  int64_t count;
  unique_ptr<Level> next;
};

static void printTraceLevel(Level *level, std::ofstream &file, const std::string &parent) {
  if (level != nullptr) {
    for (const auto &item : level->traces) {
      auto trace_name = "_[" + std::to_string(item.id) + "]_" + *item.method;
      file << "data[0].labels.push('" + trace_name + "');\n";
      file << "data[0].parents.push('" + parent + "');\n";
      file << "data[0].values.push(" + std::to_string(item.count) + ");\n";
      printTraceLevel(item.next.get(), file, trace_name);
    }
  }
}

static std::atomic_int counter = 0;

static void printTraces(
    const TraceStorage::Traces &traces,
    std::ofstream &file,
    const string &parent,
    const TraceInfo &trace_info) {
  unique_ptr<Level> root = std::make_unique<Level>(Level{});

  std::function<void(shared_ptr<ProcessedTraceRecord>)> func = [&root, &trace_info](
      const shared_ptr<ProcessedTraceRecord> &trace_record) {
    Level *current_level = root.get();
    for (auto frame = trace_record->stack_trace->end(); frame != trace_record->stack_trace->begin();) {
      --frame;
      if (trace_record->time >= trace_info.start && trace_record->time <= trace_info.end) {
        auto method = std::make_unique<string>(*(*frame)->base + ":" + *(*frame)->frame);
        bool found = false;
        for (auto &trace_count : current_level->traces) {
          if (*trace_count.method == *method) {
            found = true;
            ++trace_count.count;
            current_level = trace_count.next.get();
          }
        }
        if (!found) {
          auto next_level = std::make_unique<Level>(Level{});
          current_level->traces.push_back(TraceCount{
            ++counter,
            std::move(method),
            1L,
            std::move(next_level)});
          current_level = current_level->traces.back().next.get();
        }
      }
    }
  };
  traces->forEach(func);
  printTraceLevel(root.get(), file, parent);
}

static void print_trace_info(const string &parent,
                             const TraceInfo &trace_info,
                             jlong coroutine_id,
                             std::ofstream &file,
                             const TraceStorage &storage) {
  auto coroutine_info = storage.getCoroutineInfo(coroutine_id);
  if (coroutine_info == nullptr) {
    logDebug("Coroutine:" + std::to_string(coroutine_id) + "not found");
    throw std::runtime_error("Coroutine:" + std::to_string(coroutine_id) + "not found");
  }
  auto const coroutineName = string("coroutine#" + std::to_string(coroutine_id));
  auto running_wall_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::nanoseconds(coroutine_info->wall_clock_running_time.get()));
  auto running_cpu_user_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::microseconds(coroutine_info->cpu_user_clock_running_time_us.get()));
  auto running_cpu_system_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::microseconds(coroutine_info->cpu_system_clock_running_time_us.get()));

  file << "data[0].labels.push('" + coroutineName + "');\n";
  file << "data[0].parents.push('" + parent + "');\n";
  file << "data[0].values.push(0);\n";

  auto running_span_name = "'running#" + std::to_string(coroutine_id) + "'";
  file << "data[0].labels.push(" + running_span_name + ");\n";
  file << "data[0].parents.push('" + coroutineName + "');\n";
  file << "data[0].values.push(" + std::to_string(running_wall_clock_time.count()) + ");\n";

  file << "data[0].labels.push('running_cpu_user#" + std::to_string(coroutine_id) + "');\n";
  file << "data[0].parents.push(" + running_span_name + ");\n";
  file << "data[0].values.push(" + std::to_string(running_cpu_user_clock_time.count()) + ");\n";

  file << "data[0].labels.push('running_cpu_system#" + std::to_string(coroutine_id) + "');\n";
  file << "data[0].parents.push(" + running_span_name + ");\n";
  file << "data[0].values.push(" + std::to_string(running_cpu_system_clock_time.count()) + ");\n";

  file << "data[0].labels.push('voluntary_switches#" + std::to_string(coroutine_id) + "');\n";
  file << "data[0].parents.push(" + running_span_name + ");\n";
  file << "data[0].values.push(" + std::to_string(coroutine_info->voluntary_switches) + ");\n";

  file << "data[0].labels.push('involuntary_switches#" + std::to_string(coroutine_id) + "');\n";
  file << "data[0].parents.push(" + running_span_name + ");\n";
  file << "data[0].values.push(" + std::to_string(coroutine_info->involuntary_switches) + ");\n";

  auto traces = storage.getProcessedTraces(coroutine_id);
  if (traces != nullptr) {
    printTraces(traces, file, "running#" + std::to_string(coroutine_id), trace_info);
  }

  std::list<std::shared_ptr<SuspensionInfo>> chain;
  shared_ptr<SuspensionInfo> lastSuspensionInfo;
  std::function<void(shared_ptr<SuspensionInfo>)> suspensionPrint =
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
    std::function<void(jlong)> childCoroutinePrint =
        [&file, &coroutineName, &storage, &trace_info](jlong childCoroutine) {
          print_trace_info(coroutineName, trace_info, childCoroutine, file, storage);
        };
    auto children = storage.getChildCoroutines(coroutine_id);
    children->forEach(childCoroutinePrint);
  }
}

static void print_gc_info(const string &parent,
                          const TraceInfo &trace_info,
                          std::ofstream &file,
                          const TraceStorage &storage) {
  int gc_event_counter = 0;
  std::function<void(shared_ptr<TraceStorage::GCEvent>)> for_each =
      [&gc_event_counter, &parent, &file](const shared_ptr<TraceStorage::GCEvent> &gc_event) {
        auto gc_time = gc_event->end - gc_event->start;
        auto running_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::nanoseconds(gc_time.get()));
        file << "data[0].labels.push('#" + std::to_string(++gc_event_counter) + "GC');\n";
        file << "data[0].parents.push('" + parent + "');\n";
        file << "data[0].values.push(" + std::to_string(running_time.count()) + ");\n";
      };
  storage.findGcEvents(trace_info.start, trace_info.end, for_each);
}

void plot(const string &path, const TraceInfo &trace_info, const TraceStorage &storage) {
  std::ofstream file;
  file.open(path);
  if (file.is_open()) {
    file << *top_half;
    auto suspensions = storage.getCoroutineInfo(trace_info.coroutine_id);
    print_trace_info("", trace_info, trace_info.coroutine_id, file, storage);
    auto const coroutineName = string("coroutine#" + std::to_string(trace_info.coroutine_id));
    print_gc_info(coroutineName, trace_info, file, storage);
    file << *bottom_half;
  } else {
    throw std::runtime_error("Failed to open file" + path);
  }
}

}  // namespace kotlin_tracer
