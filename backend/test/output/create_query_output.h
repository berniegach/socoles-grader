#include <boost/test/unit_test.hpp>
// Test case for a CREATE query.
BOOST_AUTO_TEST_CASE(create_output_test_case_1)
{
    std::cout << "Running test case 1 of create table" << std::endl;
    // Setup default database options.
    Admin::database_options db_opts = {"", 1, 0, "", "", "", true, false};

    // Create a StudentQuery for a CREATE TABLE query.
    StudentQuery student_query("1", "CREATE TABLE test_create (id INTEGER, name VARCHAR);");

    // Pre-process the query to execute the diff logic
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);

    process_queries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();

    // Check that the diff output indicates a new table "test_create" was added.
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_create" && row[1] == "created")
        {
            found = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found, "CREATE query diff output did not detect new table creation with 'created' marker.");
}
// Test case 2: CREATE TABLE with PRIMARY KEY.
BOOST_AUTO_TEST_CASE(create_output_test_case_2)
{
    std::cout << "Running test case 2 of create table" << std::endl;
    Admin::database_options db_opts = {"", 1, 0, "", "", "", true};

    StudentQuery student_query("2", "CREATE TABLE test_create_pk (id INTEGER PRIMARY KEY, name VARCHAR);");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);
    process_queries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_create_pk" && row[1] == "created")
        {
            found = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found, "CREATE query with PRIMARY KEY did not produce a 'created' marker for test_create_pk.");
}

// Test case 3: CREATE TABLE with a UNIQUE constraint.
BOOST_AUTO_TEST_CASE(create_output_test_case_3)
{
    std::cout << "Running test case 3 of create table" << std::endl;
    Admin::database_options db_opts = {"", 1, 0, "", "", "", true};

    StudentQuery student_query("3", "CREATE TABLE test_create_unique (id INTEGER, name VARCHAR, UNIQUE(name));");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);
    process_queries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_create_unique" && row[1] == "created")
        {
            found = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found, "CREATE query with UNIQUE constraint did not produce a 'created' marker for test_create_unique.");
}

// Test case 4: CREATE TABLE IF NOT EXISTS semantics.
// First, create the table. Then, reissue the CREATE with IF NOT EXISTS and check that no diff is produced.
BOOST_AUTO_TEST_CASE(create_output_test_case_4)
{
    std::cout << "Running test case 4 of create table" << std::endl;
    // Setup default database options.
    // the 4th option creates the table so we can check it later with our test
    Admin::database_options db_opts = {"", 1, 0, "CREATE TABLE test_if_not_exists (id INTEGER, name VARCHAR);", "", "", true};
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);

    // Now, try to create the same table using IF NOT EXISTS.
    StudentQuery student_query2("4b", "CREATE TABLE IF NOT EXISTS test_if_not_exists (id INTEGER, name VARCHAR);");
    process_queries.pre_process_student_query(&student_query2, db_opts);

    auto diff_output = student_query2.get_output();
    BOOST_CHECK_MESSAGE(diff_output.empty(), "CREATE TABLE IF NOT EXISTS should not produce diff output when table already exists.");
}

// Test case 5: CREATE TABLE with multiple columns and various data types.
BOOST_AUTO_TEST_CASE(create_output_test_case_5)
{
    std::cout << "Running test case 5 of create table" << std::endl;
    Admin::database_options db_opts = {"", 1, 0, "", "", "", true};

    StudentQuery student_query("5", "CREATE TABLE test_multi (id INTEGER, name VARCHAR, salary DOUBLE, active BOOLEAN);");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);
    process_queries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    bool found = false;
    for (const auto &row : diff_output)
    {
        if (row.size() >= 3 && row[0] == "test_multi" && row[1] == "created")
        {
            found = true;
            break;
        }
    }
    BOOST_CHECK_MESSAGE(found, "CREATE query diff output did not detect new table creation for test_multi.");
}

// Test case 6: Erroneous CREATE TABLE query (e.g., no columns specified) should fail.
BOOST_AUTO_TEST_CASE(create_output_test_case_6)
{
    std::cout << "Running test case 6 of create table" << std::endl;
    Admin::database_options db_opts = {"", 1, 0, "", "", "", true};

    StudentQuery student_query("6", "CREATE TABLE test_error ();");
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;
    Grader grader;
    ProcessQueries process_queries(model_queries, student_queries, db_opts, Admin{}, grader);
    process_queries.pre_process_student_query(&student_query, db_opts);

    auto diff_output = student_query.get_output();
    // The query should fail so no diff output should be produced.
    BOOST_CHECK_MESSAGE(diff_output.empty(), "Erroneous CREATE query should produce no diff output.");
    // Check that feedback contains an error message.
    BOOST_CHECK_MESSAGE(student_query.get_feedback().find("error") != std::string::npos,
                        "Expected error feedback for CREATE TABLE with no columns.");
}