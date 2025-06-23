#include "query_engine.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

Query_Engine::Query_Engine(const Admin::database_options &opts)
    : opts_(opts), use_pg_(opts.use_postgresql)
{
    if (use_pg_)
    {
        // opts_.postgresql_dbname is your PG database name
        std::string conninfo = "dbname=" + opts_.postgresql_dbname;
        pg_ = std::make_unique<MyPostgres>(conninfo);
        std::cout << "Using Postgres backend" << std::endl;
    }
    else
    {
        if (!opts_.is_front_end)
        {
            if (opts_.auto_db == 0)
            {
                duckdb_ = std::make_unique<MyDuckDB>(opts_.sql_file);
            }
            else
            {
                duckdb_ = std::make_unique<MyDuckDB>(opts_.sql_create);
            }
        }
        else
        {
            duckdb_ = std::make_unique<MyDuckDB>();
        }
    }
}

void Query_Engine::initialize()
{
    if (use_pg_)
    {
        if (opts_.is_front_end)
        {
            // If we are using the front end, we need to create the database
            std::string err;
            if (!pg_->execute_query_cud(opts_.sql_create, err))
                throw std::runtime_error("Pg init failed: " + err);
            else
                std::cout << "Postgres database created successfully for front-end." << std::endl;
        }
        // only if we need to load the SQL file into Postgres

        else
        {
            std::ifstream f(opts_.sql_file);
            if (!f.is_open())
                throw std::runtime_error("Cannot open " + opts_.sql_file);
            std::stringstream buf;
            buf << f.rdbuf();
            std::string sql = buf.str(), err;
            if (!pg_->execute_query_cud(sql, err))
                throw std::runtime_error("Pg init failed: " + err);
            else
                std::cout << "Postgres database created successfully for console." << std::endl;
        }
    }
    else
    {
        if (opts_.is_front_end)
        {
            std::string err;
            if (opts_.auto_db == 0)
            {
                if (!duckdb_->execute_query_cud(opts_.sql_file, err))
                    throw std::runtime_error("DuckDB init failed: " + err);
            }
            else
            {
                if (!duckdb_->execute_query_cud(opts_.sql_create, err))
                    throw std::runtime_error("DuckDB init failed: " + err);
            }
        }
    }
}

std::vector<std::vector<std::string>>
Query_Engine::execute_select(const std::string &sql)
{
    std::string err;
    if (use_pg_)
    {
        return pg_->execute_query_select(sql, err);
    }
    else
    {
        return duckdb_->execute_query_select(sql, err);
    }
}

std::vector<std::vector<std::string>>
Query_Engine::execute_non_select(const std::string &sql, std::string &error)
{
    if (use_pg_)
    {
        return pg_->execute_query_not_select(sql, error);
    }
    else
    {
        return duckdb_->execute_query_not_select(sql, error);
    }
}
void Query_Engine::clear()
{
    if (use_pg_)
        pg_->clear();
}
