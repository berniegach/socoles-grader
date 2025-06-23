#include "from_clause.h"
#include "../common.h"
#include <iostream>
#include "where_clause.h"
#include <set>

From_clause::table_info From_clause::extract_table_info(const std::shared_ptr<AbstractSyntaxTree::Node> &range_var_node)
{
    From_clause::table_info tab_info;
    int i = 0;
    for (const auto &child : range_var_node->children)
    {
        if (child->key == "relname")
        {
            tab_info.table_name = Common::strip_quotes(child->value);
        }
        else if (child->key == "alias")
        {
            for (const auto &aliasChild : child->children)
            {
                if (aliasChild->key == "aliasname")
                {
                    tab_info.alias = Common::strip_quotes(aliasChild->value);
                }
            }
        }
    }
    return tab_info;
}
std::string From_clause::extract_column_ref(const std::shared_ptr<AbstractSyntaxTree::Node> &column_ref_node, const join_info &info)
{
    std::vector<std::string> fields;
    for (const auto &child : column_ref_node->children)
    {
        if (child->key == "fields")
        {
            for (const auto &field_node : child->children)
            {
                if (field_node->key == "String")
                {
                    for (const auto &sval_node : field_node->children)
                    {
                        if (sval_node->key == "sval")
                        {
                            fields.push_back(Common::strip_quotes(sval_node->value));
                        }
                    }
                }
            }
        }
    }
    // replace the alias with the full table name
    std::string alias_or_table_name;
    std::string column_name;
    if (fields.size() == 2)
    {
        alias_or_table_name = fields[0];
        column_name = fields[1];

        // first check the alias in the left table
        if (info.left_table.alias == alias_or_table_name)
        {
            alias_or_table_name = info.left_table.table_name;
        }
        // then check the alias in the right table
        else if (info.right_table.alias == alias_or_table_name)
        {
            alias_or_table_name = info.right_table.table_name;
        }
    }
    else if (fields.size() == 1)
    {
        column_name = fields[0];
    }

    if (!alias_or_table_name.empty())
    {
        return alias_or_table_name + "." + column_name;
    }
    else
    {
        return column_name;
    }
    // return fields.size() == 2 ? fields[0] + "." + fields[1] : fields[0];
}
std::string From_clause::extract_join_condition(const std::shared_ptr<AbstractSyntaxTree::Node> &quals_node, const join_info &info)
{
    if (quals_node->key == "A_Expr")
    {
        std::string left_expr, operator_, right_expr;
        for (const auto &child : quals_node->children)
        {
            if (child->key == "lexpr")
            {
                left_expr = extract_expression(child->children.front(), info);
            }
            else if (child->key == "rexpr")
            {
                right_expr = extract_expression(child->children.front(), info);
            }
            else if (child->key == "name")
            {
                for (const auto &op_node : child->children)
                {
                    if (op_node->key == "String")
                    {
                        for (const auto &sval_node : op_node->children)
                        {
                            if (sval_node->key == "sval")
                            {
                                operator_ = Common::strip_quotes(sval_node->value);
                            }
                        }
                    }
                }
            }
        }
        return left_expr + " " + operator_ + " " + right_expr;
    }
    return "";
}
std::string From_clause::extract_expression(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const join_info &info)
{
    if (expr_node->key == "ColumnRef")
    {
        return extract_column_ref(expr_node, info);
    }
    // Handle other expression types if necessary
    return "(unknown expression)";
}
From_clause::table_info From_clause::extract_table_or_join(const std::shared_ptr<AbstractSyntaxTree::Node> &node, from_clause_info &info)
{
    if (node->key == "RangeVar")
    {
        return extract_table_info(node);
    }
    else if (node->key == "JoinExpr")
    {
        process_join_expr(node, info);
        return table_info{"(nested join)", ""};
    }
    else if (node->key == "RangeSubselect")
    {
        // Handle subqueries in FROM clause
        table_info subquery_info;
        subquery_info.table_name = "(subquery)";
        auto alias_node = node->get_child("alias");
        if (alias_node)
        {
            subquery_info.alias = alias_node->get_value("aliasname");
        }
        info.tables.push_back(subquery_info);
        return subquery_info;
    }
    else if (node->key == "RangeFunction")
    {
        // Handle functions in FROM clause
        table_info func_info;
        func_info.table_name = "(function)";
        auto alias_node = node->get_child("alias");
        if (alias_node)
        {
            func_info.alias = alias_node->get_value("aliasname");
        }
        info.tables.push_back(func_info);
        return func_info;
    }
    return table_info{"(unknown)", ""};
}

