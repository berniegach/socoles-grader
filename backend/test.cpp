#define BOOST_TEST_MODULE My Test 1
#include <boost/test/included/unit_test.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <algorithm>
#include "utils.h" // Include the header file for the function
#include "tree_edit_distance.h"
#include <pg_query.h>
#include "grader.h"
#include <stdio.h>
#include "abstract_syntax_tree.h"
#include "student_query.h"
#include "process_queries.h"
#include <tuple>
#include "my_evosql.h"
#include <regex>
#include "goals.h"
#include "clauses/select/group_by_clause.h"
#include "clauses/select/having_clause.h"
#include "clauses/select/order_by_clause.h"
#include "test/insert_statements.h"
#include "test/create_statements.h"
#include "test/create_view_statements.h"
#include "test/update_statements.h"
#include "test/delete_statements.h"
#include "test/output/create_query_output.h"
#include "test/output/insert_query_output.h"
#include "test/output/update_query_output.h"
#include "test/output/delete_query_output.h"
#include "test/grades/insert_query_grades.h"
#include "test/grades/create_query_grades.h"
// #include "query_generator.h"
// #include "pg_query.pb-c.h"
//--------------------------------------------------------------------------------------------------------------
//  These are utility functions that are used in the test cases. They are not part of the test cases themselves.
//--------------------------------------------------------------------------------------------------------------
void print_comparison_result(const Utils::comparison_result &result)
{
	std::cout << "{is_equal: " << std::boolalpha << result.is_equal
			  << ", is_subset: " << result.is_subset
			  << ", difference: " << result.difference << "}";
}
// Function to print a comparison_result
namespace boost
{
	namespace test_tools
	{
		namespace tt_detail
		{
			// output for comparison_result
			template <>
			struct print_log_value<Utils::comparison_result>
			{
				void operator()(std::ostream &os, Utils::comparison_result const &cr)
				{
					os << "{equal: " << std::boolalpha << cr.is_equal
					   << ", subset: " << cr.is_subset
					   << ", mismatch_count: " << cr.difference << "}";
				}
			};
			// output for Grader::property_state
			template <>
			struct print_log_value<Grader::property_state>
			{
				void operator()(std::ostream &os, Grader::property_state const &ps)
				{
					std::string state;
					switch (ps)
					{
					case Grader::property_state::CORRECT:
						state = "CORRECT";
						break;
					case Grader::property_state::MINOR_INCORRECT:
						state = "MINOR_INCORRECT";
						break;
					case Grader::property_state::SM_5:
						state = "SM_5";
						break;
					case Grader::property_state::SM_4:
						state = "SM_4";
						break;
					case Grader::property_state::SM_3:
						state = "SM_3";
						break;
					case Grader::property_state::SM_2:
						state = "SM_2";
						break;
					case Grader::property_state::SM_1:
						state = "SM_1";
						break;
					case Grader::property_state::INCORRECT:
						state = "INCORRECT";
						break;
					case Grader::property_state::INVALID:
						state = "INVALID";
						break;
					}
					os << state;
				}
			};

		}
	}
}
namespace boost
{
	namespace test_tools
	{
		namespace tt_detail
		{
			template <>
			struct print_log_value<std::vector<int>>
			{
				void operator()(std::ostream &os, std::vector<int> const &v)
				{
					os << "[";
					for (size_t i = 0; i < v.size(); ++i)
					{
						if (i != 0)
							os << ", ";
						os << v[i];
					}
					os << "]";
				}
			};
		}
	}
}

// Utility function to build and return the parse tree for a SQL query
std::shared_ptr<AbstractSyntaxTree::Node> build_sql_tree_json(const std::string &sql_query)
{
	AbstractSyntaxTree ast;
	auto parse_tree_json = nlohmann::json::parse(sql_query);
	std::shared_ptr<AbstractSyntaxTree::Node> root = std::make_shared<AbstractSyntaxTree::Node>("root", "");
	ast.build_tree(parse_tree_json, root);
	return root;
}
// Utility function to build and return the parse tree for a SQL query
std::shared_ptr<AbstractSyntaxTree::Node> build_sql_tree(const std::string &sql_query)
{
	AbstractSyntaxTree ast;
	PgQueryParseResult result = pg_query_parse(sql_query.c_str());
	auto parse_tree_json = nlohmann::json::parse(result.parse_tree);
	std::shared_ptr<AbstractSyntaxTree::Node> root = std::make_shared<AbstractSyntaxTree::Node>("root", "");
	ast.build_tree(parse_tree_json, root);
	pg_query_free_parse_result(result);
	return root;
}
//--------------------------------------------------------------------------------------------------------------
// Test suite for checking the correctness of the function to replace double quotes with single quotes.
// -------------------------------------------------------------------------------------------------------------
// Function to test replace_double_quotes_with_single_quotes
BOOST_AUTO_TEST_SUITE(replace_double_quotes_with_single_quotes_tests)

BOOST_AUTO_TEST_CASE(test_case_1)
{
	Utils my_utils;
	std::string query = "SELECT * FROM movies WHERE title = \"Star Wars\";";
	auto [corrected_query, query_altered] = my_utils.replace_double_quotes_with_single_quotes(query);
	std::string expected_query = "SELECT * FROM movies WHERE title = 'Star Wars';";
	bool expected_altered = true;

	BOOST_CHECK_EQUAL(corrected_query, expected_query);
	BOOST_CHECK_EQUAL(query_altered, expected_altered);
}

BOOST_AUTO_TEST_CASE(test_case_2)
{
	Utils my_utils;
	std::string query = "SELECT \"firstName\", \"lastName\" FROM \"Employees\";";
	auto [corrected_query, query_altered] = my_utils.replace_double_quotes_with_single_quotes(query);
	std::string expected_query = "SELECT \"firstName\", \"lastName\" FROM \"Employees\";";
	bool expected_altered = false;

	BOOST_CHECK_EQUAL(corrected_query, expected_query);
	BOOST_CHECK_EQUAL(query_altered, expected_altered);
}

BOOST_AUTO_TEST_CASE(test_case_3)
{
	Utils my_utils;
	std::string query = "SELECT * FROM movies WHERE title = \"Star Wars\" and album = \"my album\";";
	auto [corrected_query, query_altered] = my_utils.replace_double_quotes_with_single_quotes(query);
	std::string expected_query = "SELECT * FROM movies WHERE title = 'Star Wars' and album = 'my album';";
	bool expected_altered = true;

	BOOST_CHECK_EQUAL(corrected_query, expected_query);
	BOOST_CHECK_EQUAL(query_altered, expected_altered);
}

BOOST_AUTO_TEST_CASE(test_case_4)
{
	Utils my_utils;
	std::string query = "SELECT Theme.name, Theme.theme_id FROM Teacher, Theme WHERE Theme.teacher_id = (SELECT Teacher.teacher_id WHERE Teacher.name = \"Djoerd Hiemstra\");";
	auto [corrected_query, query_altered] = my_utils.replace_double_quotes_with_single_quotes(query);
	std::string expected_query = "SELECT Theme.name, Theme.theme_id FROM Teacher, Theme WHERE Theme.teacher_id = (SELECT Teacher.teacher_id WHERE Teacher.name = 'Djoerd Hiemstra');";
	bool expected_altered = true;

	BOOST_CHECK_EQUAL(corrected_query, expected_query);
	BOOST_CHECK_EQUAL(query_altered, expected_altered);
}
// Add more test cases as needed
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for checking comparisons of vectors.
																																							 * This is used to compare the results of the query with the expected results.
																																							 *************************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(compare_vectors_test)
/**
 * In this test case, the two vectors are equal.
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	// exact match
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	vector<vector<string>> correct_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	Utils::comparison_result expected_result{true, true, 0};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
/**
 * In this test case, the two vectors are not equal.
 * There are two extra elements that are left unmatched (g and h).
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	// mis-matched elements
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"a", "b", "c"},
		{"d", "e", "g"}};

	vector<vector<string>> correct_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	Utils::comparison_result expected_result{false, false, 2};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
/**
 * In this test case, the correct vector is contained within the query results.
 * The query results have two extra elements that are left unmatched (a and d).
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	// subset
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	vector<vector<string>> correct_results = {
		{"b", "c"},
		{"e", "f"}};

	Utils::comparison_result expected_result{false, true, 2};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
/**
 * In this test case, the two vectors are equal but have different order of rows.
 * The order of the rows is not important.
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	// order not required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	vector<vector<string>> correct_results = {
		{"d", "e", "f"},
		{"a", "b", "c"}};

	Utils::comparison_result expected_result{true, true, 0};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
/**
 * In this test case, the two vectors are not equal since they have different order of rows.
 */
BOOST_AUTO_TEST_CASE(test_case_5)
{
	// order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	vector<vector<string>> correct_results = {
		{"d", "e", "f"},
		{"a", "b", "c"}};

	Utils::comparison_result expected_result{false, false, 6};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
/**
 * In this test case, query results are contained within the correct results. We do not care about this containment.
 * There are two extra elements that are left unmatched ("2" and "2").
 */
BOOST_AUTO_TEST_CASE(test_case_6)
{
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"SQL"}};

	vector<vector<string>> correct_results = {
		{"2", "2", "SQL"}};

	Utils::comparison_result expected_result{false, false, 2};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
/**
 * In this case, the two vectors are very different.
 * THere are 12 extra elements that are left unmatched.
 */
BOOST_AUTO_TEST_CASE(test_case_7)
{
	// subset with order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"2"}, {"2"}, {"2"}, {"2"}, {"2"}, {"2"}, {"2"}, {"2"}};

	vector<vector<string>> correct_results = {
		{"2", "SQL"}, {"3", "DB"}, {"4", "IR"}};

	Utils::comparison_result expected_result{false, false, 12};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
/**
 * In this case, the two vectors are very different.
 * THere are 12 extra elements that are left unmatched.
 */
BOOST_AUTO_TEST_CASE(test_case_8)
{
	// subset with order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"SMITH"}, {"ALLEN"}, {"WARD"}, {"JONES"}, {"MARTIN"}, {"BLAKE"}, {"CLARK"}, {"SCOTT"}, {"KING"}, {"TURNER"}, {"ADAMS"}, {"JAMES"}, {"FORD"}, {"MILLER"}};

	vector<vector<string>> correct_results = {
		{"CLARK"}, {"KING"}, {"MILLER"}};

	Utils::comparison_result expected_result{false, true, 15};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
/**
 * In this case, the two vectors are very different.
 * THere are 12 extra elements that are left unmatched.
 */
BOOST_AUTO_TEST_CASE(test_case_9)
{
	// subset with order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"SMITH"}, {"ALLEN"}, {"WARD"}, {"JONES"}, {"MARTIN"}, {"BLAKE"}, {"CLARK"}, {"SCOTT"}, {"KING"}, {"TURNER"}, {"ADAMS"}, {"JAMES"}, {"FORD"}, {"MILLER"}};

	vector<vector<string>> correct_results = {
		{"CLARK"}, {"KING"}, {"MILLER"}};

	Utils::comparison_result expected_result{false, true, 15}; // NB 15 VALUE NEEDS TO BE CHANGED
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_CASE(test_case_10)
{
	// order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"b", "a", "c"},
		{"e", "d", "f"}};

	vector<vector<string>> correct_results = {
		{"a", "b", "c"},
		{"d", "e", "f"}};

	Utils::comparison_result expected_result{true, true, 0};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_CASE(test_case_11)
{
	// order not required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"d", "a", "j"},
		{"z", "g", "b"},
		{"f", "c", "o"}};

	vector<vector<string>> correct_results = {
		{"j", "d", "a"},
		{"b", "z", "g"},
		{"o", "f", "c"}};

	Utils::comparison_result expected_result{true, true, 0};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_CASE(test_case_12)
{
	// order required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"d", "a", "j"},
		{"z", "g", "b"},
		{"f", "c", "o"}};

	vector<vector<string>> correct_results = {
		{"j", "d", "a"},
		{"o", "f", "c"},
		{"b", "z", "g"}};

	Utils::comparison_result expected_result{false, false, 12};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_CASE(test_case_13)
{
	// order not required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"4", "1", "20"},
		{"10", "3", "8"},
		{"1", "2", "1"}};

	vector<vector<string>> correct_results = {
		{"1", "4", "20"},
		{"3", "10", "8"},
		{"2", "1", "1"}};

	Utils::comparison_result expected_result{true, true, 0};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, false);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_CASE(test_case_14)
{
	// order not required
	Utils my_utils;
	using namespace std;

	vector<vector<string>> query_results = {
		{"4", "1", "20"},
		{"10", "3", "8"},
		{"1", "2", "1"}};

	vector<vector<string>> correct_results = {
		{"1", "4", "20"},
		{"2", "1", "1"},
		{"3", "10", "8"}};

	Utils::comparison_result expected_result{false, false, 12};
	Utils::comparison_result result = my_utils.compare_vectors(query_results, correct_results, true);

	BOOST_TEST(result == expected_result);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for checking the general edit distance algorithm for text using random words.
																																							 * This is the levenstein distance algorithm.
																																							 *************************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(test_general_edit_distance)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Utils my_utils;

	std::string source = "kitten";
	std::string target = "sitting";
	std::string::size_type expected = 3;
	std::string::size_type result = my_utils.general_edit_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	Utils my_utils;

	std::string source = "rosettacode";
	std::string target = "raisethysword";
	std::string::size_type expected = 8;
	std::string::size_type result = my_utils.general_edit_distance(source, target);

	BOOST_TEST(result == expected);
}
/**
 * In this test case, the two strings are equal but one string has extra spaces.
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	Utils my_utils;

	std::string source = "select * from table1";
	std::string target = "select  * from     table1";

	my_utils.preprocess_query(source);
	my_utils.preprocess_query(target);

	std::string::size_type expected = 0;
	std::string::size_type result = my_utils.general_edit_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	Utils my_utils;

	// create a wide multibyte string

	string source = "´Nevermind´";
	// string source = "`Nevermind`";
	string target = "'Nevermind'";

	my_utils.preprocess_query(source);
	my_utils.preprocess_query(target);

	std::string::size_type expected = 4;
	std::string::size_type result = my_utils.general_edit_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_SUITE_END()
/*//--------------------------------------------------------------------------------------------------------------
// Test suite for checking the normalized edit distance algorithm for text using random words.
// This is the levenstein distance algorithm.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(test_normalized_edit_distance)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Utils my_utils;

	std::string source = "kitten";
	std::string target = "sitting";
	double expected = 0.5;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	Utils my_utils;

	std::string source = "rosettacode";
	std::string target = "raisethysword";
	double expected = 0.6153846153846154;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	Utils my_utils;

	std::string source = "select";
	std::string target = "selext";
	double expected = 0.6153846153846154;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	Utils my_utils;

	std::string source = "select";
	std::string target = "selexta";
	double expected = 0.6153846153846154;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	Utils my_utils;

	std::string source = "select";
	std::string target = "selectaaa";
	double expected = 0.6153846153846154;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_6)
{
	Utils my_utils;

	std::string source = "select";
	std::string target = "SELECT";
	double expected = 0.6153846153846154;
	double result = my_utils.normalized_levenshtein_distance(source, target);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_SUITE_END()*/
//--------------------------------------------------------------------------------------------------------------
// Test suite for checking building of a parse tree.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(parse_query_tree_tests)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	AbstractSyntaxTree ast;
	std::string input_json_str = R"({
        "RawStmt": {
            "stmt": {
                "SelectStmt": {
                    "targetList": [
                        {
                            "ResTarget": {
                                "name": "column1",
                                "val": {
                                    "ColumnRef": {
                                        "fields": [
                                            {
                                                "String": {
                                                    "str": "column1"
                                                }
                                            }
                                        ],
                                        "location": 7
                                    }
                                },
                                "location": 7
                            }
                        }
                    ],
                    "fromClause": [
                        {
                            "RangeVar": {
                                "relname": "table1",
                                "inh": true,
                                "relpersistence": "p",
                                "location": 15
                            }
                        }
                    ],
                    "op": 0
                }
            }
        }
    })";

	nlohmann::json input_json = nlohmann::json::parse(input_json_str);
	std::shared_ptr<AbstractSyntaxTree::Node> root = std::make_shared<AbstractSyntaxTree::Node>("Root", "");
	ast.build_tree(input_json, root);

	// The following is a simple test to check if the root node has been populated
	BOOST_REQUIRE_EQUAL(root->children.size(), 1);
	BOOST_REQUIRE_EQUAL(root->children[0]->key, "RawStmt");
}
/**
 * We check that the type of a statement: update statement.
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	AbstractSyntaxTree ast;
	// we remove the ; at the end of the query
	// This is because libpg_query removes it in its processing
	ModelQuery model_query("1", "UPDATE emp SET ename = 'NAME1' WHERE empno = 7934");

	model_query.create_abstract_syntax_tree();
	string statement_type = ast.get_statement_type(model_query.get_parse_tree());

	BOOST_CHECK_EQUAL(statement_type, "UpdateStmt");
}
/**
 * We check that the type of a statement: select statement.
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	AbstractSyntaxTree ast;
	// we remove the ; at the end of the query
	// This is because libpg_query removes it in its processing
	ModelQuery model_query("1", "SELECT * FROM emp WHERE empno = 7934");

	model_query.create_abstract_syntax_tree();
	string statement_type = ast.get_statement_type(model_query.get_parse_tree());

	BOOST_CHECK_EQUAL(statement_type, "SelectStmt");
}
/**
 * We check that the type of a statement: insert statement.
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	AbstractSyntaxTree ast;
	// we remove the ; at the end of the query
	// This is because libpg_query removes it in its processing
	ModelQuery model_query("1", "INSERT INTO emp (empno, ename) VALUES (7934, 'NAME1')");

	model_query.create_abstract_syntax_tree();
	string statement_type = ast.get_statement_type(model_query.get_parse_tree());

	BOOST_CHECK_EQUAL(statement_type, "InsertStmt");
}
/**
 * We check that the type of a statement: delete statement.
 */
