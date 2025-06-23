#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(create_grade_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
    Admin::database_options db_opts = {"", 1, 0, "", "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("create1", "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, CHECK (weight >= 10 AND weight <= 1000),  "
                                      "bookcase INT,  PRIMARY KEY(isbn, serial_number),  "
                                      "FOREIGN KEY (isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE );");
    ModelQuery model_query2("create2", "ALTER TABLE Copy ADD CHECK (weight >= 10 AND weight <= 1000);");
    model_queries.push_back(model_query);
    model_queries.push_back(model_query2);

    StudentQuery student_query("1", "ALTER table Copy ADD CONSTRAINT weight_limit CHECK (10 <= weight AND weight <= 1000);");
    student_queries.push_back(student_query);

    StudentQuery student_query_2("2", "CREATE TABLE Copy(isbn VARCHAR(13), serial_number INT PRIMARY KEY, weight INT NOT NULL CHECK (weight<=1000 AND weight>=10 ) , "
                                      "bookcase INT NOT NULL, FOREIGN KEY(isbn) REFERENCES Book(isbn) ON UPDATE CASCADE ON DELETE CASCADE);");
    student_queries.push_back(student_query_2);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    std::vector<ProcessQueries::grading_info> grading_info = process_queries.get_grading_info(&student_queries, admin);
    // std::cout << grading_info.at(1).feedback << std::endl;

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);

    BOOST_CHECK_EQUAL(student_queries[1].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[1].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_semantics_outcome(), Grader::property_state::SM_3);
    BOOST_CHECK_EQUAL(student_queries[1].get_results_outcome(), Grader::property_state::INCORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_correctness_level(), 16);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
    Admin::database_options db_opts = {"", 1, 0, "", "", "grader", true, true};

    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
    Grader grader;

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("create1", "CREATE TABLE Book( isbn VARCHAR(13), title VARCHAR NOT NULL, author VARCHAR, PRIMARY KEY(isbn));"
                                      " CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, "
                                      "PRIMARY KEY(isbn, serial_number), FOREIGN KEY (isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE );");
    model_queries.push_back(model_query);

    StudentQuery student_query("1", "CREATE TABLE Book( isbn VARCHAR(13) PRIMARY KEY, title VARCHAR(255) NOT NULL,  author VARCHAR(255)  );"
                                    "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT PRIMARY KEY, weight INT NOT NULL, bookcase INT, "
                                    "FOREIGN KEY (isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE );");
    student_queries.push_back(student_query);

    StudentQuery student_query2("2", "CREATE TABLE Book(isbn TEXT PRIMARY KEY,  title TEXT NOT NULL, author TEXT NOT NULL);"
                                     "CREATE TABLE Copy(isbn TEXT, serial_number INT PRIMARY KEY, weight INT NOT NULL, bookcase INT NOT NULL, "
                                     "FOREIGN KEY(isbn) REFERENCES Book(isbn) ON UPDATE CASCADE ON DELETE CASCADE);");
    student_queries.push_back(student_query2);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);
    BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);

    BOOST_CHECK_EQUAL(student_queries[1].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[1].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_semantics_outcome(), Grader::property_state::SM_3);
    BOOST_CHECK_EQUAL(student_queries[1].get_results_outcome(), Grader::property_state::INCORRECT);
    BOOST_CHECK_EQUAL(student_queries[1].get_correctness_level(), 16);
    // BOOST_CHECK_EQUAL(student_queries[1].get_grade(), 1);
}

BOOST_AUTO_TEST_SUITE_END()