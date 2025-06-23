#include "group_by_clause.h"
#include "where_clause.h"
#include <iostream>

std::pair<std::string, Group_by_clause::group_by_clause_info> Group_by_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    group_by_clause_info group_by_info = get_info(node, from_info);

    std::ostringstream oss;

    int total_items = group_by_info.columns.size() + group_by_info.functions.size() + group_by_info.expressions.size();

    // Handle grouping sets, rollup, cube
    if (group_by_info.has_grouping_sets)
    {
        oss << "Group the result using GROUPING SETS on ";
    }
    else if (group_by_info.has_rollup)
    {
        oss << "Group the result using ROLLUP on ";
    }
    else if (group_by_info.has_cube)
    {
        oss << "Group the result using CUBE on ";
    }
    else
    {
        if (total_items == 0)
        {
            oss << "No grouping applied.";
            return std::make_pair(oss.str(), group_by_info);
        }
        else if (total_items == 1)
        {
            oss << "Group the result by 1 element: ";
        }
        else
        {
            oss << "Group the result by " << total_items << " elements: ";
        }
    }

    // Combine all items into a single list
    std::vector<std::string> all_items;
    all_items.insert(all_items.end(), group_by_info.columns.begin(), group_by_info.columns.end());
    all_items.insert(all_items.end(), group_by_info.functions.begin(), group_by_info.functions.end());
    all_items.insert(all_items.end(), group_by_info.expressions.begin(), group_by_info.expressions.end());

    // Join the items into a single string
    for (size_t i = 0; i < all_items.size(); ++i)
    {
        oss << all_items[i];
        if (i < all_items.size() - 2)
        {
            oss << ", ";
        }
        else if (i == all_items.size() - 2)
        {
            oss << ", and ";
        }
    }
    return std::make_pair(oss.str(), group_by_info);
}

Group_by_clause::group_by_clause_info Group_by_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    group_by_clause_info info;

    if (!node)
    {
        return info;
    }

    if (node->key == "groupClause")
    {
        for (const auto &child : node->children)
        {
            if (child->key == "GroupingSet")
            {
                // Handle GROUPING SETS, ROLLUP, CUBE
                for (const auto &gs_child : child->children)
                {
                    if (gs_child->key == "kind")
                    {
                        if (gs_child->value == "GROUPING SETS")
                        {
                            info.has_grouping_sets = true;
                        }
                        else if (gs_child->value == "ROLLUP")
                        {
                            info.has_rollup = true;
                        }
                        else if (gs_child->value == "CUBE")
                        {
                            info.has_cube = true;
                        }
                    }
                    else if (gs_child->key == "content")
                    {
                        // Process grouping elements
                        for (const auto &elem : gs_child->children)
                        {
                            auto elem_info = get_info(elem, from_info);
                            // Merge elem_info into info
                            info.columns.insert(info.columns.end(), elem_info.columns.begin(), elem_info.columns.end());
                            info.functions.insert(info.functions.end(), elem_info.functions.begin(), elem_info.functions.end());
                            info.expressions.insert(info.expressions.end(), elem_info.expressions.begin(), elem_info.expressions.end());
                            info.the_columns.insert(info.the_columns.end(), elem_info.the_columns.begin(), elem_info.the_columns.end());
                            info.the_functions.insert(info.the_functions.end(), elem_info.the_functions.begin(), elem_info.the_functions.end());
                            info.the_expressions.insert(info.the_expressions.end(), elem_info.the_expressions.begin(), elem_info.the_expressions.end());
                            info.has_grouping_sets |= elem_info.has_grouping_sets;
                            info.has_rollup |= elem_info.has_rollup;
                            info.has_cube |= elem_info.has_cube;
                        }
                    }
                }
            }
            else if (child->key == "ColumnRef")
            {
                // It's a column
                std::string column_name = Select_clause::extract_column_name(child, from_info);
                // Add to the_columns vector
                Select_clause::column_info ci;
                ci.column_name = column_name;
                ci.alias = ""; // GROUP BY doesn't have aliases
                info.the_columns.push_back(ci);
                // Add to the columns vector
                info.columns.push_back(column_name);
            }
            else if (child->key == "FuncCall")
            {
                // It's a function
                std::string func_name = Select_clause::extract_function(child, from_info);
                // Add to the_functions vector
                Select_clause::function_info fi;
                fi.function_name = func_name;
                fi.alias = ""; // GROUP BY doesn't have aliases
                info.the_functions.push_back(fi);
                // Add to the functions vector
                info.functions.push_back(func_name);
            }
            else
            {
                // Handle expressions or other types
                std::string expr = Where_clause::extract_expression(child, from_info, {});
                // Add to the_expressions vector
                Select_clause::expression_info ei;
                ei.expression = expr;
                ei.alias = ""; // GROUP BY doesn't have aliases
                info.the_expressions.push_back(ei);
                // Add to the expressions vector
                info.expressions.push_back(expr);
            }
        }
    }
    else
    {
        // Recursively search for groupClause
        for (const auto &child : node->children)
        {
            auto child_info = get_info(child, from_info);
            // Merge child_info into info
            info.columns.insert(info.columns.end(), child_info.columns.begin(), child_info.columns.end());
            info.functions.insert(info.functions.end(), child_info.functions.begin(), child_info.functions.end());
            info.expressions.insert(info.expressions.end(), child_info.expressions.begin(), child_info.expressions.end());
            info.the_columns.insert(info.the_columns.end(), child_info.the_columns.begin(), child_info.the_columns.end());
            info.the_functions.insert(info.the_functions.end(), child_info.the_functions.begin(), child_info.the_functions.end());
            info.the_expressions.insert(info.the_expressions.end(), child_info.the_expressions.begin(), child_info.the_expressions.end());
            info.has_grouping_sets |= child_info.has_grouping_sets;
            info.has_rollup |= child_info.has_rollup;
            info.has_cube |= child_info.has_cube;
        }
    }
    return info;
}
void Group_by_clause::print(const group_by_clause_info &group_by)
{
    std::cout << "Group By Info: " << std::endl;

    // Print grouping flags
    if (group_by.has_grouping_sets)
        std::cout << "Grouping Sets: Yes" << std::endl;
    if (group_by.has_rollup)
        std::cout << "Rollup: Yes" << std::endl;
    if (group_by.has_cube)
        std::cout << "Cube: Yes" << std::endl;

    // Print columns
    if (!group_by.the_columns.empty())
    {
        std::cout << "Group By Columns: " << std::endl;
        for (const auto &column : group_by.the_columns)
        {
            std::cout << "\t" << column.column_name << std::endl;
        }
    }

    // Print functions
    if (!group_by.the_functions.empty())
    {
        std::cout << "Group By Functions: " << std::endl;
        for (const auto &function : group_by.the_functions)
        {
            std::cout << "\t" << function.function_name << std::endl;
        }
    }

    // Print expressions
    if (!group_by.the_expressions.empty())
    {
        std::cout << "Group By Expressions: " << std::endl;
        for (const auto &expression : group_by.the_expressions)
        {
            std::cout << "\t" << expression.expression << std::endl;
        }
    }

    std::cout << std::endl;
}

