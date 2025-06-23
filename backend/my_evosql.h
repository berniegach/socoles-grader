#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <string>
#include <vector>
#include "my_duckdb.h"
/**
 * NB:
 * EvoSQL is case sensitive. Therefore, the table names in the model query should be in lower case.
 */
class MyEvoSQL
{
public:
    MyEvoSQL();
    MyEvoSQL(std::string model_sql_file, std::string reference_query_sql_file, std::string database, MyDuckDB &my_duckdb);
    bool init(const std::string command, std::string model_sql_file, std::string reference_query_sql_file, std::string database, MyDuckDB &my_duckdb);
    std::vector<std::string> execute(std::string command);
    std::vector<std::string> extract_insert_statements(const std::string &fixture_line);
    std::vector<MyDuckDB *> get_databases() const;
    bool create_database(MyDuckDB &my_duckdb, const std::string reference_query_sql_file, std::vector<std::string> insert_statements);
    const std::string read_model_query(const std::string &model_sql_file);
    void write_statements(const std::vector<std::string> &insert_statements, std::string label);

private:
    std::string postgresql_username = "benard";
    std::string postgresql_password = "hjkl";
    // std::vector<MyDuckDB> databases;
};