void From_clause::process_join_expr(const std::shared_ptr<AbstractSyntaxTree::Node> &join_expr_node, from_clause_info &info)
{

    join_info join;
    join.join_type = join_expr_node->get_value("jointype");

    // Left argument
    auto larg_node = join_expr_node->get_child("larg");
    if (larg_node)
    {
        join.left_table = extract_table_or_join(larg_node->children.front(), info);
    }

    // Right argument
    auto rarg_node = join_expr_node->get_child("rarg");
    if (rarg_node)
    {
        join.right_table = extract_table_or_join(rarg_node->children.front(), info);
    }

    // Join condition
    auto quals_node = join_expr_node->get_child("quals");
    if (quals_node)
    {
        join.join_condition = extract_join_condition(quals_node->children.front(), join);
    }

    // USING clause
    auto using_clause_node = join_expr_node->get_child("usingClause");
    if (using_clause_node)
    {
        join.has_using_clause = true;
        for (const auto &col_node : using_clause_node->children)
        {
            if (col_node->key == "String")
            {
                join.using_columns.push_back(Common::strip_quotes(col_node->get_child("sval")->value));
            }
        }
    }

    // NATURAL JOIN
    std::string is_natural = join_expr_node->get_value("isNatural");
    if (is_natural == "true")
    {
        join.is_natural = true;
    }

    info.joins.push_back(join);
    // Add tables if not already present
    info.tables.push_back(join.left_table);
    info.tables.push_back(join.right_table);
}