BOOST_AUTO_TEST_CASE(test_case_5)
{
	AbstractSyntaxTree ast;
	// we remove the ; at the end of the query
	// This is because libpg_query removes it in its processing
	ModelQuery model_query("1", "DELETE FROM emp WHERE empno = 7934");

	model_query.create_abstract_syntax_tree();
	string statement_type = ast.get_statement_type(model_query.get_parse_tree());

	BOOST_CHECK_EQUAL(statement_type, "DeleteStmt");
}
/**
 * We check that the type of a statement: create statement.
 */
BOOST_AUTO_TEST_CASE(test_case_6)
{
	AbstractSyntaxTree ast;
	// we remove the ; at the end of the query
	// This is because libpg_query removes it in its processing
	ModelQuery model_query("1", "CREATE TABLE emp (empno int, ename varchar(255))");

	model_query.create_abstract_syntax_tree();
	string statement_type = ast.get_statement_type(model_query.get_parse_tree());

	BOOST_CHECK_EQUAL(statement_type, "CreateStmt");
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for checking the post-order traversal of a parse tree.
// The tree is used in the tree edit distance algorithm.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(post_order_traversal_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto root = std::make_shared<AbstractSyntaxTree::Node>("root", "1");
	auto child1 = std::make_shared<AbstractSyntaxTree::Node>("child1", "2");
	auto child2 = std::make_shared<AbstractSyntaxTree::Node>("child2", "3");
	auto grandchild1 = std::make_shared<AbstractSyntaxTree::Node>("grandchild1", "4");
	auto grandchild2 = std::make_shared<AbstractSyntaxTree::Node>("grandchild2", "5");

	root->add_child(child1);
	root->add_child(child2);
	child1->add_child(grandchild1);
	child1->add_child(grandchild2);

	// Perform post-order traversal.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> result = tree_edit_distance.post_order_traversal(root);

	// Check if the traversal result has the correct size.
	BOOST_CHECK_EQUAL(result.size(), 5);

	// Check if the traversal result has the correct order.
	BOOST_CHECK_EQUAL(result[0]->key, "grandchild1");
	BOOST_CHECK_EQUAL(result[1]->key, "grandchild2");
	BOOST_CHECK_EQUAL(result[2]->key, "child1");
	BOOST_CHECK_EQUAL(result[3]->key, "child2");
	BOOST_CHECK_EQUAL(result[4]->key, "root");
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto root = std::make_shared<AbstractSyntaxTree::Node>("F", "1");
	auto node1 = std::make_shared<AbstractSyntaxTree::Node>("A", "2");
	auto node2 = std::make_shared<AbstractSyntaxTree::Node>("B", "3");
	auto node3 = std::make_shared<AbstractSyntaxTree::Node>("C", "4");
	auto node4 = std::make_shared<AbstractSyntaxTree::Node>("D", "5");
	auto node5 = std::make_shared<AbstractSyntaxTree::Node>("E", "6");
	auto node6 = std::make_shared<AbstractSyntaxTree::Node>("G", "7");
	auto node7 = std::make_shared<AbstractSyntaxTree::Node>("H", "8");
	auto node8 = std::make_shared<AbstractSyntaxTree::Node>("I", "9");

	root->add_child(node2);
	root->add_child(node6);
	node2->add_child(node1);
	node2->add_child(node4);
	node4->add_child(node3);
	node4->add_child(node5);
	node6->add_child(node8);
	node8->add_child(node7);

	// Perform post-order traversal.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> result = tree_edit_distance.post_order_traversal(root);

	// Check if the traversal result has the correct size.
	BOOST_CHECK_EQUAL(result.size(), 9);

	// Check if the traversal result has the correct order.
	BOOST_CHECK_EQUAL(result[0]->key, "A");
	BOOST_CHECK_EQUAL(result[1]->key, "C");
	BOOST_CHECK_EQUAL(result[2]->key, "E");
	BOOST_CHECK_EQUAL(result[3]->key, "D");
	BOOST_CHECK_EQUAL(result[4]->key, "B");
	BOOST_CHECK_EQUAL(result[5]->key, "H");
	BOOST_CHECK_EQUAL(result[6]->key, "I");
	BOOST_CHECK_EQUAL(result[7]->key, "G");
	BOOST_CHECK_EQUAL(result[8]->key, "F");
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for the find_ancestors function.
// This suite contains tests to verify the correctness of the find_ancestors function
// for different nodes in a sample parse tree. It checks if the ancestor results have
// the correct size and order.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(ancestor_tests)
BOOST_AUTO_TEST_CASE(test_find_ancestors)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto node_1 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "1");
	auto node_2 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "2");
	auto node_3 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "3");
	auto node_4 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "4");
	auto node_5 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "5");

	node_1->add_child(node_2);
	node_1->add_child(node_3);
	node_2->add_child(node_4);
	node_2->add_child(node_5);

	// Test find_ancestors for node_4.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> ancestors1 = tree_edit_distance.find_ancestors(node_1, node_4);

	// Check if the ancestor result has the correct size.
	BOOST_CHECK_EQUAL(ancestors1.size(), 2);

	// Check if the ancestor result has the correct order.
	BOOST_CHECK_EQUAL(ancestors1[0]->key, "node_1");
	BOOST_CHECK_EQUAL(ancestors1[1]->key, "node_2");

	// Test find_ancestors for node_3.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> ancestors2 = tree_edit_distance.find_ancestors(node_1, node_3);

	// Check if the ancestor result has the correct size.
	BOOST_CHECK_EQUAL(ancestors2.size(), 1);

	// Check if the ancestor result has the correct order.
	BOOST_CHECK_EQUAL(ancestors2[0]->key, "node_1");
}

BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for the find_leftmost_leaf_descendants function.
// This suite contains tests to verify the correctness of the find_leftmost_leaf_descendants
// function for a sample parse tree. It checks if the leftmost leaf descendants result has
// the correct size and order for each node in the parse tree.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(leftmost_leaf_descendant_tests)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto root = std::make_shared<AbstractSyntaxTree::Node>("root", "r");
	auto node_1 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "1");
	auto node_2 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "2");
	auto node_3 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "3");
	auto node_4 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "4");
	auto node_5 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "5");
	auto node_6 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "6");
	auto node_7 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "7");
	auto node_8 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "8");
	auto node_9 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "9");
	auto node_10 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "10");

	root->add_child(node_1);
	root->add_child(node_2);
	node_1->add_child(node_3);
	node_1->add_child(node_4);
	node_4->add_child(node_5);
	node_2->add_child(node_6);
	node_6->add_child(node_7);
	node_6->add_child(node_8);
	node_8->add_child(node_9);
	node_8->add_child(node_10);

	// Test find_leftmost_leaf_descendants.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> leftmost_leaf_descendants = tree_edit_distance.find_leftmost_leaf_descendants(root);

	// Check if the leftmost_leaf_descendants result has the correct size.
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants.size(), 4);

	// Check if the leftmost_leaf_descendants result has the correct order.
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants[0], node_3); // node_2's leftmost leaf descendant
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants[1], node_5); // node_1's leftmost leaf descendant
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants[2], node_7); // node_3's leftmost leaf descendant
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants[3], node_9); // node_4's leftmost leaf descendant
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto node_1 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "1");
	auto node_2 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "2");
	auto node_3 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "3");
	auto node_4 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "4");
	auto node_5 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "5");

	node_1->add_child(node_2);
	node_1->add_child(node_3);
	node_2->add_child(node_4);
	node_2->add_child(node_5);

	// Test find_leftmost_leaf_descendants.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> leftmost_leaf_descendants = tree_edit_distance.find_leftmost_leaf_descendants(node_1);

	// Check if the leftmost_leaf_descendants result has the correct size.
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants.size(), 1);

	// Check if the leftmost_leaf_descendants result has the correct order.
	BOOST_CHECK_EQUAL(leftmost_leaf_descendants[0], node_4); // node_2's leftmost leaf descendant
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for testing the left right keyroots function.
// This function is used in the tree edit distance alorithm.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(lr_keyroots_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto root = std::make_shared<AbstractSyntaxTree::Node>("root", "r");
	auto node_1 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "1");
	auto node_2 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "2");
	auto node_3 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "3");
	auto node_4 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "4");
	auto node_5 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "5");
	auto node_6 = std::make_shared<AbstractSyntaxTree::Node>("node_1", "6");
	auto node_7 = std::make_shared<AbstractSyntaxTree::Node>("node_2", "7");
	auto node_8 = std::make_shared<AbstractSyntaxTree::Node>("node_3", "8");
	auto node_9 = std::make_shared<AbstractSyntaxTree::Node>("node_4", "9");
	auto node_10 = std::make_shared<AbstractSyntaxTree::Node>("node_5", "10");

	root->add_child(node_1);
	root->add_child(node_2);
	node_1->add_child(node_3);
	node_1->add_child(node_4);
	node_4->add_child(node_5);
	node_2->add_child(node_6);
	node_6->add_child(node_7);
	node_6->add_child(node_8);
	node_8->add_child(node_9);
	node_8->add_child(node_10);

	// Test find_keyroots.
	// Perform post-order traversal.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> postorder = tree_edit_distance.post_order_traversal(root);
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> keyroots = tree_edit_distance.find_lr_keyroots_node(postorder);

	// Check if the keyroots result has the correct size.
	BOOST_CHECK_EQUAL(keyroots.size(), 5);

	// Check if the keyroots result has the correct order.
	BOOST_CHECK_EQUAL(keyroots[0], node_4);
	BOOST_CHECK_EQUAL(keyroots[1], node_10);
	BOOST_CHECK_EQUAL(keyroots[2], node_8);
	BOOST_CHECK_EQUAL(keyroots[3], node_2);
	BOOST_CHECK_EQUAL(keyroots[4], root);
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for testing the tree edit distance algorithm.
// The test suite uses the letter of the alphabet as nodes for simplicity.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(general_tree_edit_distances)
/*
 * 				f				f
 *			   / \             / \
 *			  d   e			  c   e
 *			 / \             /
 *     	    a   c           d
 *               |          / \
 *               b         a   b
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");

	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c_2 = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e_2 = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f_2 = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");

	node_f->add_child(node_d);
	node_f->add_child(node_e);
	node_d->add_child(node_a);
	node_d->add_child(node_c);
	node_c->add_child(node_b);

	node_f_2->add_child(node_d_2);
	node_f_2->add_child(node_e_2);
	node_d_2->add_child(node_a_2);
	node_d_2->add_child(node_c_2);
	node_c_2->add_child(node_b_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_f, node_f_2);
	// print_tree(node_f);
	// print_tree(node_f_2);

	BOOST_CHECK_EQUAL(edit_distance, 0);
}
/*
 * 				f				f
 *			   / \             / \
 *			  d   e			  c   e
 *			 / \             /
 *     	    a   c           d
 *               |          / \
 *               b         a   b
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");

	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c_2 = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e_2 = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f_2 = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");

	node_f->add_child(node_d);
	node_f->add_child(node_e);
	node_d->add_child(node_a);
	node_d->add_child(node_c);
	node_c->add_child(node_b);

	node_f_2->add_child(node_c_2);
	node_f_2->add_child(node_e_2);
	node_c_2->add_child(node_d_2);
	node_d_2->add_child(node_a_2);
	node_d_2->add_child(node_b_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_f, node_f_2);
	// print_tree(node_f);
	// print_tree(node_f_2);

	BOOST_CHECK_EQUAL(edit_distance, 2);
}
/*
Tree 1:                Tree 2:

		 a                        a
	   /   \                    /   \
	  b     c                  b     d
	 / \   / \                / \   / \
	e   f g   h              e   f i   j
*/
BOOST_AUTO_TEST_CASE(test_case_3)
{
	TreeEditDistance tree_edit_distance;
	// tree 1.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");
	auto node_g = std::make_shared<AbstractSyntaxTree::Node>("node_g", "g");
	auto node_h = std::make_shared<AbstractSyntaxTree::Node>("node_h", "h");

	// tree 2.
	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e_2 = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f_2 = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");
	auto node_i_2 = std::make_shared<AbstractSyntaxTree::Node>("node_i", "i");
	auto node_j_2 = std::make_shared<AbstractSyntaxTree::Node>("node_j", "j");

	node_a->add_child(node_b);
	node_a->add_child(node_c);
	node_b->add_child(node_e);
	node_b->add_child(node_f);
	node_c->add_child(node_g);
	node_c->add_child(node_h);

	node_a_2->add_child(node_b_2);
	node_a_2->add_child(node_d_2);
	node_b_2->add_child(node_e_2);
	node_b_2->add_child(node_f_2);
	node_d_2->add_child(node_i_2);
	node_d_2->add_child(node_j_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_a, node_a_2);
	// print_tree(node_a);
	// print_tree(node_a_2);

	BOOST_CHECK_EQUAL(edit_distance, 3);
}
/*
Tree 1:                Tree 2:

		 a                        a
	   /   \                    /   \
	  b     c                  b     c
	 / \   / \                / \   / \
	e   f g   h              d   f g   h
*/
BOOST_AUTO_TEST_CASE(test_case_4)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree for Tree 1.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");
	auto node_g = std::make_shared<AbstractSyntaxTree::Node>("node_g", "g");
	auto node_h = std::make_shared<AbstractSyntaxTree::Node>("node_h", "h");

	// Create a sample parse tree for Tree 2.
	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c_2 = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_f_2 = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");
	auto node_g_2 = std::make_shared<AbstractSyntaxTree::Node>("node_g", "g");
	auto node_h_2 = std::make_shared<AbstractSyntaxTree::Node>("node_h", "h");

	node_a->add_child(node_b);
	node_a->add_child(node_c);
	node_b->add_child(node_e);
	node_b->add_child(node_f);
	node_c->add_child(node_g);
	node_c->add_child(node_h);

	node_a_2->add_child(node_b_2);
	node_a_2->add_child(node_c_2);
	node_b_2->add_child(node_d_2);
	node_b_2->add_child(node_f_2);
	node_c_2->add_child(node_g_2);
	node_c_2->add_child(node_h_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_a, node_a_2);
	// print_tree(node_a);
	// print_tree(node_a_2);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
/*
Tree 1:                Tree 2:

		 a                        a
	   / | \                    / | \
	  b  c  d                  b  c  d
	  |                        |
	  e                        f
*/
BOOST_AUTO_TEST_CASE(test_case_5)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree for Tree 1.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");

	// Create a sample parse tree for Tree 2.
	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c_2 = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_f_2 = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");

	node_a->add_child(node_b);
	node_a->add_child(node_c);
	node_a->add_child(node_d);
	node_b->add_child(node_e);

	node_a_2->add_child(node_b_2);
	node_a_2->add_child(node_c_2);
	node_a_2->add_child(node_d_2);
	node_b_2->add_child(node_f_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_a, node_a_2);
	// print_tree(node_a);
	// print_tree(node_a_2);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
