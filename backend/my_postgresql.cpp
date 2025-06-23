// MyPostgres.cpp
#include "my_postgresql.h"
#include <regex>
#include <algorithm>
#include <sstream>
#include "clauses/common.h"

using namespace pqxx;

MyPostgres::MyPostgres()
    : conn() // uses PGSERVICE or environment defaults
{
    if (!conn.is_open())
    {
        throw std::runtime_error("Postgres connection failed (check PGSERVICE)");
    }
}
MyPostgres::MyPostgres(const std::string &conninfo)
    : conn(conninfo)
{
    if (!conn.is_open())
    {
        throw std::runtime_error("Postgres connection failed for '" + conninfo + "'");
    }
}

std::vector<std::vector<std::string>>
MyPostgres::execute_query_select(const std::string &query, std::string &error)
{
    std::vector<std::vector<std::string>> data;
    try
    {
        work tx{conn};
        auto res = tx.exec(query);
        for (auto const &row : res)
        {
            std::vector<std::string> rec;
            rec.reserve(row.size());
            for (auto const &field : row)
            {
                rec.push_back(field.c_str());
            }
            data.push_back(std::move(rec));
        }
        tx.abort(); // rollback
    }
    catch (const sql_error &e)
    {
        error = e.what();
    }
    return data;
}

