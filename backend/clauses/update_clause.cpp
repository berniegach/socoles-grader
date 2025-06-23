#include "update_clause.h"
#include "common.h"
#include <unordered_map>

Update_clause::update_clause_info Update_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    update_clause_info info;
    if (!node)
        return info;

    if (node->key == "UpdateStmt")
    {
        // Table name
        auto rel_node = node->get_child("relation");
        if (rel_node)
        {
            info.table_name = Common::strip_quotes(rel_node->get_value("relname"));
        }

        // SET clauses
        auto targetList_node = node->get_child("targetList");
        if (targetList_node)
        {
            for (auto &res_target : targetList_node->children)
            {
                if (res_target->key == "ResTarget")
                {
                    std::string col_name = Common::strip_quotes(res_target->get_value("name"));
                    // val node holds the new expression for the column
                    auto val_node = res_target->get_child("val");
                    std::string val_expr;
                    if (val_node && !val_node->children.empty())
                    {
                        val_expr = Where_clause::extract_expression(val_node->children.front(), from_info, {});
                    }
                    info.set_clauses.push_back({col_name, val_expr});
                }
            }
        }

        // Process FROM clause if present
        auto from_clause_node = node->get_child("fromClause");
        if (from_clause_node)
        {
            info.from_info = From_clause::get_info(from_clause_node);
        }

        // WHERE clause
        auto where_node = node->get_child("whereClause");
        if (where_node)
        {
            info.where_info = Where_clause::get_info(where_node, from_info, {});
        }
    }

    return info;
}

std::pair<std::string, Update_clause::update_clause_info> Update_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    auto info = get_info(node, from_info);
    std::ostringstream oss;

    if (!info.table_name.empty())
    {
        oss << "Update table '" << info.table_name << "' by setting ";
        for (size_t i = 0; i < info.set_clauses.size(); ++i)
        {
            oss << info.set_clauses[i].first << " = " << info.set_clauses[i].second;
            if (i < info.set_clauses.size() - 2)
                oss << ", ";
            else if (i == info.set_clauses.size() - 2)
                oss << " and ";
        }

        // If a FROM clause is present, append its string representation.
        std::string from_clause_str = From_clause::from_clause_info_to_string(info.from_info);
        if (!from_clause_str.empty())
        {
            oss << " from " << from_clause_str;
        }

        if (info.where_info.condition_root)
        {
            oss << " where " << Where_clause::condition_to_string(info.where_info.condition_root);
        }
        oss << ".";
    }
    else
    {
        oss << "No UPDATE statement found.";
    }

    return std::make_pair(oss.str(), info);
}

Common::comparision_result Update_clause::compare(const update_clause_info &reference, const update_clause_info &other)
{
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // --- Compare target table names ---
    if (reference.table_name == other.table_name)
    {
        correct_parts.push_back("Target table");
    }
    else
    {
        incorrect_parts.push_back("Target table");
        message << "● The UPDATE should target table '" << reference.table_name << "', but your query uses '" << other.table_name << "'.\n";
        next_steps.push_back("💡 Update the target table to '" + reference.table_name + "'.");
        equal = false;
    }

    // --- Compare SET clauses ---
    if (reference.set_clauses.size() != other.set_clauses.size())
    {
        incorrect_parts.push_back("Set clauses count");
        message << "● The reference UPDATE sets " << reference.set_clauses.size() << " column(s), but your query sets " << other.set_clauses.size() << " column(s).\n";
        next_steps.push_back("💡 Ensure you set the correct number of columns in your UPDATE.");
        equal = false;
    }
    else
    {
        // Compare SET clauses irrespective of their order using unordered maps.
        std::unordered_map<std::string, std::string> refSet, stuSet;
        for (const auto &clause : reference.set_clauses)
            refSet[clause.first] = clause.second;
        for (const auto &clause : other.set_clauses)
            stuSet[clause.first] = clause.second;

        for (const auto &refPair : refSet)
        {
            auto it = stuSet.find(refPair.first);
            if (it == stuSet.end())
            {
                incorrect_parts.push_back("Set clause column (" + refPair.first + ")");
                message << "● Missing SET clause for column '" << refPair.first << "'.\n";
                next_steps.push_back("💡 Add the SET clause for column '" + refPair.first + "'.");
                equal = false;
            }
            else if (refPair.second != it->second)
            {
                incorrect_parts.push_back("Value for '" + refPair.first + "'");
                message << "● For column '" << refPair.first << "', the value should be '" << refPair.second << "', but you provided '" << it->second << "'.\n";
                next_steps.push_back("💡 Fix the value assignment for column '" + refPair.first + "'.");
                equal = false;
            }
        }
        if (equal)
        {
            correct_parts.push_back("Set clauses");
        }
    }

    // --- Compare FROM clause ---
    {
        auto fromCmp = From_clause::compare(reference.from_info, other.from_info);
        if (fromCmp.first == 1)
        {
            correct_parts.push_back("FROM clause");
        }
        else if (fromCmp.first == 0)
        {
            incorrect_parts.push_back("FROM clause");
            message << fromCmp.second;
            equal = false;
        }
        // If fromCmp.first == -1, both queries have no FROM clause, so ignore it.
    }

    // --- Compare WHERE clause (if present) ---
    // If at least one of the UPDATE statements has a WHERE clause, compare them.
    if (reference.where_info.condition_root || other.where_info.condition_root)
    {
        auto whereCmp = Where_clause::compare(reference.where_info, other.where_info, next_steps);
        if (whereCmp.first == 1)
        {
            correct_parts.push_back("WHERE clause");
        }
        else if (whereCmp.first == -1)
        {
            // Both don't have a WHERE clause
            // ignore it
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
        // Neither UPDATE has a WHERE clause.
        correct_parts.push_back("WHERE clause");
    }

    Common::comparision_result comp;
    comp.equal = equal;
    comp.correct_parts = correct_parts;
    comp.incorrect_parts = incorrect_parts;
    comp.next_steps = next_steps;
    comp.message = message.str();
    return comp;
}
