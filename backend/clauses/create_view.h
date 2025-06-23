#ifndef CREATE_VIEW_H
#define CREATE_VIEW_H

#include <string>
#include <vector>
#include <memory>
#include "../abstract_syntax_tree.h"
#include "common.h"
#include "select/select_clause.h"
#include "select/from_clause.h"

class Create_view
{
public:
    struct create_view_info
    {
        std::string view_name;       // The name of the view.
        std::string view_definition; // A normalized string representation of the SELECT query.
        // Detailed select clause information ---
        Select_clause::select_clause_info view_select_info;
        bool if_not_exists = false;
    };

    // Extracts view information from the AST node.
    static create_view_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Processes the AST node and returns a summary along with the view info.
    static std::pair<std::string, create_view_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Compares the reference view info with the student's view info.
    static Common::comparision_result compare(const create_view_info &reference, const create_view_info &student);
};

#endif // CREATE_VIEW_H
