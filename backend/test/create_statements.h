#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include "../abstract_syntax_tree.h"
#include "../clauses/create_clause.h"
#include "../model_query.h"
#include "../student_query.h"
#include "../clauses/common.h"

// Test case 1: Correct CREATE TABLE statement.
BOOST_AUTO_TEST_CASE(create_test_case_1)
{
    std::cout << "Test case 1: Correct CREATE TABLE statement\n";
    ModelQuery model_query("C1",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C1",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Correct"), std::string::npos);
}

// Test case 2: Missing column definition (student omits the 'author' column).
BOOST_AUTO_TEST_CASE(create_test_case_2)
{
    std::cout << "Test case 2: CREATE TABLE missing column 'author'\n";
    ModelQuery model_query("C2",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C2",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("missing"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("author"), std::string::npos);
}

// Test case 3: Wrong column type (student uses INT instead of VARCHAR(20) for 'isbn').
BOOST_AUTO_TEST_CASE(create_test_case_3)
{
    std::cout << "Test case 3: CREATE TABLE with wrong type for column 'isbn'\n";
    ModelQuery model_query("C3",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C3",
                           "CREATE TABLE Book (isbn INT NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("varchar"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("int"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Type"), std::string::npos);
}

// Test case 4: NOT NULL constraint violation (student omits NOT NULL for 'title').
BOOST_AUTO_TEST_CASE(create_test_case_4)
{
    std::cout << "Test case 4: CREATE TABLE with missing NOT NULL on 'title'\n";
    ModelQuery model_query("C4",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C4",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255), author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("NOT NULL constraint for 'title'"), std::string::npos);
}

// Test case 5: Extra column in student query (student adds an extra column 'publisher').
BOOST_AUTO_TEST_CASE(create_test_case_5)
{
    std::cout << "Test case 5: CREATE TABLE with extra column 'publisher'\n";
    ModelQuery model_query("C5",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C5",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, publisher VARCHAR(100), PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("extra"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("publisher"), std::string::npos);
}

// Test case 6: Primary key mismatch (student uses 'title' instead of 'isbn').
BOOST_AUTO_TEST_CASE(create_test_case_6)
{
    std::cout << "Test case 6: CREATE TABLE with primary key mismatch\n";
    ModelQuery model_query("C6",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C6",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (title));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("primary key"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("isbn"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("title"), std::string::npos);
}

// Test case 7: Different column order but equivalent definitions.
BOOST_AUTO_TEST_CASE(create_test_case_7)
{
    std::cout << "Test case 7: CREATE TABLE with same columns in different order\n";
    ModelQuery model_query("C7",
                           "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C7",
                           "CREATE TABLE Book (author VARCHAR(255) NOT NULL, title VARCHAR(255) NOT NULL, isbn VARCHAR(20) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Since the compare function sorts the column names, this should pass as correct.
    BOOST_CHECK_NE(cmp_info.message.find("Column names"), std::string::npos);
}

// Test case 9: Missing table/view name (student query leaves out the name).
// The second query cannot be parsed so we skip this test case
/**
BOOST_AUTO_TEST_CASE(create_test_case_9)
{
    std::cout << "Test case 9: CREATE TABLE with missing table name\n";
    ModelQuery model_query("C9",
        "CREATE TABLE Book (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");
    StudentQuery stu_query("C9",
        "CREATE TABLE  (isbn VARCHAR(20) NOT NULL, title VARCHAR(255) NOT NULL, author VARCHAR(255) NOT NULL, PRIMARY KEY (isbn));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    std::cout << cmp_info;
    BOOST_CHECK_NE(cmp_info.message.find("Table name"), std::string::npos);
}*/

// Test case 10: Correct CREATE TABLE with a FOREIGN KEY constraint.
BOOST_AUTO_TEST_CASE(create_test_case_10)
{
    std::cout << "Test case 10: Correct CREATE TABLE with FOREIGN KEY constraint\n";
    ModelQuery model_query("10",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Customers (id));");
    StudentQuery stu_query("10",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Customers (id));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect that the foreign key constraint is recognized and correct.
    BOOST_CHECK_NE(cmp_info.message.find("Correct"), std::string::npos);
}

// Test case 11: CREATE TABLE with an incorrect FOREIGN KEY reference.
BOOST_AUTO_TEST_CASE(create_test_case_11)
{
    std::cout << "Test case 11: CREATE TABLE with incorrect FOREIGN KEY constraint\n";
    ModelQuery model_query("11",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Customers (id));");
    StudentQuery stu_query("11",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Clients (id));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect a message indicating a foreign key mismatch (e.g., "Customers" vs. "Clients").
    BOOST_CHECK_NE(cmp_info.message.find("Foreign keys"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("customers"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("clients"), std::string::npos);
}

// Test case 12: CREATE TABLE with an extra FOREIGN KEY constraint.
BOOST_AUTO_TEST_CASE(create_test_case_12)
{
    std::cout << "Test case 12: CREATE TABLE with extra FOREIGN KEY constraint\n";
    ModelQuery model_query("12",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Customers (id));");
    StudentQuery stu_query("12",
                           "CREATE TABLE Orders (order_id INT NOT NULL, customer_id INT NOT NULL, amount DECIMAL(10,2), "
                           "PRIMARY KEY (order_id), FOREIGN KEY (customer_id) REFERENCES Customers (id), FOREIGN KEY (order_id) REFERENCES Extra (oid));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback indicating extra foreign key constraint.
    BOOST_CHECK_NE(cmp_info.message.find("Foreign keys"), std::string::npos);
    // BOOST_CHECK_NE(cmp_info.message.find("extra"), std::string::npos);
}

// Test case 13: Correct CREATE TABLE with a UNIQUE constraint.
BOOST_AUTO_TEST_CASE(create_test_case_13)
{
    std::cout << "Test case 13: Correct CREATE TABLE with UNIQUE constraint\n";
    ModelQuery model_query("13",
                           "CREATE TABLE Products (product_id INT NOT NULL, product_name VARCHAR(100) NOT NULL, "
                           "PRIMARY KEY (product_id), UNIQUE (product_name));");
    StudentQuery stu_query("13",
                           "CREATE TABLE Products (product_id INT NOT NULL, product_name VARCHAR(100) NOT NULL, "
                           "PRIMARY KEY (product_id), UNIQUE (product_name));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Unique constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent!"), std::string::npos);
}

// Test case 14: CREATE TABLE with an incorrect UNIQUE constraint.
BOOST_AUTO_TEST_CASE(create_test_case_14)
{
    std::cout << "Test case 14: CREATE TABLE with incorrect UNIQUE constraint\n";
    ModelQuery model_query("14",
                           "CREATE TABLE Products (product_id INT NOT NULL, product_name VARCHAR(100) NOT NULL, "
                           "PRIMARY KEY (product_id), UNIQUE (product_name));");
    StudentQuery stu_query("14",
                           "CREATE TABLE Products (product_id INT NOT NULL, product_name VARCHAR(100) NOT NULL, "
                           "PRIMARY KEY (product_id), UNIQUE (productName));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback indicating the UNIQUE constraint does not match.
    BOOST_CHECK_NE(cmp_info.message.find("Unique constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("product_name"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("productname"), std::string::npos);
}

// Test case 15: Correct CREATE TABLE with a CHECK constraint.
BOOST_AUTO_TEST_CASE(create_test_case_15)
{
    std::cout << "Test case 15: Correct CREATE TABLE with CHECK constraint\n";
    ModelQuery model_query("15",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary > 0));");
    StudentQuery stu_query("15",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary > 0));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    BOOST_CHECK_NE(cmp_info.message.find("Check constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Excellent!"), std::string::npos);
}

// Test case 16: CREATE TABLE with an incorrect CHECK constraint.
BOOST_AUTO_TEST_CASE(create_test_case_16)
{
    std::cout << "Test case 16: CREATE TABLE with incorrect CHECK constraint\n";
    ModelQuery model_query("16",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary > 0));");
    StudentQuery stu_query("16",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary >= 0));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback indicating a mismatch in the CHECK condition.
    BOOST_CHECK_NE(cmp_info.message.find("Check constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find(">"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find(">="), std::string::npos);
}

// Test case 17: CREATE TABLE with multiple constraints mismatches (foreign key, unique and check).
BOOST_AUTO_TEST_CASE(create_test_case_17)
{
    std::cout << "Test case 17: CREATE TABLE with multiple constraint mismatches\n";
    ModelQuery model_query("17",
                           "CREATE TABLE Inventory (item_id INT NOT NULL, warehouse_id INT NOT NULL, quantity INT, "
                           "PRIMARY KEY (item_id), FOREIGN KEY (warehouse_id) REFERENCES Warehouses (id), UNIQUE (item_id, warehouse_id), "
                           "CHECK (quantity >= 0));");
    StudentQuery stu_query("17",
                           "CREATE TABLE Inventory (item_id INT NOT NULL, warehouse_id INT NOT NULL, quantity INT, "
                           "PRIMARY KEY (item_id), FOREIGN KEY (warehouse_id) REFERENCES Storage (id), "
                           "CHECK (quantity > 0));"); // UNIQUE constraint omitted and foreign key & check differ

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);
    // Expect feedback for foreign keys, unique constraints, and check constraints.
    BOOST_CHECK_NE(cmp_info.message.find("Foreign keys"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("warehouses"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("storage"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Unique constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("Check constraints"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find(">="), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find(">"), std::string::npos);
}

// Test case 18: CREATE TABLE with differing CHECK constraint values (salary >2000 vs. salary >4000)
BOOST_AUTO_TEST_CASE(create_test_case_18)
{
    std::cout << "Test case 18: CREATE TABLE with differing CHECK constraint values (salary >2000 vs. salary >4000)" << std::endl;
    ModelQuery model_query("18",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary > 2000));");
    StudentQuery stu_query("18",
                           "CREATE TABLE Employees (emp_id INT NOT NULL, salary INT, "
                           "PRIMARY KEY (emp_id), CHECK (salary > 4000));");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);

    // Expect feedback that mentions both "2000" and "4000" in the CHECK constraint comparison.
    BOOST_CHECK_NE(cmp_info.message.find("2000"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("4000"), std::string::npos);
}
// Test case 18: CREATE TABLE with differing CHECK constraint values (salary >2000 vs. salary >4000)
BOOST_AUTO_TEST_CASE(create_test_case_19)
{
    std::cout << "Test case 18: CREATE TABLE with differing CHECK constraint values (salary >2000 vs. salary >4000)" << std::endl;
    ModelQuery model_query("18",
                           "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, "
                           "CHECK (weight >= 20 AND weight <= 2000),  bookcase INT,  PRIMARY KEY(isbn, serial_number),  "
                           "FOREIGN KEY (isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE );");
    StudentQuery stu_query("18",
                           "CREATE TABLE Copy( isbn VARCHAR(13), serial_number INT, weight INT, bookcase INT, "
                           "CHECK(weight > 10 AND weight < 4000), PRIMARY KEY (isbn, serial_number), "
                           "FOREIGN KEY (isbn) REFERENCES Book(isbn) ON DELETE CASCADE ON UPDATE CASCADE NOT DEFERRABLE ) ;");

    model_query.create_abstract_syntax_tree();
    stu_query.create_abstract_syntax_tree();

    auto ref_ast = model_query.get_parse_tree();
    auto stu_ast = stu_query.get_parse_tree();

    Common::comparision_result cmp_info = Goals::compare_queries(ref_ast, stu_ast);

    // Expect feedback that mentions both "2000" and "4000" in the CHECK constraint comparison.
    BOOST_CHECK_NE(cmp_info.message.find("2000"), std::string::npos);
    BOOST_CHECK_NE(cmp_info.message.find("4000"), std::string::npos);
}
