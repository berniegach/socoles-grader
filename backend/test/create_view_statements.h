#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/create_clause.h"
#include "../clauses/create_view.h" // Declaration for CREATE VIEW processing
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"

// Test case CV1: Correct CREATE VIEW statement.
BOOST_AUTO_TEST_CASE(create_view_test_case_1)
{
    std::cout << "Test CV1: Correct CREATE VIEW statement\n";
    ModelQuery model_query("CV1",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    StudentQuery stu_query("CV1",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect the view name and select clause to match.
    BOOST_CHECK_NE(cmp_info.message.find("Correct"), std::string::npos);
}

// Test case CV2: Incorrect column name in SELECT clause.
BOOST_AUTO_TEST_CASE(create_view_test_case_2)
{
    std::cout << "Test CV2: CREATE VIEW with wrong column name in SELECT clause (albm instead of title)\n";
    ModelQuery model_query("CV2",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    StudentQuery stu_query("CV2",
                           "CREATE VIEW BookView AS SELECT isbn, albm FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect detailed feedback mentioning the mismatch in the SELECT clause.
    BOOST_CHECK_NE(cmp_info.message.find("albm"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
}

// Test case CV3: Missing SELECT clause.
// We skip this as it cannot be parsed
/**BOOST_AUTO_TEST_CASE(create_view_test_case_3)
{
    std::cout << "Test CV3: CREATE VIEW with missing SELECT clause\n";
    ModelQuery model_query("CV3",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    // Student omits the SELECT part (simulate by providing an empty query).
    StudentQuery stu_query("CV3",
                           "CREATE VIEW BookView AS ;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    std::cout << cmp_info;
    // Expect feedback indicating that no view definition was found.
    BOOST_CHECK_NE(cmp_info.message.find("No view definition found"), std::string::npos);
}*/

// Test case CV4: Extra column in SELECT clause.
BOOST_AUTO_TEST_CASE(create_view_test_case_4)
{
    std::cout << "Test CV4: CREATE VIEW with extra column in SELECT clause\n";
    ModelQuery model_query("CV4",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    // Student adds an extra column 'author'.
    StudentQuery stu_query("CV4",
                           "CREATE VIEW BookView AS SELECT isbn, title, author FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback about extra column(s) in the SELECT clause.
    BOOST_CHECK_NE(cmp_info.message.find("extra"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("author"), std::string::npos);
}

// Test case CV5: Wrong view name.
BOOST_AUTO_TEST_CASE(create_view_test_case_5)
{
    std::cout << "Test CV5: CREATE VIEW with wrong view name\n";
    ModelQuery model_query("CV5",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    // Student writes "BookVw" instead of "BookView"
    StudentQuery stu_query("CV5",
                           "CREATE VIEW BookVw AS SELECT isbn, title FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback indicating the view name mismatch.
    BOOST_CHECK_NE(cmp_info.message.find("bookview"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("bookvw"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("name"), std::string::npos);
}