/*
Tree 1:                Tree 2:

		 a                        a
	   / | \                    / | \
	  b  c  d                  b  c  d
	  |  |  |                  |  |  |
	  e  f  g                  h  i  j
*/
BOOST_AUTO_TEST_CASE(test_case_6)
{
	TreeEditDistance tree_edit_distance;
	// Create a sample parse tree for Tree 1.
	auto node_a = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_e = std::make_shared<AbstractSyntaxTree::Node>("node_e", "e");
	auto node_f = std::make_shared<AbstractSyntaxTree::Node>("node_f", "f");
	auto node_g = std::make_shared<AbstractSyntaxTree::Node>("node_g", "g");

	// Create a sample parse tree for Tree 2.
	auto node_a_2 = std::make_shared<AbstractSyntaxTree::Node>("node_a", "a");
	auto node_b_2 = std::make_shared<AbstractSyntaxTree::Node>("node_b", "b");
	auto node_c_2 = std::make_shared<AbstractSyntaxTree::Node>("node_c", "c");
	auto node_d_2 = std::make_shared<AbstractSyntaxTree::Node>("node_d", "d");
	auto node_h_2 = std::make_shared<AbstractSyntaxTree::Node>("node_h", "h");
	auto node_i_2 = std::make_shared<AbstractSyntaxTree::Node>("node_i", "i");
	auto node_j_2 = std::make_shared<AbstractSyntaxTree::Node>("node_j", "j");

	node_a->add_child(node_b);
	node_a->add_child(node_c);
	node_a->add_child(node_d);
	node_b->add_child(node_e);
	node_c->add_child(node_f);
	node_d->add_child(node_g);

	node_a_2->add_child(node_b_2);
	node_a_2->add_child(node_c_2);
	node_a_2->add_child(node_d_2);
	node_b_2->add_child(node_h_2);
	node_c_2->add_child(node_i_2);
	node_d_2->add_child(node_j_2);

	int edit_distance = tree_edit_distance.zhang_shasha(node_a, node_a_2);
	// print_tree(node_a);
	// print_tree(node_a_2);

	BOOST_CHECK_EQUAL(edit_distance, 3);
}
BOOST_AUTO_TEST_SUITE_END()
//---------------------------------------------------------------------------
// Test cases for the tree edit distance algorithm.
// This test suite uses SQL queries as input.
//---------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(sql_tree_edit_distances)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	// queries
	std::string query_a = "select col1 from table1;";
	std::string query_b = "select col2 from table1;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	// queries
	std::string query_a = "select col1 from table1;";
	std::string query_b = "select col1 from table2;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	// queries
	std::string query_a = "select col1 from table1 where val1 = 2;";
	std::string query_b = "select col1 from table1 where val1 = 3;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	// queries
	std::string query_a = "select col1, col2 from table1;";
	std::string query_b = "select col1, col3 from table1;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	// queries
	std::string query_a = "select col1, col2 from table1;";
	std::string query_b = "select col1, col2 from table2;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_6)
{
	// queries
	std::string query_a = "select col1 from table1 where val1 = 2;";
	std::string query_b = "select col1 from table2 where val1 = 2;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_7)
{
	// queries
	std::string query_a = "select col1 from table1 where val1 = 2 ;";
	std::string query_b = "select col3, col5, col6 from table3 where val1 = 2 and val2 = 4;";

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;
	// print_tree(tree_a);
	// print_tree(tree_b);
	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);

	// for now the edit distance is big because there is no pruning of some unnecessary nodes of the tree. each column added comes with a lot of other nodes.
	BOOST_CHECK_EQUAL(edit_distance, 31);
}
BOOST_AUTO_TEST_CASE(test_case_8)
{
	// queries
	std::string query_a = "select * from table1 where val1 = 2;";
	std::string query_b = "select * from table1 where val2 = 2;";

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// print_tree(tree_a);
	// print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_9)
{
	/**
	 * This test case highlights the limitation of parsing sql statements with different lengths.
	 * Without the pruning of the tree, the edit distance is 4.
	 * With the pruning of the tree, the edit distance is 1.
	 */
	AbstractSyntaxTree ast;
	// queries
	std::string query_a = "select * from theme where theme_id = 2;";
	std::string query_b = "select * from theme where themeid = 2;";

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	// ast.print_tree(tree_a);
	// ast.print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for testing the correctness matrix function.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(correctness_matrix_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Grader grader;

	int syntax = 2;	   // 0: incorrect, 1: correct
	int semantics = 2; // 0: incorrect, 1: correct
	int results = 2;   // 0: incorrect, 1: correct

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<std::vector<int>> expected{
		{0, 0, 0}, // level 1: all properties incorrect
		{0, 0, 1}, // level 2: only syntax correct
		//{0, 1, 0}, // level 3: only semantics correct
		//{0, 1, 1}, // level 4: syntax and semantics correct, results incorrect
		{1, 0, 0}, // level 5: only results correct
		{1, 0, 1}, // level 6: results and syntax correct, semantics incorrect
		{1, 1, 0}, // level 7: results and semantics correct, syntax incorrect
		{1, 1, 1}  // level 8: all properties correct
	};
	// in this test case the results are most important. then semantics and finally the syntax.
	// A change in results makes the level jump from 1 to 5. A change in semantics makes the level jump from 1 to 3. a change in syntax makes the level jump from 1 to 2.
	/*std::vector<std::vector<int>> actual = grader.get_correctness_matrix(syntax, semantics, results, 3);

	BOOST_CHECK_EQUAL_COLLECTIONS(
		expected.begin(), expected.end(),
		actual.begin(), actual.end()
	);*/
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	Grader grader;

	int syntax = 3;	   // 0: Incorrect, 1: Correct, 2: minor incorrect
	int semantics = 3; // 0: Incorrect, 1: Correct, 2: minor incorrect
	int results = 3;   // 0: Incorrect, 1: Correct, 2: minor incorrect

	// The order of the outcomes in each subvector is: [result, semantics, syntax].
	std::vector<std::vector<int>> expected{
		{0, 0, 0}, // level 1: all properties incorrect
		{0, 0, 1}, // level 2: only syntax correct
		{0, 0, 2}, // level 3: syntax minor incorrect
		//{0, 1, 0}, // level 4: only semantics correct
		//{0, 1, 1}, // level 5: syntax and semantics correct, results incorrect
		//{0, 1, 2}, // level 6: syntax minor incorrect, semantics correct, results incorrect
		// {0, 2, 0}, // level 7: semantics minor incorrect
		//{0, 2, 1}, // level 8: syntax correct, semantics minor incorrect, results incorrect
		//{0, 2, 2}, // level 9: syntax and semantics minor incorrect, results incorrect
		//{1, 0, 0}, // level 10: only results correct
		{1, 0, 1}, // level 11: results and syntax correct, semantics incorrect
		{1, 0, 2}, // level 12: results correct, syntax minor incorrect, semantics incorrect
				   // {1, 1, 0}, // level 13: results and semantics correct, syntax incorrect
		//{1, 1, 1}, // level 14: all properties correct
		//{1, 1, 2}, // level 15: results and semantics correct, syntax minor incorrect
		//{1, 2, 0}, // level 16: results correct, semantics minor incorrect, syntax incorrect
		//{1, 2, 1}, // level 17: results correct, semantics minor incorrect, syntax correct
		//{1, 2, 2}, // level 18: results correct, syntax and semantics minor incorrect
		//{2, 0, 0}, // level 19: results minor incorrect
		//{2, 0, 1}, // level 20: results minor incorrect, syntax correct, semantics incorrect
		//{2, 0, 2}, // level 21: results and syntax minor incorrect, semantics incorrect
		// {2, 1, 0}, // level 22: results minor incorrect, semantics correct, syntax incorrect
		{2, 1, 1}, // level 23: results minor incorrect, syntax and semantics correct
		{2, 1, 2}, // level 24: all properties minor incorrect
		//{2, 2, 0}, // level 25: results and semantics minor incorrect, syntax incorrect
		{2, 2, 1}, // level 26: results and semantics minor incorrect, syntax correct
		{2, 2, 2}  // level 27: all properties
	};

	/*std::vector<std::vector<int>> actual = grader.get_correctness_matrix(syntax, semantics, results, 3);

	BOOST_CHECK_EQUAL_COLLECTIONS(
		expected.begin(), expected.end(),
		actual.begin(), actual.end()
	);*/
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},   // level 2: only syntax correct
		//{ps::INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 3: only semantics correct
		//{ps::INCORRECT, ps::CORRECT, ps::CORRECT}, // level 4: syntax and semantics correct, results incorrect
		{ps::CORRECT, ps::INCORRECT, ps::INCORRECT}, // level 5: only results correct
		{ps::CORRECT, ps::INCORRECT, ps::CORRECT},	 // level 6: results and syntax correct, semantics incorrect
		{ps::CORRECT, ps::CORRECT, ps::INCORRECT},	 // level 7: results and semantics correct, syntax incorrect
		{ps::CORRECT, ps::CORRECT, ps::CORRECT}		 // level 8: all properties correct
	};
	BOOST_REQUIRE_EQUAL(matrix.size(), 6);

	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::SM_SY_RE;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::CORRECT, ps::INCORRECT, ps::INCORRECT},   // level 2: only semantics correct
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},   // level 3: only syntax correct
		{ps::CORRECT, ps::INCORRECT, ps::CORRECT},	   // level 4: syntax and semantics correct, results incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 5: only results correct
		{ps::CORRECT, ps::CORRECT, ps::INCORRECT}, // level 6: results and semantics correct, syntax incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::CORRECT}, // level 7: results and syntax correct, semantics incorrect
		{ps::CORRECT, ps::CORRECT, ps::CORRECT} // level 8: all properties correct
	};
	BOOST_REQUIRE_EQUAL(matrix.size(), 6);

	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::THREE_LEVELS;
	pl semantics_level = pl::THREE_LEVELS;
	pl syntax_level = pl::THREE_LEVELS;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT},		 // level 1: all properties incorrect
		{ps::INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 2: only syntax MINOR correct
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},		 // level 3: syntax correct
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 4: only semantics MINOR correct
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 5: syntax MINOR correct, semantics MINOR correct, results incorrect
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 6: syntax correct, semantics MINOR correct, results incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 7: only semantics correct
		//{ps::INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 8: semantics correct, syntax MINOR correct, results incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::CORRECT}, // level 9: semantics correct
		//{ps::MINOR_INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 10: only results MINOR correct
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 11: results MINOR correct, syntax MINOR correct, semantics incorrect
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::CORRECT},		   // level 12: results MINOR correct, syntax correct, semantics incorrect
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 13: results MINOR correct, semantics MINOR correct, syntax incorrect
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 14: results MINOR correct, syntax MINOR correct, semantics MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 15: results MINOR correct, syntax MINOR correct, semantics correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 16: results MINOR correct, semantics correct, syntax incorrect
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 17: results MINOR correct, semantics correct, syntax MINOR correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::CORRECT}, // level 18: results MINOR correct, semantics correct
		//{ps::CORRECT, ps::INCORRECT, ps::INCORRECT}, // level 19: only results correct
		//{ps::CORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 20: results correct, syntax MINOR correct, semantics incorrect
		//{ps::CORRECT, ps::INCORRECT, ps::CORRECT}, // level 21: results correct, syntax correct, semantics incorrect
		//{ps::CORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 22: results correct, semantics MINOR correct, syntax incorrect
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 23: results correct, syntax MINOR correct, semantics MINOR correct
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::CORRECT},		 // level 24: results correct, syntax correct, semantics MINOR correct
		//{ps::CORRECT, ps::CORRECT, ps::INCORRECT}, // level 25: only semantics correct
		{ps::CORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 26: semantics correct, syntax MINOR correct, results incorrect
		{ps::CORRECT, ps::CORRECT, ps::CORRECT}			 // level 27: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 9);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_5_1)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::THREE_LEVELS;
	pl semantics_level = pl::THREE_LEVELS;
	pl syntax_level = pl::THREE_LEVELS;
	po order = po::SY_SM_RE;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		//{ps::MINOR_INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 2: only results MINOR correct
		//{ps::CORRECT, ps::INCORRECT, ps::INCORRECT}, // level 3: only results correct
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 4: only semantics MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 5: results minor incorrect, semantics minor incorrect, syntax incorrect
		//{ps::CORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 6: results correct, semantics minor incorrect, syntax incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 7: only semantics correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 8: results minor incorrect, semantics correct, syntax incorrect
		//{ps::CORRECT, ps::CORRECT, ps::INCORRECT}, // level 9: results correct, semantics correct, syntax incorrect
		{ps::INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT},	   // level 10: only syntax MINOR correct
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 11: results minor incorrect, syntax minor correct, semantics incorrect
		//{ps::CORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 12: results correct, syntax minor correct, semantics incorrect
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 13: only semantics MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 14: results minor incorrect, semantics minor incorrect, syntax minor correct
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 15: results correct, semantics minor incorrect, syntax minor correct
		//{ps::INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 16: only semantics correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 17: results minor incorrect, semantics correct, syntax minor correct
		{ps::CORRECT, ps::CORRECT, ps::MINOR_INCORRECT},   // level 18: results correct, semantics correct, syntax minor correct
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},	   // level 19: only syntax correct
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::CORRECT}, // level 20: results minor incorrect, syntax correct, semantics incorrect
		//{ps::CORRECT, ps::INCORRECT, ps::CORRECT}, // level 21: results correct, syntax correct, semantics incorrect
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 22: only semantics MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 23: results minor incorrect, semantics minor incorrect, syntax correct
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 24: results correct, semantics minor incorrect, syntax correct
		//{ps::INCORRECT, ps::CORRECT, ps::CORRECT}, // level 25: only semantics correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::CORRECT}, // level 26: results minor incorrect, semantics correct, syntax correct
		{ps::CORRECT, ps::CORRECT, ps::CORRECT} // level 27: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 9);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_5_2)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::THREE_LEVELS;
	pl semantics_level = pl::THREE_LEVELS;
	pl syntax_level = pl::THREE_LEVELS;
	po order = po::SY_RE_SM;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 2: only semantics MINOR correct
		//{ps::INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 3: only semantics correct
		//{ps::MINOR_INCORRECT, ps::INCORRECT, ps::INCORRECT}, // level 4: only results MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 5: results minor incorrect, semantics minor incorrect, syntax incorrect
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::INCORRECT}, // level 6: results minor incorrect, semantics correct, syntax incorrect
		//{ps::CORRECT, ps::INCORRECT, ps::INCORRECT}, // level 7: only results correct
		//{ps::CORRECT, ps::MINOR_INCORRECT, ps::INCORRECT}, // level 8: results correct, semantics minor incorrect, syntax incorrect
		//{ps::CORRECT, ps::CORRECT, ps::INCORRECT}, // level 9: results correct, semantics correct, syntax incorrect
		{ps::INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 10: only syntax MINOR correct
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 11: only syntax MINOR correct, semantics minor incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 12: syntax MINOR correct, semantics correct, results incorrect
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 13: only results MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 14: results minor incorrect, semantics minor incorrect, syntax minor correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT}, // level 15: results minor incorrect, semantics correct, syntax minor correct
		//{ps::CORRECT, ps::INCORRECT, ps::MINOR_INCORRECT}, // level 16: only results correct
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT}, // level 17: results correct, semantics minor incorrect, syntax minor correct
		{ps::CORRECT, ps::CORRECT, ps::MINOR_INCORRECT},		 // level 18: results correct, semantics correct, syntax minor correct
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},			 // level 19: only semantics correct
		//{ps::INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 20: only semantics MINOR correct, syntax correct, results incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::CORRECT}, // level 21: semantics correct, syntax correct, results incorrect
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::CORRECT}, // level 22: only results MINOR correct
		//{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 23: results minor incorrect, semantics minor incorrect, syntax correct
		//{ps::MINOR_INCORRECT, ps::CORRECT, ps::CORRECT}, // level 24: results minor incorrect, semantics correct, syntax correct
		//{ps::CORRECT, ps::INCORRECT, ps::CORRECT}, // level 25: only results correct
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::CORRECT}, // level 26: results correct, semantics minor incorrect, syntax correct
		{ps::CORRECT, ps::CORRECT, ps::CORRECT}			 // level 27: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 9);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_6)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::ABSENT;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INVALID}, // level 1: all properties incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::INVALID}, // level 2: only semantics correct
		{ps::CORRECT, ps::INCORRECT, ps::INVALID}, // level 3: only results correct
		{ps::CORRECT, ps::CORRECT, ps::INVALID}	   // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 3);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_7)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::ABSENT;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INVALID}, // level 1: all properties incorrect
		{ps::INVALID, ps::CORRECT, ps::INVALID}	   // level 2: only semantics correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 2);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_8)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::ABSENT;
	po order = po::SM_SY_RE;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INVALID}, // level 1: all properties incorrect
		{ps::CORRECT, ps::INCORRECT, ps::INVALID},	 // level 2: only results correct
		//{ps::INCORRECT, ps::CORRECT, ps::INVALID}, // level 3: only semantics correct
		{ps::CORRECT, ps::CORRECT, ps::INVALID} // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 3);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_9)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::SM_SY_RE;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::INVALID, ps::INCORRECT, ps::CORRECT},	 // level 2: only results correct
		{ps::INVALID, ps::CORRECT, ps::INCORRECT},	 // level 3: only semantics correct
		{ps::INVALID, ps::CORRECT, ps::CORRECT}		 // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 4);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_10)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::INVALID, ps::INCORRECT, ps::CORRECT},	 // level 2: only results correct
		{ps::INVALID, ps::CORRECT, ps::INCORRECT},	 // level 3: only semantics correct
		{ps::INVALID, ps::CORRECT, ps::CORRECT}		 // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 4);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_11)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::ABSENT;
	po order = po::RE_SM_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INVALID}, // level 1: all properties incorrect
		{ps::INVALID, ps::CORRECT, ps::INVALID},   // level 2: only results correct
												   //{ps::INVALID, ps::CORRECT, ps::INCORRECT}, // level 3: only semantics correct
												   //{ps::INVALID, ps::CORRECT, ps::CORRECT}  // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 2);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_12)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::SY_RE_SM;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::INVALID, ps::CORRECT, ps::INCORRECT},	 // level 2: only results correct
		{ps::INVALID, ps::INCORRECT, ps::CORRECT},	 // level 3: only semantics correct
		{ps::INVALID, ps::CORRECT, ps::CORRECT}		 // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 4);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_13)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::TWO_LEVELS;
	po order = po::SM_RE_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INCORRECT}, // level 1: all properties incorrect
		{ps::INVALID, ps::INCORRECT, ps::CORRECT},	 // level 2: only results correct
		{ps::INVALID, ps::CORRECT, ps::INCORRECT},	 // level 3: only semantics correct
		{ps::INVALID, ps::CORRECT, ps::CORRECT}		 // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 4);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_14)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::TWO_LEVELS;
	pl syntax_level = pl::ABSENT;
	po order = po::RE_SY_SM;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INVALID}, // level 1: all properties incorrect
		//{ps::INCORRECT, ps::CORRECT, ps::INVALID}, // level 2: only results correct
		{ps::CORRECT, ps::INCORRECT, ps::INVALID}, // level 3: only semantics correct
		{ps::CORRECT, ps::CORRECT, ps::INVALID}	   // level 4: all properties correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 3);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_15)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::ABSENT;
	pl semantics_level = pl::SEMATICS_LEVELS_6;
	pl syntax_level = pl::ABSENT;
	po order = po::SM_RE_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INVALID, ps::INCORRECT, ps::INVALID},		 // level 1: all properties incorrect
		{ps::INVALID, ps::SM_1, ps::INVALID},			 // level 2: semantics has only 1 correct clause
		{ps::INVALID, ps::SM_2, ps::INVALID},			 // level 3: semantics has only 2 correct clauses
		{ps::INVALID, ps::SM_3, ps::INVALID},			 // level 4: semantics has only 3 correct clauses
		{ps::INVALID, ps::SM_4, ps::INVALID},			 // level 5: semantics has only 4 correct clauses
		{ps::INVALID, ps::SM_5, ps::INVALID},			 // level 6: semantics has only 5 correct clauses
		{ps::INVALID, ps::MINOR_INCORRECT, ps::INVALID}, // level 7: semantics minor incorrect
		{ps::INVALID, ps::CORRECT, ps::INVALID}			 // level 8: semantics has all correct clauses
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 8);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_16)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::TWO_LEVELS;
	pl semantics_level = pl::SEMATICS_LEVELS_6;
	pl syntax_level = pl::ABSENT;
	po order = po::SM_RE_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	// grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INVALID},	 // level 1: all properties incorrect
		{ps::INCORRECT, ps::SM_1, ps::INVALID},			 // level 2: semantics has only 1 correct clause, results incorrect
		{ps::INCORRECT, ps::SM_2, ps::INVALID},			 // level 3: semantics has only 2 correct clauses, results incorrect
		{ps::INCORRECT, ps::SM_3, ps::INVALID},			 // level 4: semantics has only 3 correct clauses, results incorrect
		{ps::INCORRECT, ps::SM_4, ps::INVALID},			 // level 5: semantics has only 4 correct clauses, results incorrect
		{ps::INCORRECT, ps::SM_5, ps::INVALID},			 // level 6: semantics has only 5 correct clauses, results incorrect
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::INVALID}, // level 7: semantics minor incorrect, results correct
		{ps::CORRECT, ps::CORRECT, ps::INVALID}			 // level 8: semantics has all correct clauses, results correct
	};

	BOOST_REQUIRE_EQUAL(matrix.size(), 8);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_CASE(test_case_17)
{
	Grader grader;

	using ps = Grader::property_state;
	using pl = Grader::property_level;
	using po = Grader::property_order;

	pl results_level = pl::THREE_LEVELS;
	pl semantics_level = pl::SEMATICS_LEVELS_6;
	pl syntax_level = pl::THREE_LEVELS;
	po order = po::SM_RE_SY;

	std::vector<Grader::properties> matrix = grader.get_correctness_matrix(results_level, semantics_level, syntax_level, order);
	grader.display_correctness_matrix(matrix);

	// the order od the outcomes in each subvector is: [results, semantics, syntax]
	std::vector<Grader::properties> expected{
		{ps::INCORRECT, ps::INCORRECT, ps::INCORRECT},
		{ps::INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::INCORRECT, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::INCORRECT},
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::INCORRECT, ps::CORRECT},
		{ps::INCORRECT, ps::SM_1, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::SM_1, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::SM_1, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::SM_1, ps::CORRECT},
		{ps::INCORRECT, ps::SM_2, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::SM_2, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::SM_2, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::SM_2, ps::CORRECT},
		{ps::INCORRECT, ps::SM_3, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::SM_3, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::SM_3, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::SM_3, ps::CORRECT},
		{ps::INCORRECT, ps::SM_4, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::SM_4, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::SM_4, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::SM_4, ps::CORRECT},
		{ps::INCORRECT, ps::SM_5, ps::MINOR_INCORRECT},
		{ps::INCORRECT, ps::SM_5, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::SM_5, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::SM_5, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::INCORRECT},
		{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::MINOR_INCORRECT, ps::CORRECT},
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::INCORRECT},
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::MINOR_INCORRECT},
		{ps::CORRECT, ps::MINOR_INCORRECT, ps::CORRECT},
		{ps::MINOR_INCORRECT, ps::CORRECT, ps::MINOR_INCORRECT},
		{ps::MINOR_INCORRECT, ps::CORRECT, ps::CORRECT},
		{ps::CORRECT, ps::CORRECT, ps::MINOR_INCORRECT},
		{ps::CORRECT, ps::CORRECT, ps::CORRECT}};

	BOOST_REQUIRE_EQUAL(matrix.size(), 36);
	for (size_t i = 0; i < matrix.size(); ++i)
	{
		BOOST_CHECK(matrix[i].results == expected[i].results);
		BOOST_CHECK(matrix[i].semantics == expected[i].semantics);
		BOOST_CHECK(matrix[i].syntax == expected[i].syntax);
	}
}
BOOST_AUTO_TEST_SUITE_END()
//--------------------------------------------------------------------------------------------------------------
// Test suite for checking fingerprint.
// -------------------------------------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE(fingerprint_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Grader grader;
	AbstractSyntaxTree ast;

	std::string query_a = "SELECT first_name AS forename, last_name AS surname FROM students WHERE class = 'modelling';";
	std::string query_b = "SELECT first_name AS forename_alias, last_name AS surname_alias FROM students WHERE class = 'modelling';";
	std::string query_c = "SELECT first_name, last_name FROM students WHERE class = 'modelling';";
	std::string query_d = "SELECT first_name AS forename, last_name AS surname FROM students WHERE class = 'modelling' ORDER BY first_name;";
	std::string query_e = "SELECT name FROM users WHERE id = 1;";
	std::string query_f = "SELECT name FROM users WHERE id = 2;";
	std::string query_g = "SELECT theme.name AS theme_name, teacher.name AS teacher_name FROM theme, teacher WHERE theme.teacher_id = teacher.teacher_id   AND (theme.name = 'SQL' OR theme.name = 'ORM');";
	std::string query_h = "SELECT theme.name AS theme_name, teacher.name AS teacher_name FROM theme JOIN teacher ON theme.teacher_id = teacher.teacher_id WHERE theme.name = 'SQL' UNION SELECT theme.name AS theme_name, teacher.name AS teacher_name FROM theme JOIN teacher ON theme.teacher_id = teacher.teacher_id WHERE theme.name = 'ORM';";

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_c = build_sql_tree(query_c);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_d = build_sql_tree(query_d);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_e = build_sql_tree(query_e);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_f = build_sql_tree(query_f);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_g = build_sql_tree(query_g);
	std::shared_ptr<AbstractSyntaxTree::Node> tree_h = build_sql_tree(query_h);

	PgQueryFingerprintResult result_a, result_b, result_c, result_d, result_e, result_f, result_g, result_h;

	result_a = pg_query_fingerprint(query_a.c_str());
	result_b = pg_query_fingerprint(query_b.c_str());
	result_c = pg_query_fingerprint(query_c.c_str());
	result_d = pg_query_fingerprint(query_d.c_str());
	result_e = pg_query_fingerprint(query_e.c_str());
	result_f = pg_query_fingerprint(query_f.c_str());
	result_g = pg_query_fingerprint(query_g.c_str());
	result_h = pg_query_fingerprint(query_h.c_str());

	// printf("%s\n", result_a.fingerprint_str);
	// printf("%s\n", result_b.fingerprint_str);
	// printf("%s\n", result_c.fingerprint_str);
	// printf("%s\n", result_d.fingerprint_str);
	// printf("%s\n", result_e.fingerprint_str);
	// printf("%s\n", result_f.fingerprint_str);
	// printf("%s\n", result_g.fingerprint_str);
	// printf("%s\n", result_h.fingerprint_str);

	BOOST_CHECK_EQUAL(result_a.fingerprint_str, result_b.fingerprint_str);

	pg_query_free_fingerprint_result(result_a);
	pg_query_free_fingerprint_result(result_b);
	pg_query_free_fingerprint_result(result_c);
	pg_query_free_fingerprint_result(result_d);
	pg_query_free_fingerprint_result(result_e);
	pg_query_free_fingerprint_result(result_f);
	pg_query_free_fingerprint_result(result_g);
	pg_query_free_fingerprint_result(result_h);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for checking query syntax repair
																																							 *
																																							 *************************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(string_differences_test_suite)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Utils my_utils;
	// Tokenize sentences into words
	string query = "selext * from emp;";

	std::tuple<bool, string> corrected = my_utils.fix_query_syntax_using_keywords(query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "SELECT * from emp;");
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	Utils my_utils;
	// Tokenize sentences into words
	string query = "selext * fro emp;";

	std::tuple<bool, string> corrected = my_utils.fix_query_syntax_using_keywords(query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "SELECT * FROM emp;");
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	Utils my_utils;
	// Tokenize sentences into words
	string query = "dselext empno from emp;";

	std::tuple<bool, string> corrected = my_utils.fix_query_syntax_using_keywords(query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "SELECT empno from emp;");
}
/**
 * In this test case, the student query has both syntax error and semantic error.
 * syntax error: extra characters d and n before the select clause.
 * semantic error: extra column job3 in the select clause. Missing column sal
 * We test fixing only the syntax error.
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	Utils my_utils;
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::RE_SM_SY, 0);

	// Tokenize sentences into words
	string query = "d n select empno, ename,job3,mgr,hiredate,comm,deptno from emp;";
	string query_model = "select empno, ename,job,sal,mgr,hiredate,comm,deptno from emp;";

	std::tuple<bool, string, int> corrected = my_utils.fix_query_syntax_using_another_query(admin, query, query_model, query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "select empno, ename,job3,mgr,hiredate,comm,deptno from emp;");

	/*PgQueryScanResult result;
	PgQuery__ScanResult *scan_result;
	PgQuery__ScanToken *scan_token;
	const ProtobufCEnumValue *token_kind;
	const ProtobufCEnumValue *keyword_kind;

	result = pg_query_scan(query_model.c_str());
	scan_result = pg_query__scan_result__unpack(nullptr, result.pbuf.len, reinterpret_cast<const uint8_t*>(result.pbuf.data));

	printf("  version: %d, tokens: %ld, size: %d\n", scan_result->version, scan_result->n_tokens, result.pbuf.len);
	for (size_t j = 0; j < scan_result->n_tokens; j++) {
		scan_token = scan_result->tokens[j];
		token_kind = protobuf_c_enum_descriptor_get_value(&pg_query__token__descriptor, scan_token->token);
		keyword_kind = protobuf_c_enum_descriptor_get_value(&pg_query__keyword_kind__descriptor, scan_token->keyword_kind);
		printf("  \"%.*s\" = [ %d, %d, %s, %s ]\n", scan_token->end - scan_token->start, &(query_model.c_str()[scan_token->start]), scan_token->start, scan_token->end, token_kind->name, keyword_kind->name);
	}

	pg_query__scan_result__free_unpacked(scan_result, nullptr);
	pg_query_free_scan_result(result);*/
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	Utils my_utils;
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	// Tokenize sentences into words
	string query = "selext empno, ename,job3,mgr,hiredate,comm,deptno from emp;";
	string query_model = "select empno,ename,job,sal,mgr,hiredate,comm,deptno from emp;";

	std::tuple<bool, string, int> corrected = my_utils.fix_query_syntax_using_another_query(admin, query, query_model, query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "select empno,ename,job3,mgr,hiredate,comm,deptno from emp;");
}
BOOST_AUTO_TEST_CASE(test_case_6)
{
	Utils my_utils;
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	// Tokenize sentences into words
	string model_query = "SELECT email FROM Person WHERE person_name = 'John Doe';";
	string student_query = "SELECT email FROM Person WHERE person_name = ´John Doe´;";

	std::tuple<bool, string, int> corrected = my_utils.fix_query_syntax_using_another_query(admin, student_query, model_query, student_query);

	BOOST_CHECK_EQUAL(std::get<0>(corrected), true);
	BOOST_CHECK_EQUAL(std::get<1>(corrected), "SELECT email FROM Person WHERE person_name = 'John Doe';");
}
BOOST_AUTO_TEST_SUITE_END()

