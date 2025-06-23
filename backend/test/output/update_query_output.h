#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(update_output_test_suite)

// Test case 1: Basic UPDATE of a single row.
BOOST_AUTO_TEST_CASE(update_output_test_case_1_basic)
{
    // Create a table with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (1, 'Alice');",
                                       "", "", true};

    // Update the row: change name from 'Alice' to 'Alicia'.
    StudentQuery student_query("update1", "UPDATE test_update SET name = 'Alicia' WHERE id = 1;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false, addedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed" && row[2].find("Alice") != std::string::npos)
                removedFound = true;
            if (row[1] == "added" && row[2].find("Alicia") != std::string::npos)
                addedFound = true;
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "Basic update did not remove the old row 'Alice'.");
    BOOST_CHECK_MESSAGE(addedFound, "Basic update did not add the new row 'Alicia'.");
}

// Test case 2: UPDATE with no change (setting the column to the same value).
BOOST_AUTO_TEST_CASE(update_output_test_case_2_no_change)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (2, 'Bob');",
                                       "", "", true};

    StudentQuery student_query("update2", "UPDATE test_update SET name = 'Bob' WHERE id = 2;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    BOOST_CHECK_MESSAGE(diff_output.empty(), "UPDATE with no change should produce no diff output.");
}

// Test case 3: UPDATE matching multiple rows.
BOOST_AUTO_TEST_CASE(update_output_test_case_3_multiple_rows)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (3, 'Charlie');"
                                       "INSERT INTO test_update VALUES (4, 'Charlie');",
                                       "", "", true};

    // Update all rows where name is 'Charlie' to 'Charles'.
    StudentQuery student_query("update3", "UPDATE test_update SET name = 'Charles' WHERE name = 'Charlie';");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    int removedCount = 0, addedCount = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed" && row[2].find("Charlie") != std::string::npos)
                removedCount++;
            if (row[1] == "added" && row[2].find("Charles") != std::string::npos)
                addedCount++;
        }
    }
    BOOST_CHECK_MESSAGE(removedCount == 2, "UPDATE multiple rows: Expected 2 removed rows for 'Charlie'.");
    BOOST_CHECK_MESSAGE(addedCount == 2, "UPDATE multiple rows: Expected 2 added rows for 'Charles'.");
}

// Test case 4: UPDATE using an expression.
BOOST_AUTO_TEST_CASE(update_output_test_case_4_expression)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (5, 'Daisy');",
                                       "", "", true};

    // Update using a concatenation expression.
    StudentQuery student_query("update4", "UPDATE test_update SET name = name || ' Updated' WHERE id = 5;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false, addedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed" && row[2].find("Daisy") != std::string::npos)
                removedFound = true;
            if (row[1] == "added" && row[2].find("Daisy Updated") != std::string::npos)
                addedFound = true;
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "UPDATE expression: Old row 'Daisy' was not removed.");
    BOOST_CHECK_MESSAGE(addedFound, "UPDATE expression: New row 'Daisy Updated' was not added.");
}

// Test case 5: UPDATE changing a NULL value.
BOOST_AUTO_TEST_CASE(update_output_test_case_5_null_update)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (6, NULL);",
                                       "", "", true};

    StudentQuery student_query("update5", "UPDATE test_update SET name = 'Eve' WHERE id = 6;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false, addedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            // Depending on implementation, NULL might be represented as "NULL" or an empty string.
            if (row[1] == "removed" && (row[2].find("NULL") != std::string::npos || row[2].empty()))
                removedFound = true;
            if (row[1] == "added" && row[2].find("Eve") != std::string::npos)
                addedFound = true;
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "UPDATE with NULL: Old row with NULL was not removed.");
    BOOST_CHECK_MESSAGE(addedFound, "UPDATE with NULL: New row 'Eve' was not added.");
}

// Test case 6: UPDATE with a subquery.
BOOST_AUTO_TEST_CASE(update_output_test_case_6_subquery)
{
    // Create two tables: test_update and source.
    std::string sql_create =
        "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
        "INSERT INTO test_update VALUES (7, 'George');"
        "CREATE TABLE source (new_name VARCHAR);"
        "INSERT INTO source VALUES ('Georgina');";
    Admin::database_options db_opts = {"", 1, 0, sql_create, "", "", true};

    // Update using a subquery to retrieve a new name.
    StudentQuery student_query("update6", "UPDATE test_update SET name = (SELECT new_name FROM source) WHERE id = 7;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false, addedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed" && row[2].find("George") != std::string::npos)
                removedFound = true;
            if (row[1] == "added" && row[2].find("Georgina") != std::string::npos)
                addedFound = true;
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "UPDATE with subquery: Old row 'George' was not removed.");
    BOOST_CHECK_MESSAGE(addedFound, "UPDATE with subquery: New row 'Georgina' was not added.");
}

// Test case 9: UPDATE without a WHERE clause (affects all rows).
BOOST_AUTO_TEST_CASE(update_output_test_case_9_no_where)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_update VALUES (10, 'Jack');"
                                       "INSERT INTO test_update VALUES (11, 'Jill');",
                                       "", "", true};

    // Update all rows.
    StudentQuery student_query("update9", "UPDATE test_update SET name = 'Updated';");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    int removedCount = 0, addedCount = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed")
                removedCount++;
            if (row[1] == "added")
                addedCount++;
        }
    }
    BOOST_CHECK_MESSAGE(removedCount == 2, "UPDATE without WHERE: Expected 2 removed rows.");
    BOOST_CHECK_MESSAGE(addedCount == 2, "UPDATE without WHERE: Expected 2 added rows.");
}

// Test case 10: UPDATE multiple columns.
BOOST_AUTO_TEST_CASE(update_output_test_case_10_multiple_columns)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_update (id INTEGER, name VARCHAR, age INTEGER);"
                                       "INSERT INTO test_update VALUES (11, 'Kevin', 30);",
                                       "", "", true};

    // Update both name and age.
    StudentQuery student_query("update10", "UPDATE test_update SET name = 'Kevin Updated', age = 31 WHERE id = 11;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false, addedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_update")
        {
            if (row[1] == "removed" && row[2].find("Kevin") != std::string::npos && row[2].find("30") != std::string::npos)
                removedFound = true;
            if (row[1] == "added" && row[2].find("Kevin Updated") != std::string::npos && row[2].find("31") != std::string::npos)
                addedFound = true;
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "UPDATE multiple columns: Old row 'Kevin 30' not removed.");
    BOOST_CHECK_MESSAGE(addedFound, "UPDATE multiple columns: New row 'Kevin Updated 31' not added.");
}

BOOST_AUTO_TEST_SUITE_END()
