#include "metrics.h"
#include <cstdlib>
#include <algorithm>

namespace Metrics
{

    static std::atomic<int> g_active{0};
    static std::atomic<long long> g_total_graded{0};
    static std::atomic<long long> g_capacity_rejected{0};
    static std::atomic<long long> g_select_truncated{0};
    static std::atomic<long long> g_snapshot_truncated{0};

    static int load_capacity_from_env()
    {
        int def = 8; // sensible default (was 4)
        if (const char *v = std::getenv("SOCOLES_MAX_CONCURRENT_GRADE"))
        {
            try
            {
                int n = std::max(1, std::stoi(v));
                return n;
            }
            catch (...)
            {
                return def;
            }
        }
        return def;
    }

    int capacity()
    {
        static int cap = load_capacity_from_env();
        return cap;
    }

    int active() { return g_active.load(); }

    bool try_acquire_slot()
    {
        for (;;)
        {
            int cur = g_active.load(std::memory_order_relaxed);
            if (cur >= capacity())
                return false;
            if (g_active.compare_exchange_weak(cur, cur + 1, std::memory_order_acq_rel))
                return true;
        }
    }

    void release_slot()
    {
        g_active.fetch_sub(1, std::memory_order_acq_rel);
    }

    void increment_capacity_reject() { g_capacity_rejected.fetch_add(1, std::memory_order_relaxed); }
    void increment_total_graded() { g_total_graded.fetch_add(1, std::memory_order_relaxed); }
    void increment_select_truncated() { g_select_truncated.fetch_add(1, std::memory_order_relaxed); }
    void increment_snapshot_truncated() { g_snapshot_truncated.fetch_add(1, std::memory_order_relaxed); }

    long long total_graded() { return g_total_graded.load(); }
    long long capacity_rejected() { return g_capacity_rejected.load(); }
    long long select_truncated() { return g_select_truncated.load(); }
    long long snapshot_truncated() { return g_snapshot_truncated.load(); }

} // namespace Metrics