std::pair<int, std::string> Group_by_clause::compare(
    const group_by_clause_info &reference,
    const group_by_clause_info &other)
{
    std::string message = "";
    int equal = 1;

    // Check if both have no GROUP BY clause
    bool ref_empty = reference.the_columns.empty() &&
                     reference.the_functions.empty() &&
                     reference.the_expressions.empty() &&
                     !reference.has_grouping_sets &&
                     !reference.has_rollup &&
                     !reference.has_cube;

    bool other_empty = other.the_columns.empty() &&
                       other.the_functions.empty() &&
                       other.the_expressions.empty() &&
                       !other.has_grouping_sets &&
                       !other.has_rollup &&
                       !other.has_cube;

    if (ref_empty && other_empty)
    {
        return std::make_pair(-1, "Both queries have no GROUP BY clause.");
    }

    // Reference has GROUP BY but other does not
    if (!ref_empty && other_empty)
    {
        message = "GROUP BY clause missing";
        return std::make_pair(0, message);
    }

    // Other has GROUP BY but reference does not
    if (ref_empty && !other_empty)
    {
        message = "GROUP BY clause unexpected";
        return std::make_pair(0, message);
    }

    // Compare grouping flags
    if (reference.has_grouping_sets != other.has_grouping_sets)
    {
        message += "\n ● Grouping Sets: Mismatch.\n";
        equal = 0;
    }
    if (reference.has_rollup != other.has_rollup)
    {
        message += "● Rollup: Mismatch.\n";
        equal = 0;
    }
    if (reference.has_cube != other.has_cube)
    {
        message += "● Cube: Mismatch.\n";
        equal = 0;
    }

    // Compare columns (order-independent)
    std::vector<std::string> ref_columns;
    std::vector<std::string> other_columns;

    for (const auto &col : reference.the_columns)
    {
        ref_columns.push_back(col.column_name);
    }
    for (const auto &col : other.the_columns)
    {
        other_columns.push_back(col.column_name);
    }

    std::sort(ref_columns.begin(), ref_columns.end());
    std::sort(other_columns.begin(), other_columns.end());

    if (ref_columns != other_columns)
    {
        message += "● Columns: Mismatch in GROUP BY columns.\n";
        equal = 0;
    }

    // Compare functions (order-independent)
    std::vector<std::string> ref_functions;
    std::vector<std::string> other_functions;

    for (const auto &func : reference.the_functions)
    {
        ref_functions.push_back(func.function_name);
    }
    for (const auto &func : other.the_functions)
    {
        other_functions.push_back(func.function_name);
    }

    std::sort(ref_functions.begin(), ref_functions.end());
    std::sort(other_functions.begin(), other_functions.end());

    if (ref_functions != other_functions)
    {
        message += "● Functions: Mismatch in GROUP BY functions.\n";
        equal = 0;
    }

    // Compare expressions (order-independent)
    std::vector<std::string> ref_expressions;
    std::vector<std::string> other_expressions;

    for (const auto &expr : reference.the_expressions)
    {
        ref_expressions.push_back(expr.expression);
    }
    for (const auto &expr : other.the_expressions)
    {
        other_expressions.push_back(expr.expression);
    }

    std::sort(ref_expressions.begin(), ref_expressions.end());
    std::sort(other_expressions.begin(), other_expressions.end());

    if (ref_expressions != other_expressions)
    {
        message += "● Expressions: Mismatch in GROUP BY expressions.\n";
        equal = 0;
    }

    return std::make_pair(equal, message);
}
