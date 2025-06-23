#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include "admin.h"
#include "my_duckdb.h"
#include "my_postgresql.h"

/// hides all DBMS-specifics behind these two calls
class Query_Engine
{
public:
    explicit Query_Engine(const Admin::database_options &opts);
    /// load schema + initial data
    void initialize();

    /// run a SELECT-style query (no side-effects)
    std::vector<std::vector<std::string>>
    execute_select(const std::string &sql);

    /// run a DML/DDL query and return the “diff” rows
    std::vector<std::vector<std::string>>
    execute_non_select(const std::string &sql, std::string &error);

    /// Clears all tables in the chosen backend so you can run again from a blank slate.
    /// Throws on failure.
    void clear();

private:
    Admin::database_options opts_;
    bool use_pg_;
    std::unique_ptr<MyDuckDB> duckdb_;
    std::unique_ptr<MyPostgres> pg_;
};

#endif // QUERY_ENGINE_H
