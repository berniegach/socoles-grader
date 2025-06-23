/**
 * @file my_duckdb.h
 * @brief This file contains the declaration of the MyDuckDB class.
 * The class is used to connect to DuckDB and execute queries.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef MY_DUCKDB_H
#define MY_DUCKDB_H

#include "duckdb/duckdb.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "clauses/common.h"

class MyDuckDB
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
    MyDuckDB();
    MyDuckDB(const std::string &filename);
    /**
     * Executes a query and returns the result.
     * @param query: the query to be executed.
     * @param error: the error message if the query fails.
     * @return the result of the query in a 2 dimensional vector.
     */
    std::vector<std::vector<std::string>> execute_query_select(const std::string &query, std::string &error);
    std::vector<std::vector<std::string>> execute_query_not_select(const std::string &query, std::string &error);
    static results_info get_info(const std::vector<std::vector<std::string>> &data);
    static Common::comparision_result compare(const results_info &ref, const results_info &stu);
    bool execute_query_cud(const std::string &query, std::string &error);

    std::vector<std::string> get_column_types(const std::string &table_name);

private:
    duckdb::DuckDB db;
};

#endif // MY_DUCKDB_H