std::vector<std::vector<std::string>>
MyPostgres::execute_query_not_select(const std::string &query, std::string &error)
{
    std::vector<std::vector<std::string>> data;

    // Strip explicit transaction keywords
    std::regex txrx(R"(\b(begin|commit|rollback)\b)", std::regex::icase);
    std::string clean_q = std::regex_replace(query, txrx, "");

    try
    {
        work tx{conn};

        // 1) Snapshot tables before
        std::vector<std::string> before_tables;
        {
            auto tbls = tx.exec(
                "SELECT tablename FROM pg_catalog.pg_tables "
                "WHERE schemaname='public';");
            for (auto const &r : tbls)
            {
                before_tables.push_back(r[0].c_str());
            }
        }

        // 2) Snapshot full data before
        std::map<std::string, std::vector<std::vector<std::string>>> before_state;
        for (auto const &tbl : before_tables)
        {
            std::ostringstream q;
            q << "SELECT * FROM \"" << tbl << "\";";
            auto res = tx.exec(q.str());
            std::vector<std::vector<std::string>> rows;
            rows.reserve(res.size());
            for (auto const &r : res)
            {
                std::vector<std::string> rec;
                rec.reserve(r.size());
                for (auto const &f : r)
                {
                    rec.push_back(f.c_str());
                }
                rows.push_back(std::move(rec));
            }
            before_state[tbl] = std::move(rows);
        }

        // 3) Snapshot schema before
        std::map<std::string, std::vector<std::string>> before_schema;
        for (auto const &tbl : before_tables)
        {
            std::ostringstream q;
            q << "SELECT column_name FROM information_schema.columns "
                 "WHERE table_schema='public' AND table_name='"
              << tbl << "' ORDER BY ordinal_position;";
            auto res = tx.exec(q.str());
            std::vector<std::string> cols;
            cols.reserve(res.size());
            for (auto const &r : res)
            {
                cols.push_back(r[0].c_str());
            }
            before_schema[tbl] = std::move(cols);
        }

        // capture column types before
        std::map<std::string, std::vector<std::string>> before_types;
        for (auto const &tbl : before_tables)
        {
            std::ostringstream q;
            q << "SELECT data_type FROM information_schema.columns "
                 "WHERE table_schema='public' AND table_name='"
              << tbl << "' ORDER BY ordinal_position;";
            auto res = tx.exec(q.str());
            std::vector<std::string> types;
            types.reserve(res.size());
            for (auto const &r : res)
            {
                types.push_back(r[0].c_str());
            }
            before_types[tbl] = std::move(types);
        }

        // 4) Execute user DDL/DML
        try
        {
            tx.exec(clean_q);
        }
        catch (const sql_error &e)
        {
            std::string msg = e.what();
            std::string lower = msg;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            if (lower.find("constraint") != std::string::npos)
            {
                std::vector<std::string> row = {"constraint error"};
                if (lower.find("unique") != std::string::npos ||
                    lower.find("primary key") != std::string::npos)
                {
                    row.push_back("unique constraint");
                }
                else if (lower.find("foreign key") != std::string::npos)
                {
                    row.push_back("foreign key constraint");
                }
                else if (lower.find("check") != std::string::npos)
                {
                    row.push_back("check constraint");
                }
                else if (lower.find("not null") != std::string::npos)
                {
                    row.push_back("not null constraint");
                }
                else
                {
                    row.push_back("other constraint");
                }
                data.push_back(std::move(row));
            }
            else if (lower.find("permission") != std::string::npos ||
                     lower.find("access denied") != std::string::npos)
            {
                data.push_back({"permission error", "access denied"});
            }
            tx.abort();
            return data;
        }

        // 5) Snapshot tables after
        std::vector<std::string> after_tables;
        {
            auto tbls = tx.exec(
                "SELECT tablename FROM pg_catalog.pg_tables "
                "WHERE schemaname='public';");
            for (auto const &r : tbls)
            {
                after_tables.push_back(r[0].c_str());
            }
        }

        // 6) Snapshot full data after
        std::map<std::string, std::vector<std::vector<std::string>>> after_state;
        for (auto const &tbl : after_tables)
        {
            std::ostringstream q;
            q << "SELECT * FROM \"" << tbl << "\";";
            auto res = tx.exec(q.str());
            std::vector<std::vector<std::string>> rows;
            rows.reserve(res.size());
            for (auto const &r : res)
            {
                std::vector<std::string> rec;
                rec.reserve(r.size());
                for (auto const &f : r)
                {
                    rec.push_back(f.c_str());
                }
                rows.push_back(std::move(rec));
            }
            after_state[tbl] = std::move(rows);
        }

        // 7) Snapshot schema after
        std::map<std::string, std::vector<std::string>> after_schema;
        for (auto const &tbl : after_tables)
        {
            std::ostringstream q;
            q << "SELECT column_name FROM information_schema.columns "
                 "WHERE table_schema='public' AND table_name='"
              << tbl << "' ORDER BY ordinal_position;";
            auto res = tx.exec(q.str());
            std::vector<std::string> cols;
            cols.reserve(res.size());
            for (auto const &r : res)
            {
                cols.push_back(r[0].c_str());
            }
            after_schema[tbl] = std::move(cols);
        }

        // capture column types after
        std::map<std::string, std::vector<std::string>> after_types;
        for (auto const &tbl : after_tables)
        {
            std::ostringstream q;
            q << "SELECT data_type FROM information_schema.columns "
                 "WHERE table_schema='public' AND table_name='"
              << tbl << "' ORDER BY ordinal_position;";
            auto res = tx.exec(q.str());
            std::vector<std::string> types;
            types.reserve(res.size());
            for (auto const &r : res)
            {
                types.push_back(r[0].c_str());
            }
            after_types[tbl] = std::move(types);
        }

        tx.abort(); // rollback everything

        // 8) Detect diffs

        // 8a) Column additions/removals
        for (auto const &tbl : before_tables)
        {
            if (after_schema.count(tbl) && before_schema.count(tbl))
            {
                auto const &bcols = before_schema[tbl];
                auto const &acols = after_schema[tbl];
                for (auto const &c : acols)
                {
                    if (std::find(bcols.begin(), bcols.end(), c) == bcols.end())
                    {
                        data.push_back({tbl, "column_added", c});
                    }
                }
                for (auto const &c : bcols)
                {
                    if (std::find(acols.begin(), acols.end(), c) == acols.end())
                    {
                        data.push_back({tbl, "column_removed", c});
                    }
                }
            }
        }

        // detect column type changes
        for (auto const &tbl : before_tables)
        {
            if (after_schema.count(tbl) && before_schema.count(tbl))
            {
                auto const &bcols = before_schema[tbl];
                auto const &acols = after_schema[tbl];
                auto const &btypes = before_types[tbl];
                auto const &atypes = after_types[tbl];
                for (size_t i = 0; i < bcols.size(); ++i)
                {
                    const auto &col = bcols[i];
                    auto it = std::find(acols.begin(), acols.end(), col);
                    if (it != acols.end())
                    {
                        size_t j = std::distance(acols.begin(), it);
                        if (btypes[i] != atypes[j])
                        {
                            data.push_back({tbl,
                                            "column_type_changed",
                                            col,
                                            btypes[i],
                                            atypes[j]});
                        }
                    }
                }
            }
        }

        // 8b) New tables & added rows
        for (auto const &tbl : after_tables)
        {
            if (std::find(before_tables.begin(), before_tables.end(), tbl) ==
                before_tables.end())
            {
                // canonicalize new-table column/type list
                {
                    auto cols = after_schema[tbl];
                    auto types = after_types[tbl];
                    std::vector<std::pair<std::string, std::string>> defs;
                    for (size_t i = 0; i < cols.size(); ++i)
                    {
                        defs.emplace_back(cols[i], types[i]);
                    }
                    std::sort(defs.begin(), defs.end(),
                              [](auto &L, auto &R)
                              { return L.first < R.first; });
                    std::ostringstream canon;
                    for (size_t i = 0; i < defs.size(); ++i)
                    {
                        canon << defs[i].first << " " << defs[i].second;
                        if (i + 1 < defs.size())
                            canon << ", ";
                    }
                    data.push_back({tbl, "table_created", canon.str()});
                }

                auto const &rows = after_state[tbl];
                if (rows.empty())
                {
                    data.push_back({tbl, "created", "Empty table created."});
                }
                else
                {
                    for (auto const &r : rows)
                    {
                        std::string str;
                        for (auto const &f : r)
                            str += f + " ";
                        data.push_back({tbl, "added", str});
                    }
                }
            }
        }

        // 8c) Row removals/additions in existing tables
        for (auto const &tbl : before_tables)
        {
            if (std::find(after_tables.begin(), after_tables.end(), tbl) !=
                after_tables.end())
            {
                auto const &brows = before_state[tbl];
                auto const &arows = after_state[tbl];
                for (auto const &r : brows)
                {
                    if (std::find(arows.begin(), arows.end(), r) == arows.end())
                    {
                        std::string str;
                        for (auto const &f : r)
                            str += f + " ";
                        data.push_back({tbl, "removed", str});
                    }
                }
                for (auto const &r : arows)
                {
                    if (std::find(brows.begin(), brows.end(), r) == brows.end())
                    {
                        std::string str;
                        for (auto const &f : r)
                            str += f + " ";
                        data.push_back({tbl, "added", str});
                    }
                }
            }
        }
    }
    catch (const sql_error &e)
    {
        error = e.what();
    }
    catch (const std::exception &e)
    {
        error = e.what();
    }

    return data;
}

