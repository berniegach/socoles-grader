#include "select_clause.h"
#include "../common.h"
#include "where_clause.h"
#include <iostream>

std::string Select_clause::extract_column_name(const std::shared_ptr<AbstractSyntaxTree::Node> &column_ref_node, const From_clause::from_clause_info &from_info)
{
    std::string column_name;
    std::string alias_or_table_name;

    for (const auto &child : column_ref_node->children)
    {
        if (child->key == "fields")
        {
            std::vector<std::string> fields;
            for (const auto &field_node : child->children)
            {
                if (field_node->key == "String")
                {
                    for (const auto &sval_node : field_node->children)
                    {
                        if (sval_node->key == "sval")
                        {
                            fields.push_back(Common::Common::strip_quotes(sval_node->value));
                        }
                    }
                }
                else if (field_node->key == "A_Star")
                {
                    // Handle the '*' wildcard
                    fields.push_back("*");
                }
            }
            if (fields.size() == 2)
            {
                alias_or_table_name = fields[0];
                column_name = fields[1];

                // Replace alias with full table name
                // use the info in the table_info struct to get the full table name
                for (const auto &table : from_info.tables)
                {
                    if (table.alias == alias_or_table_name)
                    {
                        alias_or_table_name = table.table_name;
                    }
                }
            }
            else if (fields.size() == 1)
            {
                column_name = fields[0];
            }
        }
    }

    if (!alias_or_table_name.empty())
    {
        return alias_or_table_name + "." + column_name;
    }
    else
    {
        return column_name;
    }
}
std::string Select_clause::extract_function(const std::shared_ptr<AbstractSyntaxTree::Node> &func_call_node, const From_clause::from_clause_info &from_info)
{
    std::string func_name;
    std::vector<std::string> args;

    for (const auto &child : func_call_node->children)
    {
        if (child->key == "funcname")
        {
            for (const auto &func_name_node : child->children)
            {
                if (func_name_node->key == "String")
                {
                    for (const auto &sval_node : func_name_node->children)
                    {
                        if (sval_node->key == "sval")
                        {
                            func_name = Common::strip_quotes(sval_node->value);
                        }
                    }
                }
            }
        }
        else if (child->key == "args")
        {
            for (const auto &arg_node : child->children)
            {
                std::string arg_value = Where_clause::extract_expression(arg_node, from_info, {});
                args.push_back(arg_value);
            }
        }
    }

    std::string func_representation = func_name + "(";
    for (size_t i = 0; i < args.size(); ++i)
    {
        func_representation += args[i];
        if (i != args.size() - 1)
        {
            func_representation += ", ";
        }
    }
    func_representation += ")";

    return func_representation;
}
std::pair<std::string, Select_clause::select_clause_info> Select_clause::process(std::shared_ptr<AbstractSyntaxTree::Node> node, const From_clause::from_clause_info &from_info)
{
    select_clause_info select_info = get_info(node, from_info);

    std::ostringstream oss;

    // Handle DISTINCT
    if (select_info.is_distinct)
    {
        oss << "Return distinct data from ";
    }
    else
    {
        oss << "Return data from ";
    }

    int total_items = select_info.columns.size() + select_info.functions.size() + select_info.expressions.size();

    // Handle singular/plural
    if (total_items == 0)
    {
        oss << "0 columns";
    }
    else if (total_items == 1)
    {
        oss << "1 column: ";
    }
    else
    {
        oss << total_items << " columns: ";
    }

    // Combine all items into a single list
    std::vector<std::string> all_items;
    all_items.insert(all_items.end(), select_info.columns.begin(), select_info.columns.end());
    all_items.insert(all_items.end(), select_info.functions.begin(), select_info.functions.end());
    all_items.insert(all_items.end(), select_info.expressions.begin(), select_info.expressions.end());

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
    return std::make_pair(oss.str(), select_info);
    // return oss.str();
}

