#ifndef FROM_CLAUSE_H
#define FROM_CLAUSE_H

#include <string>
#include <vector>
#include <memory>
#include "../../abstract_syntax_tree.h"

class From_clause
{
public:
    /**
     * Struct to hold table information
     * Each table has a name and an optional alias
     */
    struct table_info
    {
        std::string table_name;
        std::string alias;
    };
    // Structure to hold join information
    struct join_info
    {
        std::string join_type;
        table_info left_table;
        table_info right_table;
        std::string join_condition;
        bool is_natural = false;
        bool has_using_clause = false;
        std::vector<std::string> using_columns;
    };
    // Structure to hold information about the FROM clause
    struct from_clause_info
    {
        std::vector<table_info> tables;
        std::vector<join_info> joins;
        std::vector<std::string> ctes;
    };
    /**
     * This function processes the FROM clause and returns the clause information
     * @param node: This is a pointer to the root node of the FROM clause
     * @return: A pair containing the clause information as a single string and the from_clause_info structure
     */
    static std::pair<std::string, From_clause::from_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    /**
     * This function extracts the information about the FROM clause
     * THis is a helper function for the from_clause_process function
     * @param node: This is a pointer to the root node of the FROM clause
     * @return: A from_clause_info structure containing the information about the FROM clause
     */
    static from_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    /**
     * This function compares two FROM clauses and returns the comparison information
     * @param reference: This is a from_clause_info structure containing the reference FROM clause information
     * @param other: This is a from_clause_info structure containing the student FROM clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const from_clause_info &reference, const from_clause_info &other);
    /**
     * This function prints the FROM clause information
     * @param from: This is a from_clause_info structure containing the FROM clause information
     */
    static void print(const from_clause_info &from);
    static std::string from_clause_info_to_string(const From_clause::from_clause_info &info);

private:
    /**
     * This function extracts the information about a table
     * @param range_var_node: This is a pointer to the node containing the table information
     * @return: A table_info structure containing the information about the table
     */
    static table_info extract_table_info(const std::shared_ptr<AbstractSyntaxTree::Node> &range_var_node);
    /**
     * This function extracts the column together with its alias.
     * @param column_ref_node: This is a pointer to the node containing the column information
     * @param info: This is a join_info structure containing information about the join
     * @return: A column name as a string together with its alias
     */
    static std::string extract_column_ref(const std::shared_ptr<AbstractSyntaxTree::Node> &column_ref_node, const join_info &info);
    /**
     * This function extracts the expression from an expression node
     * @param expr_node: This is a pointer to the node containing the expression
     * @param info: This is a join_info structure containing information about the join
     * @return: A string containing the expression
     */
    static std::string extract_expression(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const join_info &info);
    /**
     * This function extracts the join condition
     * @param quals_node: This is the type of the join condition
     * @param info: This is a join_info structure containing information about the join
     * @return: A string containing the join condition
     */
    static std::string extract_join_condition(const std::shared_ptr<AbstractSyntaxTree::Node> &quals_node, const join_info &info);
    /**
     * This function extracts a table or join information from a node, whichever is present
     * @param node: This is a pointer to the node containing the table or join information
     * @param info: This is a from_clause_info structure containing information about the FROM clause
     * @return: A table_info structure containing the information about the table or join
     */
    static table_info extract_table_or_join(const std::shared_ptr<AbstractSyntaxTree::Node> &node, from_clause_info &info);
    /**
     * This function processes a join expression node
     * @param join_expr_node: This is a pointer to the join expression node
     * @param info: This is a from_clause_info structure containing information about the FROM clause
     */
    static void process_join_expr(const std::shared_ptr<AbstractSyntaxTree::Node> &join_expr_node, from_clause_info &info);
};

#endif // FROM_CLAUSE_H