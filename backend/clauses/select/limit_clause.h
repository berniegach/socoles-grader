#ifndef LIMIT_CLAUSE_H
#define LIMIT_CLAUSE_H

#include <string>
#include <utility>
#include <memory>
#include "../../abstract_syntax_tree.h"

class Limit_clause
{
public:
    struct limit_clause_info
    {
        bool has_limit = false;
        bool has_offset = false;
        std::string limit_count;  // textual representation (e.g., 5, :param, expression)
        std::string limit_offset; // textual representation
    };

    // Extract LIMIT/OFFSET info from a SelectStmt subtree
    static limit_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Produce a human-readable description and the info struct
    static std::pair<std::string, limit_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Compare two LIMIT/OFFSET clauses
    // return {-1, msg} if both missing, {1, msg} if equal, {0, details} otherwise
    static std::pair<int, std::string> compare(const limit_clause_info &reference, const limit_clause_info &other);
};

#endif // LIMIT_CLAUSE_H
