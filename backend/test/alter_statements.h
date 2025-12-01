#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"

BOOST_AUTO_TEST_CASE(alter_test_case_1)
{
    std::cout << "Test case 1: Correct ALTER TABLE statement\n";
    ModelQuery model_query("A1",
                           "ALTER TABLE Copy ADD CHECK (weight >= 10 AND weight <= 1000);");
    StudentQuery stu_query("A1",
                           "ALTER TABLE Copy ADD CHECK (weight >= 10 AND weight <= 1000);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Correct"), std::string::npos);
}
BOOST_AUTO_TEST_CASE(alter_test_case_2)
{
    string sql_file = "../samples/filled-book.sql";
    // initialize administrative features
    using pl = Grader::property_level;
    using po = Grader::property_order;
    Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_SY_RE, 0);
    Grader grader;

    // grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

    // initialize the queries
    vector<ModelQuery> model_queries;
    vector<StudentQuery> student_queries;

    ModelQuery model_query("1", "ALTER TABLE Copy ADD CHECK (weight >= 10 AND weight <= 1000);");
    Admin::database_options db_opts = {sql_file, 0, 0, "", "", "grader", false, true};

    model_queries.push_back(model_query);

    StudentQuery student_query("1", "ALTER TABLE Copy ADD CHECK (weight >= 10 AND weight <= 1000);");

    student_queries.push_back(student_query);

    // process the queries
    ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

    BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
    BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
    BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
    // BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 4);
    BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
    BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}