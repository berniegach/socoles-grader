// limits.h - runtime configurable safety limits for query execution
#ifndef SOCOLES_LIMITS_H
#define SOCOLES_LIMITS_H

#include <cstddef>
#include <string>

struct SocolesLimits
{
    // Maximum rows returned for a SELECT before truncating with a sentinel row
    size_t max_select_rows = 500; // 0 means unlimited
    // Maximum rows per table to snapshot for diffing (non-select). If exceeded we skip enumerating rows
    size_t max_snapshot_rows = 1000; // 0 unlimited
    // Postgres statement timeout in milliseconds (applied via SET LOCAL). 0 -> no change
    int statement_timeout_ms = 3000;
    // DuckDB memory limit in MB (0 -> leave default)
    size_t duckdb_memory_limit_mb = 512;
    // Whether to force adding LIMIT if user omits one (only for SELECT)
    bool enforce_limit = true;
    // Auto-injected LIMIT value when enforce_limit && query lacks LIMIT
    size_t enforced_limit_value = 500;
};

// Load limits from environment variables if present; fall back to defaults above.
SocolesLimits load_limits_from_env();

// Singleton accessor (lazy init, thread-safe on first call via function-static initialization)
const SocolesLimits &get_limits();

// Utility helpers
std::string maybe_inject_limit(const std::string &sql, const SocolesLimits &limits);
bool contains_limit(const std::string &sql);

#endif // SOCOLES_LIMITS_H
