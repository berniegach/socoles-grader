#include "order_by_clause.h"
#include "where_clause.h"
#include <iostream>
#include "../common.h"

Order_by_clause::order_by_clause_info Order_by_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    order_by_clause_info info;
    if (!node)
    {
        return info;
    }
    if (node->key == "sortClause")
    {
        for (const auto &child : node->children)
        {
            if (child->key == "SortBy")
            {
                order_by_clause_info::order_item item;
                auto node_expr = child->get_child("node");
                if (node_expr)
                {
                    // Check if the node is an integer constant (position reference)
                    if (node_expr->children.front()->key == "A_Const")
                    {
                        std::string pos_str = Where_clause::extract_constant(node_expr->children.front());
                        int position = std::stoi(pos_str);
                        if (position >= 1 && position <= static_cast<int>(select_info.the_columns.size()))
                        {
                            // Get the corresponding expression from SELECT clause
                            item.expression = select_info.the_columns[position - 1].column_name;
                        }
                        else
                        {
                            // Invalid position; handle error accordingly
                            item.expression = "<Invalid Position Reference>";
                        }
                    }
                    else
                    {
                        // Regular expression
                        item.expression = Where_clause::extract_expression(node_expr->children.front(), from_info, select_info);
                    }
                }
                item.direction = child->get_value("sortby_dir");
                item.nulls_order = child->get_value("sortby_nulls");

                // Handle collation
                auto collation_node = child->get_child("collation");
                if (collation_node)
                {
                    item.collation = extract_collation(collation_node);
                }

                info.order_items.push_back(item);
            }
        }
    }
    else
    {
        for (const auto &child : node->children)
        {
            auto child_info = get_info(child, from_info, select_info);
            info.order_items.insert(info.order_items.end(), child_info.order_items.begin(), child_info.order_items.end());
        }
    }
    return info;
}

std::pair<std::string, Order_by_clause::order_by_clause_info> Order_by_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    auto order_info = get_info(node, from_info, select_info);
    if (!order_info.order_items.empty())
    {
        std::ostringstream oss;
        oss << "Sort the output data by ";
        for (size_t i = 0; i < order_info.order_items.size(); ++i)
        {
            const auto &item = order_info.order_items[i];
            oss << item.expression;
            // Convert direction to readable format
            if (item.direction == "SORTBY_ASC")
            {
                oss << " ascending";
            }
            else if (item.direction == "SORTBY_DESC")
            {
                oss << " descending";
            }
            // Handle NULLS FIRST/LAST
            if (item.nulls_order == "SORTBY_NULLS_FIRST")
            {
                oss << " nulls first";
            }
            else if (item.nulls_order == "SORTBY_NULLS_LAST")
            {
                oss << " nulls last";
            }
            // Include collation if present
            if (!item.collation.empty())
            {
                oss << " collate \"" << item.collation << "\"";
            }
            if (i < order_info.order_items.size() - 2)
            {
                oss << ", ";
            }
            else if (i == order_info.order_items.size() - 2)
            {
                oss << ", and ";
            }
        }
        return std::make_pair(oss.str(), order_info);
    }
    else
    {
        return std::make_pair("No ORDER BY clause present", order_info);
    }
}
void Order_by_clause::print(const order_by_clause_info &order_by)
{
    std::cout << "Order By Info:" << std::endl;

    if (order_by.order_items.empty())
    {
        std::cout << "\tNo ORDER BY clause present." << std::endl;
        return;
    }

    for (size_t i = 0; i < order_by.order_items.size(); ++i)
    {
        const auto &item = order_by.order_items[i];
        std::cout << "\tOrder Item " << i + 1 << ":" << std::endl;
        std::cout << "\t\tExpression: " << item.expression << std::endl;
        std::cout << "\t\tDirection: ";
        if (item.direction == "SORTBY_ASC")
        {
            std::cout << "ASC" << std::endl;
        }
        else if (item.direction == "SORTBY_DESC")
        {
            std::cout << "DESC" << std::endl;
        }
        else
        {
            std::cout << "DEFAULT" << std::endl;
        }

        std::cout << "\t\tNulls Order: ";
        if (item.nulls_order == "SORTBY_NULLS_FIRST")
        {
            std::cout << "NULLS FIRST" << std::endl;
        }
        else if (item.nulls_order == "SORTBY_NULLS_LAST")
        {
            std::cout << "NULLS LAST" << std::endl;
        }
        else
        {
            std::cout << "DEFAULT" << std::endl;
        }

        if (!item.collation.empty())
        {
            std::cout << "\t\tCollation: " << item.collation << std::endl;
        }
    }
    std::cout << std::endl;
}
std::pair<int, std::string> Order_by_clause::compare(const order_by_clause_info &reference, const order_by_clause_info &other)
{
    std::string message;
    int equal = 1;

    // Check if both have no ORDER BY clause
    if (reference.order_items.empty() && other.order_items.empty())
    {
        return std::make_pair(-1, "Both queries have no ORDER BY clause.");
    }

    // Compare the number of order items
    if (reference.order_items.size() != other.order_items.size())
    {
        message += "\n ● Order By: Different number of order items.\n";
        equal = 0;
    }

    // Determine the minimum number of items to compare
    size_t min_size = std::min(reference.order_items.size(), other.order_items.size());

    for (size_t i = 0; i < min_size; ++i)
    {
        const auto &ref_item = reference.order_items[i];
        const auto &other_item = other.order_items[i];

        // Compare expressions
        if (ref_item.expression != other_item.expression)
        {
            message += "● Order Item " + std::to_string(i + 1) + ": Mismatch in expressions.\n";
            equal = 0;
        }

        // Compare directions
        if (ref_item.direction != other_item.direction)
        {
            message += "● Order Item " + std::to_string(i + 1) + ": Mismatch in sorting direction.\n";
            equal = 0;
        }

        // Compare nulls ordering
        if (ref_item.nulls_order != other_item.nulls_order)
        {
            message += "● Order Item " + std::to_string(i + 1) + ": Mismatch in NULLS ordering.\n";
            equal = 0;
        }

        // Compare collations
        if (ref_item.collation != other_item.collation)
        {
            message += "● Order Item " + std::to_string(i + 1) + ": Mismatch in collation.\n";
            equal = 0;
        }
    }

    // If one has more items than the other, note the extra items
    if (reference.order_items.size() > min_size)
    {
        message += "● Order By: Reference has additional order items.\n";
        equal = 0;
    }
    else if (other.order_items.size() > min_size)
    {
        message += "● Order By: Other query has additional order items.\n";
        equal = 0;
    }

    return std::make_pair(equal, message);
}
std::string Order_by_clause::extract_collation(const std::shared_ptr<AbstractSyntaxTree::Node> &collation_node)
{
    std::string collation_name;
    for (const auto &child : collation_node->children)
    {
        if (child->key == "String")
        {
            for (const auto &sval_node : child->children)
            {
                if (sval_node->key == "sval")
                {
                    collation_name += Common::strip_quotes(sval_node->value);
                }
            }
        }
    }
    return collation_name;
}
