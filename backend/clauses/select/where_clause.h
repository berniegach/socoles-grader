#ifndef WHERE_CLAUSE_H
#define WHERE_CLAUSE_H

#include <string>
#include "../../abstract_syntax_tree.h"
#include "select_clause.h"
#include "from_clause.h"

class Where_clause
{
public:
    // Enum to represent the type of condition
    enum class ConditionType
    {
        SIMPLE, // A simple condition (e.g., a = b)
        AND,    // Logical AND of conditions
        OR,     // Logical OR of conditions
        NOT     // Logical NOT of a condition
    };
    // Structure to represent a condition node
    struct ConditionNode
    {
        ConditionType type;
        // For SIMPLE type
        std::string left_operand;
        std::string operator_;
        std::string right_operand;
        // For AND, OR, NOT types
        std::vector<std::shared_ptr<ConditionNode>> children;

        // Constructor for simple conditions
        ConditionNode(const std::string &left, const std::string &op, const std::string &right)
            : type(ConditionType::SIMPLE), left_operand(left), operator_(op), right_operand(right) {}

        // Constructor for compound conditions (AND, OR, NOT)
        ConditionNode(ConditionType t, const std::vector<std::shared_ptr<ConditionNode>> &child_nodes)
            : type(t), left_operand(""), operator_(""), right_operand(""), children(child_nodes) {}
    };
    struct where_clause_info
    {
        std::shared_ptr<ConditionNode> condition_root;
    };
    /**
     * This function extracts a constant from a constant node
     * @param const_node: This is a pointer to the constant node
     * @return: A string containing the constant
     */
    static std::string extract_constant(const std::shared_ptr<AbstractSyntaxTree::Node> &const_node);
    /**
     * This function extracts the expression from an expression node
     * @param expr_node: This is a pointer to the node containing the expression
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A string containing the expression
     */
    static std::string extract_expression(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);

    /**
     * This function extracts the operator from an operator node
     * @param name_node: This is a pointer to the node containing the operator
     * @return: A string containing the operator
     */
    static std::string extract_operator(const std::shared_ptr<AbstractSyntaxTree::Node> &name_node);
    /**
     * THisfunction creates a string of join elements separated by a delimiter
     * @param elements: This is a vector of strings containing the join elements
     * @param delimiter: This is a string containing the delimiter
     * @return: A string containing the join elements separated by the delimiter
     */
    static std::string join_elements_to_str(const std::vector<std::string> &elements, const std::string &delimiter);
    /**
     * This function extracts the condition from a condition node
     * @param expr_node: This is a pointer to the node containing the condition
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: ConditionNode structure containing the condition information
     */
    static std::shared_ptr<ConditionNode> extract_condition(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function processes the WHERE clause and returns the clause information
     * @param node: This is a pointer to the root node of the WHERE clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A pair containing the clause information as a single string and the Where_clause::where_clause_info structure
     */
    static std::pair<std::string, Where_clause::where_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function extracts the information about the WHERE clause
     * THis is a helper function for the where_clause_process function
     * @param node: This is a pointer to the root node of the WHERE clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A Where_clause::where_clause_info structure containing the information about the WHERE clause
     */
    static Where_clause::where_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function compares two WHERE clauses and returns the comparison information
     * @param reference: This is a Where_clause::where_clause_info structure containing the reference WHERE clause information
     * @param other: This is a Where_clause::where_clause_info structure containing the student WHERE clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const Where_clause::where_clause_info &reference, const Where_clause::where_clause_info &other, std::vector<std::string> &next_steps);
    /**
     * This function prints the WHERE clause information
     * @param where: This is a Where_clause::where_clause_info structure containing the WHERE clause information
     */
    static void print(const Where_clause::where_clause_info &where);
    /**
     * This function creates a string containing the whereby clause information.
     * @param condition: This is a pointer to the condition node
     * @return: A string containing the whereby clause information
     */
    static std::string condition_to_string(const std::shared_ptr<Where_clause::ConditionNode> &condition);
    /**
     * This function prints the condition node information
     * @param node: This is a pointer to the condition node
     * @param indent_level: This is an integer indicating the indentation level
     */
    static void print(const std::shared_ptr<Where_clause::ConditionNode> &node, int indent_level);
    /**
     * This function compares two condition nodes and returns the comparison information
     * @param node1: This is a pointer to the first condition node
     * @param node2: This is a pointer to the second condition node
     * @param message: This is a string containing the comparison information
     * @return: A boolean value indicating whether the condition nodes are equal
     */
    static bool compare_condition_nodes(const std::shared_ptr<Where_clause::ConditionNode> &node1, const std::shared_ptr<Where_clause::ConditionNode> &node2, std::string &message);
    /**
     * This is a helper function to generate a normalized signature of a condition node
     * @param node: This is a pointer to the condition node
     * @return: A string containing the normalized signature of the condition node
     */
    static std::string generate_condition_signature(const std::shared_ptr<Where_clause::ConditionNode> &node);

private:
    static std::string condition_type_to_string(Where_clause::ConditionType type);
    /**
     * This function checks whether an operator is commutative
     * i.e whether the operands can be swapped without changing the result
     * @param op: This is a string containing the operator
     * @return: A boolean value indicating whether the operator is commutative
     */
    static bool operators_commutative(const std::string &op);
};

#endif // WHERE_CLAUSE_H