#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(insert_grade_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL,author VARCHAR,PRIMARY KEY(isbn));"
                                       "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, PRIMARY KEY(isbn, serial_number), FOREIGN KEY(isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE);"
                                       "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');",
                                       "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("insert11", "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A second course in Database Systems', 'Jeffrey D. Ullman');");
    model_queries.push_back(model_query);

    StudentQuery student_query("1", "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A sein Database Systems', 'JUllman');");
    student_queries.push_back(student_query);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);
    BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}

// Test case 2: unique constraint violation
BOOST_AUTO_TEST_CASE(test_case_2)
{
    Admin::database_options db_opts = {"", 0, 0,
                                       "CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL,author VARCHAR,PRIMARY KEY(isbn));"
                                       "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, PRIMARY KEY(isbn, serial_number), FOREIGN KEY(isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE);",
                                       "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("insert11", "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');"
                                       "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A second course in Database Systems', 'Jeffrey D. Ullman');");

    model_queries.push_back(model_query);

    StudentQuery student_query("1", "INSERT INTO Book (isbn, title, author) VALUES (1234, 'ab', 'mr'); INSERT INTO Book (isbn, title, author) VALUES (1234, 'bc', 'ms');");
    StudentQuery student_query_2("2", "INSERT INTO book (isbn, title, author) VALUES (56789, 'ab', 'mr'); INSERT INTO book (isbn, title, author) VALUES (56789, 'bc', 'ms');");

    student_queries.push_back(student_query);
    student_queries.push_back(student_query_2);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    // student_queries[0].print_output();

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);
    BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
    BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);

    BOOST_CHECK_EQUAL(student_queries[1].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[1].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_correctness_level(), 36);
    BOOST_CHECK_EQUAL(student_queries[1].get_grade(), 1);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
    Admin::database_options db_opts = {"", 1, 0,
                                       "CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL,author VARCHAR,PRIMARY KEY(isbn));"
                                       "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, PRIMARY KEY(isbn, serial_number), FOREIGN KEY(isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE);"
                                       "INSERT INTO Book(isbn, title, author) VALUES ('9781292025827', 'A first course in Database Systems', 'Jennifer Widom');",
                                       "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("insert11", "INSERT INTO Copy(isbn, serial_number, weight, bookcase) VALUES ('9781523853960', 1, 200, 23);");

    model_queries.push_back(model_query);

    StudentQuery student_query("1", "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES (1234, 4321, 5, 6);");

    student_queries.push_back(student_query);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);
    BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
    BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
BOOST_AUTO_TEST_SUITE_END()