#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(insert_output_test_suite)

// Test case 1: Basic INSERT with specified columns.
BOOST_AUTO_TEST_CASE(insert_output_test_case_1_basic)
{
    // Setup database options so that table test_insert is created.
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true, false};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // Create an INSERT statement that adds one row.
    StudentQuery student_query("insert1", "INSERT INTO test_insert (id, name) VALUES (1, 'Alice');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    // student_queries.push_back(student_query);

    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);

    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    // Check that diff output indicates an added row in test_insert.
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("1") != std::string::npos && row[2].find("Alice") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "Basic INSERT did not produce expected 'added' diff output for test_insert.");
}

// Test case 2: INSERT without explicitly specifying columns.
BOOST_AUTO_TEST_CASE(insert_output_test_case_2_without_columns)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    StudentQuery student_query("insert2", "INSERT INTO test_insert VALUES (2, 'Bob');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("2") != std::string::npos && row[2].find("Bob") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT without columns did not produce expected 'added' diff output for test_insert.");
}

// Test case 3: INSERT with multiple rows.
BOOST_AUTO_TEST_CASE(insert_output_test_case_3_multiple_rows)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Insert statement inserting two rows.
    StudentQuery student_query("insert3", "INSERT INTO test_insert (id, name) VALUES (3, 'Charlie'), (4, 'David');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    int count_added = 0;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            // Check for either row.
            if ((row[2].find("3") != std::string::npos && row[2].find("Charlie") != std::string::npos) ||
                (row[2].find("4") != std::string::npos && row[2].find("David") != std::string::npos))
            {
                count_added++;
            }
        }
    }
    BOOST_CHECK_MESSAGE(count_added == 2, "Multiple rows INSERT did not produce two 'added' diff outputs for test_insert.");
}

// Test case 4: INSERT statement with extra spaces and mixed casing.
BOOST_AUTO_TEST_CASE(insert_output_test_case_4_with_spaces_and_case)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // INSERT with irregular spacing and mixed-case SQL keywords.
    StudentQuery student_query("insert4", "  InSeRt   InTo   test_insert  (id, name) VALUES ( 5 ,'Eve' );");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("5") != std::string::npos && row[2].find("Eve") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with spaces and mixed casing did not produce expected diff output for test_insert.");
}

// Test case 5: INSERT with subquery.
// This test creates two tables (destination and source) and then uses a subquery.
BOOST_AUTO_TEST_CASE(insert_output_test_case_5_with_subquery)
{
    // Create two tables: test_insert and source_table, and pre-populate source_table.
    std::string sql_create = "CREATE TABLE test_insert (id INTEGER, name VARCHAR);"
                             "CREATE TABLE source_table (id INTEGER, name VARCHAR);"
                             "INSERT INTO source_table (id, name) VALUES (6, 'Frank');";
    Admin::database_options db_opts = {"", 1, 0, sql_create, "", "", true};

    // Use an INSERT with a subquery to insert from source_table into test_insert.
    StudentQuery student_query("insert5", "INSERT INTO test_insert SELECT id, name FROM source_table;");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("6") != std::string::npos && row[2].find("Frank") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with subquery did not produce expected diff output for test_insert.");
}

// Test case 6: Erroneous INSERT statement.
// This test uses an INSERT with mismatched column/value count and expects an error.
BOOST_AUTO_TEST_CASE(insert_output_test_case_6_erroneous_insert)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    StudentQuery student_query("insert6", "INSERT INTO test_insert (id, name) VALUES (7);");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    // An erroneous INSERT should produce no diff output.
    BOOST_CHECK_MESSAGE(diff_output.empty(), "Erroneous INSERT should produce no diff output.");
    // Additionally, check that feedback contains an error message.
    BOOST_CHECK_MESSAGE(student_query.get_feedback().find("error") != std::string::npos,
                        "Expected error feedback for erroneous INSERT statement.");
}
// Test case 7: INSERT with columns specified in different order.
BOOST_AUTO_TEST_CASE(insert_output_test_case_7_columns_different_order)
{
    // Table is created as (id, name) but we insert as (name, id)
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    StudentQuery student_query("insert7", "INSERT INTO test_insert (name, id) VALUES ('Grace', 7);");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    // The row output should show the values in table order: id then name.
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("7") != std::string::npos && row[2].find("Grace") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with columns in different order did not produce expected diff output.");
}

