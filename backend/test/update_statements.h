#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/update_clause.h"
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"

// -----------------------------------------------------------------
// Test Group 1: Correct UPDATE with single SET and WHERE clause
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_1)
{
    std::cout << "Update Test Case 1: Correct UPDATE with single SET and WHERE clause - variant 1\n";
    ModelQuery model_query("U1", "UPDATE Students SET grade = 'A' WHERE id = 123;");
    StudentQuery stu_query("U1", "UPDATE Students SET grade = 'A' WHERE id = 123;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_2)
{
    std::cout << "Update Test Case 2: Correct UPDATE with single SET and WHERE clause - variant 2\n";
    ModelQuery model_query("U2", "UPDATE Students SET name = 'John Doe' WHERE id = 1;");
    StudentQuery stu_query("U2", "UPDATE Students SET name = 'John Doe' WHERE id = 1;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_3)
{
    std::cout << "Update Test Case 3: Correct UPDATE with single SET and WHERE clause - variant 3\n";
    ModelQuery model_query("U3", "UPDATE Students SET status = 'active' WHERE enrolled = 1;");
    StudentQuery stu_query("U3", "UPDATE Students SET status = 'active' WHERE enrolled = 1;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 2: Correct UPDATE with multiple SET clauses and WHERE clause
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_4)
{
    std::cout << "Update Test Case 4: Correct UPDATE with multiple SET clauses - variant 1\n";
    ModelQuery model_query("U4", "UPDATE Employees SET salary = 5000, department = 'HR' WHERE emp_id = 101;");
    StudentQuery stu_query("U4", "UPDATE Employees SET salary = 5000, department = 'HR' WHERE emp_id = 101;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_5)
{
    std::cout << "Update Test Case 5: Correct UPDATE with multiple SET clauses - variant 2\n";
    ModelQuery model_query("U5", "UPDATE Employees SET bonus = 1000, grade = 'B' WHERE emp_id = 202;");
    StudentQuery stu_query("U5", "UPDATE Employees SET bonus = 1000, grade = 'B' WHERE emp_id = 202;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_6)
{
    std::cout << "Update Test Case 6: Correct UPDATE with multiple SET clauses - variant 3\n";
    ModelQuery model_query("U6", "UPDATE Employees SET level = 'Senior', title = 'Manager' WHERE emp_id = 303;");
    StudentQuery stu_query("U6", "UPDATE Employees SET level = 'Senior', title = 'Manager' WHERE emp_id = 303;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 3: Correct UPDATE with no WHERE clause
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_7)
{
    std::cout << "Update Test Case 7: Correct UPDATE with no WHERE clause - variant 1\n";
    ModelQuery model_query("U7", "UPDATE Products SET price = 19.99, stock = 100;");
    StudentQuery stu_query("U7", "UPDATE Products SET price = 19.99, stock = 100;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_8)
{
    std::cout << "Update Test Case 8: Correct UPDATE with no WHERE clause - variant 2\n";
    ModelQuery model_query("U8", "UPDATE Products SET discount = 10, available = 'yes';");
    StudentQuery stu_query("U8", "UPDATE Products SET discount = 10, available = 'yes';");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_9)
{
    std::cout << "Update Test Case 9: Correct UPDATE with no WHERE clause - variant 3\n";
    ModelQuery model_query("U9", "UPDATE Inventory SET quantity = 0, status = 'out of stock';");
    StudentQuery stu_query("U9", "UPDATE Inventory SET quantity = 0, status = 'out of stock';");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 4: Incorrect target table name
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_10)
{
    std::cout << "Update Test Case 10: Incorrect target table name - variant 1\n";
    ModelQuery model_query("U10", "UPDATE Orders SET amount = 50 WHERE order_id = 1;");
    StudentQuery stu_query("U10", "UPDATE Orderss SET amount = 50 WHERE order_id = 1;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Target table"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_11)
{
    std::cout << "Update Test Case 11: Incorrect target table name - variant 2\n";
    ModelQuery model_query("U11", "UPDATE Customers SET city = 'NY' WHERE id = 5;");
    StudentQuery stu_query("U11", "UPDATE Client SET city = 'NY' WHERE id = 5;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Target table"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_12)
{
    std::cout << "Update Test Case 12: Incorrect target table name - variant 3\n";
    ModelQuery model_query("U12", "UPDATE Books SET title = 'New Title' WHERE isbn = '12345';");
    StudentQuery stu_query("U12", "UPDATE Book SET title = 'New Title' WHERE isbn = '12345';");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Target table"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 5: Incorrect SET clause column name
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_13)
{
    std::cout << "Update Test Case 13: Incorrect SET clause column name - variant 1\n";
    ModelQuery model_query("U13", "UPDATE Students SET grade = 'A' WHERE id = 123;");
    StudentQuery stu_query("U13", "UPDATE Students SET mark = 'A' WHERE id = 123;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Set clause column (grade)"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_14)
{
    std::cout << "Update Test Case 14: Incorrect SET clause column name - variant 2\n";
    ModelQuery model_query("U14", "UPDATE Employees SET salary = 5000 WHERE emp_id = 101;");
    StudentQuery stu_query("U14", "UPDATE Employees SET wage = 5000 WHERE emp_id = 101;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Set clause column (salary)"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_15)
{
    std::cout << "Update Test Case 15: Incorrect SET clause column name - variant 3\n";
    ModelQuery model_query("U15", "UPDATE Products SET price = 19.99 WHERE product_id = 10;");
    StudentQuery stu_query("U15", "UPDATE Products SET cost = 19.99 WHERE product_id = 10;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Set clause column (price)"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 6: Incorrect SET clause value
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_16)
{
    std::cout << "Update Test Case 16: Incorrect SET clause value - variant 1\n";
    ModelQuery model_query("U16", "UPDATE Students SET grade = 'A' WHERE id = 123;");
    StudentQuery stu_query("U16", "UPDATE Students SET grade = 'B' WHERE id = 123;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Value for 'grade'"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_17)
{
    std::cout << "Update Test Case 17: Incorrect SET clause value - variant 2\n";
    ModelQuery model_query("U17", "UPDATE Employees SET bonus = 1000 WHERE emp_id = 202;");
    StudentQuery stu_query("U17", "UPDATE Employees SET bonus = 500 WHERE emp_id = 202;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Value for 'bonus'"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_18)
{
    std::cout << "Update Test Case 18: Incorrect SET clause value - variant 3\n";
    ModelQuery model_query("U18", "UPDATE Products SET stock = 100 WHERE product_id = 10;");
    StudentQuery stu_query("U18", "UPDATE Products SET stock = 90 WHERE product_id = 10;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Value for 'stock'"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 7: Incorrect WHERE clause differences
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_19)
{
    std::cout << "Update Test Case 19: Incorrect WHERE clause - variant 1\n";
    ModelQuery model_query("U19", "UPDATE Students SET grade = 'A' WHERE id = 123;");
    StudentQuery stu_query("U19", "UPDATE Students SET grade = 'A' WHERE id = 321;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("WHERE clause"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_20)
{
    std::cout << "Update Test Case 20: Incorrect WHERE clause - variant 2\n";
    ModelQuery model_query("U20", "UPDATE Employees SET department = 'HR' WHERE emp_id = 101;");
    StudentQuery stu_query("U20", "UPDATE Employees SET department = 'HR' WHERE emp_id = 110;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("WHERE clause"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_21)
{
    std::cout << "Update Test Case 21: Incorrect WHERE clause - variant 3\n";
    ModelQuery model_query("U21", "UPDATE Products SET price = 19.99 WHERE product_id = 10;");
    StudentQuery stu_query("U21", "UPDATE Products SET price = 19.99 WHERE product_id = 20;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("WHERE clause"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 8: UPDATE with FROM clause
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_22)
{
    std::cout << "Update Test Case 22: Correct UPDATE with FROM clause - variant 1\n";
    ModelQuery model_query("U22", "UPDATE Sales SET total = total + extra FROM Extra WHERE Sales.id = Extra.sale_id;");
    StudentQuery stu_query("U22", "UPDATE Sales SET total = total + extra FROM Extra WHERE Sales.id = Extra.sale_id;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_23)
{
    std::cout << "Update Test Case 23: Correct UPDATE with FROM clause - variant 2\n";
    ModelQuery model_query("U23", "UPDATE Orders SET amount = amount - discount FROM Discounts WHERE Orders.order_id = Discounts.order_id;");
    StudentQuery stu_query("U23", "UPDATE Orders SET amount = amount - discount FROM Discounts WHERE Orders.order_id = Discounts.order_id;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_24)
{
    std::cout << "Update Test Case 24: Correct UPDATE with FROM clause - variant 3\n";
    ModelQuery model_query("U24", "UPDATE Inventory SET quantity = quantity - sold FROM Sales WHERE Inventory.item_id = Sales.item_id;");
    StudentQuery stu_query("U24", "UPDATE Inventory SET quantity = quantity - sold FROM Sales WHERE Inventory.item_id = Sales.item_id;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

// -----------------------------------------------------------------
// Test Group 9: SET clause order differences
// -----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(update_test_case_25)
{
    std::cout << "Update Test Case 25: SET clause order difference - variant 1\n";
    ModelQuery model_query("U25", "UPDATE Employees SET bonus = 1000, salary = 5000 WHERE emp_id = 101;");
    StudentQuery stu_query("U25", "UPDATE Employees SET salary = 5000, bonus = 1000 WHERE emp_id = 101;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Because the compare function checks in order, this should report an error.
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_26)
{
    std::cout << "Update Test Case 26: SET clause order difference - variant 2\n";
    ModelQuery model_query("U26", "UPDATE Inventory SET quantity = 50, status = 'in stock' WHERE item_id = 20;");
    StudentQuery stu_query("U26", "UPDATE Inventory SET status = 'in stock', quantity = 50 WHERE item_id = 20;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}

BOOST_AUTO_TEST_CASE(update_test_case_27)
{
    std::cout << "Update Test Case 27: SET clause order difference - variant 3\n";
    ModelQuery model_query("U27", "UPDATE Customers SET city = 'LA', country = 'USA' WHERE id = 7;");
    StudentQuery stu_query("U27", "UPDATE Customers SET country = 'USA', city = 'LA' WHERE id = 7;");
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();
    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent"), std::string::npos);
}
