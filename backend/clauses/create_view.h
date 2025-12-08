#ifndef CREATE_VIEW_H
#define CREATE_VIEW_H

#include <string>
#include <vector>
#include <memory>
#include "../abstract_syntax_tree.h"
#include "common.h"
#include "select/select_clause.h"
#include "select/from_clause.h"
#include "select/where_clause.h"

class Create_view
{
public:
    struct create_view_info
    {
        std::string view_name;       // The name of the view.
        std::string view_definition; // A normalized string representation of the SELECT query.
        std::string from_definition;  // Summary of the FROM clause.
        std::string where_definition; // Summary of the WHERE clause if present.
        // Detailed clause information used for comparison.
        Select_clause::select_clause_info view_select_info;
        From_clause::from_clause_info view_from_info;
        Where_clause::where_clause_info view_where_info;
        bool if_not_exists = false;
        bool has_where_clause = false;
        bool with_check_option = false;
        std::string check_option_type; // e.g., "LOCAL" or "CASCADED"
    };

    // Extracts view information from the AST node.
    static create_view_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Processes the AST node and returns a summary along with the view info.
    static std::pair<std::string, create_view_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node);

    // Compares the reference view info with the student's view info.
    static Common::comparision_result compare(const create_view_info &reference, const create_view_info &student);
};

#endif // CREATE_VIEW_H
