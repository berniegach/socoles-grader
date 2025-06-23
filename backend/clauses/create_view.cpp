#include "create_view.h"
#include "common.h"
#include <iostream>
#include <sstream>

// get_info: Extract view name and detailed SELECT clause info from the AST node.
Create_view::create_view_info Create_view::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    create_view_info info;
    if (!node)
        return info;

    // We expect a view statement (the parser uses "ViewStmt").
    if (node->key != "ViewStmt")
    {
        std::cout << "Unsupported statement type in Create_view::get_info: " << node->key << std::endl;
        return info;
    }

    // Extract the view name from the "view" child.
    auto view_node = node->get_child("view");
    if (view_node)
    {
        info.view_name = Common::strip_quotes(view_node->get_value("relname"));
    }

    // Extract the view definition.
    // The view's SELECT query is stored under the "query" child, which in turn contains a "SelectStmt".
    auto query_node = node->get_child("query");
    if (query_node)
    {
        auto select_stmt_node = query_node->get_child("SelectStmt");
        if (select_stmt_node)
        {
            // To process the SELECT clause, we need a FROM clause info.
            // For a view, we assume the SELECT query has its own FROM clause;
            // here we pass a dummy from_info (assuming an empty default-constructed one is acceptable).
            From_clause::from_clause_info dummy_from;
            auto select_result = Select_clause::process(select_stmt_node, dummy_from);
            info.view_definition = select_result.first;   // e.g., "SELECT isbn, title FROM book"
            info.view_select_info = select_result.second; // Detailed select clause info.
        }
        else
        {
            info.view_definition = "";
        }
    }

    return info;
}

std::pair<std::string, Create_view::create_view_info> Create_view::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;

    if (!info.view_name.empty())
    {
        oss << "Create a view named '" << info.view_name << "'. ";
        if (!info.view_definition.empty())
        {
            oss << "Definition: " << info.view_definition << ".";
        }
        else
        {
            oss << "No view definition found.";
        }
    }
    else
    {
        oss << "No CREATE VIEW statement found.";
    }

    return std::make_pair(oss.str(), info);
}

// compare: Uses the integrated select clause comparison to provide detailed feedback.
Common::comparision_result Create_view::compare(const create_view_info &reference, const create_view_info &student)
{
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // Compare view names.
    if (reference.view_name == student.view_name)
    {
        correct_parts.push_back("View name");
    }
    else
    {
        incorrect_parts.push_back("View name");
        message << "● The view name should be '" << reference.view_name << "', but found '" << student.view_name << "'.\n";
        next_steps.push_back("💡 Change the view name to '" + reference.view_name + "'.");
        equal = false;
    }

    // Integrate the existing SELECT clause comparison to compare the view definitions.
    // We'll call Select_clause::compare on the detailed select_clause_info stored in the view info.
    std::vector<std::string> select_next_steps;
    auto select_comparison = Select_clause::compare(reference.view_select_info, student.view_select_info, select_next_steps);
    if (select_comparison.first == 1)
    {
        correct_parts.push_back("View SELECT clause");
    }
    else
    {
        incorrect_parts.push_back("View SELECT clause");
        message << "● The SELECT clause in the view definition does not match.\n"
                << select_comparison.second << "\n";
        // Append next steps provided by the SELECT clause comparison.
        for (const auto &step : select_next_steps)
        {
            next_steps.push_back(step);
        }
        equal = false;
    }

    Common::comparision_result comp;
    comp.equal = equal;
    comp.correct_parts = correct_parts;
    comp.incorrect_parts = incorrect_parts;
    comp.next_steps = next_steps;
    comp.message = message.str();
    return comp;
}
