#ifndef GOALS_H
#define GOALS_H

#include <string>
#include <vector>
#include <memory>
#include "abstract_syntax_tree.h"
#include "clauses/select/select_clause.h"
#include "clauses/select/where_clause.h"
#include "clauses/common.h"

class Goals
{
public:
    //--------------------------------------------------------
    // General data members for all clauses
    struct Goal
    {
        std::string type;    // e.g., "select", "from", "where", "group_by", etc.
        std::string content; // content of the goal
    };

    Goals();
    /**
     * THis function takes a query parse tree and processes it to extract the different clauses goals
     * @param root_node: This is a pointer to the root node of the parse tree
     * @return: A vector of Goal structures conatining the clause information
     */
    static std::vector<Goal> process_query(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node);
    static Common::comparision_result compare_single_statement(const std::shared_ptr<AbstractSyntaxTree::Node> &reference_root, const std::shared_ptr<AbstractSyntaxTree::Node> &student_root);
    /**
     * This function compares two queries and returns the comparison information
     * @param reference_root: This is a pointer to the root node of the reference query
     * @param student_root: This is a pointer to the root node of the student query
     * @return: A comparison_info structure containing the comparison information
     */
    static Common::comparision_result compare_queries(const std::shared_ptr<AbstractSyntaxTree::Node> &reference_root, const std::shared_ptr<AbstractSyntaxTree::Node> &student_root);

    static std::vector<std::string> generate_query_goal_general(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node);
    static std::vector<std::string> generate_query_goal_specific(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node);

private:
    static std::string generate_goal_for_stmt_general(const std::shared_ptr<AbstractSyntaxTree::Node> &stmt_node);
    static std::string generate_goal_for_stmt_specific(const std::shared_ptr<AbstractSyntaxTree::Node> &stmt_node);
};
#endif