/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for checking grading of queries
																																							 * In this test suit we use tables used in the book sql cookbook by Anthony Molinaro.
																																							 *************************************************************************************************************************************************************/
/**
 * This grading suite is used to test binary grading of queries.
 */
BOOST_AUTO_TEST_SUITE(grading_test_suite_2)
/**
 * Simple select all query
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features

	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::ABSENT, pl::ABSENT, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select * from emp;");

	student_queries.push_back(student_query);

	// process the queries

	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 2);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
/**
 * select all the columns from a table
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::ABSENT, pl::ABSENT, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empno, ename,job,sal,mgr,hiredate,comm,deptno from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 2);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
/**
 * select the wrong column
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::ABSENT, pl::ABSENT, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empno from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 1);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select ename, job from emp order by ename;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select job,ename from emp order by ename;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
BOOST_AUTO_TEST_SUITE_END()
/**
 * This grading suite is used to test 8 magnitude grading of queries.
 */
BOOST_AUTO_TEST_SUITE(grading_test_suite_8)
/**
 * Simple select query
 * correctness level = 1
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selexta ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0);
}
/**
 * Simple select query
 * In this query, the student query has a simple mistake in both syntax and semantics.
 * correctness level = 2
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selext empn, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 2);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.2);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.2);
}
/**
 * Simple select query
 * In this query, the student query has a mistake in semantics.
 * correctness level = 3
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 3);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.4);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.4);
}
/**
 * Simple select query
 * In this query, the student query has a mistake in semantics.
 * correctness level = 4
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empn, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 4);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.6);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.6);
}
/**
 * Simple select query
 * In this query, the student query has a simple mistake in both syntax and semantics.
 * correctness level = 5
 */