// Test case 8: INSERT with a NULL value.
BOOST_AUTO_TEST_CASE(insert_output_test_case_8_null_value)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    StudentQuery student_query("insert8", "INSERT INTO test_insert (id, name) VALUES (8, NULL);");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            // Check for '8' and a representation of NULL (could be "NULL" or an empty string depending on implementation).
            if (row[2].find("8") != std::string::npos &&
                (row[2].find("NULL") != std::string::npos || row[2].find("null") != std::string::npos))
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with NULL value did not produce expected diff output.");
}

// Test case 9: INSERT with quoted column names.
BOOST_AUTO_TEST_CASE(insert_output_test_case_9_quoted_columns)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Using double quotes around column names.
    StudentQuery student_query("insert9", "INSERT INTO test_insert (\"id\", \"name\") VALUES (9, 'Hank');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("9") != std::string::npos && row[2].find("Hank") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with quoted column names did not produce expected diff output.");
}

// Test case 10: INSERT with newlines in the statement.
BOOST_AUTO_TEST_CASE(insert_output_test_case_10_newlines)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true, false};

    // Insert statement containing newline characters.
    StudentQuery student_query("insert10", "INSERT INTO test_insert (id, name) \n VALUES \n (10, 'Ivy');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_insert" && row[1] == "added")
        {
            if (row[2].find("10") != std::string::npos && row[2].find("Ivy") != std::string::npos)
            {
                found = true;
                break;
            }
        }
    }
    BOOST_CHECK_MESSAGE(found, "INSERT with newlines did not produce expected diff output.");
}

// Test case 11: INSERT that violates a UNIQUE constraint.
BOOST_AUTO_TEST_CASE(insert_output_test_case_11_unique_constraint)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL,author VARCHAR,PRIMARY KEY(isbn));"
                                       "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, PRIMARY KEY(isbn, serial_number), FOREIGN KEY(isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE);",
                                       "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    // Attempt to insert a duplicate id.
    StudentQuery student_query("insert11", "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');"
                                           "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A second course in Database Systems', 'Jeffrey D. Ullman');");
    ModelQuery model_query("insert11", "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');"
                                       "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A second course in Database Systems', 'Jeffrey D. Ullman');");

    student_queries.push_back(student_query);
    model_queries.push_back(model_query);

    ProcessQueries processQueries(model_queries, student_queries, db_opts, admin, grader);

    auto diff_output = student_queries.at(0).get_output();
    bool found = false;

    // Check that the diff output contains exactly the UNIQUE constraint violation marker:
    for (const auto &row : diff_output)
    {
        if (row.size() >= 2 && row[0] == "constraint error" && row[1] == "unique constraint")
        {
            found = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found, "Expected diff_output to contain a {'constraint error', 'unique constraint'} row.");
}
// Test case 12: Two queries inserting identical values using the same statement.
// Their diff outputs should match exactly.
BOOST_AUTO_TEST_CASE(insert_output_test_case_12)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Both student queries use the same INSERT statement.
    StudentQuery student_query1("query1", "INSERT INTO test_insert (id, name) VALUES (200, 'Alice');");
    StudentQuery student_query2("query2", "INSERT INTO test_insert (id, name) VALUES (200, 'Alice');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query1, db_opts);
    processQueries.pre_process_student_query(&student_query2, db_opts);

    auto diff1 = student_query1.get_output();
    auto diff2 = student_query2.get_output();

    BOOST_CHECK_MESSAGE(diff1 == diff2, "The diff outputs for identical INSERT statements do not match between the two student queries.");
}

