#ifndef GROUP_BY_CLAUSE_H
#define GROUP_BY_CLAUSE_H

#include <string>
#include <vector>
#include "select_clause.h"

class Group_by_clause
{
public:
    struct group_by_clause_info
    {
        std::vector<std::string> columns;
        std::vector<std::string> functions;
        std::vector<std::string> expressions;
        std::vector<Select_clause::column_info> the_columns;
        std::vector<Select_clause::function_info> the_functions;
        std::vector<Select_clause::expression_info> the_expressions;
        bool has_grouping_sets = false;
        bool has_rollup = false;
        bool has_cube = false;
    };
    /**
     * This function processes the GROUPBY clause and returns the clause information
     * @param node: This is a pointer to the root node of the GROUPBY clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A pair containing the clause information as a single string and the group_by_clause_info structure
     */
    static std::pair<std::string, group_by_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    /**
     * This function extracts the information about the GROUPBY clause
     * THis is a helper function for the group_by_clause_process function
     * @param node: This is a pointer to the root node of the GROUPBY clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A group_by_clause_info structure containing the information about the GROUPBY clause
     */
    static group_by_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    /**
     * This function compares two GROUPBY clauses and returns the comparison information
     * @param reference: This is a group_by_clause_info structure containing the reference GROUPBY clause information
     * @param other: This is a group_by_clause_info structure containing the student GROUPBY clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const group_by_clause_info &reference, const group_by_clause_info &other);
    /**
     * This function prints the GROUPBY clause information
     * @param group_by: This is a group_by_clause_info structure containing the GROUPBY clause information
     */
    static void print(const group_by_clause_info &group_by);
};

#endif // GROUP_BY_CLAUSE_H