BOOST_AUTO_TEST_CASE(test_case_5)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selext empno, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 5);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.8);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.8);
}
/**
 * Simple select query
 * In this query, the student query is the same as as the model query.
 * correctness level = 6
 */
BOOST_AUTO_TEST_CASE(test_case_6)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::TWO_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());
	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empno, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 6);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
BOOST_AUTO_TEST_CASE(test_case_7)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::TWO_LEVELS, pl::TWO_LEVELS, pl::ABSENT, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 2);
	// BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.4);
	// BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.4);
}
BOOST_AUTO_TEST_CASE(test_case_8)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::ABSENT, pl::TWO_LEVELS, pl::ABSENT, po::RE_SM_SY, 0);
	Grader grader;

	vector<MyDuckDB> duck_dbs;
	MyDuckDB duck_db(sql_file);
	duck_dbs.push_back(duck_db);
	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empno, name from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INVALID);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 1);
	// BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.4);
	// BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.4);
}
BOOST_AUTO_TEST_SUITE_END()

/**
 * This grading suite is used to test 27 magnitude grading of queries.
 */
BOOST_AUTO_TEST_SUITE(grading_test_suite_27)
/**
 * Simple select query
 * correctness level = 1
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "d n selexta ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), false);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0);
}
/**
 * Simple select query
 * correctness level = 2
 * syntax: minor incorrect: mispelled select
 * semantics: incorrect: missing column empno
 * results: incorrect
 *
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selexta ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 2);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.125);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.125);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select ename from emp join dept on emp.deptno = dept.deptno where dept.loc = 'NEW YORK';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selext ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 3);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.25);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.25);
}
/**
 * Simple select query
 * In this query, the student query has a mistake in semantics.
 * correctness level = 4
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::SM_SY_RE, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 4);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.375);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.375);
}
/**
 * Simple select query
 * In this query, the student query has a mistake in semantics.
 * correctness level = 5
 */
BOOST_AUTO_TEST_CASE(test_case_5)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select ename from emp join dept on emp.deptno = dept.deptno where dept.loc = 'NEW YORK';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 5);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.5);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.5);
}
/**
 * Simple select query
 * In this query, the student query has a simple mistake in both syntax and semantics.
 * correctness level = 6
 */
BOOST_AUTO_TEST_CASE(test_case_6)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selext empn, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 6);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.625);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.625);
}
/**
 * Simple select query
 * In this query, the student query has a mistake in semantics.
 * correctness level = 7
 */
BOOST_AUTO_TEST_CASE(test_case_7)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empn, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].is_parseable(), true);
	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 7);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.75);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.75);
}
/**
 * Simple select query
 * In this query, the student query has a simple mistake in both syntax and semantics.
 * correctness level = 8
 */
BOOST_AUTO_TEST_CASE(test_case_8)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "selext empno, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 8);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 0.875);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 0.875);
}
/**
 * Simple select query
 * In this query, the student query is the same as as the model query.
 * correctness level = 27
 */
BOOST_AUTO_TEST_CASE(test_case_9)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());
	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select empno, ename from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select empno, ename from emp;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
/**
 * Simple select query
 * In this query, the student query is the same as as the model query.
 * correctness level = 27
 */
BOOST_AUTO_TEST_CASE(test_case_10)
{
	string sql_file = "../samples/employees.sql";
	// initialize administrative features

	int text_edit_distance = 2;
	int tree_edit_distance = 1;
	int check_order = 0;

	Admin admin;
	using pl = Grader::property_level;
	using po = Grader::property_order;
	admin.init(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::SM_SY_RE, check_order, text_edit_distance, tree_edit_distance);
	Grader grader;

	// grader.display_correctness_matrix(admin.get_num_of_syntax_outcomes(), admin.get_num_of_semantics_outcomes(), admin.get_num_of_results_outcomes(), admin.get_order_of_importance());
	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "SELECT name, hire_date FROM Employee WHERE salary BETWEEN 2500.00 AND 3500.00;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query_1("1", "SELECT name, hire_date FROM Employee WHERE salary >= 2500 AND <= 3500;");

	StudentQuery student_query_2("2", "select name, hire_date from employee where salary >= 2500 and salary <= 2500;");

	StudentQuery student_query_3("3", "SELECT name, hire_date FROM Employee WHERE salary >= 2500 AND salary <= 3500;");

	StudentQuery student_query_4("4", "SELECT name, hire_date FROM Employee WHERE salary >= 2500 AND salary <= 2000;");

	StudentQuery student_query_5("5", "SELECT name, hire_date FROM Employee WHERE salary >= 2500 AND sfghry <= 3500;");
	student_queries.push_back(student_query_1);
	student_queries.push_back(student_query_2);
	student_queries.push_back(student_query_3);
	student_queries.push_back(student_query_4);
	student_queries.push_back(student_query_5);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::INCORRECT);

	BOOST_CHECK_EQUAL(student_queries[1].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[1].get_semantics_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[1].get_results_outcome(), Grader::property_state::CORRECT);

	BOOST_CHECK_EQUAL(student_queries[2].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[2].get_semantics_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[2].get_results_outcome(), Grader::property_state::CORRECT);

	BOOST_CHECK_EQUAL(student_queries[3].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[3].get_semantics_outcome(), Grader::property_state::MINOR_INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[3].get_results_outcome(), Grader::property_state::CORRECT);

	BOOST_CHECK_EQUAL(student_queries[4].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[4].get_semantics_outcome(), Grader::property_state::INCORRECT);
	BOOST_CHECK_EQUAL(student_queries[4].get_results_outcome(), Grader::property_state::INCORRECT);
}
BOOST_AUTO_TEST_SUITE_END()
/**
 * This grading suite is used to test 36 magnitude grading of queries.
 */
BOOST_AUTO_TEST_SUITE(grading_test_suite_36)
/**
 * Simple select query
 * correctness level = 1
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/discography.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::SEMATICS_LEVELS_6, pl::THREE_LEVELS, po::SM_RE_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = 'Nevermind';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "SELECT song FROM Album A, Track T WHERE A.title = 'Nevermind' AND A.album_id = T.album_id;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_syntax_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_semantics_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_results_outcome(), Grader::property_state::CORRECT);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 36);
	BOOST_CHECK_EQUAL(student_queries[0].get_normalized_value(), 1);
	BOOST_CHECK_EQUAL(student_queries[0].get_grade(), 1);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for checking grading off random queries
																																							 * This queries were found to have problems during grading for IMDB 2023 class.
																																							 *
																																							 *************************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(query_test_2023_students_queries)
/**
 * recursive query
 * correctness level = 1
 * checking if a recusrsive query can be run
 * FOR NOW WE REMOVE THIS TESTING UNTIL WE FIND A WAY TO MAKE THE QUERIES RUN SMOOTHLY
 */
/*BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/genealogy2021-10-19.sql";
	//initialize administrative features
	int num_syntax_outcomes = 3;
	int num_semantics_outcomes = 3;
	int num_results_outcomes = 3;
	int order_of_properties = 2; //semantics most important.
	int check_order = 0;

	Admin admin(num_syntax_outcomes, num_semantics_outcomes, num_results_outcomes, order_of_properties, check_order);
	Grader grader;

	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor ) SELECT * FROM Ancestor;");
	model_queries.push_back(model_query);

	StudentQuery student_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor ) SELECT * FROM Ancestor;");
	student_queries.push_back(student_query);

	//process the queries
	ProcessQueries process_queries( model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	string sql_file = "../samples/genealogy2021-10-19.sql";
	//initialize administrative features
	int num_syntax_outcomes = 3;
	int num_semantics_outcomes = 3;
	int num_results_outcomes = 3;
	int order_of_properties = 2; //semantics most important.
	int check_order = 0;

	Admin admin(num_syntax_outcomes, num_semantics_outcomes, num_results_outcomes, order_of_properties, check_order);
	Grader grader;

	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor) SELECT S2.* FROM Scientist S1, Scientist S2, Ancestor A WHERE S1.id = A.advisor AND S2.id = A.student AND S1.name = 'Desiderius Erasmus';");
	model_queries.push_back(model_query);

	StudentQuery student_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor) SELECT S2.* FROM Scientist S1, Scientist S2, Ancestor A WHERE S1.id = A.advisor AND S2.id = A.student AND S1.name = 'Desiderius Erasmus';");
	student_queries.push_back(student_query);

	//process the queries
	ProcessQueries process_queries( model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	string sql_file = "../samples/genealogy2021-10-19.sql";
	//initialize administrative features
	int num_syntax_outcomes = 3;
	int num_semantics_outcomes = 3;
	int num_results_outcomes = 3;
	int order_of_properties = 2; //semantics most important.
	int check_order = 0;

	Admin admin(num_syntax_outcomes, num_semantics_outcomes, num_results_outcomes, order_of_properties, check_order);
	Grader grader;

	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor) SELECT S2.* FROM Scientist S1, Scientist S2, Ancestor A WHERE S1.id = A.advisor AND S2.id = A.student AND S1.name = 'Desiderius Erasmus';");
	model_queries.push_back(model_query);

	StudentQuery student_query("1", "WITH RECURSIVE Ancestor(advisor, student) AS (SELECT advisor, student from Teaches UNION SELECT T.advisor, A.student FROM Teaches T, Ancestor A WHERE T.student = A.advisor) SELECT S2.* FROM Scientist S1, Scientist S2, Ancestor A WHERE S1.id = A.advisor AND S2.id = A.student AND S1.name = 'Desiderius Erasmus';");
	student_queries.push_back(student_query);
	StudentQuery student_query_2("2", "WITH RECURSIVE Descendants(id, name, university, year) AS (     SELECT id, name, university, year     FROM Scientist     WHERE name = 'Desiderius Erasmus'       UNION ALL       SELECT s.id, s.name, s.university, s.year     FROM Scientist s, Descendants d     WHERE s.id = d.id )   SELECT id, name, university, year FROM Descendants; ");
	student_queries.push_back(student_query_2);

	//process the queries
	ProcessQueries process_queries( model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 1);
}*/
BOOST_AUTO_TEST_SUITE_END()

/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for making sure that one query execution does not affect the other
																																							 * **********************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(queries_that_alter_data)
/**
 * We check that the data does not change even if we execute a query that inserts data
 * correctness level = 9
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select * from emp;");

	student_queries.push_back(student_query);
	// a query that inserts data
	StudentQuery student_query_2("2", "INSERT INTO emp VALUES (7935, 'NAME1', 'NAME2', 7783, '1982-04-23', 1400.00, NULL, 11);");

	student_queries.push_back(student_query_2);

	// verify that the data is not present in the rest of the queries
	StudentQuery student_query_3("3", "select * from emp;");

	student_queries.push_back(student_query_3);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[2].get_correctness_level(), 9);
}
/**
 * We check that the data does not change even if we execute a query that deletes data
 * correctness level = 9
 */
BOOST_AUTO_TEST_CASE(test_case_2)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select * from emp;");

	student_queries.push_back(student_query);
	// a query that inserts data
	StudentQuery student_query_2("2", "DELETE FROM emp WHERE empno = 7934;");

	student_queries.push_back(student_query_2);

	// verify that the data is not present in the rest of the queries
	StudentQuery student_query_3("3", "select * from emp;");

	student_queries.push_back(student_query_3);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[2].get_correctness_level(), 9);
}
/**
 * We check that the data does not change even if we execute a query that updates data
 * correctness level = 9
 */
BOOST_AUTO_TEST_CASE(test_case_3)
{
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select * from emp;");

	student_queries.push_back(student_query);
	// a query that inserts data
	StudentQuery student_query_2("2", "UPDATE emp SET ename = 'NAME1' WHERE empno = 7934;");

	student_queries.push_back(student_query_2);

	// verify that the data is not present in the rest of the queries
	StudentQuery student_query_3("3", "select * from emp;");

	student_queries.push_back(student_query_3);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 9);
	BOOST_CHECK_EQUAL(student_queries[2].get_correctness_level(), 9);
}
/**
 * query with empty data
 * correctness level = 9
 */
BOOST_AUTO_TEST_CASE(test_case_4)
{
	// NB: when the query does not select any data, the empty results are not the same for both the model and student queries even if they have the same query.
	string sql_file = "../samples/test_tables.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	Admin admin(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "select * from emp where empno = -1;");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "", false};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "select * from emp where empno = -1;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 7);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for modyfying query parse trees
																																							 * **********************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(generating_queries)
/**
 * We check that the value of a query is the same after deparsing when we dont change anything
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{ // NB: when the query does not select any data, the empty results are not the same for both the model and student queries even if they have the same query.

	// queries
	std::string query_a = "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = 'Nevermind';";
	// std::string query_b = "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = Â´NevermindÂ´;";
	std::string query_b = "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = 'Neverminds';";

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	AbstractSyntaxTree ast;
	// print_tree(tree_a);
	// ast.print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 1);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{ // NB: when the query does not select any data, the empty results are not the same for both the model and student queries even if they have the same query.

	// queries
	std::string query_a = "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = 'Nevermind';";
	std::string query_b = "SELECT song FROM Album A, Track T WHERE A.album_id = T.album_id AND A.title = Â´NevermindÂ´;";
	// std::cout << "Query 1: " << query_a << std::endl;
	// std::cout << "Query 2: " << query_b << std::endl;

	// Build the parse trees for the SQL queries
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_a = build_sql_tree(query_a);
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> tree_b = build_sql_tree(query_b);

	TreeEditDistance tree_edit_distance;

	int edit_distance = tree_edit_distance.zhang_shasha(tree_a, tree_b);
	AbstractSyntaxTree ast;
	// ast.print_tree(tree_a);
	// ast.print_tree(tree_b);

	BOOST_CHECK_EQUAL(edit_distance, 4);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	/**
	 * The edit distance between these two queries is 2. In SOCOLES though, the edit distance is calculated as 4.
	 * In UTF-8, characters outside the ASCII range can be represented using multiple bytes.
	 * In the second query, two bytes are used to store character ´ therefore we get this difference in edit distance calculation.
	 */

	// queries
	std::string query_a = "SELECT email FROM Person WHERE person_name = 'John Doe';";
	std::string query_b = "SELECT email FROM Person WHERE person_name = ´John Doe´;";
	Utils my_utils;

	std::string::size_type expected = 4;
	std::string::size_type result = my_utils.general_edit_distance(query_a, query_b);

	BOOST_TEST(result == expected);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	// NB: when the query does not select any data, the empty results are not the same for both the model and student queries even if they have the same query.
	string sql_file = "../samples/journal.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;

	int text_edit_distance = 10;
	int tree_edit_distance = 10;

	Admin admin;
	admin.init(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0, text_edit_distance, tree_edit_distance);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "SELECT email FROM Person WHERE person_name = 'John Doe';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "grader", false, true};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "SELECT email FROM Person WHERE person_name = ´John Doe´;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 8);
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	// NB: when the query does not select any data, the empty results are not the same for both the model and student queries even if they have the same query.
	string sql_file = "../samples/journal.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	int text_edit_distance = 10;
	int tree_edit_distance = 10;

	Admin admin;
	admin.init(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::RE_SM_SY, 0, text_edit_distance, tree_edit_distance);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "SELECT email FROM Person WHERE person_name = 'John Doe';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "grader", false, true};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "SELECT email  FROM Person  WHERE person_name = ´John Doe´;");

	student_queries.push_back(student_query);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 8);
}
/**
 * When the query has a simple mistake in syntax but,
 * the closest query to it is another student query that is wrong
 */
