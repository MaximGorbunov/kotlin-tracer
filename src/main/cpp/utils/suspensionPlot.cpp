#include "suspensionPlot.hpp"

#include <atomic>
#include <fstream>
#include <list>
#include <string>
#include <utility>

#include "log.h"
#include "protos/perfetto/trace/trace.pb.h"

namespace kotlin_tracer {
struct Level;
using std::string, std::unique_ptr, std::shared_ptr, std::chrono::duration_cast,
    std::chrono::milliseconds, std::chrono::nanoseconds;

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

static inline void add_suspension(const shared_ptr<SuspensionInfo>& suspension,
                                  uint64_t* track_counter, const string& coroutine_name,
                                  perfetto::protos::Trace& trace, uint32_t sequence_id,
                                  const TraceTime& trace_end_time) {
  auto track_id = ++*track_counter;
  auto packet = trace.add_packet();
  packet->set_trusted_packet_sequence_id(sequence_id);
  auto track_descriptor = packet->mutable_track_descriptor();
  track_descriptor->set_name(coroutine_name);
  track_descriptor->set_uuid(track_id);
  // start
  packet = trace.add_packet();
  packet->set_timestamp(suspension->start.value);  // nanos
  auto track_event = packet->mutable_track_event();
  track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_BEGIN);
  track_event->set_name("Suspension");
  track_event->set_track_uuid(track_id);
  packet->set_trusted_packet_sequence_id(sequence_id);

  // end
  packet = trace.add_packet();
  track_event = packet->mutable_track_event();
  track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_END);
  track_event->set_track_uuid(track_id);
  if (suspension->end.value != 0) {
    packet->set_timestamp(suspension->end.value);  // nanos
  } else {
    packet->set_timestamp(trace_end_time.value);
  }
  packet->set_trusted_packet_sequence_id(sequence_id);
  auto event_callstack = track_event->mutable_callstack();
  for (auto& frame : *suspension->suspension_stack_trace) {
    auto event_callstack_frame = event_callstack->add_frames();
    auto method = *frame->base + ":" + *frame->frame;
    event_callstack_frame->set_function_name(method);
  }
}

static void add_call_stack(const TraceStorage::Traces& traces, const TraceInfo& trace_info,
                           perfetto::protos::Trace& trace, uint32_t sequence_id,
                           uint64_t track_id) {
  std::function<void(shared_ptr<ProcessedTraceRecord>)> func =
      [&trace_info, &trace, &track_id,
       &sequence_id](const shared_ptr<ProcessedTraceRecord>& trace_record) {
        if (trace_record->stack_trace->size() > 0) {
          auto packet = trace.add_packet();
          packet->set_timestamp(trace_record->time.get());  // nanos
          auto track_event = packet->mutable_track_event();
          track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_INSTANT);
          track_event->set_name("Stack trace");
          track_event->set_track_uuid(track_id);
          packet->set_trusted_packet_sequence_id(sequence_id);
          auto track_event_callstack = track_event->mutable_callstack();
          for (auto frame = trace_record->stack_trace->begin();
               frame != trace_record->stack_trace->end(); ++frame) {
            if (trace_record->time >= trace_info.start && trace_record->time <= trace_info.end) {
              auto method = *(*frame)->base + ":" + *(*frame)->frame;
              auto track_event_callstack_frame = track_event_callstack->add_frames();
              track_event_callstack_frame->set_function_name(method);
            }
          }
        }
      };
  traces->forEach(func);
}

