#include "create_clause.h"
#include "common.h"
#include <iostream>

Create_clause::create_clause_info Create_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    create_clause_info info;
    if (!node)
        return info;

    if (node->key == "CreateStmt")
    {
        info.object_type = "table";
    }
    else
    {
        std::cout << node->key << std::endl;
        return info;
    }

    // Extract object (table/view) name
    auto relation_node = node->get_child("relation");
    if (relation_node)
    {
        info.object_name = Common::strip_quotes(relation_node->get_value("relname"));
    }

    // For CREATE TABLE, extract column and constraint definitions.
    if (info.object_type == "table")
    {
        auto tableElts_node = node->get_child("tableElts");
        if (tableElts_node)
        {
            for (auto &elt : tableElts_node->children)
            {
                if (elt->key == "ColumnDef")
                {
                    create_clause_info::column_definition col;
                    col.name = Common::strip_quotes(elt->get_value("colname"));

                    // Extract type info from typeName
                    auto typeName_node = elt->get_child("typeName");
                    if (typeName_node)
                    {
                        // Usually varchar or int is under names:
                        auto names_node = typeName_node->get_child("names");
                        if (names_node && !names_node->children.empty())
                        {
                            // Last String node often has the actual type name
                            auto last_name_node = names_node->children.back();
                            if (last_name_node->key == "String")
                            {
                                col.type = Common::strip_quotes(last_name_node->get_child("sval")->value);
                            }
                        }
                    }

                    // Check constraints in ColumnDef for NOT NULL
                    auto constraints_node = elt->get_child("constraints");
                    if (constraints_node)
                    {
                        for (auto &cst : constraints_node->children)
                        {
                            if (cst->key == "Constraint")
                            {
                                std::string contype = cst->get_value("contype");
                                if (contype == "CONSTR_NOTNULL")
                                {
                                    col.not_null = true;
                                }
                                else if (contype == "CONSTR_PRIMARY")
                                {
                                    // If a column-level primary key is defined, add it to the primary key list.
                                    info.primary_key_columns.push_back(col.name);
                                }
                                else if (contype == "CONSTR_CHECK")
                                {
                                    auto raw_expr_node = cst->get_child("raw_expr");
                                    if (raw_expr_node && !raw_expr_node->children.empty())
                                    {
                                        // ALWAYS extract from the BoolExpr child
                                        auto cond_node = Where_clause::extract_condition(
                                            raw_expr_node->children.front(),
                                            From_clause::from_clause_info(),
                                            Select_clause::select_clause_info());
                                        if (cond_node)
                                        {
                                            std::string check_expr = Where_clause::condition_to_string(cond_node);
                                            info.check_condition_nodes.push_back(cond_node);
                                            info.check_constraints.push_back("CHECK (" + check_expr + ")");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    info.columns.push_back(col);
                }
                else if (elt->key == "Constraint")
                {
                    // Handle table-level constraints (e.g., PRIMARY KEY)
                    std::string contype = elt->get_value("contype");
                    if (contype == "CONSTR_PRIMARY")
                    {
                        // Extract primary key columns
                        auto keys_node = elt->get_child("keys");
                        if (keys_node)
                        {
                            for (auto &key_child : keys_node->children)
                            {
                                if (key_child->key == "String")
                                {
                                    std::string pk_col = Common::strip_quotes(key_child->get_child("sval")->value);
                                    info.primary_key_columns.push_back(pk_col);
                                }
                            }
                        }
                    }
                    else if (contype == "CONSTR_FOREIGN")
                    {
                        create_clause_info::foreign_key_constraint fk;
                        // support both fk_cols and fk_attrs
                        auto fk_cols_node = elt->get_child("fk_cols");
                        if (!fk_cols_node)
                            fk_cols_node = elt->get_child("fk_attrs");
                        if (fk_cols_node)
                        {
                            for (auto &child : fk_cols_node->children)
                                if (child->key == "String")
                                    fk.local_columns.push_back(Common::strip_quotes(child->get_child("sval")->value));
                        }
                        // Extract referenced table from "pktable".
                        auto pktable_node = elt->get_child("pktable");
                        if (pktable_node)
                        {
                            fk.referenced_table = Common::strip_quotes(pktable_node->get_value("relname"));
                        }
                        // support both pk_cols and pk_attrs
                        auto pk_cols_node = elt->get_child("pk_cols");
                        if (!pk_cols_node)
                            pk_cols_node = elt->get_child("pk_attrs");
                        if (pk_cols_node)
                        {
                            for (auto &child : pk_cols_node->children)
                                if (child->key == "String")
                                    fk.referenced_columns.push_back(Common::strip_quotes(child->get_child("sval")->value));
                        }
                        info.foreign_key_constraints.push_back(fk);
                    }
                    else if (contype == "CONSTR_UNIQUE")
                    {
                        std::string unique_info = "UNIQUE (";
                        std::vector<std::string> unique_columns;
                        auto keys_node = elt->get_child("keys");
                        if (keys_node)
                        {
                            for (auto &child : keys_node->children)
                            {
                                if (child->key == "String")
                                {
                                    unique_columns.push_back(Common::strip_quotes(child->get_child("sval")->value));
                                }
                            }
                        }
                        for (size_t i = 0; i < unique_columns.size(); i++)
                        {
                            unique_info += unique_columns[i];
                            if (i < unique_columns.size() - 1)
                                unique_info += ", ";
                        }
                        unique_info += ")";
                        info.unique_constraints.push_back(unique_info);
                    }
                    else if (contype == "CONSTR_CHECK")
                    {
                        // use ConditionNode tree to stringify BoolExpr
                        std::string check_expr;
                        auto raw_expr_node = elt->get_child("raw_expr");
                        if (raw_expr_node && !raw_expr_node->children.empty())
                        {
                            //  Always extract a ConditionNode from the first child (BoolExpr / A_Expr)
                            auto cond_node = Where_clause::extract_condition(raw_expr_node->children.front(), From_clause::from_clause_info(), Select_clause::select_clause_info());
                            if (cond_node)
                            {
                                // store both the string and the node
                                check_expr = Where_clause::condition_to_string(cond_node);
                                info.check_condition_nodes.push_back(cond_node);
                            }
                            else
                            {
                                // (optional) fallback if something really went wrong
                                check_expr = Where_clause::extract_expression(raw_expr_node->children.front(), From_clause::from_clause_info(), Select_clause::select_clause_info());
                            }
                        }
                        info.check_constraints.push_back("CHECK (" + check_expr + ")");
                    }
                }
            }
        }
    }

    return info;
}
std::pair<std::string, Create_clause::create_clause_info> Create_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;

    if (!info.object_name.empty())
    {
        oss << "Create a " << info.object_type << " named '" << info.object_name << "'. ";
        if (!info.columns.empty())
        {
            oss << "Columns: ";
            for (size_t i = 0; i < info.columns.size(); ++i)
            {
                oss << info.columns[i].name << " " << info.columns[i].type;
                if (info.columns[i].not_null)
                    oss << " NOT NULL";
                if (i < info.columns.size() - 2)
                    oss << ", ";
                else if (i == info.columns.size() - 2)
                    oss << " and ";
            }
            oss << ". ";
        }
        if (!info.primary_key_columns.empty())
        {
            oss << "Primary key on (";
            for (size_t i = 0; i < info.primary_key_columns.size(); ++i)
            {
                oss << info.primary_key_columns[i];
                if (i < info.primary_key_columns.size() - 1)
                    oss << ", ";
            }
            oss << "). ";
        }
        if (!info.foreign_key_constraints.empty())
        {
            oss << "Foreign keys: ";
            for (size_t i = 0; i < info.foreign_key_constraints.size(); ++i)
            {
                auto &fk = info.foreign_key_constraints[i];
                oss << "FOREIGN KEY (";
                for (size_t j = 0; j < fk.local_columns.size(); ++j)
                {
                    oss << fk.local_columns[j];
                    if (j < fk.local_columns.size() - 1)
                        oss << ", ";
                }
                oss << ") REFERENCES " << fk.referenced_table << " (";
                for (size_t j = 0; j < fk.referenced_columns.size(); ++j)
                {
                    oss << fk.referenced_columns[j];
                    if (j < fk.referenced_columns.size() - 1)
                        oss << ", ";
                }
                oss << ")";
                if (i < info.foreign_key_constraints.size() - 1)
                    oss << "; ";
            }
            oss << ". ";
        }
        if (!info.unique_constraints.empty())
        {
            oss << "Unique constraints: ";
            for (size_t i = 0; i < info.unique_constraints.size(); ++i)
            {
                oss << info.unique_constraints[i];
                if (i < info.unique_constraints.size() - 1)
                    oss << "; ";
            }
            oss << ". ";
        }
        if (!info.check_constraints.empty())
        {
            oss << "Check constraints: ";
            for (size_t i = 0; i < info.check_constraints.size(); ++i)
            {
                oss << info.check_constraints[i];
                if (i < info.check_constraints.size() - 1)
                    oss << "; ";
            }
            oss << ". ";
        }
    }
    else
    {
        oss << "No CREATE statement found.";
    }

    return std::make_pair(oss.str(), info);
}
Common::comparision_result Create_clause::compare(const create_clause_info &reference, const create_clause_info &other)
{
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // --- Compare table (object) name ---
    if (reference.object_name == other.object_name)
    {
        correct_parts.push_back("Table name");
    }
    else
    {
        incorrect_parts.push_back("Table name");
        message << "● The table name should be '" << reference.object_name << "', but your query uses '" << other.object_name << "'.\n";
        next_steps.push_back("💡 Change the table name to '" + reference.object_name + "'.");
        equal = false;
    }

    // --- Compare column names ---
    std::vector<std::string> refColNames;
    std::vector<std::string> stuColNames;
    for (const auto &col : reference.columns)
    {
        refColNames.push_back(col.name);
    }
    for (const auto &col : other.columns)
    {
        stuColNames.push_back(col.name);
    }
    std::sort(refColNames.begin(), refColNames.end());
    std::sort(stuColNames.begin(), stuColNames.end());

    if (refColNames == stuColNames)
    {
        if (!refColNames.empty())
        {
            correct_parts.push_back("Column names");
        }
    }
    else
    {
        incorrect_parts.push_back("Column names");
        std::vector<std::string> missing, extra;
        std::set_difference(refColNames.begin(), refColNames.end(),
                            stuColNames.begin(), stuColNames.end(),
                            std::back_inserter(missing));
        std::set_difference(stuColNames.begin(), stuColNames.end(),
                            refColNames.begin(), refColNames.end(),
                            std::back_inserter(extra));
        if (!missing.empty())
        {
            message << "● Your CREATE statement is missing these column(s): " << Where_clause::join_elements_to_str(missing, ", ") << ".\n";
            next_steps.push_back("💡 Add the missing column(s): " + Where_clause::join_elements_to_str(missing, ", ") + ".");
        }
        if (!extra.empty())
        {
            message << "● Your CREATE statement includes extra column(s): " << Where_clause::join_elements_to_str(extra, ", ") << ".\n";
            next_steps.push_back("💡 Remove the extra column(s): " + Where_clause::join_elements_to_str(extra, ", ") + ".");
        }
        equal = false;
    }

    // --- Compare column types and NOT NULL constraints ---
    for (const auto &refCol : reference.columns)
    {
        // Look for a matching column by name in the student query.
        auto it = std::find_if(other.columns.begin(), other.columns.end(),
                               [&refCol](const create_clause_info::column_definition &col)
                               {
                                   return col.name == refCol.name;
                               });
        if (it != other.columns.end())
        {
            // Compare data types.
            if (refCol.type != it->type)
            {
                incorrect_parts.push_back("Type of '" + refCol.name + "'");
                message << "● The type for column '" << refCol.name << "' should be '" << refCol.type << "', but you have '" << it->type << "'.\n";
                next_steps.push_back("💡 Change the type of column '" + refCol.name + "' to '" + refCol.type + "'.");
                equal = false;
            }
            // Compare NOT NULL constraint.
            if (refCol.not_null != it->not_null)
            {
                incorrect_parts.push_back("NOT NULL constraint for '" + refCol.name + "'");
                message << "● Column '" << refCol.name << "' should " << (refCol.not_null ? "have" : "not have")
                        << " a NOT NULL constraint, but your statement " << (it->not_null ? "has" : "does not have") << " it.\n";
                next_steps.push_back("💡 Adjust the NOT NULL constraint for column '" + refCol.name + "'.");
                equal = false;
            }
        }
    }

    // --- Compare primary key definitions ---
    std::vector<std::string> refPK = reference.primary_key_columns;
    std::vector<std::string> stuPK = other.primary_key_columns;
    std::sort(refPK.begin(), refPK.end());
    std::sort(stuPK.begin(), stuPK.end());
    if (refPK == stuPK)
    {
        if (!refPK.empty())
        {
            correct_parts.push_back("Primary key");
        }
    }
    else
    {
        incorrect_parts.push_back("Primary key");
        // Special case: reference has a composite PK, student only a single-column PK
        if (refPK.size() > 1 && stuPK.size() == 1)
        {
            message << "● Your statement defines a primary key only on '"
                    << stuPK[0]
                    << "', but the requirement is a composite primary key on ("
                    << Where_clause::join_elements_to_str(refPK, ", ") << ").\n";
            next_steps.push_back("💡 Define a composite primary key on (" + Where_clause::join_elements_to_str(refPK, ", ") + ").");
        }
        else
        {
            // the existing missing/extra logic
            std::vector<std::string> missingPK, extraPK;
            std::set_difference(refPK.begin(), refPK.end(),
                                stuPK.begin(), stuPK.end(),
                                std::back_inserter(missingPK));
            std::set_difference(stuPK.begin(), stuPK.end(),
                                refPK.begin(), refPK.end(),
                                std::back_inserter(extraPK));
            if (!missingPK.empty())
            {
                message << "● Missing primary key column(s): "
                        << Where_clause::join_elements_to_str(missingPK, ", ") << ".\n";
                next_steps.push_back("💡 Include the missing primary key column(s): " + Where_clause::join_elements_to_str(missingPK, ", ") + ".");
            }
            if (!extraPK.empty())
            {
                message << "● Extra primary key column(s): "
                        << Where_clause::join_elements_to_str(extraPK, ", ") << ".\n";
                next_steps.push_back("💡 Remove the extra primary key column(s): " + Where_clause::join_elements_to_str(extraPK, ", ") + ".");
            }
        }
        equal = false;
    }
    // Compare foreign key constraints.
    {
        bool check_ok = true;
        // Special case: reference has no FOREIGN KEYs, but student does
        if (reference.foreign_key_constraints.empty() && !other.foreign_key_constraints.empty())
        {
            incorrect_parts.push_back("Foreign keys");
            message << "● Unexpected FOREIGN KEY constraint(s) found.\n";
            next_steps.push_back("💡Remove the FOREIGN KEY constraint definitions.");
            check_ok = false;
            equal = false;
        }
        else if (reference.foreign_key_constraints.size() != other.foreign_key_constraints.size())
        {
            incorrect_parts.push_back("Foreign keys");
            message << "● Expected " << reference.foreign_key_constraints.size()
                    << " foreign key constraint(s), but found " << other.foreign_key_constraints.size() << ".\n";
            next_steps.push_back("💡Review your FOREIGN KEY definitions.");
            check_ok = false;
            equal = false;
        }
        else
        {
            for (size_t i = 0; i < reference.foreign_key_constraints.size(); ++i)
            {
                const auto &ref_fk = reference.foreign_key_constraints[i];
                const auto &stu_fk = other.foreign_key_constraints[i];

                // Compare local columns.
                if (ref_fk.local_columns != stu_fk.local_columns)
                {
                    incorrect_parts.push_back("Foreign keys");
                    message << "● For FOREIGN KEY constraint " << (i + 1) << ", expected local column(s): ("
                            << Where_clause::join_elements_to_str(ref_fk.local_columns, ", ")
                            << ") but found: (" << Where_clause::join_elements_to_str(stu_fk.local_columns, ", ") << ").\n";
                    next_steps.push_back("💡Adjust the local columns in your FOREIGN KEY definition " + std::to_string(i + 1) + ".");
                    check_ok = false;
                    equal = false;
                }
                // Compare referenced table.
                if (ref_fk.referenced_table != stu_fk.referenced_table)
                {
                    incorrect_parts.push_back("Foreign keys");
                    message << "● For FOREIGN KEY constraint " << (i + 1) << ", expected referenced table '"
                            << ref_fk.referenced_table << "' but found '" << stu_fk.referenced_table << "'.\n";
                    next_steps.push_back("💡Adjust the referenced table in your FOREIGN KEY definition " + std::to_string(i + 1) + ".");
                    check_ok = false;
                    equal = false;
                }
                // Compare referenced columns.
                if (ref_fk.referenced_columns != stu_fk.referenced_columns)
                {
                    incorrect_parts.push_back("Foreign keys");
                    message << "● For FOREIGN KEY constraint " << (i + 1) << ", expected referenced column(s): ("
                            << Where_clause::join_elements_to_str(ref_fk.referenced_columns, ", ")
                            << ") but found: (" << Where_clause::join_elements_to_str(stu_fk.referenced_columns, ", ") << ").\n";
                    next_steps.push_back("💡Adjust the referenced columns in your FOREIGN KEY definition " + std::to_string(i + 1) + ".");
                    check_ok = false;
                    equal = false;
                }
            }
            if (check_ok)
            {
                if (!reference.foreign_key_constraints.empty())
                {
                    correct_parts.push_back("Foreign keys");
                }
            }
        }
    }
    // Compare unique constraints.
    {
        bool check_ok = true;

        // Special case: reference has no UNIQUE, but student does
        if (reference.unique_constraints.empty() && !other.unique_constraints.empty())
        {
            incorrect_parts.push_back("Unique constraints");
            message << "● Unexpected UNIQUE constraint(s) found.\n";
            next_steps.push_back("💡Remove the UNIQUE constraint definitions.");
            check_ok = false;
            equal = false;
        }
        else if (reference.unique_constraints.size() != other.unique_constraints.size())
        {
            incorrect_parts.push_back("Unique constraints");
            message << "● Expected " << reference.unique_constraints.size() << " UNIQUE constraint(s), but found "
                    << other.unique_constraints.size() << ".\n";
            next_steps.push_back("💡Review your UNIQUE constraint definitions.");
            check_ok = false;
            equal = false;
        }
        else
        {
            for (size_t i = 0; i < reference.unique_constraints.size(); ++i)
            {
                // Assume constraints are in the format "UNIQUE (col1, col2, ...)"
                std::string refConstraint = reference.unique_constraints[i];
                std::string stuConstraint = other.unique_constraints[i];

                auto refStart = refConstraint.find("(");
                auto refEnd = refConstraint.find(")");
                auto stuStart = stuConstraint.find("(");
                auto stuEnd = stuConstraint.find(")");

                if (refStart != std::string::npos && refEnd != std::string::npos &&
                    stuStart != std::string::npos && stuEnd != std::string::npos)
                {
                    std::string refCols = refConstraint.substr(refStart + 1, refEnd - refStart - 1);
                    std::string stuCols = stuConstraint.substr(stuStart + 1, stuEnd - stuStart - 1);
                    // Remove spaces (a simple trim)
                    auto trim = [](const std::string &s)
                    {
                        size_t start = s.find_first_not_of(" \t");
                        size_t end = s.find_last_not_of(" \t");
                        return (start == std::string::npos) ? std::string("") : s.substr(start, end - start + 1);
                    };
                    refCols = trim(refCols);
                    stuCols = trim(stuCols);
                    if (refCols != stuCols)
                    {
                        incorrect_parts.push_back("Unique constraints");
                        message << "● For UNIQUE constraint " << (i + 1) << ", expected column(s): "
                                << refCols << " but found: " << stuCols << ".\n";
                        next_steps.push_back("💡Adjust the UNIQUE constraint definition " + std::to_string(i + 1) + ".");
                        check_ok = false;
                        equal = false;
                    }
                }
                else if (refConstraint != stuConstraint)
                {
                    incorrect_parts.push_back("Unique constraints");
                    message << "● UNIQUE constraint " << (i + 1) << " does not match. Expected: "
                            << refConstraint << ", but found: " << stuConstraint << ".\n";
                    next_steps.push_back("💡Review your UNIQUE constraint definition " + std::to_string(i + 1) + ".");
                    check_ok = false;
                    equal = false;
                }
            }
            if (check_ok)
            {
                if (!reference.unique_constraints.empty())
                {
                    correct_parts.push_back("Unique constraints");
                }
            }
        }
    }

    // Compare check constraints via ConditionNode signatures
    // Compare check constraints only if the reference actually has any
    if (!reference.check_condition_nodes.empty())
    {
        bool check_ok = true;

        if (reference.check_condition_nodes.size() != other.check_condition_nodes.size())
        {
            incorrect_parts.push_back("Check constraints");
            message << "● Expected " << reference.check_condition_nodes.size() << " CHECK constraint(s), but found " << other.check_condition_nodes.size() << ".\n";
            next_steps.push_back("💡Review your CHECK constraint definitions.");
            check_ok = false;
        }
        else
        {
            for (size_t i = 0; i < reference.check_condition_nodes.size(); ++i)
            {
                auto refCond = reference.check_condition_nodes[i];
                auto stuCond = other.check_condition_nodes[i];

                // Generate canonical signatures (handles commutativity and ordering)
                std::string sigRef = Where_clause::generate_condition_signature(refCond);
                std::string sigStu = Where_clause::generate_condition_signature(stuCond);

                if (sigRef != sigStu)
                {
                    incorrect_parts.push_back("Check constraints");
                    message << "● For CHECK constraint " << (i + 1) << ", expected: " << Where_clause::condition_to_string(refCond) << " but found: " << Where_clause::condition_to_string(stuCond) << ".\n";
                    next_steps.push_back("💡Adjust the CHECK constraint definition " + std::to_string(i + 1) + ".");
                    check_ok = false;
                }
            }
        }

        if (check_ok)
            correct_parts.push_back("Check constraints");

        equal = equal && check_ok;
    }
    else if (!other.check_condition_nodes.empty())
    {
        // Reference had no checks, but student added some
        incorrect_parts.push_back("Check constraints");
        message << "● Unexpected CHECK constraint(s) found.\n";
        next_steps.push_back("💡Remove the CHECK constraint definitions.");
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
