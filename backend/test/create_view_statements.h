#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/create_clause.h"
#include "../clauses/create_view.h" // Declaration for CREATE VIEW processing
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"
#include "../my_duckdb.h"

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

// Test case CV6: Wrong table in FROM clause.
BOOST_AUTO_TEST_CASE(create_view_test_case_6)
{
    std::cout << "Test CV6: CREATE VIEW with wrong table in FROM clause\n";
    ModelQuery model_query("CV6",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");
    StudentQuery stu_query("CV6",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Album;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_MESSAGE(cmp_info.message.find("FROM clause") != std::string::npos, cmp_info.message);
}

// Test case CV7: Missing WHERE clause.
BOOST_AUTO_TEST_CASE(create_view_test_case_7)
{
    std::cout << "Test CV7: CREATE VIEW missing WHERE clause\n";
    ModelQuery model_query("CV7",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book WHERE author = 'Ullman';");
    StudentQuery stu_query("CV7",
                           "CREATE VIEW BookView AS SELECT isbn, title FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_MESSAGE(cmp_info.message.find("WHERE clause") != std::string::npos, cmp_info.message);
}

// Test case CV8: Missing WITH CHECK OPTION.
BOOST_AUTO_TEST_CASE(create_view_test_case_8)
{
    std::cout << "Test CV8: CREATE VIEW missing WITH CHECK OPTION\n";
    ModelQuery model_query("CV8",
                           R"(CREATE VIEW NonEnglishMovie AS
SELECT *
FROM Movie M
WHERE NOT EXISTS (SELECT 1 FROM Language L WHERE L.mid = M.mid AND L.language = 'English')
WITH CHECK OPTION;)");
    StudentQuery stu_query("CV8",
                           R"(CREATE VIEW NonEnglishMovie AS
SELECT *
FROM Movie M
WHERE NOT EXISTS (SELECT 1 FROM Language L WHERE L.mid = M.mid AND L.language = 'English');)");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_MESSAGE(cmp_info.message.find("WITH CHECK OPTION") != std::string::npos, cmp_info.message);
}

// Test case CV9: Results diff captures CREATE VIEW side effects.
BOOST_AUTO_TEST_CASE(create_view_test_case_9)
{
    std::cout << "Test CV9: Results diff tracks CREATE VIEW schema change\n";
    MyDuckDB duck_db;
    std::string error;
    auto diff_rows = duck_db.execute_query_not_select("CREATE VIEW nonenglishmovie AS SELECT 1 AS col;", error);
    BOOST_CHECK_MESSAGE(error.empty(), error);

    bool saw_view_created = false;
    for (const auto &row : diff_rows)
    {
        if (row.size() >= 2 && row[0] == "nonenglishmovie" && row[1] == "view_created")
        {
            saw_view_created = true;
            break;
        }
    }
    BOOST_CHECK(saw_view_created);

    auto ref_info = MyDuckDB::get_info(diff_rows);
    MyDuckDB::results_info stu_info;
    auto comparison = MyDuckDB::compare(ref_info, stu_info);
    BOOST_CHECK(!comparison.equal);
    BOOST_CHECK_NE(comparison.message.find("view 'nonenglishmovie'"), std::string::npos);
}
