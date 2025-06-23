#include "insert_clause.h"
#include "select/where_clause.h"
#include "select/from_clause.h"
#include <iostream>

Insert_clause::insert_clause_info Insert_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    insert_clause_info info;
    if (!node)
        return info;

    if (node->key == "InsertStmt")
    {
        // Extract target table name
        std::shared_ptr<AbstractSyntaxTree::Node> rel_node = node->get_child("relation");
        if (!rel_node)
        {
            for (const auto &child : node->children)
            {
                if (child->key.find("rel") != std::string::npos || child->key.find("table") != std::string::npos)
                {
                    rel_node = child;
                    break;
                }
            }
        }
        if (rel_node)
        {
            info.table_name = Common::strip_quotes(rel_node->get_value("relname"));
            if (info.table_name.empty())
            {
                for (const auto &child : rel_node->children)
                {
                    if (child->key == "String")
                    {
                        info.table_name = Common::strip_quotes(child->get_value("sval"));
                        break;
                    }
                }
            }
        }

        // Extract target columns
        std::shared_ptr<AbstractSyntaxTree::Node> cols_node = node->get_child("cols");
        if (cols_node)
        {
            for (const auto &col_node : cols_node->children)
            {
                if (col_node->key == "ResTarget")
                {
                    std::string col_name = col_node->get_value("name");
                    col_name = Common::strip_quotes(col_name);
                    if (!col_name.empty())
                        info.columns.push_back(col_name);
                }
            }
        }

        // Check if this is an INSERT...SELECT statement.
        std::shared_ptr<AbstractSyntaxTree::Node> select_stmt_wrapper = node->get_child("selectStmt");
        if (select_stmt_wrapper)
        {
            info.is_select_insert = true;
            // Navigate into the nested SELECT.
            if (!select_stmt_wrapper->children.empty())
            {
                auto select_stmt_node = select_stmt_wrapper->children.front();
                std::shared_ptr<AbstractSyntaxTree::Node> values_lists_node = select_stmt_node->get_child("valuesLists");
                if (values_lists_node)
                {
                    for (const auto &list_node : values_lists_node->children)
                    {
                        if (list_node->key == "List")
                        {
                            std::shared_ptr<AbstractSyntaxTree::Node> items_node = list_node->get_child("items");
                            if (items_node)
                            {
                                std::vector<std::string> row;
                                for (const auto &item : items_node->children)
                                {
                                    if (item->key == "SetToDefault")
                                    {
                                        row.push_back("DEFAULT");
                                    }
                                    else if (item->key == "A_Const")
                                    {
                                        std::string val = Where_clause::extract_constant(item);
                                        row.push_back(val);
                                    }
                                    else
                                    {
                                        std::string val = Where_clause::extract_expression(item, From_clause::from_clause_info(), Select_clause::select_clause_info());
                                        row.push_back(val);
                                    }
                                }
                                if (!row.empty())
                                    info.values.push_back(row);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            std::shared_ptr<AbstractSyntaxTree::Node> values_node = node->get_child("valuesClause");
            if (!values_node)
                values_node = node->get_child("values");
            if (values_node)
            {
                for (const auto &row_node : values_node->children)
                {
                    std::vector<std::string> row_values;
                    for (const auto &val_node : row_node->children)
                    {
                        if (val_node->key == "SetToDefault")
                        {
                            row_values.push_back("DEFAULT");
                        }
                        else if (val_node->key == "A_Const")
                        {
                            std::string val = Where_clause::extract_constant(val_node);
                            row_values.push_back(val);
                        }
                        else
                        {
                            std::string val = Where_clause::extract_expression(val_node, From_clause::from_clause_info(), Select_clause::select_clause_info());
                            row_values.push_back(val);
                        }
                    }
                    if (!row_values.empty())
                        info.values.push_back(row_values);
                }
            }
        }
    }
    return info;
}

std::pair<std::string, Insert_clause::insert_clause_info> Insert_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;
    if (!info.table_name.empty())
    {

        oss << "Insert into table '" << info.table_name << "'. ";
        if (!info.columns.empty())
        {
            oss << "Target columns: ";
            for (size_t i = 0; i < info.columns.size(); ++i)
            {
                oss << "'" << info.columns[i] << "'";
                if (i < info.columns.size() - 1)
                {
                    oss << ", ";
                }
            }
            oss << ". ";
        }
        if (info.is_select_insert)
        {
            if (!info.values.empty())
            {
                oss << "Insert values: ";
                for (size_t r = 0; r < info.values.size(); r++)
                {
                    oss << "(";

                    for (size_t c = 0; c < info.values[r].size(); c++)
                    {
                        oss << info.values[r][c];
                        if (c < info.values[r].size() - 1)
                            oss << ", ";
                    }
                    oss << ")";
                    if (r < info.values.size() - 1)
                        oss << "; ";
                }
            }
            else
            {
                oss << "No values provided.";
            }
        }
        else if (!info.values.empty())
        {
            if (info.values.size() == 1)
            {
                oss << "Insert values: (";
                for (size_t i = 0; i < info.values[0].size(); ++i)
                {
                    oss << info.values[0][i];
                    if (i < info.values[0].size() - 1)
                        oss << ", ";
                }
                oss << ").";
            }
            else
            {
                oss << "Insert " << info.values.size() << " rows. First row: (";
                for (size_t i = 0; i < info.values[0].size(); ++i)
                {
                    oss << info.values[0][i];
                    if (i < info.values[0].size() - 1)
                        oss << ", ";
                }
                oss << ").";
            }
        }
        else
        {
            oss << "No values provided.";
        }
        std::cout << oss.str() << std::endl;
    }
    else
    {
        oss << "No INSERT statement found.";
    }
    return std::make_pair(oss.str(), info);
}
Common::comparision_result Insert_clause::compare(const insert_clause_info &reference, const insert_clause_info &other)
{
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // Compare target table names.
    if (reference.table_name == other.table_name)
    {
        correct_parts.push_back("Target table");
    }
    else
    {
        incorrect_parts.push_back("Target table");
        message << "● Target table mismatch: expected '" << reference.table_name << "', but found '" << other.table_name << "'.\n";
        next_steps.push_back("💡 Ensure the INSERT targets the correct table.");
        equal = false;
    }

    // --- Compare column lists ---
    {
        std::vector<std::string> ref_columns = reference.columns;
        std::vector<std::string> stu_columns = other.columns;
        std::sort(ref_columns.begin(), ref_columns.end());
        std::sort(stu_columns.begin(), stu_columns.end());
        if (ref_columns == stu_columns)
        {
            correct_parts.push_back("Target columns");
        }
        else
        {
            incorrect_parts.push_back("Target columns");
            std::vector<std::string> missing_columns, extra_columns;
            std::set_difference(ref_columns.begin(), ref_columns.end(),
                                stu_columns.begin(), stu_columns.end(),
                                std::back_inserter(missing_columns));
            std::set_difference(stu_columns.begin(), stu_columns.end(),
                                ref_columns.begin(), ref_columns.end(),
                                std::back_inserter(extra_columns));
            if (!missing_columns.empty())
            {
                message << "● Missing columns: " << Where_clause::join_elements_to_str(missing_columns, ", ") << ".\n";
                next_steps.push_back("💡 Add the missing columns to the INSERT statement.");
            }
            if (!extra_columns.empty())
            {
                message << "● Extra columns: " << Where_clause::join_elements_to_str(extra_columns, ", ") << ".\n";
                next_steps.push_back("💡 Remove the extra columns from the INSERT statement.");
            }
            equal = false;
        }
    }

    // --- Compare the INSERT style and values ---
    if (reference.is_select_insert != other.is_select_insert)
    {
        incorrect_parts.push_back("INSERT style");
        message << "● Mismatch in INSERT style: one query uses INSERT...SELECT while the other uses INSERT...VALUES.\n";
        {
            std::string refStyle = reference.is_select_insert ? "INSERT...SELECT" : "INSERT...VALUES";
            next_steps.push_back("💡 Use the " + refStyle + " style.");
        }
        equal = false;
    }
    else
    {
        if (!reference.is_select_insert)
        {
            // Both use INSERT...VALUES.
            if (reference.values.size() != other.values.size())
            {
                incorrect_parts.push_back("Number of rows");
                message << "● Mismatch in number of rows: expected " << reference.values.size() << ", found " << other.values.size() << ".\n";
                next_steps.push_back("💡 Ensure the correct number of rows are inserted.");
                equal = false;
            }
            else
            {
                // Compare each row's values.
                for (size_t i = 0; i < reference.values.size(); ++i)
                {
                    const auto &ref_row = reference.values[i];
                    const auto &stu_row = other.values[i];
                    if (ref_row.size() != stu_row.size())
                    {
                        incorrect_parts.push_back("Row " + std::to_string(i + 1) + " values");
                        message << "● Row " << i + 1 << " has different number of values: expected "
                                << ref_row.size() << ", found " << stu_row.size() << ".\n";
                        next_steps.push_back("💡 Check row " + std::to_string(i + 1) + " for correct number of values.");
                        equal = false;
                    }
                    else
                    {
                        for (size_t j = 0; j < ref_row.size(); ++j)
                        {
                            // If the reference value is "*", accept anything here.
                            if (ref_row[j] == "*")
                            {
                                correct_parts.push_back("Row " + std::to_string(i + 1) + ", column " + std::to_string(j + 1) + " (any)");
                            }
                            else if (ref_row[j] != stu_row[j])
                            {
                                incorrect_parts.push_back("Row " + std::to_string(i + 1) + ", column " + std::to_string(j + 1));
                                message << "● Mismatch at row " << i + 1 << ", column " << j + 1 << ": expected '"
                                        << ref_row[j] << "', found '" << stu_row[j] << "'.\n";
                                if (stu_row.size() > ref_row.size())
                                {
                                    message << "● Extra value(s): ";
                                    for (size_t k = ref_row.size(); k < stu_row.size(); ++k)
                                    {
                                        message << "'" << stu_row[k] << "' ";
                                    }
                                    message << ".\n";
                                    next_steps.push_back("💡 Remove the extra value(s) at row " + std::to_string(i + 1) + ".");
                                }
                                else
                                {
                                    message << "● Missing value(s): ";
                                    for (size_t k = stu_row.size(); k < ref_row.size(); ++k)
                                    {
                                        message << "'" << ref_row[k] << "' ";
                                    }
                                    message << ".\n";
                                    next_steps.push_back("💡 Add the missing value(s) at row " + std::to_string(i + 1) + ".");
                                }
                                equal = false;
                            }
                            else
                            {
                                // Exact match
                                correct_parts.push_back("Row " + std::to_string(i + 1) + ", column " + std::to_string(j + 1));
                            }
                        }
                    }
                }
                if (incorrect_parts.empty())
                {
                    correct_parts.push_back("Inserted values");
                }
            }
        }
        else
        {
            // Both use INSERT...SELECT.
            if (reference.values.empty() || other.values.empty())
            {
                incorrect_parts.push_back("Inserted values");
                message << "● Could not find values.\n";
                next_steps.push_back("💡 Ensure the values are specified.");
                equal = false;
            }
            else
            {
                if (reference.values.size() != other.values.size())
                {
                    incorrect_parts.push_back("Number of rows.");
                    message << "● Mismatch in number of rows: expected " << reference.values.size()
                            << ", found " << other.values.size() << ".\n";
                    next_steps.push_back("💡 Ensure the correct number of rows are inserted.");
                    equal = false;
                }
                else
                {
                    for (size_t i = 0; i < reference.values.size(); ++i)
                    {
                        const auto &ref_row = reference.values[i];
                        const auto &stu_row = other.values[i];
                        if (ref_row.size() != stu_row.size())
                        {
                            incorrect_parts.push_back("Row " + std::to_string(i + 1) + " values");
                            message << "● Row " << i + 1 << " has different number of values: expected "
                                    << ref_row.size() << ", found " << stu_row.size() << ".\n";
                            if (stu_row.size() > ref_row.size())
                            {
                                message << "● Extra value(s): ";
                                for (size_t k = ref_row.size(); k < stu_row.size(); ++k)
                                {
                                    message << "'" << stu_row[k] << "' ";
                                }
                                message << ".\n";
                                next_steps.push_back("💡 Remove the extra value(s) at row " + std::to_string(i + 1) + ".");
                            }
                            else
                            {
                                message << "● Missing value(s): ";
                                for (size_t k = stu_row.size(); k < ref_row.size(); ++k)
                                {
                                    message << "'" << ref_row[k] << "' ";
                                }
                                message << ".\n";
                                next_steps.push_back("💡 Add the missing value(s) at row " + std::to_string(i + 1) + ".");
                            }
                            equal = false;
                        }
                        else
                        {
                            for (size_t j = 0; j < ref_row.size(); ++j)
                            {
                                if (ref_row[j] != stu_row[j])
                                {
                                    std::string col;
                                    if (j < reference.columns.size())
                                        col = reference.columns[j];
                                    else
                                        col = "column " + std::to_string(j + 1);
                                    incorrect_parts.push_back("Row " + std::to_string(i + 1) + ": " + col);
                                    message << "● Mismatch in values at row " << i + 1 << ", " << col << ": expected '"
                                            << ref_row[j] << "', found '" << stu_row[j] << "'.\n";
                                    next_steps.push_back("💡 Fix the value at row " + std::to_string(i + 1) + ": " + col + ".");
                                    equal = false;
                                }
                            }
                        }
                    }
                    if (incorrect_parts.empty())
                    {
                        correct_parts.push_back("Inserted values");
                    }
                }
            }
        }
    }

    Common::comparision_result comp;
    comp.equal = equal;
    comp.correct_parts = correct_parts;
    comp.incorrect_parts = incorrect_parts;
    comp.next_steps = next_steps;
    comp.message = message.str();
    return comp;
}
