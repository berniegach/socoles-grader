#include "having_clause.h"
#include <iostream>

Where_clause::where_clause_info Having_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    Where_clause::where_clause_info info;
    if (!node)
    {
        return info;
    }
    if (node->key == "havingClause")
    {
        info.condition_root = Where_clause::extract_condition(node->children.front(), from_info, select_info);
    }
    else
    {
        for (const auto &child : node->children)
        {
            info = get_info(child, from_info, select_info);
            if (info.condition_root)
            {
                break;
            }
        }
    }
    return info;
}

std::pair<std::string, Where_clause::where_clause_info> Having_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    Where_clause::where_clause_info having_info = get_info(node, from_info, select_info);
    if (having_info.condition_root)
    {
        std::string condition_str = Where_clause::condition_to_string(having_info.condition_root);
        return std::make_pair("To filter grouped data having " + condition_str, having_info);
        // return "To filter grouped data having " + condition_str;
    }
    else
    {
        return std::make_pair("No conditions in HAVING clause", having_info);
        // return "No conditions in HAVING clause";
    }
}
// Function to print the HAVING clause
void Having_clause::print(const Where_clause::where_clause_info &having_info)
{
    if (!having_info.condition_root)
    {
        std::cout << "No HAVING clause." << std::endl;
    }
    else
    {
        std::cout << "HAVING clause conditions:" << std::endl;
        Where_clause::print(having_info.condition_root, 0);
    }
}

// Function to compare the HAVING clause information
std::pair<int, std::string> Having_clause::compare(const Where_clause::where_clause_info &reference, const Where_clause::where_clause_info &other)
{
    std::string message = "";
    bool equal = true;

    if (!reference.condition_root && !other.condition_root)
    {
        return std::make_pair(-1, "Both queries have no HAVING clause.");
    }

    if ((reference.condition_root && !other.condition_root) || (!reference.condition_root && other.condition_root))
    {
        message = "Mismatch in HAVING clause presence.\n";
        equal = false;
    }
    else
    {
        if (!Where_clause::compare_condition_nodes(reference.condition_root, other.condition_root, message))
        {
            equal = false;
        }
    }

    return std::make_pair(equal, message);
}