static void add_gc_info(const TraceInfo& trace_info, const TraceStorage& storage,
                        uint64_t* track_counter, perfetto::protos::Trace& trace,
                        uint32_t sequence_id) {
  std::function<void(shared_ptr<TraceStorage::GCEvent>)> for_each =
      [&track_counter, &trace, &sequence_id](const shared_ptr<TraceStorage::GCEvent>& gc_event) {
        auto track_id = ++*track_counter;
        auto packet = trace.add_packet();
        packet->set_trusted_packet_sequence_id(sequence_id);
        auto track_descriptor = packet->mutable_track_descriptor();
        track_descriptor->set_name("GC");
        track_descriptor->set_uuid(track_id);
        // start
        packet = trace.add_packet();
        packet->set_timestamp(gc_event->start.value);  // nanos
        auto track_event = packet->mutable_track_event();
        track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_BEGIN);
        track_event->set_name("GC");
        track_event->set_track_uuid(track_id);
        packet->set_trusted_packet_sequence_id(sequence_id);

        // end
        packet = trace.add_packet();
        packet->set_timestamp(gc_event->end.value);  // nanos
        track_event = packet->mutable_track_event();
        track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_END);
        track_event->set_track_uuid(track_id);
        packet->set_trusted_packet_sequence_id(sequence_id);
      };
  storage.findGcEvents(trace_info.start, trace_info.end, for_each);
}

static void add_trace_info(const TraceInfo& trace_info, const TraceStorage& storage,
                           uint64_t* track_counter, perfetto::protos::Trace& trace,
                           uint32_t sequence_id) {
  struct coroutine_queue_item {
    jlong coroutine_id;
    jlong parent_coroutine_id;
  };
  std::vector<coroutine_queue_item> coroutines{};
  coroutines.push_back(coroutine_queue_item{trace_info.coroutine_id, 0});
  std::set<jlong> visited_coroutines{};
  while (!coroutines.empty()) {
    auto [coroutine_id, parent_coroutine_id] = coroutines.front();
    visited_coroutines.insert(coroutine_id);
    coroutines.erase(coroutines.begin());
    auto coroutine_info = storage.getCoroutineInfo(coroutine_id);
    if (coroutine_info == nullptr) {
      logDebug("Coroutine:" + std::to_string(coroutine_id) + "not found");
      // throw std::runtime_error("Coroutine:" + std::to_string(coroutine_id) + "not found");
      continue;
    }
    auto const coroutineName = string("coroutine#" + std::to_string(coroutine_id));
    auto running_wall_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::nanoseconds(coroutine_info->wall_clock_running_time.get()));
    auto running_cpu_user_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds(coroutine_info->cpu_user_clock_running_time_us.get()));
    auto running_cpu_system_clock_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::microseconds(coroutine_info->cpu_system_clock_running_time_us.get()));

    auto track_id = ++*track_counter;
    auto packet = trace.add_packet();
    packet->set_trusted_packet_sequence_id(sequence_id);
    auto track_descriptor = packet->mutable_track_descriptor();
    track_descriptor->set_name(coroutineName);
    track_descriptor->set_uuid(track_id);
    // start
    packet = trace.add_packet();
    if (parent_coroutine_id == 0) {
      packet->set_timestamp(trace_info.start.value);  // nanos
    } else {
      packet->set_timestamp(coroutine_info->start.value);
    }
    auto track_event = packet->mutable_track_event();
    track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_BEGIN);
    track_event->set_name(coroutineName);
    track_event->set_track_uuid(track_id);
    auto flow_ids = track_event->mutable_flow_ids();
    if (parent_coroutine_id == 0) {
      flow_ids->Add(coroutine_id);
    } else {
      flow_ids->Add(parent_coroutine_id);
      flow_ids->Add(coroutine_id);
    }
    packet->set_trusted_packet_sequence_id(sequence_id);
    auto wall_clock_time_annotation = track_event->add_debug_annotations();
    wall_clock_time_annotation->set_name("Wall CPU time ms");
    wall_clock_time_annotation->set_int_value(running_wall_clock_time.count());
    auto user_clock_time_annotation = track_event->add_debug_annotations();
    user_clock_time_annotation->set_name("User CPU time ms");
    user_clock_time_annotation->set_int_value(running_cpu_user_clock_time.count());
    auto system_clock_time_annotation = track_event->add_debug_annotations();
    system_clock_time_annotation->set_name("System CPU time ms");
    system_clock_time_annotation->set_int_value(running_cpu_system_clock_time.count());
    if (coroutine_info->voluntary_switches == 0) {
      auto voluntary_switches_annotation = track_event->add_debug_annotations();
      voluntary_switches_annotation->set_name("Voluntary switches");
      voluntary_switches_annotation->set_int_value(coroutine_info->voluntary_switches);
    }
    if (coroutine_info->involuntary_switches == 0) {
      auto involountary_switches_annotation = track_event->add_debug_annotations();
      involountary_switches_annotation->set_name("Involuntary switches");
      involountary_switches_annotation->set_int_value(coroutine_info->involuntary_switches);
    }

    // end
    packet = trace.add_packet();
    if (parent_coroutine_id == 0) {
      packet->set_timestamp(trace_info.end.value);  // nanos
    } else {
      packet->set_timestamp(coroutine_info->end.value);
    }
    track_event = packet->mutable_track_event();
    track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_END);
    track_event->set_track_uuid(track_id);
    packet->set_trusted_packet_sequence_id(sequence_id);

    auto traces = storage.getProcessedTraces(coroutine_id);
    auto samples_track_id = ++*track_counter;
    packet = trace.add_packet();
    packet->set_trusted_packet_sequence_id(sequence_id);
    track_descriptor = packet->mutable_track_descriptor();
    track_descriptor->set_name("Samples");
    track_descriptor->set_uuid(samples_track_id);
    if (traces != nullptr) {
      add_call_stack(traces, trace_info, trace, sequence_id, samples_track_id);
    }
    std::function<void(shared_ptr<SuspensionInfo>)> suspensionPrint =
        [&track_counter, &trace, &sequence_id, &coroutineName,
         &trace_info](const shared_ptr<SuspensionInfo>& suspension) {
          add_suspension(suspension, track_counter, coroutineName, trace, sequence_id,
                         trace_info.end);
        };
    coroutine_info->suspensions_list->forEach(suspensionPrint);
    parent_coroutine_id = coroutine_id;
    if (storage.containsChildCoroutineStorage(coroutine_id)) {
      std::function<void(jlong)> child_coroutine_add =
          [&coroutines, &parent_coroutine_id, &visited_coroutines](jlong child_coroutine) {
            if (!visited_coroutines.contains(child_coroutine)) {
              coroutines.push_back(coroutine_queue_item{child_coroutine, parent_coroutine_id});
            }
          };
      auto children = storage.getChildCoroutines(coroutine_id);
      children->forEach(child_coroutine_add);
    }
  }
}

