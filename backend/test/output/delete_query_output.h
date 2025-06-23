// #BEGIN CHANGES
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(delete_output_test_suite)

// Test case 1: Basic DELETE of a single row.
BOOST_AUTO_TEST_CASE(delete_output_test_case_1_basic)
{
    // Setup: Create table 'test_delete' with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (1, 'Alice');",
                                       "", "", true};

    // Delete the row where id = 1.
    StudentQuery student_query("delete1", "DELETE FROM test_delete WHERE id = 1;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false;
    for (const auto &row : diff_output)
    {
        // Expect a diff row with "removed" marker containing details of the deleted row.
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
        {
            if (row[2].find("Alice") != std::string::npos)
            {
                removedFound = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "Basic DELETE did not remove the expected row 'Alice'.");
}

// Test case 2: DELETE without a WHERE clause (delete all rows).
BOOST_AUTO_TEST_CASE(delete_output_test_case_2_delete_all)
{
    // Setup: Create table with two rows.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (1, 'Alice');"
                                       "INSERT INTO test_delete VALUES (2, 'Bob');",
                                       "", "", true};

    // Delete all rows.
    StudentQuery student_query("delete2", "DELETE FROM test_delete;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    int removedCount = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
            removedCount++;
    }
    BOOST_CHECK_MESSAGE(removedCount == 2, "DELETE without WHERE did not remove all rows as expected.");
}

// Test case 3: DELETE with condition matching multiple rows.
BOOST_AUTO_TEST_CASE(delete_output_test_case_3_multiple_rows)
{
    // Setup: Create table with three rows, two having name 'Bob'.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (1, 'Bob');"
                                       "INSERT INTO test_delete VALUES (2, 'Bob');"
                                       "INSERT INTO test_delete VALUES (3, 'Charlie');",
                                       "", "", true};

    // Delete rows where name = 'Bob'.
    StudentQuery student_query("delete3", "DELETE FROM test_delete WHERE name = 'Bob';");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    int removedCount = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
        {
            if (row[2].find("Bob") != std::string::npos)
                removedCount++;
        }
    }
    BOOST_CHECK_MESSAGE(removedCount == 2, "DELETE with condition did not remove exactly 2 rows with name 'Bob'.");
}

// Test case 4: DELETE with condition that matches no rows.
BOOST_AUTO_TEST_CASE(delete_output_test_case_4_no_match)
{
    // Setup: Create table with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (1, 'Alice');",
                                       "", "", true};

    // DELETE with a condition that matches no row.
    StudentQuery student_query("delete4", "DELETE FROM test_delete WHERE id = 999;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    BOOST_CHECK_MESSAGE(diff_output.empty(), "DELETE with no matching condition should produce empty diff output.");
}

// Test case 5: DELETE with extra spaces and mixed casing.
BOOST_AUTO_TEST_CASE(delete_output_test_case_5_spaces_and_case)
{
    // Setup: Create table with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (2, 'Bob');",
                                       "", "", true};

    // DELETE statement with irregular spacing and mixed-case SQL keywords.
    StudentQuery student_query("delete5", "  DeLeTe   FrOm   test_delete   WhErE   id = 2;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
        {
            if (row[2].find("Bob") != std::string::npos)
            {
                removedFound = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "DELETE with extra spaces and mixed case did not remove the expected row.");
}

// Test case 7: DELETE with a subquery in the WHERE clause.
BOOST_AUTO_TEST_CASE(delete_output_test_case_7_subquery)
{
    // Setup: Create two tables: test_delete and source_table.
    std::string sql_create =
        "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
        "INSERT INTO test_delete VALUES (4, 'David');"
        "CREATE TABLE source_table (id INTEGER, filter VARCHAR);"
        "INSERT INTO source_table VALUES (4, 'delete');";
    Admin::database_options db_opts = {"", 1, 0, sql_create, "", "", true};

    // Delete row from test_delete where id is in the result of the subquery.
    StudentQuery student_query("delete7", "DELETE FROM test_delete WHERE id IN (SELECT id FROM source_table WHERE filter = 'delete');");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
        {
            if (row[2].find("David") != std::string::npos)
            {
                removedFound = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "DELETE with subquery did not remove the expected row 'David'.");
}

// Test case 8: DELETE with newlines in the statement.
BOOST_AUTO_TEST_CASE(delete_output_test_case_8_newlines)
{
    // Setup: Create table with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (5, 'Eve');",
                                       "", "", true};

    // DELETE statement containing newline characters.
    StudentQuery student_query("delete8", "DELETE FROM test_delete\nWHERE id = 5;");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool removedFound = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
        {
            if (row[2].find("Eve") != std::string::npos)
            {
                removedFound = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(removedFound, "DELETE with newlines did not remove the expected row 'Eve'.");
}

// Test case 9: DELETE with condition on a non-existent column.
BOOST_AUTO_TEST_CASE(delete_output_test_case_9_non_existent_column)
{
    // Setup: Create table with one row.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (6, 'Frank');",
                                       "", "", true};

    // DELETE with condition on a non-existent column.
    StudentQuery student_query("delete9", "DELETE FROM test_delete WHERE non_existent = 'x';");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    BOOST_CHECK_MESSAGE(diff_output.empty(), "DELETE with condition on non-existent column should produce empty diff output.");
    BOOST_CHECK_MESSAGE(student_query.get_feedback().find("error") != std::string::npos,
                        "Expected error feedback for DELETE statement with non-existent column.");
}

// Test case 10: DELETE with multiple conditions using AND/OR.
BOOST_AUTO_TEST_CASE(delete_output_test_case_10_multiple_conditions)
{
    // Setup: Create table with three rows.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_delete (id INTEGER, name VARCHAR);"
                                       "INSERT INTO test_delete VALUES (7, 'George');"
                                       "INSERT INTO test_delete VALUES (8, 'Hannah');"
                                       "INSERT INTO test_delete VALUES (9, 'Ian');",
                                       "", "", true};

    // Delete rows where id = 7 OR name = 'Hannah'.
    StudentQuery student_query("delete10", "DELETE FROM test_delete WHERE id = 7 OR name = 'Hannah';");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    int removedCount = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_delete" && row[1] == "removed")
            removedCount++;
    }
    BOOST_CHECK_MESSAGE(removedCount == 2, "DELETE with multiple conditions did not remove exactly 2 rows as expected.");
}

BOOST_AUTO_TEST_SUITE_END()
