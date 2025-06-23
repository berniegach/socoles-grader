#include "assertion_clause.h"
#include "common.h"

Assertion_clause::assertion_info Assertion_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    assertion_info info;
    if (!node)
        return info;

    if (node->key == "CreateAssertionStmt")
    {
        info.assertion_name = Common::strip_quotes(node->get_value("assertion_name"));
        // Extract check condition, likely stored in a child node named "check_expr"
        auto check_node = node->get_child("check_expr");
        if (check_node)
        {
            // Reuse the where_clause parsing logic to handle this expression
            // The assertion check is just a boolean condition, like a WHERE clause condition.
            info.check_condition = Where_clause::get_info(check_node, {}, {});
        }
    }

    return info;
}
std::pair<std::string, Assertion_clause::assertion_info> Assertion_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;

    if (!info.assertion_name.empty())
    {
        oss << "Create an assertion named '" << info.assertion_name << "' that requires ";
        if (info.check_condition.condition_root)
        {
            oss << Where_clause::condition_to_string(info.check_condition.condition_root);
        }
        else
        {
            oss << "no specific condition.";
        }
    }
    else
    {
        oss << "No assertion found.";
    }

    return std::make_pair(oss.str(), info);
}
Common::comparision_result Assertion_clause::compare(const assertion_info &reference, const assertion_info &other)
{
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // --- Compare assertion names ---
    if (reference.assertion_name == other.assertion_name)
    {
        correct_parts.push_back("Assertion name");
    }
    else
    {
        incorrect_parts.push_back("Assertion name");
        message << "● The assertion should be named '" << reference.assertion_name << "', but your query uses '" << other.assertion_name << "'.\n";
        next_steps.push_back("💡 Change the assertion name to '" + reference.assertion_name + "'.");
        equal = false;
    }

    // --- Compare assertion check conditions ---
    // The check condition is stored as a WHERE clause equivalent.
    if (reference.check_condition.condition_root || other.check_condition.condition_root)
    {
        auto condCmp = Where_clause::compare(reference.check_condition, other.check_condition, next_steps);
        if (condCmp.first == 1)
        {
            correct_parts.push_back("Assertion condition");
        }
        else if (condCmp.first == -1)
        {
            // Both assertions lack a condition (unlikely, but we'll consider that as correct)
            correct_parts.push_back("Assertion condition");
        }
        else
        {
            incorrect_parts.push_back("Assertion condition");
            message << "● The assertion condition does not match: " << condCmp.second << "\n";
            equal = false;
        }
    }
    else
    {
        // Neither assertion has a check condition
        correct_parts.push_back("Assertion condition");
    }

    Common::comparision_result comp;
    comp.equal = equal;
    comp.correct_parts = correct_parts;
    comp.incorrect_parts = incorrect_parts;
    comp.next_steps = next_steps;
    comp.message = message.str();
    return comp;
}
