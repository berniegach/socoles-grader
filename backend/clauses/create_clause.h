#ifndef CREATE_CLAUSE_H
#define CREATE_CLAUSE_H

#include <string>
#include <vector>
#include <memory>
#include "../abstract_syntax_tree.h"
#include "common.h"
#include "select/where_clause.h"

class Create_clause
{
public:
    struct create_clause_info
    {
        std::string object_type; // e.g., "table"
        std::string object_name;
        struct column_definition
        {
            std::string name;
            std::string type;
            bool not_null = false;
        };
        std::vector<column_definition> columns;
        std::vector<std::string> primary_key_columns;
        struct foreign_key_constraint
        {
            std::vector<std::string> local_columns;
            std::string referenced_table;
            std::vector<std::string> referenced_columns;
        };
        std::vector<foreign_key_constraint> foreign_key_constraints;
        std::vector<std::string> unique_constraints;
        std::vector<std::string> check_constraints;
        std::vector<std::shared_ptr<Where_clause::ConditionNode>> check_condition_nodes;
        bool if_not_exists = false;
    };
    static Create_clause::create_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static std::pair<std::string, Create_clause::create_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static Common::comparision_result compare(const create_clause_info &reference, const create_clause_info &other);
};

#endif // CREATE_CLAUSE_H