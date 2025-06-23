#ifndef ORDER_BY_CLAUSE_H
#define ORDER_BY_CLAUSE_H

#include <vector>
#include <string>
#include <utility>
#include <memory>
#include "../../abstract_syntax_tree.h"
#include "from_clause.h"
#include "select_clause.h"

class Order_by_clause
{
public:
    struct order_by_clause_info
    {
        struct order_item
        {
            std::string expression;
            std::string direction;   // ASC or DESC
            std::string nulls_order; // NULLS FIRST or NULLS LAST
            std::string collation;   // Collation name
        };
        std::vector<order_item> order_items;
    };
    /**
     * This function processes the ORDERBY clause and returns the clause information
     * @param node: This is a pointer to the root node of the ORDERBY clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A pair containing the clause information as a single string and the order_by_clause_info structure
     */
    static std::pair<std::string, order_by_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function extracts the information about the ORDERBY clause
     * THis is a helper function for the order_by_clause_process function
     * @param node: This is a pointer to the root node of the ORDERBY clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A order_by_clause_info structure containing the information about the ORDERBY clause
     */
    static order_by_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function prints the ORDERBY clause information
     * @param order_by: This is a order_by_clause_info structure containing the ORDERBY clause information
     */
    static void print(const order_by_clause_info &order_by);
    /**
     * This function compares two ORDERBY clauses and returns the comparison information
     * @param reference: This is a order_by_clause_info structure containing the reference ORDERBY clause information
     * @param other: This is a order_by_clause_info structure containing the student ORDERBY clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const order_by_clause_info &reference, const order_by_clause_info &other);
    /**
     * This functions extracts the collation information from a collation node
     * @param collation_node: This is a pointer to the collation node
     * @return: A string containing the collation information
     */
    static std::string extract_collation(const std::shared_ptr<AbstractSyntaxTree::Node> &collation_node);
};

#endif // ORDER_BY_CLAUSE_H