#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/insert_clause.h"
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"

// a boost function to print comparison results
std::ostream &operator<<(std::ostream &os, const Common::comparision_result &cmp_info)
{
    os << "Comparison feedback:\n"
       << cmp_info.message << std::endl;
    os << "Correct parts: ";
    for (const auto &part : cmp_info.correct_parts)
        os << part << " ";
    os << "\nIncorrect parts: ";
    for (const auto &part : cmp_info.incorrect_parts)
        os << part << " ";
    os << "\nNext steps: ";
    for (const auto &step : cmp_info.next_steps)
        os << step << " ";
    os << "\n";
    return os;
}

BOOST_AUTO_TEST_CASE(correct)
{
    std::cout << "Testing a correct insert statement...\n";

    // CSV Row #1 example:
    // Reference (correct) query:
    ModelQuery model_query("1",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-0-12-345678-5','SQL Fundamentals','John Doe'), "
                           "('978-0-13-601267-1','Database Systems','R. Ramakrishnan');");
    // Student query with missing comma between value sets:
    StudentQuery stu_query("1",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-0-12-345678-5','SQL Fundamentals','John Doe'), "
                           "('978-0-13-601267-1','Database Systems','R. Ramakrishnan');");

    // Create AST nodes for both queries.
    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    std::shared_ptr<AbstractSyntaxTree::Node> ref_ast = model_query.get_parse_tree();
    std::shared_ptr<AbstractSyntaxTree::Node> stu_ast = stu_query.get_parse_tree();

    // compare the queries
    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);

    BOOST_CHECK_NE(cmp_info.message.find("Correct"), std::string::npos);
}
// Test case 4: Single-row INSERT (no column list) with missing value.
BOOST_AUTO_TEST_CASE(test_case_4)
{
    std::cout << "Test case 4: Single-row INSERT (no column list) with missing value\n";
    ModelQuery model_query("4",
                           "INSERT INTO Book VALUES ('978-1-4028-9462-6','Learn SQL','Alice Smith');");
    StudentQuery stu_query("4",
                           "INSERT INTO Book VALUES ('978-1-4028-9462-6','Learn SQL');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: row 1 has 2 values instead of 3.
    BOOST_CHECK_NE(cmp_info.message.find("Alice Smith"), std::string::npos);
}

// Test case 5: Single-row INSERT (no column list) with column list provided in the student query but missing a value.
BOOST_AUTO_TEST_CASE(test_case_5)
{
    std::cout << "Test case 5: Single-row INSERT (no column list) with column list provided missing a value\n";
    ModelQuery model_query("5",
                           "INSERT INTO Book VALUES ('978-1-4028-9462-6','Learn SQL','Alice Smith');");
    StudentQuery stu_query("5",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-4028-9462-6','Learn SQL');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: the student query’s column list is missing at least one column.
    BOOST_CHECK_NE(cmp_info.message.find("columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Alice Smith"), std::string::npos);
}

// Test case 6: Single-row INSERT (no column list) with an extra value.
BOOST_AUTO_TEST_CASE(test_case_6)
{
    std::cout << "Test case 6: Single-row INSERT (no column list) with extra value\n";
    ModelQuery model_query("6",
                           "INSERT INTO Book VALUES ('978-1-4028-9462-6','Learn SQL','Alice Smith');");
    StudentQuery stu_query("6",
                           "INSERT INTO Book VALUES ('978-1-4028-9462-6','Learn SQL','Alice Smith','Extra');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: row 1 has 4 values instead of 3.
    BOOST_CHECK_NE(cmp_info.message.find("Extra"), std::string::npos);
}

// Test case 7: INSERT with specified columns but with an incorrect column name ('serialnumber' vs. 'serial_number').
BOOST_AUTO_TEST_CASE(test_case_7)
{
    std::cout << "Test case 7: INSERT with specified columns using wrong column name 'serialnumber'\n";
    ModelQuery model_query("7",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-14-312779-6',1001,450,2);");
    StudentQuery stu_query("7",
                           "INSERT INTO Copy (isbn, serialnumber, weight, bookcase) VALUES ('978-0-14-312779-6',1001,450,2);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: the student query is missing 'serial_number' and has an extra 'serialnumber'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("serialnumber"), std::string::npos);
}

// Test case 8: INSERT with specified columns but using an incorrect column ('title' instead of 'serial_number').
BOOST_AUTO_TEST_CASE(test_case_8)
{
    std::cout << "Test case 8: INSERT with specified columns using wrong column 'title'\n";
    ModelQuery model_query("8",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-14-312779-6',1002,400,3);");
    StudentQuery stu_query("8",
                           "INSERT INTO Copy (isbn, title, weight, bookcase) VALUES ('978-0-14-312779-6',1002,400,3);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: extra column 'title' and missing 'serial_number'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Extra columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
}

// Test case 9: INSERT with specified columns but using a non-existent column ('shelf' instead of 'bookcase').
BOOST_AUTO_TEST_CASE(test_case_9)
{
    std::cout << "Test case 9: INSERT with specified columns using wrong column 'shelf'\n";
    ModelQuery model_query("9",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-14-312779-6',1003,500,3);");
    StudentQuery stu_query("9",
                           "INSERT INTO Copy (isbn, serial_number, weight, shelf) VALUES ('978-0-14-312779-6',1003,500,3);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: missing 'bookcase' and extra 'shelf'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("shelf"), std::string::npos);
}

// Test case 10: INSERT with expression in Copy table and a type mismatch in the weight column.
BOOST_AUTO_TEST_CASE(test_case_10)
{
    std::cout << "Test case 10: INSERT with expression, type mismatch in weight column\n";
    ModelQuery model_query("10",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1',21,2*150,5);");
    StudentQuery stu_query("10",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1',21,'heavy',5);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 3 with value 'heavy'
    BOOST_CHECK_NE(cmp_info.message.find("weight"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("heavy"), std::string::npos);
}

// Test case 11: INSERT with expression in Copy table and a type mismatch in the serial_number column.
BOOST_AUTO_TEST_CASE(test_case_11)
{
    std::cout << "Test case 11: INSERT with expression, type mismatch in serial_number column\n";
    ModelQuery model_query("11",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1',20,2*150,5);");
    StudentQuery stu_query("11",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1','twenty-two',300,5);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 2 with value 'twenty-two'
    BOOST_CHECK_NE(cmp_info.message.find("serial_number"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("twenty-two"), std::string::npos);
}

// Test case 12: INSERT with expression in Copy table and a type mismatch in the bookcase column.
BOOST_AUTO_TEST_CASE(test_case_12)
{
    std::cout << "Test case 12: INSERT with expression, type mismatch in bookcase column\n";
    ModelQuery model_query("12",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1',23,2*150,5);");
    StudentQuery stu_query("12",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-545-01022-1',23,300,'Shelf A');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 4 with value 'Shelf A'
    BOOST_CHECK_NE(cmp_info.message.find("bookcase"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Shelf A"), std::string::npos);
}

// Test case 13: Single-row INSERT (Book table) with duplicate ISBN (values differ in title/author).
BOOST_AUTO_TEST_CASE(test_case_13)
{
    std::cout << "Test case 13: Single-row INSERT (Book table) with duplicate ISBN (value mismatches)\n";
    ModelQuery model_query("13",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-56619-909-4','SQL Advanced','Bob Brown');");
    StudentQuery stu_query("13",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-56619-909-4','Another Title','Another Author');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect mismatches at row 1, column 2 and 3.
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("author"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Another Title"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Another Author"), std::string::npos);
}

// Test case 14: Single-row INSERT (Book table) where the student omits the column list.
BOOST_AUTO_TEST_CASE(test_case_14)
{
    std::cout << "Test case 14: Single-row INSERT (Book table) missing column list\n";
    ModelQuery model_query("14",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-56619-909-4','SQL Advanced','Bob Brown');");
    StudentQuery stu_query("14",
                           "INSERT INTO Book VALUES ('978-1-56619-909-4','SQL Advanced','Bob Brown');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: missing target columns information in the student query.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
}

// Test case 15: Single-row INSERT (Book table) with mismatched ISBN (duplicate ISBN violation).
BOOST_AUTO_TEST_CASE(test_case_15)
{
    std::cout << "Test case 15: Single-row INSERT (Book table) with mismatched ISBN\n";
    ModelQuery model_query("15",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-56619-909-4','SQL Advanced','Bob Brown');");
    StudentQuery stu_query("15",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-0-13-601267-1','SQL Advanced','Bob Brown');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 1 (ISBN).
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
}

// Test case 16: Single-row INSERT (Copy table) with foreign key violation (non-existent ISBN).
BOOST_AUTO_TEST_CASE(test_case_16)
{
    std::cout << "Test case 16: Single-row INSERT (Copy table) with foreign key violation on ISBN\n";
    ModelQuery model_query("16",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',2,550,4);");
    StudentQuery stu_query("16",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('999-9-99-999999-9',2,550,4);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 1 showing the incorrect ISBN.
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("999-9-99-999999-9"), std::string::npos);
}

// Test case 17: Single-row INSERT (Copy table) with foreign key violation (ISBN and serial_number mismatches).
BOOST_AUTO_TEST_CASE(test_case_17)
{
    std::cout << "Test case 17: Single-row INSERT (Copy table) with foreign key violation\n";
    ModelQuery model_query("17",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',2,600,4);");
    StudentQuery stu_query("17",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-9-87-654321-0',3,600,4);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect errors in row 1 for columns 1 and 2.
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("serial_number"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("978-9-87-654321-0"), std::string::npos);
}

// Test case 18: Single-row INSERT (Copy table) with foreign key violation (ISBN mismatch).
BOOST_AUTO_TEST_CASE(test_case_18)
{
    std::cout << "Test case 18: Single-row INSERT (Copy table) with foreign key violation\n";
    ModelQuery model_query("18",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',2,550,4);");
    StudentQuery stu_query("18",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('123-4-56-789012-3',2,550,4);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 1 showing the incorrect ISBN.
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("123-4-56-789012-3"), std::string::npos);
}

// Test case 19: Single-row INSERT (Book table) with NOT NULL violation (missing title).
BOOST_AUTO_TEST_CASE(test_case_19)
{
    std::cout << "Test case 19: Single-row INSERT (Book table) missing title (NOT NULL violation)\n";
    ModelQuery model_query("19",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-2345-6789-7','SQL for Beginners','Carol Lee');");
    StudentQuery stu_query("19",
                           "INSERT INTO Book (isbn, author) VALUES ('978-1-2345-6789-7','Carol Lee');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: missing 'title'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
}

// Test case 20: Single-row INSERT (Book table) with NOT NULL violation (missing ISBN).
BOOST_AUTO_TEST_CASE(test_case_20)
{
    std::cout << "Test case 20: Single-row INSERT (Book table) missing ISBN (NOT NULL violation)\n";
    ModelQuery model_query("20",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-2345-6789-7','SQL for Beginners','Carol Lee');");
    StudentQuery stu_query("20",
                           "INSERT INTO Book (title, author) VALUES ('SQL for Beginners','Carol Lee');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: missing 'isbn'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    // BOOST_CHECK_NE(cmp_info.message.find("978-1-2345-6789-7"), std::string::npos);
}

// Test case 21: Single-row INSERT (Book table) with NOT NULL violation (missing title and author).
BOOST_AUTO_TEST_CASE(test_case_21)
{
    std::cout << "Test case 21: Single-row INSERT (Book table) missing title and author (NOT NULL violation)\n";
    ModelQuery model_query("21",
                           "INSERT INTO Book (isbn, title, author) VALUES ('978-1-2345-6789-7','SQL for Beginners','Carol Lee');");
    StudentQuery stu_query("21",
                           "INSERT INTO Book (isbn) VALUES ('978-1-2345-6789-7');");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: missing 'title' and 'author'.
    BOOST_CHECK_NE(cmp_info.message.find("Missing columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("author"), std::string::npos);
}

// Test case 22: INSERT using DEFAULT (Copy table) with DEFAULT for isbn.
BOOST_AUTO_TEST_CASE(test_case_22)
{
    std::cout << "Test case 22: INSERT using DEFAULT for isbn (not allowed)\n";
    ModelQuery model_query("22",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-13-601267-1',11,DEFAULT,DEFAULT);");
    StudentQuery stu_query("22",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES (DEFAULT,11,DEFAULT,DEFAULT);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: DEFAULT used for 'isbn' which is not allowed.
    BOOST_CHECK_NE(cmp_info.message.find("DEFAULT"), std::string::npos);
}

// Test case 23: INSERT using DEFAULT (Copy table) with DEFAULT VALUES (constraint violation).
BOOST_AUTO_TEST_CASE(test_case_23)
{
    std::cout << "Test case 23: INSERT using DEFAULT VALUES causing constraint violation\n";
    ModelQuery model_query("23",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-13-601267-1',10,DEFAULT,DEFAULT);");
    StudentQuery stu_query("23",
                           "INSERT INTO Copy DEFAULT VALUES;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: using DEFAULT VALUES when required columns have no defaults.
    BOOST_CHECK_NE(cmp_info.message.find("columns"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("style"), std::string::npos);
}

// Test case 24: INSERT using DEFAULT (Copy table) with DEFAULT for serial_number (causes NOT NULL violation).
BOOST_AUTO_TEST_CASE(test_case_24)
{
    std::cout << "Test case 24: INSERT using DEFAULT for serial_number (NOT NULL violation)\n";
    ModelQuery model_query("24",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-13-601267-1',10,DEFAULT,DEFAULT);");
    StudentQuery stu_query("24",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-13-601267-1',DEFAULT,DEFAULT,DEFAULT);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: DEFAULT used for 'serial_number' is not allowed.
    BOOST_CHECK_NE(cmp_info.message.find("DEFAULT"), std::string::npos);
}

// Test case 25: INSERT with NULL value for bookcase (NOT NULL violation).
BOOST_AUTO_TEST_CASE(test_case_25)
{
    std::cout << "Test case 25: INSERT with explicit NULL for bookcase (NOT NULL violation)\n";
    ModelQuery model_query("25",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',6,NULL,2);");
    StudentQuery stu_query("25",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',6,NULL,NULL);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 4 where a NOT NULL value is expected.
    BOOST_CHECK_NE(cmp_info.message.find("bookcase"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("2"), std::string::npos);
}

// Test case 26: INSERT with NULL provided for primary key (isbn) (NOT NULL violation).
BOOST_AUTO_TEST_CASE(test_case_26)
{
    std::cout << "Test case 26: INSERT with NULL for primary key (isbn)\n";
    ModelQuery model_query("26",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',7,NULL,2);");
    StudentQuery stu_query("26",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES (NULL,7,300,2);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 1 where 'NULL' is provided.
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("weight"), std::string::npos);
    // BOOST_CHECK_NE(cmp_info.message.find("NULL"), std::string::npos);
}

// Test case 27: INSERT with 'NULL' as a string literal instead of the SQL NULL value.
BOOST_AUTO_TEST_CASE(test_case_27)
{
    std::cout << "Test case 27: INSERT with 'NULL' as string literal instead of SQL NULL\n";
    ModelQuery model_query("27",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',8,NULL,2);");
    StudentQuery stu_query("27",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) VALUES ('978-0-12-345678-5',8,'NULL',2);");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch at row 1, column 3 where a string literal 'NULL' is used.
    BOOST_CHECK_NE(cmp_info.message.find("weight"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("'NULL'"), std::string::npos);
}

// Test case 28: INSERT ... SELECT with target column count mismatch (student provides 3 columns vs. 4 expected).
BOOST_AUTO_TEST_CASE(test_case_28)
{
    std::cout << "Test case 28: INSERT ... SELECT with target column count mismatch (3 vs. 4)\n";
    ModelQuery model_query("28",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) SELECT isbn, 1, 0, 1 FROM Book;");
    StudentQuery stu_query("28",
                           "INSERT INTO Copy (isbn, serial_number, weight) SELECT isbn, 1, 0, 1 FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch in number of rows (or values) due to target column count mismatch.
    BOOST_CHECK_NE(cmp_info.message.find("bookcase"), std::string::npos);
}

// Test case 29: INSERT ... SELECT with target column count mismatch (4 vs. 3).
BOOST_AUTO_TEST_CASE(test_case_29)
{
    std::cout << "Test case 29: INSERT ... SELECT with target column count mismatch (4 vs. 3)\n";
    ModelQuery model_query("29",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) SELECT isbn, 1, 0, 1 FROM Book;");
    StudentQuery stu_query("29",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) SELECT isbn, 1, 0 FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: mismatch in SELECT field count versus target columns.
    BOOST_CHECK_NE(cmp_info.message.find("values"), std::string::npos);
}

// Test case 30: INSERT ... SELECT with no target columns specified and a field count mismatch.
BOOST_AUTO_TEST_CASE(test_case_30)
{
    std::cout << "Test case 30: INSERT ... SELECT with no target columns specified, field count mismatch\n";
    ModelQuery model_query("30",
                           "INSERT INTO Copy (isbn, serial_number, weight, bookcase) SELECT isbn, 1, 0, 1 FROM Book;");
    StudentQuery stu_query("30",
                           "INSERT INTO Copy SELECT isbn, 1, 0 FROM Book;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect error: no target columns specified and a mismatch between SELECT field count and table column count.
    BOOST_CHECK_NE(cmp_info.message.find("columns"), std::string::npos);
}
