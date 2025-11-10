#include "limits.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>

static size_t parse_size_t_env(const char *name, size_t def)
{
    if (const char *v = std::getenv(name))
    {
        try
        {
            return static_cast<size_t>(std::stoll(v));
        }
        catch (...)
        {
            return def;
        }
    }
    return def;
}

static int parse_int_env(const char *name, int def)
{
    if (const char *v = std::getenv(name))
    {
        try
        {
            return std::stoi(v);
        }
        catch (...)
        {
            return def;
        }
    }
    return def;
}

static bool parse_bool_env(const char *name, bool def)
{
    if (const char *v = std::getenv(name))
    {
        std::string s(v);
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        if (s == "1" || s == "true" || s == "yes" || s == "on")
            return true;
        if (s == "0" || s == "false" || s == "no" || s == "off")
            return false;
        return def;
    }
    return def;
}

SocolesLimits load_limits_from_env()
{
    SocolesLimits l;
    l.max_select_rows = parse_size_t_env("SOCOLES_MAX_SELECT_ROWS", l.max_select_rows);
    l.max_snapshot_rows = parse_size_t_env("SOCOLES_MAX_SNAPSHOT_ROWS", l.max_snapshot_rows);
    l.statement_timeout_ms = parse_int_env("SOCOLES_PG_STATEMENT_TIMEOUT_MS", l.statement_timeout_ms);
    l.duckdb_memory_limit_mb = parse_size_t_env("SOCOLES_DUCKDB_MEMORY_LIMIT_MB", l.duckdb_memory_limit_mb);
    l.enforce_limit = parse_bool_env("SOCOLES_ENFORCE_LIMIT", l.enforce_limit);
    l.enforced_limit_value = parse_size_t_env("SOCOLES_ENFORCED_LIMIT_VALUE", l.enforced_limit_value);
    return l;
}

const SocolesLimits &get_limits()
{
    static SocolesLimits limits = load_limits_from_env();
    return limits;
}

// if query seems to be a SELECT without LIMIT, append one
std::string maybe_inject_limit(const std::string &sql, const SocolesLimits &limits)
{
    if (!limits.enforce_limit || limits.enforced_limit_value == 0)
        return sql;
    // starts with SELECT (ignoring leading whitespace/comments) and no LIMIT keyword
    auto begin = sql.find_first_not_of(" \t\n\r");
    if (begin == std::string::npos)
        return sql;
    // Check for "select" prefix case-insensitive
    auto starts = sql.substr(begin, 6);
    std::string lower;
    lower.resize(starts.size());
    std::transform(starts.begin(), starts.end(), lower.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    if (lower != "select")
        return sql;

    // If already contains LIMIT (case-insensitive), do nothing
    std::string sql_lower(sql);
    std::transform(sql_lower.begin(), sql_lower.end(), sql_lower.begin(), [](unsigned char c)
                   { return std::tolower(c); });
    if (sql_lower.find(" limit ") != std::string::npos || sql_lower.rfind(" limit", sql_lower.size() - 6) != std::string::npos)
    {
        return sql;
    }

    // Append LIMIT N at the end (before semicolon if any)
    std::string out = sql;
    size_t semi = out.find_last_of(';');
    if (semi != std::string::npos && semi == out.size() - 1)
    {
        out.erase(semi);
    }
    out += " LIMIT ";
    out += std::to_string(limits.enforced_limit_value);
    return out;
}
