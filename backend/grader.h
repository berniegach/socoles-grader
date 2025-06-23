/**
 * @file grader.h
 * @brief This file contains the declaration of the Grader class.
 * The class is used to grade the queries based on the various outcomes of the properties calculated.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef GRADER_H
#define GRADER_H
#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

class Grader
{
public:
	enum class property_state
	{
		INVALID,
		INCORRECT,
		SM_1,
		SM_2,
		SM_3,
		SM_4,
		SM_5,
		MINOR_INCORRECT,
		CORRECT
	};
	enum class property_level
	{
		ABSENT,
		TWO_LEVELS,
		THREE_LEVELS,
		SEMATICS_LEVELS_6
	};
	/**
	 * The order of importance of the properties.
	 * SY_SM_RE: Syntax, Semantics, Results
	 * SM_SY_RE: Semantics, Syntax, Results
	 * RE_SM_SY: Results, Semantics, Syntax
	 * SY_RE_SM: Syntax, Results, Semantics
	 * SM_RE_SY: Semantics, Results, Syntax
	 * RE_SY_SM: Results, Syntax, Semantics
	 */
	enum class property_order
	{
		SY_SM_RE,
		SM_SY_RE,
		RE_SM_SY,
		SY_RE_SM,
		SM_RE_SY,
		RE_SY_SM
	};
	struct properties
	{
		property_state results;
		property_state semantics;
		property_state syntax;
	};
	struct grading_options
	{
		property_level syntax;
		property_level semantics;
		property_level results;
		property_order order_of_importance;
	};
	/**
	 * This function returns a correctness level matrix for given properties and their chosen outcomes.
	 * @param syntax: number of syntax outcomes
	 * @param semantics: number of semantic outcomes
	 * @param results: number of results outcomes
	 * @param order_of_importance: 1 - syntax most important, 2 - semantics most important, 3 - results most important
	 * @return: the correctness level matrix
	 */

	vector<properties> get_correctness_matrix(property_level results, property_level semantics, property_level syntax, property_order order);
	/**
	 * This function returns the normalized value of a correctness level.
	 * The value ranges from 0.0 to 1.0. This standardizes different correctness models.
	 * @param matrix: the correctness level matrix
	 * @param correctness_level: the correctness level of a particular query.
	 * @return: the normalized value of the correctness level.
	 */
	double correctness_level_normalized_value(const vector<properties> &matrix, int correctness_level);
	/**
	 * This function calculates the correctness level of a query and sets it.
	 * @param admin: the admin object that contains the grading parameters.
	 * @param query: the query to be graded.
	 */
	std::pair<int, double> calculate_correctness_level(property_level sn_level, property_level sm_level, property_level rs_level, property_order order, property_state results, property_state semantics, property_state syntax);
	/**
	 * This function displays the correctness matrix on the console.
	 * @param syntax: number of syntax outcomes
	 * @param semantics: number of semantic outcomes
	 * @param results: number of results outcomes
	 * @param order_of_importance: 1 - syntax most important, 2 - semantics most important, 3 - results most important
	 */
	void display_correctness_matrix(std::vector<properties> matrix);
	void format_output(string original_file);
	std::string property_state_to_string(property_state state);
};
#endif // !GRADER_H