From_clause::from_clause_info From_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    from_clause_info info;
    if (!node)
    {
        return info;
    }
    if (node->key == "withClause")
    {
        // Process Common Table Expressions (CTEs)
        for (const auto &cte_node : node->children)
        {
            if (cte_node->key == "CommonTableExpr")
            {
                std::string cte_name = cte_node->get_value("ctename");
                info.ctes.push_back(cte_name);
            }
        }
    }
    else if (node->key == "fromClause" || node->key == "usingClause")
    {
        for (const auto &child : node->children)
        {
            if (child->key == "JoinExpr")
            {
                process_join_expr(child, info);
            }
            else if (child->key == "RangeVar")
            {
                table_info table = extract_table_info(child);
                info.tables.push_back(table);
            }
        }
    }
    else
    {
        // Recursively process child nodes
        for (const auto &child : node->children)
        {
            auto child_info = get_info(child);
            // Merge child_info into info
            info.tables.insert(info.tables.end(), child_info.tables.begin(), child_info.tables.end());
            info.joins.insert(info.joins.end(), child_info.joins.begin(), child_info.joins.end());
            info.ctes.insert(info.ctes.end(), child_info.ctes.begin(), child_info.ctes.end());
        }
    }
    return info;
}
std::pair<std::string, From_clause::from_clause_info> From_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto from_info = get_info(node);
    std::ostringstream oss;

    // Handle Common Table Expressions (CTEs)
    if (!from_info.ctes.empty())
    {
        oss << "Using Common Table Expressions (CTEs): ";
        for (size_t i = 0; i < from_info.ctes.size(); ++i)
        {
            oss << from_info.ctes[i];
            if (i < from_info.ctes.size() - 1)
            {
                oss << ", ";
            }
        }
        oss << ". ";
    }

    if (!from_info.joins.empty())
    {
        oss << "Identify data sources by performing ";
        size_t join_count = from_info.joins.size();
        for (size_t i = 0; i < join_count; ++i)
        {
            const auto &join = from_info.joins[i];
            // Convert join type to readable format
            std::string join_type;
            if (join.is_natural)
            {
                join_type = "a natural join";
            }
            else if (join.join_type == "JOIN_INNER")
            {
                join_type = "an inner join";
            }
            else if (join.join_type == "JOIN_LEFT")
            {
                join_type = "a left join";
            }
            else if (join.join_type == "JOIN_RIGHT")
            {
                join_type = "a right join";
            }
            else if (join.join_type == "JOIN_FULL")
            {
                join_type = "a full join";
            }
            else if (join.join_type == "JOIN_CROSS")
            {
                join_type = "a cross join";
            }
            else
            {
                join_type = "a join";
            }

            oss << join_type << " between ";
            // Left table
            oss << join.left_table.table_name;
            if (!join.left_table.alias.empty())
            {
                // For now we do nothing with the alias
                // oss << " AS " << join.left_table.alias;
            }
            // Right table
            oss << " and " << join.right_table.table_name;
            if (!join.right_table.alias.empty())
            {
                // For now we do nothing with the alias
                // oss << " AS " << join.right_table.alias;
            }
            // Join condition
            if (join.is_natural)
            {
                oss << " (natural join)";
            }
            else if (join.has_using_clause)
            {
                oss << " using (" << join.using_columns.front();
                for (size_t j = 1; j < join.using_columns.size(); ++j)
                {
                    oss << ", " << join.using_columns[j];
                }
                oss << ")";
            }
            else if (!join.join_condition.empty())
            {
                oss << " on " << join.join_condition;
            }
            else
            {
                std::cout << "No join condition";
            }
            if (i < join_count - 1)
            {
                oss << "; then ";
            }
        }
    }
    else if (!from_info.tables.empty())
    {
        oss << "Identify data source(s): ";
        for (size_t i = 0; i < from_info.tables.size(); ++i)
        {
            const auto &table = from_info.tables[i];
            oss << table.table_name;
            if (!table.alias.empty())
            {
                // For now we do nothing with the alias
                // oss << " AS " << table.alias;
            }
            if (i < from_info.tables.size() - 1)
            {
                oss << ", ";
            }
        }
    }
    else
    {
        oss << "No tables in FROM clause.";
    }
    return std::make_pair(oss.str(), from_info);
    // return oss.str();
}
// Function to compare the FROM clause information
std::pair<int, std::string> From_clause::compare(const from_clause_info &reference, const from_clause_info &other)
{
    std::string message = "";
    bool equal = 1;

    // check if both are empty
    if (reference.tables.empty() && other.tables.empty())
    {
        return std::make_pair(-1, "Both queries have no from clause.");
    }

    // Compare tables (using table_name and ignoring alias)
    std::vector<std::string> ref_tables;
    std::vector<std::string> other_tables;

    for (const auto &table : reference.tables)
    {
        ref_tables.push_back(table.table_name);
    }
    for (const auto &table : other.tables)
    {
        other_tables.push_back(table.table_name);
    }

    // Sort tables for comparison (order may not matter)
    std::sort(ref_tables.begin(), ref_tables.end());
    std::sort(other_tables.begin(), other_tables.end());

    if (ref_tables != other_tables)
    {
        equal = false;
        message += "\n ● Mismatch in FROM clause tables.\n";

        // Identify missing and extra tables
        std::vector<std::string> missing_tables, extra_tables;
        std::set_difference(ref_tables.begin(), ref_tables.end(),
                            other_tables.begin(), other_tables.end(),
                            std::back_inserter(missing_tables));
        std::set_difference(other_tables.begin(), other_tables.end(),
                            ref_tables.begin(), ref_tables.end(),
                            std::back_inserter(extra_tables));

        if (!missing_tables.empty())
        {
            message += "● Missing tables: " + Where_clause::join_elements_to_str(missing_tables, ", ") + ".\n";
        }
        if (!extra_tables.empty())
        {
            message += "● Extra tables: " + Where_clause::join_elements_to_str(extra_tables, ", ") + ".\n";
        }
    }

    // Compare joins
    if (reference.joins.size() != other.joins.size())
    {
        message += "● Joins: Mismatch in the number of joins.\n";
        equal = 0;
    }
    else
    {
        for (size_t i = 0; i < reference.joins.size(); ++i)
        {
            const join_info &ref_join = reference.joins[i];
            const join_info &other_join = other.joins[i];

            // Compare join types
            if (ref_join.join_type != other_join.join_type)
            {
                equal = false;
                message += "● Join " + std::to_string(i + 1) + ": Expected join type '" + ref_join.join_type + "', but found '" + other_join.join_type + "'.\n";
                continue; // Skip further comparison for this join since types differ
            }

            // For INNER JOINs, order of tables does not matter
            if (ref_join.join_type == "JOIN_INNER")
            {
                std::set<std::string> ref_join_tables = {ref_join.left_table.table_name, ref_join.right_table.table_name};
                std::set<std::string> other_join_tables = {other_join.left_table.table_name, other_join.right_table.table_name};

                if (ref_join_tables != other_join_tables)
                {
                    equal = false;
                    message += "● Join " + std::to_string(i + 1) + ": Mismatch in joined tables for INNER JOIN.\n";
                    message += "● Expected tables: " + Where_clause::join_elements_to_str(std::vector<std::string>(ref_join_tables.begin(), ref_join_tables.end()), ", ") + ".\n";
                    message += "● Found tables: " + Where_clause::join_elements_to_str(std::vector<std::string>(other_join_tables.begin(), other_join_tables.end()), ", ") + ".\n";
                }
            }
            else // For LEFT JOIN and RIGHT JOIN, order matters
            {
                if (ref_join.left_table.table_name != other_join.left_table.table_name ||
                    ref_join.right_table.table_name != other_join.right_table.table_name)
                {
                    equal = false;
                    message += "● Join " + std::to_string(i + 1) + ": Mismatch in joined tables for " + ref_join.join_type + ".\n";
                    message += "● Expected tables: " + ref_join.left_table.table_name + ", " + ref_join.right_table.table_name + ".\n";
                    message += "● Found tables: " + other_join.left_table.table_name + ", " + other_join.right_table.table_name + ".\n";
                }
            }

            // Compare join conditions (order may matter)
            if (ref_join.join_condition != other_join.join_condition)
            {
                equal = false;
                message += "● Join " + std::to_string(i + 1) + ": Mismatch in join condition.\n";
                message += "● Expected condition: " + ref_join.join_condition + ".\n";
                message += "● Found condition: " + other_join.join_condition + ".\n";
            }
        }
    }

    // Compare CTEs (order-independent)
    std::vector<std::string> ref_ctes = reference.ctes;
    std::vector<std::string> other_ctes = other.ctes;
    std::sort(ref_ctes.begin(), ref_ctes.end());
    std::sort(other_ctes.begin(), other_ctes.end());

    if (ref_ctes != other_ctes)
    {
        equal = false;
        message += "● Mismatch in common table expressions (CTEs).\n";

        // Identify missing and extra CTEs
        std::vector<std::string> missing_ctes, extra_ctes;
        std::set_difference(ref_ctes.begin(), ref_ctes.end(),
                            other_ctes.begin(), other_ctes.end(),
                            std::back_inserter(missing_ctes));
        std::set_difference(other_ctes.begin(), other_ctes.end(),
                            ref_ctes.begin(), ref_ctes.end(),
                            std::back_inserter(extra_ctes));

        if (!missing_ctes.empty())
        {
            message += "● Missing CTEs: ";
            for (const auto &cte : missing_ctes)
                message += cte + ", ";
            message.pop_back();
            message.pop_back();
            message += ".\n";
        }
        if (!extra_ctes.empty())
        {
            message += "● Extra CTEs: ";
            for (const auto &cte : extra_ctes)
                message += cte + ", ";
            message.pop_back();
            message.pop_back();
            message += ".\n";
        }
    }

    return std::make_pair(equal, message);
}
void From_clause::print(const From_clause::from_clause_info &from_info)
{
    std::cout << "From Info: " << std::endl;

    // Print tables
    if (!from_info.tables.empty())
    {
        std::cout << "Tables:\n";
        for (const auto &table : from_info.tables)
        {
            std::cout << "  Table Name: " << table.table_name;
            if (!table.alias.empty())
            {
                std::cout << " (Alias: " << table.alias << ")";
            }
            std::cout << "\n";
        }
    }

    // Print joins
    if (!from_info.joins.empty())
    {
        std::cout << "Joins:\n";
        for (const auto &join : from_info.joins)
        {
            std::cout << "  Join Type: " << join.join_type << "\n";
            std::cout << "    Left Table: " << join.left_table.table_name;
            if (!join.left_table.alias.empty())
            {
                std::cout << " (Alias: " << join.left_table.alias << ")";
            }
            std::cout << "\n";

            std::cout << "    Right Table: " << join.right_table.table_name;
            if (!join.right_table.alias.empty())
            {
                std::cout << " (Alias: " << join.right_table.alias << ")";
            }
            std::cout << "\n";

            if (join.is_natural)
            {
                std::cout << "    Natural Join\n";
            }

            if (join.has_using_clause)
            {
                std::cout << "    Using Clause: ";
                for (size_t i = 0; i < join.using_columns.size(); ++i)
                {
                    std::cout << join.using_columns[i];
                    if (i < join.using_columns.size() - 1)
                    {
                        std::cout << ", ";
                    }
                }
                std::cout << "\n";
            }
            else if (!join.join_condition.empty())
            {
                std::cout << "    Join Condition: " << join.join_condition << "\n";
            }
        }
    }
    // Print CTEs
    if (!from_info.ctes.empty())
    {
        std::cout << "Common Table Expressions (CTEs):\n";
        for (const auto &cte : from_info.ctes)
        {
            std::cout << "  " << cte << "\n";
        }
    }
}
std::string From_clause::from_clause_info_to_string(const From_clause::from_clause_info &info)
{
    std::ostringstream oss;
    if (!info.ctes.empty())
    {
        oss << "CTEs: ";
        for (size_t i = 0; i < info.ctes.size(); ++i)
        {
            oss << info.ctes[i];
            if (i < info.ctes.size() - 1)
                oss << ", ";
        }
        oss << ". ";
    }
    if (!info.joins.empty())
    {
        oss << "Joins: ";
        for (size_t i = 0; i < info.joins.size(); ++i)
        {
            const auto &join = info.joins[i];
            oss << join.join_type << " between "
                << join.left_table.table_name << " and " << join.right_table.table_name;
            if (i < info.joins.size() - 1)
                oss << "; ";
        }
        oss << ". ";
    }
    if (!info.tables.empty())
    {
        oss << "Tables: ";
        for (size_t i = 0; i < info.tables.size(); ++i)
        {
            oss << info.tables[i].table_name;
            if (i < info.tables.size() - 1)
                oss << ", ";
        }
        oss << ".";
    }
    return oss.str();
}
