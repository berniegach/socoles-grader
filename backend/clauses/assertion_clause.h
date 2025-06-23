#ifndef ASSERTION_CLAUSE_H
#define ASSERTION_CLAUSE_H

#include <string>
#include "select/where_clause.h"
#include "common.h"

class Assertion_clause
{
public:
    struct assertion_info
    {
        std::string assertion_name;
        Where_clause::where_clause_info check_condition;
    };
    static Assertion_clause::assertion_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static std::pair<std::string, Assertion_clause::assertion_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);
    static Common::comparision_result compare(const assertion_info &reference, const assertion_info &other);

private:
};

#endif // ASSERTION_CLAUSE_H