bool MyPostgres::execute_query_cud(const std::string &query, std::string &error)
{
    try
    {
        work tx{conn};
        tx.exec(query);
        tx.commit();
        return true;
    }
    catch (const sql_error &e)
    {
        error = e.what();
        return false;
    }
    catch (const std::exception &e)
    {
        error = e.what();
        return false;
    }
}

std::vector<std::string>
MyPostgres::get_column_types(const std::string &table_name)
{
    std::vector<std::string> types;
    try
    {
        work tx{conn};
        auto res = tx.exec_params(
            "SELECT data_type FROM information_schema.columns "
            "WHERE table_schema='public' AND table_name=$1 "
            "ORDER BY ordinal_position;",
            table_name);
        for (auto const &r : res)
        {
            types.push_back(r[0].c_str());
        }
        tx.abort();
    }
    catch (...)
    {
        // ignore
    }
    return types;
}

MyPostgres::results_info
MyPostgres::get_info(const std::vector<std::vector<std::string>> &data)
{
    results_info out;
    for (auto const &row : data)
    {
        if (row.size() >= 2)
        {
            results_info::diff d{row[0], row[1], row.size() > 2 ? row[2] : ""};
            out.diffs.push_back(d);
        }
    }
    return out;
}

Common::comparision_result MyPostgres::compare(const MyDuckDB::results_info &ref, const MyDuckDB::results_info &stu)
{
    // Reuse your DuckDB comparator
    return MyDuckDB::compare(ref, stu);
}

void MyPostgres::clear()
{
    std::string error;

    // 1) List all tables in public schema
    auto tables = execute_query_select(
        "SELECT tablename FROM pg_catalog.pg_tables WHERE schemaname='public';",
        error);
    if (!error.empty())
        throw std::runtime_error("Failed to list tables: " + error);

    // 2) Drop each table
    for (auto &row : tables)
    {
        const std::string tbl = row[0];
        std::string sql =
            "DROP TABLE IF EXISTS \"" + tbl + "\" CASCADE;";
        if (!execute_query_cud(sql, error))
            throw std::runtime_error("Failed to drop table '" + tbl + "': " + error);
    }
}
