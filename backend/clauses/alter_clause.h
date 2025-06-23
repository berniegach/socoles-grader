// alter_clause.h
#ifndef ALTER_CLAUSE_H
#define ALTER_CLAUSE_H

#include <string>
#include <vector>
#include <memory>
#include "../abstract_syntax_tree.h"
#include "common.h"

class Alter_clause
{
public:
    struct alter_clause_info
    {
        std::string object_type; // e.g. "table"
        std::string object_name;

        enum OperationType
        {
            ADD_COLUMN,
            DROP_COLUMN,
            ALTER_COLUMN_TYPE,
            SET_NOT_NULL,
            DROP_NOT_NULL,
            RENAME_COLUMN,
            RENAME_TABLE,
            ADD_CONSTRAINT,
            DROP_CONSTRAINT,
            SET_SCHEMA,
            SET_TABLESPACE,
            OWNER_TO
        };

        struct operation
        {
            OperationType type;

            // For column ops
            std::string column_name;
            std::string data_type;
            bool not_null = false;

            // For rename / set-to ops
            std::string new_name;
            std::string behavior; // e.g. "DROP_CASCADE" or "DROP_RESTRICT"

            // For constraint ops
            std::string constraint_name;
            std::string constraint_expr;
            // For CHECK constraints: a normalized signature
            std::string constraint_signature;
        };

        std::vector<operation> operations;
    };

    static alter_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static std::pair<std::string, alter_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static Common::comparision_result compare(const alter_clause_info &reference, const alter_clause_info &other);
    static void print(const alter_clause_info &info);
};

#endif // ALTER_CLAUSE_H
