#ifndef UPDATE_CLAUSE_H
#define UPDATE_CLAUSE_H

#include <string>
#include <vector>
#include "select/where_clause.h"
#include "select/from_clause.h"
#include "common.h"

class Update_clause
{
public:
    struct update_clause_info
    {
        std::string table_name;
        std::vector<std::pair<std::string, std::string>> set_clauses; // column -> new_value
        From_clause::from_clause_info from_info;
        Where_clause::where_clause_info where_info;
    };
    static Update_clause::update_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    static std::pair<std::string, Update_clause::update_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    static Common::comparision_result compare(const update_clause_info &reference, const update_clause_info &other);
};

#endif // UPDATE_CLAUSE_H