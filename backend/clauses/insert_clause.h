#ifndef INSERT_CLAUSE_H
#define INSERT_CLAUSE_H

#include <string>
#include <vector>
#include <memory>
#include "../abstract_syntax_tree.h"
#include "common.h"

class Insert_clause
{
public:
    struct insert_clause_info
    {
        std::string table_name;
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> values; // Each inner vector represents one row's values.
        bool is_select_insert = false;                // True if the INSERT uses a SELECT subquery.
    };

    static Insert_clause::insert_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static std::pair<std::string, Insert_clause::insert_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static Common::comparision_result compare(const insert_clause_info &reference, const insert_clause_info &other);
};

#endif // INSERT_CLAUSE_H