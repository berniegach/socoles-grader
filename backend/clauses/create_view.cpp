#include "create_view.h"
#include "common.h"
#include <algorithm>
#include <cctype>
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
            From_clause::from_clause_info from_info;

            auto from_clause_node = select_stmt_node->get_child("fromClause");
            if (from_clause_node)
            {
                auto from_result = From_clause::process(from_clause_node);
                info.from_definition = from_result.first;
                info.view_from_info = from_result.second;
                from_info = info.view_from_info;
            }

            auto select_result = Select_clause::process(select_stmt_node, from_info);
            info.view_definition = select_result.first;
            info.view_select_info = select_result.second;

            auto where_clause_node = select_stmt_node->get_child("whereClause");
            if (where_clause_node)
            {
                auto where_result = Where_clause::process(where_clause_node, info.view_from_info, info.view_select_info);
                info.where_definition = where_result.first;
                info.view_where_info = where_result.second;
                info.has_where_clause = (info.view_where_info.condition_root != nullptr);
            }
            else
            {
                info.where_definition.clear();
                info.view_where_info = {};
                info.has_where_clause = false;
            }
        }
        else
        {
            info.view_definition = "";
        }
    }

    std::string check_option_value = node->get_value("withCheckOption");
    if (!check_option_value.empty())
    {
        if (check_option_value == "NO_CHECK_OPTION")
        {
            info.with_check_option = false;
            info.check_option_type.clear();
        }
        else
        {
            info.with_check_option = true;
            if (check_option_value == "LOCAL_CHECK_OPTION")
            {
                info.check_option_type = "LOCAL";
            }
            else if (check_option_value == "CASCADED_CHECK_OPTION")
            {
                info.check_option_type = "CASCADED";
            }
            else
            {
                info.check_option_type = check_option_value;
            }
        }
    }

    return info;
}

std::pair<std::string, Create_view::create_view_info> Create_view::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;

    auto append_sentence = [&oss](const std::string &text)
    {
        if (text.empty())
        {
            return;
        }
        if (!text.empty() && text.back() == '.')
        {
            oss << " " << text;
        }
        else
        {
            oss << " " << text << ".";
        }
    };

    if (!info.view_name.empty())
    {
        oss << "Create a view named '" << info.view_name << "'.";
        if (!info.view_definition.empty())
        {
            append_sentence("Definition: " + info.view_definition);
        }
        else
        {
            append_sentence("No view definition found");
        }
        append_sentence(info.from_definition);
        if (info.has_where_clause)
        {
            append_sentence(info.where_definition);
        }
        if (info.with_check_option)
        {
            std::string check_phrase = "Includes WITH CHECK OPTION";
            if (!info.check_option_type.empty())
            {
                check_phrase += " (" + info.check_option_type + ")";
            }
            append_sentence(check_phrase);
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

    auto from_comparison = From_clause::compare(reference.view_from_info, student.view_from_info);
    if (from_comparison.first == 1)
    {
        correct_parts.push_back("View FROM clause");
    }
    else if (from_comparison.first != -1)
    {
        incorrect_parts.push_back("View FROM clause");
        message << "● The FROM clause in the view definition does not match.\n"
                << from_comparison.second << "\n";
        next_steps.push_back("💡 Align the FROM clause tables, joins, and CTEs with the expected solution.");
        equal = false;
    }

    std::vector<std::string> where_next_steps;
    auto where_comparison = Where_clause::compare(reference.view_where_info, student.view_where_info, where_next_steps);
    if (where_comparison.first == 1)
    {
        correct_parts.push_back("View WHERE clause");
    }
    else if (where_comparison.first != -1)
    {
        incorrect_parts.push_back("View WHERE clause");
        message << "● The WHERE clause in the view definition does not match.\n"
                << where_comparison.second;
        for (const auto &step : where_next_steps)
        {
            next_steps.push_back(step);
        }
        equal = false;
    }

    auto normalize_check_option = [](const create_view_info &info) -> std::string
    {
        if (!info.with_check_option)
        {
            return "NONE";
        }
        if (info.check_option_type.empty())
        {
            return "LOCAL";
        }
        std::string normalized = info.check_option_type;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c)
                       { return static_cast<char>(std::toupper(c)); });
        return normalized;
    };

    std::string reference_check_option = normalize_check_option(reference);
    std::string student_check_option = normalize_check_option(student);

    if (reference_check_option == student_check_option)
    {
        if (reference_check_option != "NONE")
        {
            correct_parts.push_back("WITH CHECK OPTION");
        }
    }
    else
    {
        incorrect_parts.push_back("WITH CHECK OPTION");
        if (reference_check_option == "NONE")
        {
            message << "● Unnecessary WITH CHECK OPTION.\n";
            next_steps.push_back("💡 Remove WITH CHECK OPTION from the view definition.");
        }
        else if (student_check_option == "NONE")
        {
            message << "● The view must include WITH CHECK OPTION (" << reference_check_option << ").\n";
            next_steps.push_back("💡 Add WITH CHECK OPTION to.");
        }
        else
        {
            message << "● The check option type should be " << reference_check_option << " but found " << student_check_option << ".\n";
            next_steps.push_back("💡 Adjust the WITH CHECK OPTION clause to use the correct scope.");
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