Select_clause::select_clause_info Select_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info)
{
    select_clause_info info;
    int count = 0;

    if (!node)
    {
        return info;
    }

    if (node->key == "SelectStmt")
    {
        // Check for DISTINCT
        for (const auto &child : node->children)
        {
            if (child->key == "distinctClause")
            {
                info.is_distinct = true;
                break;
            }
        }
    }

    if (node->key == "targetList")
    {
        // Process each ResTarget in the targetList
        for (const auto &res_target_node : node->children)
        {
            if (res_target_node->key == "ResTarget")
            {
                std::string alias;
                std::shared_ptr<AbstractSyntaxTree::Node> val_node;

                // Check if the ColumnRef contains '*'
                bool is_star = false;
                // Extract 'name' (alias) and 'val'
                for (const auto &child : res_target_node->children)
                {
                    if (child->key == "name")
                    {
                        alias = Common::strip_quotes(child->value); // Assuming child->value holds the alias name
                    }
                    else if (child->key == "val")
                    {
                        val_node = child; // Assign val_node directly to child
                    }
                }

                if (val_node)
                {
                    if (val_node->children.front()->key == "ColumnRef")
                    {
                        // It's a column
                        std::string column_name = extract_column_name(val_node->children.front(), from_info);
                        // first add column name and its alias
                        column_info ci;
                        ci.column_name = column_name;
                        ci.alias = alias;
                        info.the_columns.push_back(ci);
                        // then add the column name to the columns vector with a message
                        if (!alias.empty())
                        {
                            // For now we do nothing with the alias
                            // column_name += " AS " + alias;
                        }
                        info.columns.push_back(column_name);
                    }
                    else if (val_node->children.front()->key == "FuncCall")
                    {
                        // It's a function
                        std::string func_representation = extract_function(val_node->children.front(), from_info);
                        // first add function and its alias
                        function_info fi;
                        fi.function_name = func_representation;
                        fi.alias = alias;
                        info.the_functions.push_back(fi);
                        // then add the function to the functions vector with a message
                        if (!alias.empty())
                        {
                            // For now we do nothing with the alias
                            // func_representation += " AS " + alias;
                        }
                        info.functions.push_back(func_representation);
                    }
                    else
                    {
                        // Handle expressions or other types
                        std::string expr = Where_clause::extract_expression(val_node->children.front(), from_info, info);
                        // first add expression and its alias
                        expression_info ei;
                        ei.expression = expr;
                        ei.alias = alias;
                        info.the_expressions.push_back(ei);
                        // then add the expression to the expressions vector with a message
                        if (!alias.empty())
                        {
                            // For now we do nothing with the alias
                            // expr += " AS " + alias;
                        }
                        info.expressions.push_back(expr);
                    }
                }
            }
        }
    }
    else
    {
        // Recursively search for targetList
        for (const auto &child : node->children)
        {
            auto child_info = get_info(child, from_info);
            // Merge child_info into info
            info.is_distinct = info.is_distinct || child_info.is_distinct;
            info.columns.insert(info.columns.end(), child_info.columns.begin(), child_info.columns.end());
            info.functions.insert(info.functions.end(), child_info.functions.begin(), child_info.functions.end());
            info.expressions.insert(info.expressions.end(), child_info.expressions.begin(), child_info.expressions.end());
            info.the_columns.insert(info.the_columns.end(), child_info.the_columns.begin(), child_info.the_columns.end());
            info.the_functions.insert(info.the_functions.begin(), child_info.the_functions.begin(), child_info.the_functions.end());
            info.the_expressions.insert(info.the_expressions.end(), child_info.the_expressions.begin(), child_info.the_expressions.end());
        }
    }
    return info;
}
std::pair<int, std::string> Select_clause::compare(const Select_clause::select_clause_info &reference, const Select_clause::select_clause_info &other, std::vector<std::string> &next_steps)
{
    std::string message = "";
    int equal = 1;

    // if both dont have clauses, return true
    if (reference.the_columns.empty() && reference.the_functions.empty() && reference.the_expressions.empty() &&
        other.the_columns.empty() && other.the_functions.empty() && other.the_expressions.empty())
    {
        return std::make_pair(-1, "Both queries have no SELECT clause.");
    }

    // First we check if the distinct values are the same
    if (reference.is_distinct != other.is_distinct)
    {
        if (reference.is_distinct)
        {
            message += "● Missing DISTINCT keyword.\n";
            next_steps.push_back("💡 Add the DISTINCT keyword to eliminate duplicate rows in your SELECT clause.");
        }
        else
        {
            message += "● Unexpected DISTINCT keyword.\n";
            next_steps.push_back("💡 Remove the DISTINCT keyword from your SELECT clause if duplicates are acceptable.");
        }
        equal = 0;
    }

    // Then we check if the columns are the same, using column_name from column_info
    std::vector<std::string> ref_columns;
    std::vector<std::string> other_columns;

    // Extract column names from reference and other, using column_name
    for (const auto &col : reference.the_columns)
    {
        ref_columns.push_back(col.column_name);
    }
    for (const auto &col : other.the_columns)
    {
        other_columns.push_back(col.column_name);
    }

    // Sort the columns for comparison (order may not matter)
    std::sort(ref_columns.begin(), ref_columns.end());
    std::sort(other_columns.begin(), other_columns.end());

    // Compare the columns
    if (ref_columns != other_columns)
    {
        equal = 0;
        message += "\n● Mismatch in selected columns.\n";

        // Identify missing and extra columns
        std::vector<std::string> missing_columns, extra_columns;
        std::set_difference(ref_columns.begin(), ref_columns.end(),
                            other_columns.begin(), other_columns.end(),
                            std::back_inserter(missing_columns));
        std::set_difference(other_columns.begin(), other_columns.end(),
                            ref_columns.begin(), ref_columns.end(),
                            std::back_inserter(extra_columns));

        if (!missing_columns.empty())
        {
            message += "● Missing columns: " + Where_clause::join_elements_to_str(missing_columns, ",") + ".\n";
            next_steps.push_back("💡 Add the missing columns: " + Where_clause::join_elements_to_str(missing_columns, ",") + " to your SELECT clause.");
        }
        if (!extra_columns.empty())
        {
            message += "● Extra columns: " + Where_clause::join_elements_to_str(extra_columns, ",") + ".\n";
            next_steps.push_back("💡 Remove the extra columns: " + Where_clause::join_elements_to_str(extra_columns, ",") + " from your SELECT clause.");
        }
    }

    // Compare functions (order-independent)
    std::vector<std::string> ref_functions;
    std::vector<std::string> other_functions;

    // Extract function names from reference and other
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
        equal = 0;
        message += "\n● Mismatch in selected functions.\n";

        // Identify missing and extra functions
        std::vector<std::string> missing_functions, extra_functions;
        std::set_difference(ref_functions.begin(), ref_functions.end(),
                            other_functions.begin(), other_functions.end(),
                            std::back_inserter(missing_functions));
        std::set_difference(other_functions.begin(), other_functions.end(),
                            ref_functions.begin(), ref_functions.end(),
                            std::back_inserter(extra_functions));

        if (!missing_functions.empty())
        {
            message += "● Missing functions: " + Where_clause::join_elements_to_str(missing_functions, ",") + ".\n";
            next_steps.push_back("💡 Add the missing functions: " + Where_clause::join_elements_to_str(missing_functions, ",") + " to your SELECT clause.");
        }
        if (!extra_functions.empty())
        {
            message += "● Extra functions: " + Where_clause::join_elements_to_str(extra_functions, ",") + ".\n";
            next_steps.push_back("💡 Remove the extra functions: " + Where_clause::join_elements_to_str(extra_functions, ",") + " from your SELECT clause.");
        }
    }

    // Compare expressions (order-independent)
    std::vector<std::string> ref_expressions;
    std::vector<std::string> other_expressions;

    // Extract expressions from reference and other
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
        equal = 0;
        message += "\n● Mismatch in selected expressions.\n";

        // Identify missing and extra expressions
        std::vector<std::string> missing_expressions, extra_expressions;
        std::set_difference(ref_expressions.begin(), ref_expressions.end(),
                            other_expressions.begin(), other_expressions.end(),
                            std::back_inserter(missing_expressions));
        std::set_difference(other_expressions.begin(), other_expressions.end(),
                            ref_expressions.begin(), ref_expressions.end(),
                            std::back_inserter(extra_expressions));

        if (!missing_expressions.empty())
        {
            message += "● Missing expressions: ";
            for (const auto &expr : missing_expressions)
                message += expr + ", ";
            message.pop_back();
            message.pop_back();
            message += ".\n";
        }
        if (!extra_expressions.empty())
        {
            message += "● Extra expressions: ";
            for (const auto &expr : extra_expressions)
                message += expr + ", ";
            message.pop_back();
            message.pop_back();
            message += ".\n";
        }
    }

    return std::make_pair(equal, message);
}
void Select_clause::print(const Select_clause::select_clause_info &select)
{
    std::cout << "Select Info: " << std::endl;
    std::cout << "Distinct: " << (select.is_distinct ? "true" : "false") << std::endl;
    std::cout << "Select Columns: ";
    for (auto column : select.the_columns)
    {
        std::cout << "\t" << column.column_name << (column.alias.empty() ? "" : " AS " + column.alias) << std::endl;
    }
    std::cout << "Functions: ";
    for (auto function : select.functions)
    {
        std::cout << "\t" << function << std::endl;
    }

    std::cout << std::endl;
}