// Test case 13: Two queries using different syntaxes but logically equivalent INSERT statements.
// Query 1 uses an explicit column list; Query 2 omits it.
BOOST_AUTO_TEST_CASE(insert_output_test_case_13)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Query 1: INSERT with a column list.
    StudentQuery student_query1("query1", "INSERT INTO test_insert (id, name) VALUES (201, 'Bob');");
    // Query 2: INSERT without specifying columns.
    StudentQuery student_query2("query2", "INSERT INTO test_insert VALUES (201, 'Bob');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query1, db_opts);
    processQueries.pre_process_student_query(&student_query2, db_opts);

    auto diff1 = student_query1.get_output();
    auto diff2 = student_query2.get_output();

    BOOST_CHECK_MESSAGE(diff1 == diff2, "The diff outputs for equivalent INSERT statements (with and without column list) do not match.");
}

// Test case 14: Two queries inserting multiple rows using two techniques.
// Query 1 uses a multi-row INSERT; Query 2 uses two separate INSERT statements.
// Their final diff outputs (after combining both INSERTs) should be equivalent.
BOOST_AUTO_TEST_CASE(insert_output_test_case_14)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Query 1: A single multi-row INSERT.
    StudentQuery student_query1("query1",
                                "INSERT INTO test_insert (id, name) VALUES (202, 'Charlie'), (203, 'David');");
    // Query 2: Two separate INSERT statements combined.
    StudentQuery student_query2("query2",
                                "INSERT INTO test_insert (id, name) VALUES (202, 'Charlie'); "
                                "INSERT INTO test_insert (id, name) VALUES (203, 'David');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query1, db_opts);
    processQueries.pre_process_student_query(&student_query2, db_opts);

    auto diff1 = student_query1.get_output();
    auto diff2 = student_query2.get_output();

    BOOST_CHECK_MESSAGE(diff1 == diff2, "The diff outputs for multi-row INSERT versus multiple single-row INSERTs do not match.");
}

// Test case 15: Two queries inserting rows with NULL values.
// The outputs generated by both queries should be identical.
BOOST_AUTO_TEST_CASE(insert_output_test_case_15)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Both student queries insert a row with a NULL value in the 'name' column.
    StudentQuery student_query1("query1", "INSERT INTO test_insert (id, name) VALUES (204, NULL);");
    StudentQuery student_query2("query2", "INSERT INTO test_insert (id, name) VALUES (204, NULL);");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query1, db_opts);
    processQueries.pre_process_student_query(&student_query2, db_opts);

    auto diff1 = student_query1.get_output();
    auto diff2 = student_query2.get_output();

    BOOST_CHECK_MESSAGE(diff1 == diff2, "The diff outputs for INSERT statements with NULL values do not match.");
}

// Test case 16: Two queries using quoted column names.
// Both queries insert the same row; therefore, the diff outputs should be equal.
BOOST_AUTO_TEST_CASE(insert_output_test_case_16)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE test_insert (id INTEGER, name VARCHAR);", "", "", true};

    // Both queries use quoted identifiers.
    StudentQuery student_query1("query1", "INSERT INTO test_insert (\"id\", \"name\") VALUES (205, 'Eve');");
    StudentQuery student_query2("query2", "INSERT INTO test_insert (\"id\", \"name\") VALUES (205, 'Eve');");

    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries processQueries(model_queries, student_queries, db_opts, Admin{}, grader);
    processQueries.pre_process_student_query(&student_query1, db_opts);
    processQueries.pre_process_student_query(&student_query2, db_opts);

    auto diff1 = student_query1.get_output();
    auto diff2 = student_query2.get_output();

    BOOST_CHECK_MESSAGE(diff1 == diff2, "The diff outputs for INSERT statements using quoted columns do not match.");
}

BOOST_AUTO_TEST_SUITE_END()