BOOST_AUTO_TEST_CASE(test_case_6)
{

	string sql_file = "../samples/journal.sql";
	// initialize administrative features
	using pl = Grader::property_level;
	using po = Grader::property_order;
	int text_edit_distance = 10;
	int tree_edit_distance = 10;

	Admin admin;
	admin.init(pl::THREE_LEVELS, pl::THREE_LEVELS, pl::THREE_LEVELS, po::SM_SY_RE, 0, text_edit_distance, tree_edit_distance);
	Grader grader;

	// initialize the queries
	vector<ModelQuery> model_queries;
	vector<StudentQuery> student_queries;

	ModelQuery model_query("1", "SELECT email FROM Person WHERE person_name = 'John Doe';");
	Admin::database_options db_opts = {sql_file, 0, 0, "", "", "grader", false, true};

	model_queries.push_back(model_query);

	StudentQuery student_query("1", "SELECT Person.email  FROM Person  WHERE person_name = ´John Doe´;");
	StudentQuery student_query_2("2", "SELECT email  FROM Person  WHERE person_name = 'John Doe';");
	StudentQuery student_query_3("3", "SELECT Person.email  FROM Person  WHERE person_name = 'Jo Doe';");
	StudentQuery student_query_4("4", "SELECT Person.email  FROM Person  WHERE person_name = 'John Doe';");

	student_queries.push_back(student_query);
	student_queries.push_back(student_query_2);
	student_queries.push_back(student_query_3);
	student_queries.push_back(student_query_4);

	// process the queries
	ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

	BOOST_CHECK_EQUAL(student_queries[0].get_correctness_level(), 8);
	BOOST_CHECK_EQUAL(student_queries[3].get_correctness_level(), 9);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for duckdb
																																							 * **********************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(duckdb)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	Utils my_utils;
	std::string creation_queries = "CREATE TABLE Album(album_id INT,  title TEXT,  PRIMARY KEY(album_id));"
								   "CREATE TABLE Track(  track_id INT,   album_id INT,  song TEXT,  duration INT,  PRIMARY KEY(track_id),  FOREIGN KEY(album_id) REFERENCES Album(album_id));"
								   "CREATE TABLE Artist(  artist_id INT,   name TEXT,   birth_date DATE,  death_date DATE,  PRIMARY KEY(artist_id));"
								   "CREATE TABLE Performs(  artist_id INT,  track_id INT,  role TEXT,  PRIMARY KEY(artist_id, track_id, role),  FOREIGN KEY(artist_id) REFERENCES Artist(artist_id),  FOREIGN KEY(track_id) REFERENCES Track(track_id));"
								   "CREATE TABLE Release(  album_id INT,  country TEXT,  release_date DATE,  PRIMARY KEY (album_id, country),  FOREIGN KEY(album_id) REFERENCES Album(album_id));";
	// execute the queries
	std::string error1, error2, error3;

	// insert query
	std::string creation_queries_1 = creation_queries + "INSERT INTO Artist VALUES (1, 'John Lennon', '1940-10-09', '1980-12-08');";
	std::string creation_queries_2 = creation_queries + "INSERT INTO Artist VALUES (2, 'Paul McCartney', '1942-06-18', NULL);";
	std::string creation_queries_3 = creation_queries + "INSERT INTO Artist VALUES (1, 'John Lennon', '1940-10-09', '1980-12-08');";

	Admin::database_options db_opts_1 = {creation_queries_1, 0, 0, "", "", "", true};
	Admin::database_options db_opts_2 = {creation_queries_2, 0, 0, "", "", "", true};
	Admin::database_options db_opts_3 = {creation_queries_3, 0, 0, "", "", "", true};

	Query_Engine qe_1(db_opts_1);
	qe_1.initialize();
	Query_Engine qe_2(db_opts_2);
	qe_2.initialize();
	Query_Engine qe_3(db_opts_3);
	qe_3.initialize();

	// select queries
	ModelQuery model_query_1("1", "SELECT * FROM Artist;");

	ModelQuery model_query_2("1", "SELECT * FROM Artist;");

	ModelQuery model_query_3("1", "SELECT * FROM Artist;");

	// execute the queries
	ProcessQueries::pre_process_model_query(&model_query_1, qe_1);
	ProcessQueries::pre_process_model_query(&model_query_2, qe_2);
	ProcessQueries::pre_process_model_query(&model_query_3, qe_3);

	// compare the results
	Utils::comparison_result expected_result_1{true, true, 0}, expected_result_2{false, false, 8};
	Utils::comparison_result result = my_utils.compare_vectors(model_query_1.get_output(), model_query_3.get_output(), true);
	Utils::comparison_result result_2 = my_utils.compare_vectors(model_query_1.get_output(), model_query_2.get_output(), true);

	BOOST_TEST(result == expected_result_1);
	BOOST_TEST(result_2 == expected_result_2);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	Utils my_utils;
	std::string creation_queries = "CREATE TABLE Album(album_id INT,  title TEXT,  PRIMARY KEY(album_id));"
								   "CREATE TABLE Track(  track_id INT,   album_id INT,  song TEXT,  duration INT,  PRIMARY KEY(track_id),  FOREIGN KEY(album_id) REFERENCES Album(album_id));"
								   "CREATE TABLE Artist(  artist_id INT,   name TEXT,   birth_date DATE,  death_date DATE,  PRIMARY KEY(artist_id));"
								   "CREATE TABLE Performs(  artist_id INT,  track_id INT,  role TEXT,  PRIMARY KEY(artist_id, track_id, role),  FOREIGN KEY(artist_id) REFERENCES Artist(artist_id),  FOREIGN KEY(track_id) REFERENCES Track(track_id));"
								   "CREATE TABLE Release(  album_id INT,  country TEXT,  release_date DATE,  PRIMARY KEY (album_id, country),  FOREIGN KEY(album_id) REFERENCES Album(album_id));"
								   "INSERT INTO Artist VALUES (1, 'John Lennon', '1940-10-09', '1980-12-08');"
								   "INSERT INTO Artist VALUES (2, 'Paul McCartney', '1942-06-18', NULL);"
								   "INSERT INTO Artist VALUES (3, 'John Lennon', '1940-10-09', '1980-12-08');";
	// execute the queries
	std::string error1;

	// select queries
	ModelQuery model_query_1("1", "SELECT * FROM Artist;");
	Admin::database_options db_opts = {creation_queries, 0, 0, "", "", "", true, false};
	Query_Engine qe(db_opts);
	qe.initialize();

	ProcessQueries::pre_process_model_query(&model_query_1, qe);
	auto output = model_query_1.get_output();

	// compare the results
	BOOST_TEST(output.size() == 3);
	qe.clear();
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for generating test table data when given a refernce query
																																							 * **********************************************************************************************************************************************************/
BOOST_AUTO_TEST_SUITE(generating_test_data_1)
/*
 *
 */
BOOST_AUTO_TEST_CASE(test_case_1)
{
	MyEvoSQL my_evo_sql;

	std::string captured_fiexture = "* 13:09:55.655 [main] INFO  nl.tudelft.serg.evosql.EvoSQL - Generated fixture: [INSERT INTO \"performs\" VALUES ('1','856','%I5DjVn}[`3o!_)Uh$2NVO,3LY!H;UWFLDT#'), ('1','865','C!./1&8QQOX35< _KnLTCf#'), INSERT INTO \"artist\" VALUES ('2','nF6k3JD$WT~h8[v@I9O+xVSd9 m]l5xe9%_','2012-07-11','2020-09-23'), INSERT INTO \"track\" VALUES ('865','365','VMoa$=/-ELM6suV#[Gh*1S(K01sFLvsKR)','690'), ('856','1','P4qMYCWRt;]E5)e9Jj8Z|0*DSU;:=j?Y\"[0E!HP5(s0Gumr*f','3'), ('856','996','\"c~[?gr6S','1')]";
	std::vector<std::string> insert_statements = my_evo_sql.extract_insert_statements(captured_fiexture);
	std::vector<std::string> expected_insert_statements = {"INSERT INTO performs VALUES ('1','856','%I5DjVn}[`3o!_)Uh$2NVO,3LY!H;UWFLDT#'), ('1','865','C!./1&8QQOX35< _KnLTCf#')",
														   "INSERT INTO artist VALUES ('2','nF6k3JD$WT~h8[v@I9O+xVSd9 m]l5xe9%_','2012-07-11','2020-09-23')",
														   "INSERT INTO track VALUES ('865','365','VMoa$=/-ELM6suV#[Gh*1S(K01sFLvsKR)','690'), ('856','1','P4qMYCWRt;]E5)e9Jj8Z|0*DSU;:=j?Y\"[0E!HP5(s0Gumr*f','3'), ('856','996','\"c~[?gr6S','1')"};
	BOOST_CHECK_EQUAL(insert_statements[0], expected_insert_statements[0]);
	BOOST_CHECK_EQUAL(insert_statements[1], expected_insert_statements[1]);
	BOOST_CHECK_EQUAL(insert_statements[2], expected_insert_statements[2]);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	MyEvoSQL my_evo_sql;
	// my_evo_sql.init("discography_create.sql","test.sql","discography",20);
}
BOOST_AUTO_TEST_SUITE_END()
/*********************************************************************************************************************************************************/ /**
																																							 * Test suite for feedback generation
																																							 * **********************************************************************************************************************************************************/
/*namespace boost
{
	namespace test_tools
	{
		namespace tt_detail
		{
			// Output for Goals::Goal
			template <>
			struct print_log_value<Goals::Goal>
			{
				void operator()(std::ostream& os, Goals::Goal const& goal)
				{
					os << "Goal{type: \"" << goal.type << "\", content: \"" << goal.content << "\"}";
				}
			};

		}
	}
}*/
std::ostream &operator<<(std::ostream &os, const Goals::Goal &goal)
{
	return os << "Goal{type: \"" << goal.type << "\", content: \"" << goal.content << "\"}";
}
// Overload the equality operator for Goal
bool operator==(const Goals::Goal &lhs, const Goals::Goal &rhs)
{
	return lhs.type == rhs.type && lhs.content == rhs.content;
}
BOOST_AUTO_TEST_SUITE(feedback)
BOOST_AUTO_TEST_CASE(test_case_1)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	ModelQuery model_query("1", "SELECT a.name FROM artist a;");
	ModelQuery model_query_2("2", "SELECT artist.name FROM artist;");

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(tree_m);
	// ast.print_tree(tree_m_2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	std::cout << "Test for feedback..." << std::endl;
	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(from_comparison.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_2)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;
	/**
	 * Even though the two statements are the same, we avoid saying that they are equal because of the ambiguos column problem.
	 * In a grading setting, this can be solved by checking against a refernce query that doesn't use aliases.
	 */
	ModelQuery model_query("1", "SELECT name FROM artist;");
	ModelQuery model_query_2("2", "SELECT artist.name FROM artist;");

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(tree_m);
	// ast.print_tree(tree_m_2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	std::cout << "Test for feedback..." << std::endl;
	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(from_comparison.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 0);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_3)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;
	ModelQuery model_query("1", "SELECT name2 FROM artist;");
	ModelQuery model_query_2("2", "SELECT artist.name FROM artist;");

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(tree_m);
	// ast.print_tree(tree_m_2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	std::cout << "Test for feedback..." << std::endl;
	if (select_comparison.first != 0)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(from_comparison.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 0);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_4)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;
	ModelQuery model_query("1", "SELECT name as a FROM artist;");
	ModelQuery model_query_2("2", "SELECT name FROM artist;");

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);
	// ast.print_tree(tree_m_2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(from_comparison.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_5)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;
	ModelQuery model_query("1", "SELECT A.name, COUNT(P.tracks_id) AS least3 FROM Artist A JOIN Performs P ON A.artist_id = P.artist_id");
	ModelQuery model_query_2("2", "SELECT B.name, COUNT(T.tracks_id) AS least2 FROM Artist B JOIN Performs T ON B.artist_id = T.artist_id");
	std::cout << "Test for feedback..." << std::endl;

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);
	// ast.print_tree(tree_m_2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		std::cout << from_comparison.second << std::endl;
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_6)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	ModelQuery model_query("1", "SELECT COUNT(P.tracks_id) AS least3, A.name FROM Performs P JOIN Artist A ON A.artist_id = P.artist_id");
	ModelQuery model_query_2("2", "SELECT B.name, COUNT(T.tracks_id) AS least2 FROM Artist B JOIN Performs T ON B.artist_id = T.artist_id");
	std::cout << "Test for feedback..." << std::endl;

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);
	// ast.print_tree(tree_m_2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		std::cout << from_comparison.second << std::endl;
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}
/**
 * Test goals for select clause and from clause with join condition
 * The queries are the same but the order of the tables in the from clause is different
 */
