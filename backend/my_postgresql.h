#ifndef MY_POSTGRES_H
#define MY_POSTGRES_H

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <map>
#include "clauses/common.h"
#include "my_duckdb.h" // for MyDuckDB::compare

class MyPostgres
{
public:
    struct results_info
    {
        struct diff
        {
            std::string table;
            std::string op;
            std::string info;
        };
        std::vector<diff> diffs;
    };

    // Connects using service=grading (from ~/.pg_service.conf) or env vars
    MyPostgres();
    explicit MyPostgres(const std::string &conninfo);

    // For SELECT queries
    std::vector<std::vector<std::string>>
    execute_query_select(const std::string &query, std::string &error);

    // For DDL/DML: return diff‐matrix of changes
    std::vector<std::vector<std::string>> execute_query_not_select(const std::string &query, std::string &error);

    // For simple CUD statements
    bool execute_query_cud(const std::string &query, std::string &error);

    // Build results_info from raw diff‐rows
    static results_info get_info(const std::vector<std::vector<std::string>> &data);

    // Compare two results_info objects (reuses DuckDB comparator)
    static Common::comparision_result compare(const MyDuckDB::results_info &ref, const MyDuckDB::results_info &stu);

    // Get column types via information_schema
    std::vector<std::string> get_column_types(const std::string &table_name);

    /// Remove all rows from every public table and reset any sequences.
    /// Throws std::runtime_error on failure.
    void clear();

private:
    pqxx::connection conn;
};

#endif // MY_POSTGRES_H
