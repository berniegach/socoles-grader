#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/delete_clause.h"
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"
#include "../clauses/select/from_clause.h"

// Test Case 1: Correct DELETE with WHERE clause.
BOOST_AUTO_TEST_CASE(delete_test_case_1)
{
    std::cout << "Delete Test Case 1: Correct DELETE with WHERE clause\n";
    ModelQuery model_query("D1", "DELETE FROM Employees WHERE emp_id = 101;");
    StudentQuery stu_query("D1", "DELETE FROM Employees WHERE emp_id = 101;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// Test Case 2: Correct DELETE with no WHERE clause.
BOOST_AUTO_TEST_CASE(delete_test_case_2)
{
    std::cout << "Delete Test Case 2: Correct DELETE with no WHERE clause\n";
    ModelQuery model_query("D2", "DELETE FROM Products;");
    StudentQuery stu_query("D2", "DELETE FROM Products;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// Test Case 3: Incorrect target table.
BOOST_AUTO_TEST_CASE(delete_test_case_3)
{
    std::cout << "Delete Test Case 3: Incorrect target table\n";
    ModelQuery model_query("D3", "DELETE FROM Customers WHERE id = 5;");
    StudentQuery stu_query("D3", "DELETE FROM Clients WHERE id = 5;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Target table"), std::string::npos);
}

// Test Case 4: Incorrect WHERE clause.
BOOST_AUTO_TEST_CASE(delete_test_case_4)
{
    std::cout << "Delete Test Case 4: Incorrect WHERE clause\n";
    ModelQuery model_query("D4", "DELETE FROM Employees WHERE emp_id = 101;");
    StudentQuery stu_query("D4", "DELETE FROM Employees WHERE emp_id = 202;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("WHERE clause"), std::string::npos);
}

// Test Case 5: Correct DELETE with FROM clause.
BOOST_AUTO_TEST_CASE(delete_test_case_5)
{
    std::cout << "Delete Test Case 5: Correct DELETE with FROM clause\n";
    // Example using dialect that supports DELETE ... USING ... syntax.
    ModelQuery model_query("D5", "DELETE FROM Orders USING Orders JOIN Customers ON Orders.cust_id = Customers.id WHERE Customers.region = 'NA';");
    StudentQuery stu_query("D5", "DELETE FROM Orders USING Orders JOIN Customers ON Orders.cust_id = Customers.id WHERE Customers.region = 'NA';");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// Test Case 6: Incorrect FROM clause.
BOOST_AUTO_TEST_CASE(delete_test_case_6)
{
    std::cout << "Delete Test Case 6: Incorrect FROM clause\n";
    ModelQuery model_query("D6", "DELETE FROM Orders USING Orders JOIN Customers ON Orders.cust_id = Customers.id WHERE Customers.region = 'NA';");
    StudentQuery stu_query("D6", "DELETE FROM Orders USING Orders JOIN Clients ON Orders.cust_id = Clients.id WHERE Clients.region = 'NA';");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("FROM clause"), std::string::npos);
}

// Test Case 7: Both queries missing FROM clause should not mark FROM clause.
BOOST_AUTO_TEST_CASE(delete_test_case_7)
{
    std::cout << "Delete Test Case 7: Both queries missing FROM clause\n";
    ModelQuery model_query("D7", "DELETE FROM Products WHERE price > 100;");
    StudentQuery stu_query("D7", "DELETE FROM Products WHERE price > 100;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // The feedback should not mention the FROM clause if both queries lack one.
    BOOST_CHECK_EQUAL(cmp_info.message.find("FROM clause"), std::string::npos);
}
