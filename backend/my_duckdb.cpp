#include "my_duckdb.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

using namespace duckdb;

MyDuckDB::MyDuckDB()
    : db(nullptr)
{
    std::cout << "Using DuckDB (in-memory)" << std::endl;
}

MyDuckDB::MyDuckDB(const std::string &filename)
    : db(nullptr)
{
    using namespace std;
    duckdb::Connection con(db);

    std::cout << "Using DuckDB database: " << std::endl;

    // open the file
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Could not open file: " << filename << endl;
        exit(1);
    }

    // read the contents into a string
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    string query = buffer.str();

    // run the queries
    auto result = con.Query(query);
    if (!result->HasError())
    {
        cout << "Successfully executed queries in file: " << filename << endl;
    }
    else
    {
        cerr << "Error executing queries in file: " << filename << endl;
        cerr << result->GetError() << endl;
        exit(1);
    }
}
std::vector<std::vector<std::string>> MyDuckDB::execute_query_select(const std::string &query, std::string &error)
{
    duckdb::Connection con(db);
    std::vector<std::vector<std::string>> data;
    // we not want this query to persist in the database.
    bool transaction_started = false;
    // there are those queries that might sneak in a begin, commit, rollback, etc.
    // remove those phrases from the queries and proceed with the execution.
    std::regex pattern(R"(\b(begin|commit|rollback)\b)", std::regex_constants::icase);

    // Use regex_replace to replace the matched words with an empty string.
    std::string new_query = std::regex_replace(query, pattern, "");

    try
    {
        con.BeginTransaction();
        transaction_started = true;
        auto result = con.Query(new_query);

        if (result->HasError())
        {
            error = result->GetError();
            con.Rollback();
            transaction_started = false;
            return data;
        }

        for (int row = 0; row < result->RowCount(); row++)
        {
            std::vector<std::string> row_result;
            for (int col = 0; col < result->ColumnCount(); col++)
            {
                row_result.push_back(result->GetValue(col, row).ToString());
            }
            data.push_back(row_result);
        }
        if (transaction_started)
        {
            con.Rollback();
            transaction_started = false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << query << " " << e.what() << '\n';
        if (transaction_started)
            con.Rollback();
    }

    return data;
}

/**
 * Difference Output Matrix Summary:
 *
 * The diffefernce output matrix is a two-dimensional vector (vector<vector<string>>)
 * where each inner vector represents a diff entry corresponding to a change in the
 * database state after executing a non-select SQL statement (such as INSERT, CREATE, etc.).
 *
 * Each inner vector (i.e., each diff row) contains three elements:
 *   1. Table Name: The name of the affected table (e.g., "test_insert").
 *   2. Operation: The type of operation that occurred:
 *         - "added"   : Indicates a new row was inserted.
 *         - "removed" : Indicates a row was deleted.
 *         - "created" : Indicates a table was created.
 *   3. Details: A string providing details about the row (or a message, for example,
 *      "Empty table created." for a new but empty table).
 *
 * Examples:
 *
 * 1. For a basic INSERT statement:
 *      INSERT INTO test_insert (id, name) VALUES (1, 'Alice');
 *
 *    If the table "test_insert" was empty before execution, the diff matrix would be:
 *      {
 *          { "test_insert", "added", "1 Alice " }
 *      }
 *
 * 2. For a multi-row INSERT statement:
 *      INSERT INTO test_insert (id, name) VALUES (3, 'Charlie'), (4, 'David');
 *
 *    The diff matrix would include two entries:
 *      {
 *          { "test_insert", "added", "3 Charlie " },
 *          { "test_insert", "added", "4 David " }
 *      }
 */

std::vector<std::vector<std::string>> MyDuckDB::execute_query_not_select(const std::string &query, std::string &error)
{
    std::vector<std::vector<std::string>> data;
    bool transaction_started = false;

    // Remove any explicit BEGIN/COMMIT/ROLLBACK
    std::regex pattern(R"(\b(begin|commit|rollback)\b)", std::regex_constants::icase);
    std::string new_query = std::regex_replace(query, pattern, "");
    std::string lower_query = new_query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    //  1) Also strip SAVEPOINT, RELEASE, SET TRANSACTION
    std::regex tx_pattern(R"(\b(savepoint|release|set\s+transaction)\b)", std::regex_constants::icase);
    new_query = std::regex_replace(new_query, tx_pattern, "");

    // 2) Trim leading/trailing whitespace and a trailing semicolon
    new_query = std::regex_replace(new_query, std::regex(R"(^\s+|\s+$)"), "");
    if (!new_query.empty() && new_query.back() == ';')
    {
        new_query.pop_back();
    }

    // 3) Reject EXPLAIN/DESCRIBE early
    if (lower_query.rfind("explain", 0) == 0 || lower_query.rfind("describe", 0) == 0)
    {
        data.push_back({"", "unsupported", "EXPLAIN/DESCRIBE statements are not supported"});
        return data;
    }

    duckdb::Connection con(db);
    con.BeginTransaction();
    transaction_started = true;

    // Get list of tables before
    std::vector<std::string> before_tables;
    auto before_tables_result = con.Query("SHOW TABLES;");
    if (before_tables_result->HasError())
    {
        error = before_tables_result->GetError();
        return data;
    }
    for (int row = 0; row < before_tables_result->RowCount(); row++)
    {
        before_tables.push_back(before_tables_result->GetValue(0, row).ToString());
    }

    // Capture full table data before
    std::map<std::string, std::vector<std::vector<std::string>>> before_state;
    for (auto &table : before_tables)
    {
        std::string table_query = "SELECT * FROM " + table + ";";
        auto result = con.Query(table_query);
        if (!result->HasError())
        {
            std::vector<std::vector<std::string>> rows;
            for (int r = 0; r < result->RowCount(); r++)
            {
                std::vector<std::string> row_data;
                for (int c = 0; c < result->ColumnCount(); c++)
                {
                    row_data.push_back(result->GetValue(c, r).ToString());
                }
                rows.push_back(row_data);
            }
            before_state[table] = rows;
        }
    }

    //  4) Capture schema (column names) before execution
    std::map<std::string, std::vector<std::string>> before_schema;
    for (auto &table : before_tables)
    {
        auto schema_res = con.Query("PRAGMA table_info('" + table + "');");
        if (!schema_res->HasError())
        {
            std::vector<std::string> cols;
            for (int r = 0; r < schema_res->RowCount(); r++)
            {
                cols.push_back(schema_res->GetValue(1, r).ToString()); // name column
            }
            before_schema[table] = cols;
        }
    }

    // capture column types before
    std::map<std::string, std::vector<std::string>> before_types;
    for (auto &tbl : before_tables)
    {
        auto type_res = con.Query("PRAGMA table_info('" + tbl + "');");
        if (!type_res->HasError())
        {
            std::vector<std::string> types;
            for (int r = 0; r < type_res->RowCount(); r++)
                types.push_back(type_res->GetValue(2, r).ToString()); // type
            before_types[tbl] = std::move(types);
        }
    }

    // 5) Execute the user query
    auto exec_result = con.Query(new_query);
    if (exec_result->HasError())
    {
        error = "⚠️ Your query failed with error :" + exec_result->GetError();
        std::string err_lower = error;
        std::transform(err_lower.begin(), err_lower.end(), err_lower.begin(), ::tolower);

        // Constraint violations
        if (err_lower.find("constraint") != std::string::npos)
        {
            std::vector<std::string> diff_row = {"constraint error"};
            if (err_lower.find("unique") != std::string::npos || err_lower.find("primary key") != std::string::npos)
            {
                diff_row.push_back("unique constraint");
            }
            else if (err_lower.find("foreign key") != std::string::npos)
            {
                diff_row.push_back("foreign key constraint");
            }
            else if (err_lower.find("check") != std::string::npos)
            {
                diff_row.push_back("check constraint");
            }
            else if (err_lower.find("not null") != std::string::npos)
            {
                diff_row.push_back("not null constraint");
            }
            else
            {
                diff_row.push_back("other constraint");
            }
            data.push_back(diff_row);
        }
        //  Permission errors
        else if (err_lower.find("permission") != std::string::npos ||
                 err_lower.find("access denied") != std::string::npos)
        {
            data.push_back({"permission error", "access denied"});
        }

        if (transaction_started)
        {
            con.Rollback();
            transaction_started = false;
        }
        return data;
    }

    // 6) Get list of tables after
    std::vector<std::string> after_tables;
    auto after_tables_result = con.Query("SHOW TABLES;");
    if (after_tables_result->HasError())
    {
        error = after_tables_result->GetError();
        return data;
    }
    for (int row = 0; row < after_tables_result->RowCount(); row++)
    {
        after_tables.push_back(after_tables_result->GetValue(0, row).ToString());
    }

    // 7) Capture full table data after
    std::map<std::string, std::vector<std::vector<std::string>>> after_state;
    for (auto &table : after_tables)
    {
        std::string table_query = "SELECT * FROM " + table + ";";
        auto result = con.Query(table_query);
        if (!result->HasError())
        {
            std::vector<std::vector<std::string>> rows;
            for (int r = 0; r < result->RowCount(); r++)
            {
                std::vector<std::string> row_data;
                for (int c = 0; c < result->ColumnCount(); c++)
                {
                    row_data.push_back(result->GetValue(c, r).ToString());
                }
                rows.push_back(row_data);
            }
            after_state[table] = rows;
        }
    }

    //  8) Capture schema after execution
    std::map<std::string, std::vector<std::string>> after_schema;
    for (auto &table : after_tables)
    {
        auto schema_res = con.Query("PRAGMA table_info('" + table + "');");
        if (!schema_res->HasError())
        {
            std::vector<std::string> cols;
            for (int r = 0; r < schema_res->RowCount(); r++)
            {
                cols.push_back(schema_res->GetValue(1, r).ToString());
            }
            after_schema[table] = cols;
        }
    }

    // capture column types after
    std::map<std::string, std::vector<std::string>> after_types;
    for (auto &tbl : after_tables)
    {
        auto type_res = con.Query("PRAGMA table_info('" + tbl + "');");
        if (!type_res->HasError())
        {
            std::vector<std::string> types;
            for (int r = 0; r < type_res->RowCount(); r++)
                types.push_back(type_res->GetValue(2, r).ToString());
            after_types[tbl] = std::move(types);
        }
    }

    // 9) Detect column additions/removals in existing tables
    for (const auto &table : before_tables)
    {
        if (after_schema.count(table) && before_schema.count(table))
        {
            const auto &before_cols = before_schema[table];
            const auto &after_cols = after_schema[table];
            // Added
            for (auto &col : after_cols)
            {
                if (std::find(before_cols.begin(), before_cols.end(), col) == before_cols.end())
                {
                    data.push_back({table, "column_added", col});
                }
            }
            // Removed
            for (auto &col : before_cols)
            {
                if (std::find(after_cols.begin(), after_cols.end(), col) == after_cols.end())
                {
                    data.push_back({table, "column_removed", col});
                }
            }
        }
    }

    // detect column type changes
    for (auto &tbl : before_tables)
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

    // 10) Detect new tables & added rows
    for (auto &table : after_tables)
    {
        if (std::find(before_tables.begin(), before_tables.end(), table) == before_tables.end())
        {
            // canonicalize new-table column/type list
            {
                auto cols = after_schema[table];
                auto types = after_types[table];
                std::vector<std::pair<std::string, std::string>> defs;
                for (size_t i = 0; i < cols.size(); ++i)
                    defs.emplace_back(cols[i], types[i]);
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
                data.push_back({table, "table_created", canon.str()});
            }

            auto &rows = after_state[table];
            if (rows.empty())
            {
                data.push_back({table, "created", "Empty table created."});
            }
            else
            {
                for (auto &row : rows)
                {
                    std::string row_str;
                    for (auto &col : row)
                        row_str += col + " ";
                    data.push_back({table, "added", row_str});
                }
            }
        }
    }

    // 11) Detect row removals/additions in existing tables
    for (auto &table : before_tables)
    {
        if (std::find(after_tables.begin(), after_tables.end(), table) != after_tables.end())
        {
            auto &brows = before_state[table];
            auto &arows = after_state[table];
            // Removed
            for (auto &row : brows)
            {
                if (std::find(arows.begin(), arows.end(), row) == arows.end())
                {
                    std::string row_str;
                    for (auto &col : row)
                        row_str += col + " ";
                    data.push_back({table, "removed", row_str});
                }
            }
            // Added
            for (auto &row : arows)
            {
                if (std::find(brows.begin(), brows.end(), row) == brows.end())
                {
                    std::string row_str;
                    for (auto &col : row)
                        row_str += col + " ";
                    data.push_back({table, "added", row_str});
                }
            }
        }
    }

    if (transaction_started)
    {
        con.Rollback();
    }
    return data;
}
// Build a results_info from the raw diff‐rows + error
MyDuckDB::results_info MyDuckDB::get_info(const std::vector<std::vector<std::string>> &data)
{
    results_info out;
    for (auto &row : data)
    {
        if (row.size() >= 2)
        {
            results_info::diff d;
            d.table = row[0];
            d.op = row[1];
            d.info = (row.size() > 2 ? row[2] : "");
            out.diffs.push_back(d);
        }
    }
    return out;
}
Common::comparision_result MyDuckDB::compare(const results_info &ref, const results_info &stu)
{
    Common::comparision_result comp;
    std::ostringstream msg;

    // 2) Missing expected changes
    for (auto &r : ref.diffs)
    {
        bool found = false;
        for (auto &s : stu.diffs)
        {
            if (r.table == s.table && r.op == s.op && r.info == s.info)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            comp.incorrect_parts.push_back(r.op + " on " + r.table);
            if (r.op == "created")
            {
                msg << "❌ You forgot to create the table '" << r.table << "'.\n";
                comp.next_steps.push_back("Add: CREATE TABLE " + r.table + " ...;");
            }
            else if (r.op == "column_added")
            {
                msg << "❌ You did not add the column '" << r.info
                    << "' to table '" << r.table << "'.\n";
                comp.next_steps.push_back("Add column " + r.info + " to " + r.table + ".");
            }
            else if (r.op == "column_removed")
            {
                msg << "❌ You did not remove the column '" << r.info
                    << "' from table '" << r.table << "'.\n";
                comp.next_steps.push_back("Drop column " + r.info + " from " + r.table + ".");
            }
            else if (r.op == "added")
            {
                msg << "❌ You did not insert this row into '" << r.table
                    << "': " << r.info << ".\n";
                comp.next_steps.push_back("INSERT INTO " + r.table + " VALUES(" + r.info + ");");
            }
            else if (r.op == "removed")
            {
                msg << "❌ You did not delete this row from '" << r.table
                    << "': " << r.info << ".\n";
                comp.next_steps.push_back("DELETE FROM " + r.table + " WHERE ...;");
            }
            if (r.table == "constraint error")
            {
                msg << "❌ Your query should have violated a " << r.op
                    << ", but it did not.\n";
                comp.next_steps.push_back("Review the data to trigger the " + r.op + ".");
            }
            else if (r.op == "permission error")
            {
                msg << "❌ You lack permission to modify table '" << r.table << "'.\n";
                comp.next_steps.push_back("Check your GRANT privileges for " + r.table + ".");
            }
            else
            {
                // msg << "❌ You missed: [" << r.op << "] on '" << r.table << "'.\n";
            }
        }
        else
        {
            comp.correct_parts.push_back(r.op + " on " + r.table);
        }
    }

    // 3) Unexpected extra changes
    for (auto &s : stu.diffs)
    {
        bool found = false;
        for (auto &r : ref.diffs)
        {
            if (r.table == s.table && r.op == s.op && r.info == s.info)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            comp.incorrect_parts.push_back(s.op + " on " + s.table);
            if (s.op == "created")
            {
                msg << "⚠️ You created an extra table '" << s.table << "' that wasn’t needed.\n";
                comp.next_steps.push_back("Remove: DROP TABLE " + s.table + ";");
            }
            else if (s.op == "column_added")
            {
                msg << "⚠️ You added an unnecessary column '" << s.info
                    << "' to table '" << s.table << "'.\n";
                comp.next_steps.push_back("Drop column " + s.info + " from " + s.table + ".");
            }
            else if (s.op == "column_removed")
            {
                msg << "⚠️ You removed the column '" << s.info
                    << "' from table '" << s.table << "', but it should stay.\n";
                comp.next_steps.push_back("Re-add column " + s.info + " to " + s.table + ".");
            }
            else if (s.op == "added")
            {
                msg << "⚠️ You inserted an extra row into '" << s.table
                    << "': " << s.info << ".\n";
                comp.next_steps.push_back("Remove that INSERT from your query.");
            }
            else if (s.op == "removed")
            {
                msg << "⚠️ You deleted a row from '" << s.table
                    << "' that shouldn’t have been removed: " << s.info << ".\n";
                comp.next_steps.push_back("Remove that DELETE from your query.");
            }
            if (s.table == "constraint error")
            {
                msg << "⚠️ Your query unexpectedly triggered a " << s.op
                    << ".\n";
                comp.next_steps.push_back("Check why the " + s.op + " was raised.");
            }
            else if (s.op == "permission error")
            {
                msg << "⚠️ You hit a permission error on '" << s.table << "'.\n";
            }
            else
            {
                // msg << "⚠️ Unexpected change: [" << s.op << "] on '" << s.table << "'.\n";
            }
        }
    }

    comp.message = msg.str();
    comp.equal = comp.incorrect_parts.empty();
    return comp;
}

bool MyDuckDB::execute_query_cud(const std::string &query, std::string &error)
{
    duckdb::Connection con(db);
    // there are those queries that might sneak in a begin, commit, rollback, etc.
    // remove those phrases from the queries and proceed with the execution.
    std::regex pattern(R"(\b(begin|commit|rollback)\b)", std::regex_constants::icase);

    // Use regex_replace to replace the matched words with an empty string.
    std::string new_query = std::regex_replace(query, pattern, "");

    try
    {
        auto result = con.Query(new_query);

        if (result->HasError())
        {
            error = result->GetError();
            con.Rollback();
            return false;
        }
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << query << " " << e.what() << '\n';
        return false;
    }
}
