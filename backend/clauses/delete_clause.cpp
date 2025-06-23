#include "delete_clause.h"
#include "common.h"
#include <iostream>

Delete_clause::delete_clause_info Delete_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    delete_clause_info info;
    if (!node)
        return info;

    if (node->key == "DeleteStmt")
    {
        auto rel_node = node->get_child("relation");
        if (rel_node)
        {
            info.table_name = Common::strip_quotes(rel_node->get_value("relname"));
        }
        auto where_node = node->get_child("whereClause");
        if (where_node)
        {
            info.where_info = Where_clause::get_info(where_node, from_info, {});
        }
        // Process an optional FROM clause.
        // If there is no "fromClause" node, then check for a "usingClause" node.
        auto from_clause_node = node->get_child("fromClause");
        if (from_clause_node)
        {
            info.from_info = From_clause::get_info(from_clause_node);
        }
        else
        {
            auto using_clause_node = node->get_child("usingClause");
            if (using_clause_node)
            {
                info.from_info = From_clause::get_info(using_clause_node);
            }
        }
    }

    return info;
}

std::pair<std::string, Delete_clause::delete_clause_info> Delete_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    auto info = get_info(node, from_info);
    std::ostringstream oss;

    if (!info.table_name.empty())
    {
        oss << "Delete rows from '" << info.table_name << "'";
        // If a FROM clause is present (or join info via USING), append its description.
        std::string from_clause_str = "";
        // Only add if from_info is not empty.
        if (!info.from_info.ctes.empty() || !info.from_info.joins.empty() || !info.from_info.tables.empty())
        {
            from_clause_str = From_clause::from_clause_info_to_string(info.from_info);
        }
        if (!from_clause_str.empty())
        {
            oss << " using " << from_clause_str;
        }
        if (info.where_info.condition_root)
        {
            oss << " where " << Where_clause::condition_to_string(info.where_info.condition_root);
        }
        oss << ".";
    }
    else
    {
        oss << "No DELETE statement found.";
    }

    return std::make_pair(oss.str(), info);
}

Common::comparision_result Delete_clause::compare(const delete_clause_info &reference, const delete_clause_info &other)
{
    bool equal = true;
    std::ostringstream message;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;

    // --- Compare target table ---
    if (reference.table_name == other.table_name)
    {
        correct_parts.push_back("Target table");
    }
    else
    {
        incorrect_parts.push_back("Target table");
        message << "● The DELETE statement should target table '" << reference.table_name << "', but your query targets '" << other.table_name << "'.\n";
        next_steps.push_back("💡 Please change the target table to '" + reference.table_name + "'.");
        equal = false;
    }

    // --- Compare FROM clause ---
    {
        auto fromCmp = From_clause::compare(reference.from_info, other.from_info);
        // If both queries lack FROM information (fromCmp.first == -1), ignore it.
        if (fromCmp.first == 1)
        {
            correct_parts.push_back("FROM clause");
        }
        else if (fromCmp.first == 0)
        {
            incorrect_parts.push_back("FROM clause");
            message << fromCmp.second;
            next_steps.push_back("💡 Review and adjust your FROM (or USING) clause to match the expected query.");
            equal = false;
        }
    }

    // --- Compare WHERE clause (if present) ---
    // If either query has a WHERE clause, compare them.
    if (reference.where_info.condition_root || other.where_info.condition_root)
    {
        auto whereCmp = Where_clause::compare(reference.where_info, other.where_info, next_steps);
        if (whereCmp.first == 1)
        {
            correct_parts.push_back("WHERE clause");
        }
        else if (whereCmp.first == -1)
        {
            // Both queries lack a WHERE clause.
            correct_parts.push_back("WHERE clause");
        }
        else
        {
            incorrect_parts.push_back("WHERE clause");
            message << whereCmp.second;
            equal = false;
        }
    }
    else
    {
        // Neither query has a WHERE clause.
        // correct_parts.push_back("WHERE clause");
    }

    Common::comparision_result comp;
    comp.equal = equal;
    comp.correct_parts = correct_parts;
    comp.incorrect_parts = incorrect_parts;
    comp.next_steps = next_steps;
    comp.message = message.str();
    return comp;
}