void write_perfetto(const std::string& path, const TraceInfo& trace_info,
                    const TraceStorage& storage) {
  perfetto::protos::Trace trace;
  const uint32_t TRUSTED_SEQUENCE_ID = 1;
  uint64_t track_counter = 0;

  // Track descriptor
  auto packet = trace.add_packet();
  packet->set_trusted_packet_sequence_id(TRUSTED_SEQUENCE_ID);
  auto track_descriptor = packet->mutable_track_descriptor();
  track_descriptor->set_name("Trace");
  uint64_t track_id = ++track_counter;
  track_descriptor->set_uuid(track_id);

  packet = trace.add_packet();
  packet->set_timestamp(trace_info.start.value);  // nanos
  auto track_event = packet->mutable_track_event();
  track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_BEGIN);
  track_event->set_name("Trace");
  track_event->set_track_uuid(track_id);
  auto add_debug_annotations = track_event->add_debug_annotations();
  add_debug_annotations->set_name("Trace");
  packet->set_trusted_packet_sequence_id(TRUSTED_SEQUENCE_ID);

  // end
  packet = trace.add_packet();
  packet->set_timestamp(trace_info.end.value);  // nanos
  track_event = packet->mutable_track_event();
  track_event->set_type(perfetto::protos::TrackEvent_Type_TYPE_SLICE_END);
  track_event->set_track_uuid(track_id);
  packet->set_trusted_packet_sequence_id(TRUSTED_SEQUENCE_ID);

  add_gc_info(trace_info, storage, &track_counter, trace, track_id);
  add_trace_info(trace_info, storage, &track_counter, trace, TRUSTED_SEQUENCE_ID);

  std::fstream output(path, std::ios::out | std::ios::trunc | std::ios::binary);

  // Serialize the message and write it to the file
  if (!trace.SerializeToOstream(&output)) {
    std::cerr << "Failed to write perfetto trace" << std::endl;
  }
}
}  // namespace kotlin_tracer
