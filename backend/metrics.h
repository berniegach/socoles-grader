// metrics.h - simple process-wide metrics and admission control
#ifndef SOCOLES_METRICS_H
#define SOCOLES_METRICS_H

#include <atomic>

namespace Metrics
{

    // Admission control
    int capacity();          // max concurrent grading requests
    int active();            // current in-flight grading requests
    bool try_acquire_slot(); // increments active if below capacity
    void release_slot();     // decrements active
    void increment_capacity_reject();

    // Counters
    void increment_total_graded();
    void increment_select_truncated();
    void increment_snapshot_truncated();

    // Expose current values
    long long total_graded();
    long long capacity_rejected();
    long long select_truncated();
    long long snapshot_truncated();

} // namespace Metrics

#endif // SOCOLES_METRICS_H
