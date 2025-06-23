#ifndef DELETE_CLAUSE_H
#define DELETE_CLAUSE_H

#include <string>
#include "select/where_clause.h"
#include "select/from_clause.h"
#include "common.h"

class Delete_clause
{
public:
    struct delete_clause_info
    {
        std::string table_name;
        From_clause::from_clause_info from_info;
        Where_clause::where_clause_info where_info;
    };
    static Delete_clause::delete_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    static std::pair<std::string, Delete_clause::delete_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info);
    static Common::comparision_result compare(const delete_clause_info &reference, const delete_clause_info &other);
};

#endif // DELETE_CLAUSE_H