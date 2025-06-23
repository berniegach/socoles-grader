#ifndef SELECT_CLAUSE_H
#define SELECT_CLAUSE_H

#include <string>
#include <vector>
#include <memory>
#include "../../abstract_syntax_tree.h"
#include "from_clause.h"

class Select_clause
{
public:
    /**
     * Struct to hold column information
     * Each column has a name and an optional alias
     */
    struct column_info
    {
        std::string column_name;
        std::string alias;
    };
    /**
     * Struct to hold function information
     * Each function has a name and an optional alias
     */
    struct function_info
    {
        std::string function_name;
        std::string alias;
    };
    /**
     * Struct to hold experession information
     * Eahc expression has a name and an optional alias
     */
    struct expression_info
    {
        std::string expression;
        std::string alias;
    };
    /**
     * Struct to hold information about the SELECT clause
     */
    struct select_clause_info
    {
        bool is_distinct = false;
        bool select_all = false;
        std::vector<std::string> columns;
        std::vector<std::string> functions;
        std::vector<std::string> expressions;
        std::vector<column_info> the_columns;
        std::vector<function_info> the_functions;
        std::vector<expression_info> the_expressions;
    };

    /**
     * This function extracts the column name from a column reference node
     * @param column_ref_node: This is a pointer to the column reference node
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A string containing the column name
     */
    static std::string extract_column_name(const std::shared_ptr<AbstractSyntaxTree::Node> &column_ref_node, const From_clause::from_clause_info &from_info);
    /**
     * This function extracts the function name from a function call node
     * @param func_call_node: This is a pointer to the function call node
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A string containing the function name
     */
    static std::string extract_function(const std::shared_ptr<AbstractSyntaxTree::Node> &func_call_node, const From_clause::from_clause_info &from_info);
    /**
     * This function processes the SELECT clause and returns the clause information
     * @param node: This is a pointer to the root node of the SELECT clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A pair containing the clause information as a single string and the select_clause_info structure
     */
    static std::pair<std::string, Select_clause::select_clause_info> process(std::shared_ptr<AbstractSyntaxTree::Node> node, const From_clause::from_clause_info &from_info);
    /**
     * This function extracts the information about the SELECT clause
     * THis is a helper function for the select_clause_process function
     * @param node: This is a pointer to the root node of the SELECT clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @return: A select_clause_info structure containing the information about the SELECT clause
     */
    static Select_clause::select_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    /**
     * This function compares two SELECT clauses and returns the comparison information
     * @param reference: This is a select_clause_info structure containing the reference SELECT clause information
     * @param other: This is a select_clause_info structure containing the other SELECT clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const select_clause_info &reference, const select_clause_info &other, std::vector<std::string> &next_steps);
    /**
     * This function prints the SELECT clause information
     * @param select: This is a select_clause_info structure containing the SELECT clause information
     */
    static void print(const select_clause_info &select);
};

#endif // SELECT_CLAUSE_H