BOOST_AUTO_TEST_CASE(test_case_7)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;
	ModelQuery model_query("1", "SELECT COUNT(P.tracks_id) AS least3, A.name FROM Performs P JOIN Artist A ON A.track_id = P.artist_id");
	ModelQuery model_query_2("2", "SELECT B.name, COUNT(T.tracks_id) AS least2 FROM Artist B JOIN Performs T ON B.artist_id = T.artist_id");
	std::cout << "Test for feedback..." << std::endl;

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	// goals.print_from_clause(from_goal.second);
	// goals.print_from_clause(from_goal_2.second);
	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		std::cout << from_comparison.second << std::endl;
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 0);
}
BOOST_AUTO_TEST_CASE(test_case_8)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	ModelQuery model_query("1", "select ename from emp e where e.deptno = 10;");
	ModelQuery model_query_2("2", "select ename from emp where emp.deptno = 10;");
	std::cout << "Test for feedback..." << std::endl;

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);
	// ast.print_tree(tree_m_2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);

	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<std::string, Where_clause::where_clause_info> where_goal = Where_clause::process(root_node1, from_goal.second, select_goal1.second);
	std::pair<std::string, Where_clause::where_clause_info> where_goal_2 = Where_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	std::pair<int, std::string> where_comparison = Where_clause::compare(where_goal.second, where_goal_2.second, next_steps);

	if (select_comparison.first != 1)
	{
		std::cout << select_comparison.second << std::endl;
	}
	if (from_comparison.first != 1)
	{
		std::cout << from_comparison.second << std::endl;
	}
	if (where_comparison.first != 1)
	{
		std::cout << where_comparison.second << std::endl;
		Where_clause::print(where_goal.second);
		Where_clause::print(where_goal_2.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(where_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_9)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with complex WHERE clause
	ModelQuery model_query("1", "SELECT * FROM employees WHERE (salary > 50000 AND department = 'Sales') OR (salary > 70000 AND department = 'Engineering');");
	// Query 2 with conditions in different order but logically equivalent
	ModelQuery model_query_2("2", "SELECT * FROM employees WHERE (department = 'Sales' AND salary > 50000) OR (department = 'Engineering' AND salary > 70000);");

	std::cout << "Test for complex WHERE clause comparison..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process WHERE clause
	std::pair<std::string, Where_clause::where_clause_info> where_goal = Where_clause::process(root_node1, from_goal.second, select_goal1.second);
	std::pair<std::string, Where_clause::where_clause_info> where_goal_2 = Where_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	std::pair<int, std::string> where_comparison = Where_clause::compare(where_goal.second, where_goal_2.second, next_steps);

	// Assert that clauses are equivalent
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(where_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_11)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with complex WHERE clause including NOT and nested conditions
	ModelQuery model_query("1", "SELECT * FROM products WHERE (category = 'Electronics' AND NOT (price > 1000 OR brand = 'BrandX')) OR (stock > 50 AND supplier = 'SupplierA');");
	// Query 2 with a slightly different WHERE clause
	ModelQuery model_query_2("2", "SELECT * FROM products WHERE (category = 'Electronics' AND NOT (price > 1000 AND brand = 'BrandX')) OR (stock > 50 AND supplier = 'SupplierA');");

	std::cout << "Test for complex WHERE clause with slight differences..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process WHERE clause
	std::pair<std::string, Where_clause::where_clause_info> where_goal = Where_clause::process(root_node1, from_goal.second, select_goal1.second);
	std::pair<std::string, Where_clause::where_clause_info> where_goal_2 = Where_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare WHERE clauses
	std::pair<int, std::string> where_comparison = Where_clause::compare(where_goal.second, where_goal_2.second, next_steps);

	// Assert that clauses are not equivalent
	BOOST_CHECK_EQUAL(where_comparison.first, 0);
}
BOOST_AUTO_TEST_CASE(test_case_12)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with complex SELECT, FROM, and WHERE clause including NOT and nested conditions
	ModelQuery model_query("1", "SELECT product_id, product_name FROM products WHERE (category = 'Electronics' AND NOT (price > 1000 OR brand = 'BrandX')) OR (stock > 50 AND supplier = 'SupplierA');");
	// Query 2 with a slightly different WHERE clause, but same SELECT and FROM clauses
	ModelQuery model_query_2("2", "SELECT product_id, product_name FROM products WHERE (category = 'Electronics' AND NOT (price > 1000 AND brand = 'BrandX')) OR (stock > 50 AND supplier = 'SupplierA');");

	std::cout << "Test for complex SELECT, FROM, and WHERE clause with slight differences..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process WHERE clause
	std::pair<std::string, Where_clause::where_clause_info> where_goal = Where_clause::process(root_node1, from_goal.second, select_goal1.second);
	std::pair<std::string, Where_clause::where_clause_info> where_goal_2 = Where_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	std::pair<int, std::string> where_comparison = Where_clause::compare(where_goal.second, where_goal_2.second, next_steps);

	// Assertions to check that SELECT and FROM clauses are equivalent, but WHERE clauses are not
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(where_comparison.first, 0); // Expected to be unequal due to the difference in the WHERE clause logic
}
BOOST_AUTO_TEST_CASE(test_case_13)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with HAVING clause
	ModelQuery model_query("1", "SELECT department, COUNT(*) FROM employees GROUP BY department HAVING COUNT(*) > 5 AND AVG(salary) > 60000;");
	// Query 2 with HAVING clause in different order but logically equivalent
	ModelQuery model_query_2("2", "SELECT department, COUNT(*) FROM employees GROUP BY department HAVING AVG(salary) > 60000 AND COUNT(*) > 5;");

	std::cout << "Test for HAVING clause comparison..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	auto root_node1 = model_query.get_parse_tree();
	auto root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	auto from_goal = From_clause::process(root_node1);
	auto from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	auto select_goal1 = Select_clause::process(root_node1, from_goal.second);
	auto select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process HAVING clause
	auto having_goal = Having_clause::process(root_node1, from_goal.second, select_goal1.second);
	auto having_goal_2 = Having_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	auto select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	auto from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	auto having_comparison = Having_clause::compare(having_goal.second, having_goal_2.second);

	// Assertions to check that clauses are equivalent
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(having_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_14)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with a complex HAVING clause using aggregate functions
	ModelQuery model_query("1", "SELECT department, SUM(salary) FROM employees GROUP BY department HAVING SUM(salary) > 500000 OR COUNT(*) > 50;");
	// Query 2 with a slightly different HAVING clause
	ModelQuery model_query_2("2", "SELECT department, SUM(salary) FROM employees GROUP BY department HAVING COUNT(*) > 50 OR SUM(salary) > 500000;");

	std::cout << "Test for HAVING clause with different conditions..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	auto root_node1 = model_query.get_parse_tree();
	auto root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	auto from_goal = From_clause::process(root_node1);
	auto from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	auto select_goal1 = Select_clause::process(root_node1, from_goal.second);
	auto select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process HAVING clause
	auto having_goal = Having_clause::process(root_node1, from_goal.second, select_goal1.second);
	auto having_goal_2 = Having_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	auto select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	auto from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	auto having_comparison = Having_clause::compare(having_goal.second, having_goal_2.second);

	// Assertions to check that SELECT and FROM clauses are equivalent, but HAVING clauses are not
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(having_comparison.first, 1); // Expected to be equivalent
}
BOOST_AUTO_TEST_CASE(test_case_15)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with HAVING clause and aliases
	ModelQuery model_query("1", "SELECT d.name AS department_name, SUM(e.salary) AS total_salary FROM departments d JOIN employees e ON d.id = e.department_id GROUP BY d.name HAVING total_salary > 500000;");
	// Query 2 with the same logic but different alias names
	ModelQuery model_query_2("2", "SELECT d.name AS dept_name, SUM(e.salary) AS sum_salary FROM departments d JOIN employees e ON d.id = e.department_id GROUP BY d.name HAVING sum_salary > 500000;");

	std::cout << "Test for HAVING clause with aliases..." << std::endl;
	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	auto root_node1 = model_query.get_parse_tree();
	auto root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);

	// Process FROM clause
	auto from_goal = From_clause::process(root_node1);
	auto from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	auto select_goal1 = Select_clause::process(root_node1, from_goal.second);
	auto select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process HAVING clause
	auto having_goal = Having_clause::process(root_node1, from_goal.second, select_goal1.second);
	auto having_goal_2 = Having_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	auto select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	auto from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	auto having_comparison = Having_clause::compare(having_goal.second, having_goal_2.second);

	// Assertions to check that SELECT and FROM clauses are equivalent, and HAVING clauses are equivalent despite aliases
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(having_comparison.first, 1);
}
BOOST_AUTO_TEST_CASE(test_case_16)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with a comprehensive HAVING clause
	ModelQuery model_query("1",
						   "SELECT department_id, COUNT(employee_id) AS num_employees, SUM(salary) AS total_salary "
						   "FROM employees "
						   "GROUP BY department_id "
						   "HAVING AVG(salary) > 60000 AND MAX(salary) < 120000;");

	// Query 2 with a slightly different HAVING clause
	ModelQuery model_query_2("2",
							 "SELECT department_id, COUNT(employee_id) AS num_employees, SUM(salary) AS total_salary "
							 "FROM employees "
							 "GROUP BY department_id "
							 "HAVING AVG(salary) >= 60000 AND MAX(salary) <= 120000;");

	std::cout << "Test for comprehensive HAVING clause with slight differences..." << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	auto root_node1 = model_query.get_parse_tree();
	auto root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	auto from_goal = From_clause::process(root_node1);
	auto from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	auto select_goal1 = Select_clause::process(root_node1, from_goal.second);
	auto select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process HAVING clause
	auto having_goal = Having_clause::process(root_node1, from_goal.second, select_goal1.second);
	auto having_goal_2 = Having_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	auto select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	auto from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	auto having_comparison = Having_clause::compare(having_goal.second, having_goal_2.second);

	// Assertions to check that SELECT and FROM clauses are equivalent, but HAVING clauses are not
	BOOST_CHECK_EQUAL(select_comparison.first, 1);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(having_comparison.first, 0); // Expected to be not equivalent due to slight differences
}
BOOST_AUTO_TEST_CASE(test_case_17)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	// Query 1 with a comprehensive HAVING clause
	ModelQuery model_query("1",
						   "SELECT song FROM Album A, Track T WHERE A.album_id=T.album_id AND A.title='Nevermind';");

	// Query 2 with a slightly different HAVING clause
	ModelQuery model_query_2("2",
							 "SELECT song FROM Track WHERE album_id = ANY (SELECT album_id FROM Album WHERE title='Nevermind');");

	std::cout << "Test for something" << std::endl;

	// Create abstract syntax trees for both queries
	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	auto root_node1 = model_query.get_parse_tree();
	auto root_node2 = model_query_2.get_parse_tree();

	// Process FROM clause
	auto from_goal = From_clause::process(root_node1);
	auto from_goal_2 = From_clause::process(root_node2);

	// Process SELECT clause
	auto select_goal1 = Select_clause::process(root_node1, from_goal.second);
	auto select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	// Process HAVING clause
	auto having_goal = Having_clause::process(root_node1, from_goal.second, select_goal1.second);
	auto having_goal_2 = Having_clause::process(root_node2, from_goal_2.second, select_goal2.second);

	// Compare clauses
	auto select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	auto from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);
	auto having_comparison = Having_clause::compare(having_goal.second, having_goal_2.second);

	// Print comparison messages if clauses are not equivalent

	// Assertions to check that SELECT and FROM clauses are equivalent, but HAVING clauses are not
	BOOST_CHECK_EQUAL(select_comparison.first, 0);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
	BOOST_CHECK_EQUAL(having_comparison.first, -1); // Expected to be not equivalent due to slight differences
}
BOOST_AUTO_TEST_CASE(test_case_18)
{
	AbstractSyntaxTree ast;
	Goals goals;

	// Original Query 1: Simple GROUP BY clause with two columns
	ModelQuery model_query("1",
						   "SELECT m.department_id, m.job_id, COUNT(*) AS num_employ "
						   "FROM employees m "
						   "GROUP BY m.department_id, m.job_id;");

	// Query 1a: Similar to Query 1 but with table aliases
	ModelQuery model_query_1a("1a",
							  "SELECT e.department_id, e.job_id, COUNT(*) AS num_employees "
							  "FROM employees e "
							  "GROUP BY e.department_id, e.job_id;");

	// Query 1b: Slightly dissimilar GROUP BY clause with an expression
	ModelQuery model_query_1b("1b",
							  "SELECT department_id, job_id, COUNT(*) AS num_employees "
							  "FROM employees "
							  "GROUP BY department_id, UPPER(job_id);");

	// Process and compare queries for Query 1
	// ... [Processing code for Query 1, Query 1a, and Query 1b]

	// Similarly, create and process queries for Query 2, Query 3, and Query 4

	// Original Query 2: Same as Query 1 but with columns in different order
	ModelQuery model_query_2("2",
							 "SELECT m.job_id, m.department_id, COUNT(*) AS num_employees "
							 "FROM employees m "
							 "GROUP BY m.department_id, m.job_id;");

	// Query 2a: Similar to Query 2 but using table aliases
	ModelQuery model_query_2a("2a",
							  "SELECT e.job_id, e.department_id, COUNT(*) AS num_employees "
							  "FROM employees e "
							  "GROUP BY e.job_id, e.department_id;");

	// Query 2b: Slightly dissimilar GROUP BY clause
	ModelQuery model_query_2b("2b",
							  "SELECT job_id, department_id, COUNT(*) AS num_employees "
							  "FROM employees "
							  "GROUP BY job_id;"); // Missing department_id in GROUP BY

	// Process and compare queries for Query 2
	// ... [Processing code for Query 2, Query 2a, and Query 2b]

	// Original Query 3: GROUP BY clause with expressions
	ModelQuery model_query_3("3",
							 "SELECT TO_CHAR(hire_date, 'YYYY') AS hire_year, department_id, COUNT(*) "
							 "FROM employees "
							 "GROUP BY TO_CHAR(hire_date, 'YYYY'), department_id;");

	// Query 3a: Similar to Query 3 but using a different function with the same result
	ModelQuery model_query_3a("3a",
							  "SELECT EXTRACT(YEAR FROM hire_date) AS hire_year, department_id, COUNT(*) "
							  "FROM employees "
							  "GROUP BY EXTRACT(YEAR FROM hire_date), department_id;");

	// Query 3b: Slightly dissimilar GROUP BY clause
	ModelQuery model_query_3b("3b",
							  "SELECT TO_CHAR(hire_date, 'YYYY-MM') AS hire_month, department_id, COUNT(*) "
							  "FROM employees "
							  "GROUP BY TO_CHAR(hire_date, 'YYYY-MM'), department_id;");

	// Process and compare queries for Query 3
	// ... [Processing code for Query 3, Query 3a, and Query 3b]

	// Original Query 4: GROUP BY clause with one column
	ModelQuery model_query_4("4",
							 "SELECT department_id, COUNT(*) "
							 "FROM employees "
							 "GROUP BY department_id;");

	// Query 4a: Similar to Query 4 but with an explicit alias
	ModelQuery model_query_4a("4a",
							  "SELECT e.department_id AS dept_id, COUNT(*) "
							  "FROM employees e "
							  "GROUP BY department_id;");

	// Query 4b: Slightly dissimilar GROUP BY clause
	ModelQuery model_query_4b("4b",
							  "SELECT manager_id, COUNT(*) "
							  "FROM employees "
							  "GROUP BY manager_id;");

	// Process and compare queries for Query 4
	// ... [Processing code for Query 4, Query 4a, and Query 4b]

	// Now let's process each set of queries and perform comparisons

	// --- Processing for Query 1 ---

	// Create abstract syntax trees
	model_query.create_abstract_syntax_tree();
	model_query_1a.create_abstract_syntax_tree();
	model_query_1b.create_abstract_syntax_tree();

	auto root_node1 = model_query.get_parse_tree();
	auto root_node1a = model_query_1a.get_parse_tree();
	auto root_node1b = model_query_1b.get_parse_tree();

	// Process FROM clauses
	auto from_goal1 = From_clause::process(root_node1);
	auto from_goal1a = From_clause::process(root_node1a);
	auto from_goal1b = From_clause::process(root_node1b);

	// Process GROUP BY clauses
	auto group_by_result1 = Group_by_clause::process(root_node1, from_goal1.second);
	auto group_by_result1a = Group_by_clause::process(root_node1a, from_goal1a.second);
	auto group_by_result1b = Group_by_clause::process(root_node1b, from_goal1b.second);

	// Compare Query 1 with Query 1a
	auto comparison_result_1_1a = Group_by_clause::compare(group_by_result1.second, group_by_result1a.second);

	// Compare Query 1 with Query 1b
	auto comparison_result_1_1b = Group_by_clause::compare(group_by_result1.second, group_by_result1b.second);

	// Assertions for Query 1 comparisons
	BOOST_CHECK_EQUAL(comparison_result_1_1a.first, 1); // Query 1 and Query 1a should be equivalent
	BOOST_CHECK_EQUAL(comparison_result_1_1b.first, 0); // Query 1 and Query 1b should not be equivalent

	// --- Processing for Query 2 ---

	// Create abstract syntax trees
	model_query_2.create_abstract_syntax_tree();
	model_query_2a.create_abstract_syntax_tree();
	model_query_2b.create_abstract_syntax_tree();

	auto root_node2 = model_query_2.get_parse_tree();
	auto root_node2a = model_query_2a.get_parse_tree();
	auto root_node2b = model_query_2b.get_parse_tree();

	// Process FROM clauses
	auto from_goal2 = From_clause::process(root_node2);
	auto from_goal2a = From_clause::process(root_node2a);
	auto from_goal2b = From_clause::process(root_node2b);

	// Process GROUP BY clauses
	auto group_by_result2 = Group_by_clause::process(root_node2, from_goal2.second);
	auto group_by_result2a = Group_by_clause::process(root_node2a, from_goal2a.second);
	auto group_by_result2b = Group_by_clause::process(root_node2b, from_goal2b.second);

	// Compare Query 2 with Query 2a
	auto comparison_result_2_2a = Group_by_clause::compare(group_by_result2.second, group_by_result2a.second);

	// Compare Query 2 with Query 2b
	auto comparison_result_2_2b = Group_by_clause::compare(group_by_result2.second, group_by_result2b.second);

	// Assertions for Query 2 comparisons
	BOOST_CHECK_EQUAL(comparison_result_2_2a.first, 1); // Query 2 and Query 2a should not be equivalent because of the order of columns
	BOOST_CHECK_EQUAL(comparison_result_2_2b.first, 0); // Query 2 and Query 2b should not be equivalent

	// --- Processing for Query 3 ---

	// Create abstract syntax trees
	model_query_3.create_abstract_syntax_tree();
	model_query_3a.create_abstract_syntax_tree();
	model_query_3b.create_abstract_syntax_tree();

	auto root_node3 = model_query_3.get_parse_tree();
	auto root_node3a = model_query_3a.get_parse_tree();
	auto root_node3b = model_query_3b.get_parse_tree();
	// ast.print_tree(root_node3);

	// Process FROM clauses
	auto from_goal3 = From_clause::process(root_node3);
	auto from_goal3a = From_clause::process(root_node3a);
	auto from_goal3b = From_clause::process(root_node3b);

	// Process GROUP BY clauses
	auto group_by_result3 = Group_by_clause::process(root_node3, from_goal3.second);
	auto group_by_result3a = Group_by_clause::process(root_node3a, from_goal3a.second);
	auto group_by_result3b = Group_by_clause::process(root_node3b, from_goal3b.second);

	// Compare Query 3 with Query 3a
	auto comparison_result_3_3a = Group_by_clause::compare(group_by_result3.second, group_by_result3a.second);

	// Compare Query 3 with Query 3b
	auto comparison_result_3_3b = Group_by_clause::compare(group_by_result3.second, group_by_result3b.second);

	// Assertions for Query 3 comparisons
	BOOST_CHECK_EQUAL(comparison_result_3_3a.first, 0); // Query 3 and Query 3a should be equivalent semantically but in this setting they use different functions
	BOOST_CHECK_EQUAL(comparison_result_3_3b.first, 0); // Query 3 and Query 3b should not be equivalent

	// --- Processing for Query 4 ---

	// Create abstract syntax trees
	model_query_4.create_abstract_syntax_tree();
	model_query_4a.create_abstract_syntax_tree();
	model_query_4b.create_abstract_syntax_tree();

	auto root_node4 = model_query_4.get_parse_tree();
	auto root_node4a = model_query_4a.get_parse_tree();
	auto root_node4b = model_query_4b.get_parse_tree();

	// Process FROM clauses
	auto from_goal4 = From_clause::process(root_node4);
	auto from_goal4a = From_clause::process(root_node4a);
	auto from_goal4b = From_clause::process(root_node4b);

	// Process GROUP BY clauses
	auto group_by_result4 = Group_by_clause::process(root_node4, from_goal4.second);
	auto group_by_result4a = Group_by_clause::process(root_node4a, from_goal4a.second);
	auto group_by_result4b = Group_by_clause::process(root_node4b, from_goal4b.second);

	// Compare Query 4 with Query 4a
	auto comparison_result_4_4a = Group_by_clause::compare(group_by_result4.second, group_by_result4a.second);

	// Compare Query 4 with Query 4b
	auto comparison_result_4_4b = Group_by_clause::compare(group_by_result4.second, group_by_result4b.second);

	// Assertions for Query 4 comparisons
	BOOST_CHECK_EQUAL(comparison_result_4_4a.first, 1); // Query 4 and Query 4a should be equivalent
	BOOST_CHECK_EQUAL(comparison_result_4_4b.first, 0); // Query 4 and Query 4b should not be equivalent
}
BOOST_AUTO_TEST_CASE(test_case_19)
{
	// Initialize necessary objects
	AbstractSyntaxTree ast;
	Goals goals;

	// --- Query Set 1: Basic ORDER BY with columns ---

	// Original Query 1: ORDER BY a column in ascending order
	ModelQuery model_query_1("1",
							 "SELECT name, age FROM users ORDER BY name ASC;");

	// Query 1a: Same as Query 1 but with different column alias
	ModelQuery model_query_1a("1a",
							  "SELECT u.name AS username, u.age FROM users u ORDER BY username ASC;");

	// Query 1b: ORDER BY the same column but in descending order
	ModelQuery model_query_1b("1b",
							  "SELECT name, age FROM users ORDER BY name DESC;");

	// --- Query Set 2: ORDER BY with expressions and functions ---

	// Original Query 2: ORDER BY a function call
	ModelQuery model_query_2("2",
							 "SELECT id, name FROM users ORDER BY UPPER(name);");

	// Query 2a: Similar to Query 2 but using a different function with the same result
	ModelQuery model_query_2a("2a",
							  "SELECT id, name FROM users ORDER BY LOWER(name);"); // For testing purposes, consider LOWER and UPPER as different

	// Query 2b: ORDER BY a different expression
	ModelQuery model_query_2b("2b",
							  "SELECT id, name FROM users ORDER BY LENGTH(name);");

	// --- Query Set 3: ORDER BY with position numbers ---

	// Original Query 3: ORDER BY position number
	ModelQuery model_query_3("3",
							 "SELECT name, age FROM users ORDER BY 1;");

	// Query 3a: Same as Query 3 but with columns in different positions
	ModelQuery model_query_3a("3a",
							  "SELECT age, name FROM users ORDER BY 2;"); // Should be equivalent to Query 3

	// Query 3b: ORDER BY an invalid position number
	ModelQuery model_query_3b("3b",
							  "SELECT name, age FROM users ORDER BY 3;"); // Invalid position

	// --- Query Set 4: ORDER BY with aliases from SELECT clause ---

	// Original Query 4: ORDER BY an alias from SELECT clause
	ModelQuery model_query_4("4",
							 "SELECT name AS username, age FROM users ORDER BY username;");

	// Query 4a: Same as Query 4 but using the actual expression instead of the alias
	ModelQuery model_query_4a("4a",
							  "SELECT name AS username, age FROM users ORDER BY name;"); // Should be equivalent

	// Query 4b: ORDER BY a non-existent alias
	ModelQuery model_query_4b("4b",
							  "SELECT name AS username, age FROM users ORDER BY nickname;"); // nickname not defined

	// --- Query Set 5: ORDER BY with COLLATE ---

	// Original Query 5: ORDER BY with collation
	ModelQuery model_query_5("5",
							 "SELECT name FROM users ORDER BY name COLLATE \"en_US\";");

	// Query 5a: Similar to Query 5 but with a different collation
	ModelQuery model_query_5a("5a",
							  "SELECT name FROM users ORDER BY name COLLATE \"fr_FR\";");

	// --- Query Set 6: ORDER BY with NULLS FIRST/LAST ---

	// Original Query 6: ORDER BY with NULLS FIRST
	ModelQuery model_query_6("6",
							 "SELECT name, age FROM users ORDER BY age ASC NULLS FIRST;");

	// Query 6a: Same as Query 6 but with NULLS LAST
	ModelQuery model_query_6a("6a",
							  "SELECT name, age FROM users ORDER BY age ASC NULLS LAST;");

	// Process and compare queries for each set
	// We'll follow the same steps: create AST, process ORDER BY clauses, print, and compare

	// --- Processing for Query Set 1 ---

	// Create abstract syntax trees
	model_query_1.create_abstract_syntax_tree();
	model_query_1a.create_abstract_syntax_tree();
	model_query_1b.create_abstract_syntax_tree();

	auto root_node1 = model_query_1.get_parse_tree();
	auto root_node1a = model_query_1a.get_parse_tree();
	auto root_node1b = model_query_1b.get_parse_tree();

	// Process FROM clauses
	auto from_goal1 = From_clause::process(root_node1);
	auto from_goal1a = From_clause::process(root_node1a);
	auto from_goal1b = From_clause::process(root_node1b);

	// Process SELECT clauses
	auto select_goal1 = Select_clause::process(root_node1, from_goal1.second);
	auto select_goal1a = Select_clause::process(root_node1a, from_goal1a.second);
	auto select_goal1b = Select_clause::process(root_node1b, from_goal1b.second);

	// Process ORDER BY clauses
	auto order_by_result1 = Order_by_clause::process(root_node1, from_goal1.second, select_goal1.second);
	auto order_by_result1a = Order_by_clause::process(root_node1a, from_goal1a.second, select_goal1a.second);
	auto order_by_result1b = Order_by_clause::process(root_node1b, from_goal1b.second, select_goal1b.second);

	// Compare Query 1 with Query 1a
	auto comparison_result_1_1a = Order_by_clause::compare(order_by_result1.second, order_by_result1a.second);

	// Compare Query 1 with Query 1b
	auto comparison_result_1_1b = Order_by_clause::compare(order_by_result1.second, order_by_result1b.second);

	// Assertions for Query 1 comparisons
	BOOST_CHECK_EQUAL(comparison_result_1_1a.first, 0); // Query 1 and Query 1a should be equivalent but because they use different aliases they are not in this implementation. semantically they are the same.
	BOOST_CHECK_EQUAL(comparison_result_1_1b.first, 0); // Query 1 and Query 1b should not be equivalent

	// --- Processing for Query Set 2 ---

	// Create abstract syntax trees
	model_query_2.create_abstract_syntax_tree();
	model_query_2a.create_abstract_syntax_tree();
	model_query_2b.create_abstract_syntax_tree();

	auto root_node2 = model_query_2.get_parse_tree();
	auto root_node2a = model_query_2a.get_parse_tree();
	auto root_node2b = model_query_2b.get_parse_tree();

	// Process FROM clauses
	auto from_goal2 = From_clause::process(root_node2);
	auto from_goal2a = From_clause::process(root_node2a);
	auto from_goal2b = From_clause::process(root_node2b);

	// Process SELECT clauses
	auto select_goal2 = Select_clause::process(root_node2, from_goal2.second);
	auto select_goal2a = Select_clause::process(root_node2a, from_goal2a.second);
	auto select_goal2b = Select_clause::process(root_node2b, from_goal2b.second);

	// Process ORDER BY clauses
	auto order_by_result2 = Order_by_clause::process(root_node2, from_goal2.second, select_goal2.second);
	auto order_by_result2a = Order_by_clause::process(root_node2a, from_goal2a.second, select_goal2a.second);
	auto order_by_result2b = Order_by_clause::process(root_node2b, from_goal2b.second, select_goal2b.second);

	// Compare Query 2 with Query 2a
	auto comparison_result_2_2a = Order_by_clause::compare(order_by_result2.second, order_by_result2a.second);

	// Compare Query 2 with Query 2b
	auto comparison_result_2_2b = Order_by_clause::compare(order_by_result2.second, order_by_result2b.second);

	// Assertions for Query 2 comparisons
	BOOST_CHECK_EQUAL(comparison_result_2_2a.first, 0); // Query 2 and Query 2a should not be equivalent
	BOOST_CHECK_EQUAL(comparison_result_2_2b.first, 0); // Query 2 and Query 2b should not be equivalent

	// --- Processing for Query Set 3 ---

	// Create abstract syntax trees
	model_query_3.create_abstract_syntax_tree();
	model_query_3a.create_abstract_syntax_tree();
	model_query_3b.create_abstract_syntax_tree();

	auto root_node3 = model_query_3.get_parse_tree();
	auto root_node3a = model_query_3a.get_parse_tree();
	auto root_node3b = model_query_3b.get_parse_tree();

	// Process FROM clauses
	auto from_goal3 = From_clause::process(root_node3);
	auto from_goal3a = From_clause::process(root_node3a);
	auto from_goal3b = From_clause::process(root_node3b);

	// Process SELECT clauses
	auto select_goal3 = Select_clause::process(root_node3, from_goal3.second);
	auto select_goal3a = Select_clause::process(root_node3a, from_goal3a.second);
	auto select_goal3b = Select_clause::process(root_node3b, from_goal3b.second);

	// Process ORDER BY clauses
	auto order_by_result3 = Order_by_clause::process(root_node3, from_goal3.second, select_goal3.second);
	auto order_by_result3a = Order_by_clause::process(root_node3a, from_goal3a.second, select_goal3a.second);
	auto order_by_result3b = Order_by_clause::process(root_node3b, from_goal3b.second, select_goal3b.second);

	// Compare Query 3 with Query 3a
	auto comparison_result_3_3a = Order_by_clause::compare(order_by_result3.second, order_by_result3a.second);

	// Compare Query 3 with Query 3b
	auto comparison_result_3_3b = Order_by_clause::compare(order_by_result3.second, order_by_result3b.second);

	// Assertions for Query 3 comparisons
	BOOST_CHECK_EQUAL(comparison_result_3_3a.first, 1); // Query 3 and Query 3a should be equivalent
	BOOST_CHECK_EQUAL(comparison_result_3_3b.first, 0); // Query 3 and Query 3b should not be equivalent

	// --- Processing for Query Set 4 ---

	// Create abstract syntax trees
	model_query_4.create_abstract_syntax_tree();
	model_query_4a.create_abstract_syntax_tree();
	model_query_4b.create_abstract_syntax_tree();

	auto root_node4 = model_query_4.get_parse_tree();
	auto root_node4a = model_query_4a.get_parse_tree();
	auto root_node4b = model_query_4b.get_parse_tree();

	// Process FROM clauses
	auto from_goal4 = From_clause::process(root_node4);
	auto from_goal4a = From_clause::process(root_node4a);
	auto from_goal4b = From_clause::process(root_node4b);

	// Process SELECT clauses
	auto select_goal4 = Select_clause::process(root_node4, from_goal4.second);
	auto select_goal4a = Select_clause::process(root_node4a, from_goal4a.second);
	auto select_goal4b = Select_clause::process(root_node4b, from_goal4b.second);

	// Process ORDER BY clauses
	auto order_by_result4 = Order_by_clause::process(root_node4, from_goal4.second, select_goal4.second);
	auto order_by_result4a = Order_by_clause::process(root_node4a, from_goal4a.second, select_goal4a.second);
	auto order_by_result4b = Order_by_clause::process(root_node4b, from_goal4b.second, select_goal4b.second);

	// Compare Query 4 with Query 4a
	auto comparison_result_4_4a = Order_by_clause::compare(order_by_result4.second, order_by_result4a.second);

	// Compare Query 4 with Query 4b
	auto comparison_result_4_4b = Order_by_clause::compare(order_by_result4.second, order_by_result4b.second);

	// Assertions for Query 4 comparisons
	BOOST_CHECK_EQUAL(comparison_result_4_4a.first, 1); // Query 4 and Query 4a should be equivalent
	BOOST_CHECK_EQUAL(comparison_result_4_4b.first, 0); // Query 4 and Query 4b should not be equivalent

	// --- Processing for Query Set 5 ---

	// Create abstract syntax trees
	model_query_5.create_abstract_syntax_tree();
	model_query_5a.create_abstract_syntax_tree();

	auto root_node5 = model_query_5.get_parse_tree();
	auto root_node5a = model_query_5a.get_parse_tree();

	// Process FROM clauses
	auto from_goal5 = From_clause::process(root_node5);
	auto from_goal5a = From_clause::process(root_node5a);

	// Process SELECT clauses
	auto select_goal5 = Select_clause::process(root_node5, from_goal5.second);
	auto select_goal5a = Select_clause::process(root_node5a, from_goal5a.second);

	// Process ORDER BY clauses
	auto order_by_result5 = Order_by_clause::process(root_node5, from_goal5.second, select_goal5.second);
	auto order_by_result5a = Order_by_clause::process(root_node5a, from_goal5a.second, select_goal5a.second);

	// Compare Query 5 with Query 5a
	auto comparison_result_5_5a = Order_by_clause::compare(order_by_result5.second, order_by_result5a.second);

	// Assertions for Query 5 comparisons
	BOOST_CHECK_EQUAL(comparison_result_5_5a.first, 0); // Query 5 and Query 5a should not be equivalent

	// --- Processing for Query Set 6 ---

	// Create abstract syntax trees
	model_query_6.create_abstract_syntax_tree();
	model_query_6a.create_abstract_syntax_tree();

	auto root_node6 = model_query_6.get_parse_tree();
	auto root_node6a = model_query_6a.get_parse_tree();

	// Process FROM clauses
	auto from_goal6 = From_clause::process(root_node6);
	auto from_goal6a = From_clause::process(root_node6a);

	// Process SELECT clauses
	auto select_goal6 = Select_clause::process(root_node6, from_goal6.second);
	auto select_goal6a = Select_clause::process(root_node6a, from_goal6a.second);

	// Process ORDER BY clauses
	auto order_by_result6 = Order_by_clause::process(root_node6, from_goal6.second, select_goal6.second);
	auto order_by_result6a = Order_by_clause::process(root_node6a, from_goal6a.second, select_goal6a.second);

	// Compare Query 6 with Query 6a
	auto comparison_result_6_6a = Order_by_clause::compare(order_by_result6.second, order_by_result6a.second);

	// Assertions for Query 6 comparisons
	BOOST_CHECK_EQUAL(comparison_result_6_6a.first, 0); // Query 6 and Query 6a should not be equivalent
}
BOOST_AUTO_TEST_CASE(test_case_20)
{
	AbstractSyntaxTree ast;
	Goals goals;
	std::vector<std::string> next_steps;

	ModelQuery model_query("1", "SELECT DISTINCT a.name FROM artist a;");
	ModelQuery model_query_2("2", "SELECT a.name FROM artist a;");

	model_query.create_abstract_syntax_tree();
	model_query_2.create_abstract_syntax_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node1 = model_query.get_parse_tree();
	std::shared_ptr<AbstractSyntaxTree::AbstractSyntaxTree::Node> root_node2 = model_query_2.get_parse_tree();
	// ast.print_tree(root_node1);
	// ast.print_tree(root_node2);
	//  Generate goals for both queries
	// std::vector<Goals::Goal> goals1 = goals.generate_query_goals(root_node1);
	// std::vector<Goals::Goal> goals2 = goals.generate_query_goals(root_node2);

	// first process from clause to generate table info
	std::pair<std::string, From_clause::from_clause_info> from_goal = From_clause::process(root_node1);
	std::pair<std::string, From_clause::from_clause_info> from_goal_2 = From_clause::process(root_node2);
	std::pair<std::string, Select_clause::select_clause_info> select_goal1 = Select_clause::process(root_node1, from_goal.second);
	std::pair<std::string, Select_clause::select_clause_info> select_goal2 = Select_clause::process(root_node2, from_goal_2.second);

	std::pair<int, std::string> select_comparison = Select_clause::compare(select_goal1.second, select_goal2.second, next_steps);
	std::pair<int, std::string> from_comparison = From_clause::compare(from_goal.second, from_goal_2.second);

	// goals.print_select_clause(select_goal1.second);
	if (select_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(select_comparison.second);
	}
	if (from_comparison.first != 1)
	{
		BOOST_TEST_MESSAGE(from_comparison.second);
	}
	BOOST_CHECK_EQUAL(select_comparison.first, 0);
	BOOST_CHECK_EQUAL(from_comparison.first, 1);
}

BOOST_AUTO_TEST_SUITE_END()
