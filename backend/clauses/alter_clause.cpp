// alter_clause.cpp
#include "alter_clause.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include "select/where_clause.h"
#include "select/from_clause.h"
#include "select/select_clause.h"

Alter_clause::alter_clause_info
Alter_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    alter_clause_info info;
    if (!node || node->key != "AlterTableStmt")
        return info;

    info.object_type = "table";
    auto rel = node->get_child("relation");
    if (rel)
        info.object_name = Common::strip_quotes(rel->get_value("relname"));

    auto cmds_node = node->get_child("cmds");
    if (!cmds_node)
        return info;

    for (auto &cmd : cmds_node->children)
    {
        if (cmd->key != "AlterTableCmd")
            continue;

        std::string subtype = cmd->get_value("subtype");
        alter_clause_info::operation op;

        if (subtype == "AT_AddColumn")
        {
            op.type = alter_clause_info::ADD_COLUMN;
            auto def = cmd->get_child("def");
            if (def && def->key == "ColumnDef")
            {
                op.column_name = Common::strip_quotes(def->get_value("colname"));
                auto tn = def->get_child("typeName");
                if (tn)
                {
                    auto names = tn->get_child("names");
                    if (names && !names->children.empty())
                    {
                        auto last = names->children.back();
                        if (last->key == "String")
                            op.data_type = Common::strip_quotes(
                                last->get_child("sval")->value);
                    }
                }
                auto cns = def->get_child("constraints");
                if (cns)
                {
                    for (auto &cst : cns->children)
                        if (cst->key == "Constraint" &&
                            cst->get_value("contype") == "CONSTR_NOTNULL")
                            op.not_null = true;
                }
            }
        }
        else if (subtype == "AT_DropColumn")
        {
            op.type = alter_clause_info::DROP_COLUMN;
            op.column_name = Common::strip_quotes(cmd->get_value("name"));
            op.behavior = cmd->get_value("behavior");
        }
        else if (subtype == "AT_AlterColumnType")
        {
            op.type = alter_clause_info::ALTER_COLUMN_TYPE;
            op.column_name = Common::strip_quotes(cmd->get_value("name"));
            auto def = cmd->get_child("def");
            if (def && def->key == "TypeName")
            {
                auto names = def->get_child("names");
                if (names && !names->children.empty())
                {
                    auto last = names->children.back();
                    if (last->key == "String")
                        op.data_type = Common::strip_quotes(
                            last->get_child("sval")->value);
                }
            }
        }
        else if (subtype == "AT_SetNotNull")
        {
            op.type = alter_clause_info::SET_NOT_NULL;
            op.column_name = Common::strip_quotes(cmd->get_value("name"));
        }
        else if (subtype == "AT_DropNotNull")
        {
            op.type = alter_clause_info::DROP_NOT_NULL;
            op.column_name = Common::strip_quotes(cmd->get_value("name"));
        }
        else if (subtype == "AT_RenameColumn")
        {
            op.type = alter_clause_info::RENAME_COLUMN;
            op.column_name = Common::strip_quotes(cmd->get_value("name"));
            op.new_name = Common::strip_quotes(cmd->get_value("newname"));
        }
        else if (subtype == "AT_RenameTable")
        {
            op.type = alter_clause_info::RENAME_TABLE;
            op.new_name = Common::strip_quotes(cmd->get_value("newname"));
        }
        else if (subtype == "AT_AddConstraint")
        {
            op.type = alter_clause_info::ADD_CONSTRAINT;
            auto def = cmd->get_child("def");
            if (def)
            {
                auto constraint = def->get_child("Constraint");
                if (constraint)
                {
                    op.constraint_name = Common::strip_quotes(constraint->get_value("conname"));
                    std::string ct = constraint->get_value("contype");

                    if (ct == "CONSTR_CHECK")
                    {
                        auto raw = constraint->get_child("raw_expr");
                        if (raw && !raw->children.empty())
                        {
                            auto cond = Where_clause::extract_condition(raw->children.front(), From_clause::from_clause_info(), Select_clause::select_clause_info());
                            if (cond)
                            {
                                op.constraint_expr = Where_clause::condition_to_string(cond);
                                op.constraint_signature = Where_clause::generate_condition_signature(cond);
                            }
                        }
                    }
                    else if (ct == "CONSTR_PRIMARY" || ct == "CONSTR_UNIQUE")
                    {
                        auto keys = constraint->get_child("keys");
                        std::vector<std::string> cols;
                        if (keys)
                        {
                            for (auto &c : keys->children)
                                if (c->key == "String")
                                    cols.push_back(Common::strip_quotes(
                                        c->get_child("sval")->value));
                        }
                        std::ostringstream oss;
                        if (ct == "CONSTR_PRIMARY")
                            oss << "PRIMARY KEY (";
                        else
                            oss << "UNIQUE (";
                        for (size_t i = 0; i < cols.size(); ++i)
                        {
                            oss << cols[i];
                            if (i + 1 < cols.size())
                                oss << ", ";
                        }
                        oss << ")";
                        op.constraint_expr = oss.str();
                    }
                    else if (ct == "CONSTR_FOREIGN")
                    {
                        std::vector<std::string> loc, ref;
                        auto fk_attrs = constraint->get_child("fk_attrs");
                        auto pk_attrs = constraint->get_child("pk_attrs");
                        if (fk_attrs)
                            for (auto &c : fk_attrs->children)
                                if (c->key == "String")
                                    loc.push_back(Common::strip_quotes(
                                        c->get_child("sval")->value));
                        if (pk_attrs)
                            for (auto &c : pk_attrs->children)
                                if (c->key == "String")
                                    ref.push_back(Common::strip_quotes(
                                        c->get_child("sval")->value));
                        std::string rt =
                            Common::strip_quotes(constraint->get_child("pktable")->get_value("relname"));
                        std::ostringstream oss;
                        oss << "FOREIGN KEY (";
                        for (size_t i = 0; i < loc.size(); ++i)
                        {
                            oss << loc[i];
                            if (i + 1 < loc.size())
                                oss << ", ";
                        }
                        oss << ") REFERENCES " << rt << " (";
                        for (size_t i = 0; i < ref.size(); ++i)
                        {
                            oss << ref[i];
                            if (i + 1 < ref.size())
                                oss << ", ";
                        }
                        oss << ")";
                        op.constraint_expr = oss.str();
                    }
                }
            }
        }
        else if (subtype == "AT_DropConstraint")
        {
            op.type = alter_clause_info::DROP_CONSTRAINT;
            op.constraint_name = Common::strip_quotes(cmd->get_value("name"));
        }
        else if (subtype == "AT_SetSchema")
        {
            op.type = alter_clause_info::SET_SCHEMA;
            op.new_name = Common::strip_quotes(cmd->get_value("newschema"));
        }
        else if (subtype == "AT_SetTableSpace")
        {
            op.type = alter_clause_info::SET_TABLESPACE;
            op.new_name = Common::strip_quotes(cmd->get_value("tablespacename"));
        }
        else if (subtype == "AT_OwnerTo")
        {
            op.type = alter_clause_info::OWNER_TO;
            op.new_name = Common::strip_quotes(cmd->get_value("newowner"));
        }
        else
        {
            std::cerr << "Unknown AlterTableCmd subtype: " << subtype << std::endl;
            continue;
        }

        info.operations.push_back(op);
    }

    return info;
}

std::pair<std::string, Alter_clause::alter_clause_info>
Alter_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    std::ostringstream oss;

    if (!info.object_name.empty())
    {
        oss << "Alter " << info.object_type << " '" << info.object_name << "': ";
        for (size_t i = 0; i < info.operations.size(); ++i)
        {
            const auto &op = info.operations[i];
            switch (op.type)
            {
            case alter_clause_info::ADD_COLUMN:
                oss << "ADD COLUMN " << op.column_name << " " << op.data_type;
                if (op.not_null)
                    oss << " NOT NULL";
                break;
            case alter_clause_info::DROP_COLUMN:
                oss << "DROP COLUMN " << op.column_name;
                if (!op.behavior.empty())
                    oss << " "
                        << (op.behavior == "DROP_CASCADE" ? "CASCADE" : "RESTRICT");
                break;
            case alter_clause_info::ALTER_COLUMN_TYPE:
                oss << "ALTER COLUMN " << op.column_name
                    << " TYPE " << op.data_type;
                break;
            case alter_clause_info::SET_NOT_NULL:
                oss << "ALTER COLUMN " << op.column_name << " SET NOT NULL";
                break;
            case alter_clause_info::DROP_NOT_NULL:
                oss << "ALTER COLUMN " << op.column_name << " DROP NOT NULL";
                break;
            case alter_clause_info::RENAME_COLUMN:
                oss << "RENAME COLUMN " << op.column_name
                    << " TO " << op.new_name;
                break;
            case alter_clause_info::RENAME_TABLE:
                oss << "RENAME TO " << op.new_name;
                break;
            case alter_clause_info::ADD_CONSTRAINT:
                oss << "ADD CONSTRAINT ";
                if (!op.constraint_name.empty())
                    oss << op.constraint_name << " ";
                oss << op.constraint_expr;
                break;
            case alter_clause_info::DROP_CONSTRAINT:
                oss << "DROP CONSTRAINT " << op.constraint_name;
                break;
            case alter_clause_info::SET_SCHEMA:
                oss << "SET SCHEMA " << op.new_name;
                break;
            case alter_clause_info::SET_TABLESPACE:
                oss << "SET TABLESPACE " << op.new_name;
                break;
            case alter_clause_info::OWNER_TO:
                oss << "OWNER TO " << op.new_name;
                break;
            }

            oss << (i + 1 < info.operations.size() ? "; " : ".");
        }
    }
    else
    {
        oss << "No ALTER statement found.";
    }

    return {oss.str(), info};
}

Common::comparision_result
Alter_clause::compare(const alter_clause_info &reference,
                      const alter_clause_info &other)
{
    Common::comparision_result comp;
    bool equal = true;
    std::vector<std::string> correct_parts;
    std::vector<std::string> incorrect_parts;
    std::vector<std::string> next_steps;
    std::ostringstream message;

    // --- Compare table name ---
    if (reference.object_name == other.object_name)
    {
        correct_parts.push_back("Table name");
    }
    else
    {
        incorrect_parts.push_back("Table name");
        message << "● The table name should be '" << reference.object_name << "', but is '" << other.object_name << "'.\n";
        next_steps.push_back("💡 Use ALTER TABLE " + reference.object_name + " …");
        equal = false;
    }

    // --- Compare number of operations ---
    size_t ref_sz = reference.operations.size();
    size_t oth_sz = other.operations.size();
    if (ref_sz == oth_sz)
    {
        correct_parts.push_back("Number of operations");
    }
    else
    {
        incorrect_parts.push_back("Number of operations");
        message << "● Expected " << ref_sz << " operation(s), but found " << oth_sz << ".\n";
        next_steps.push_back("💡 Ensure exactly " + std::to_string(ref_sz) + " ALTER operations.");
        equal = false;
    }

    // Helper to name each OperationType
    auto op_name = [&](alter_clause_info::OperationType t)
    {
        switch (t)
        {
        case alter_clause_info::ADD_COLUMN:
            return "ADD COLUMN";
        case alter_clause_info::DROP_COLUMN:
            return "DROP COLUMN";
        case alter_clause_info::ALTER_COLUMN_TYPE:
            return "ALTER COLUMN TYPE";
        case alter_clause_info::SET_NOT_NULL:
            return "SET NOT NULL";
        case alter_clause_info::DROP_NOT_NULL:
            return "DROP NOT NULL";
        case alter_clause_info::RENAME_COLUMN:
            return "RENAME COLUMN";
        case alter_clause_info::RENAME_TABLE:
            return "RENAME TABLE";
        case alter_clause_info::ADD_CONSTRAINT:
            return "ADD CONSTRAINT";
        case alter_clause_info::DROP_CONSTRAINT:
            return "DROP CONSTRAINT";
        case alter_clause_info::SET_SCHEMA:
            return "SET SCHEMA";
        case alter_clause_info::SET_TABLESPACE:
            return "SET TABLESPACE";
        case alter_clause_info::OWNER_TO:
            return "OWNER TO";
        default:
            return "UNKNOWN";
        }
    };

    // --- Compare operation types sequence ---
    {
        bool types_ok = true;
        if (ref_sz == oth_sz)
        {
            for (size_t i = 0; i < ref_sz; ++i)
            {
                if (reference.operations[i].type != other.operations[i].type)
                {
                    types_ok = false;
                    break;
                }
            }
        }
        else
        {
            types_ok = false;
        }

        if (types_ok)
        {
            correct_parts.push_back("Operation types");
        }
        else
        {
            incorrect_parts.push_back("Operation types");
            std::ostringstream exp, got;
            for (size_t i = 0; i < reference.operations.size(); ++i)
            {
                exp << op_name(reference.operations[i].type) << (i + 1 < reference.operations.size() ? ", " : "");
            }
            for (size_t i = 0; i < other.operations.size(); ++i)
            {
                got << op_name(other.operations[i].type) << (i + 1 < other.operations.size() ? ", " : "");
            }
            message << "● Expected operation sequence: " << exp.str() << " but found: " << got.str() << ".\n";
            next_steps.push_back("💡 Use the correct sequence of ALTER operations.");
            equal = false;
        }
    }

    // --- Compare each operation in order ---
    size_t n = std::min(ref_sz, oth_sz);
    for (size_t i = 0; i < n; ++i)
    {
        const auto &rop = reference.operations[i];
        const auto &op = other.operations[i];
        bool op_equal = true;
        std::string idx = std::to_string(i + 1);

        // 1) Type must match
        if (rop.type != op.type)
        {
            incorrect_parts.push_back("Operation " + idx);
            message << "● Operation " << idx << " should be '" << op_name(rop.type) << "' but is '" << op_name(op.type) << "'.\n";
            next_steps.push_back(std::string("Use '") + op_name(rop.type) + "' for operation " + idx + ".");
            equal = op_equal = false;
        }
        else
        {
            // 2) Parameters for each subtype
            switch (rop.type)
            {
            case alter_clause_info::ADD_COLUMN:
                if (rop.column_name != op.column_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected to add column '" << rop.column_name << "' but found '" << op.column_name << "'.\n";
                    next_steps.push_back("💡 Rename to '" + rop.column_name + "' in ADD COLUMN.");
                    op_equal = equal = false;
                }
                if (rop.data_type != op.data_type)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected type '" << rop.data_type << "' but found '" << op.data_type << "'.\n";
                    next_steps.push_back("💡 Use type '" + rop.data_type + "' for column '" + rop.column_name + "'.");
                    op_equal = equal = false;
                }
                if (rop.not_null != op.not_null)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected "
                            << (rop.not_null ? "" : "no ")
                            << "NOT NULL but found "
                            << (op.not_null ? "" : "no ")
                            << "NOT NULL.\n";
                    next_steps.push_back("💡 Adjust NOT NULL for '" + rop.column_name + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::DROP_COLUMN:
                if (rop.column_name != op.column_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected to drop column '"
                            << rop.column_name << "' but found '"
                            << op.column_name << "'.\n";
                    next_steps.push_back("💡 Use DROP COLUMN '" + rop.column_name + "'.");
                    op_equal = equal = false;
                }
                if (rop.behavior != op.behavior)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected behavior '"
                            << rop.behavior << "' but found '"
                            << op.behavior << "'.\n";
                    next_steps.push_back("💡 Use behavior '" + rop.behavior + "' for DROP COLUMN.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::ALTER_COLUMN_TYPE:
                if (rop.column_name != op.column_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected to alter column '"
                            << rop.column_name << "' but found '"
                            << op.column_name << "'.\n";
                    next_steps.push_back("💡 Use ALTER COLUMN '" + rop.column_name + "'.");
                    op_equal = equal = false;
                }
                if (rop.data_type != op.data_type)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected new type '"
                            << rop.data_type << "' but found '"
                            << op.data_type << "'.\n";
                    next_steps.push_back("💡 Change type to '" + rop.data_type + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::SET_NOT_NULL:
            case alter_clause_info::DROP_NOT_NULL:
                if (rop.column_name != op.column_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected column '"
                            << rop.column_name << "' but found '"
                            << op.column_name << "'.\n";
                    next_steps.push_back("💡 Use column '" + rop.column_name + "' for NOT NULL change.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::RENAME_COLUMN:
                if (rop.column_name != op.column_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected to rename '"
                            << rop.column_name << "' but found '"
                            << op.column_name << "'.\n";
                    next_steps.push_back("💡 Use RENAME COLUMN '" + rop.column_name + "'.");
                    op_equal = equal = false;
                }
                if (rop.new_name != op.new_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected new name '"
                            << rop.new_name << "' but found '"
                            << op.new_name << "'.\n";
                    next_steps.push_back("💡 Rename to '" + rop.new_name + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::RENAME_TABLE:
                if (rop.new_name != op.new_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected new table name '"
                            << rop.new_name << "' but found '"
                            << op.new_name << "'.\n";
                    next_steps.push_back("💡 Use RENAME TO '" + rop.new_name + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::ADD_CONSTRAINT:
                // Only check name if reference actually named it:
                if (!rop.constraint_name.empty())
                {
                    if (rop.constraint_name != op.constraint_name)
                    {
                        incorrect_parts.push_back("Operation " + idx);
                        message << "● Op " << idx << ": expected constraint name '" << rop.constraint_name << "' but found '" << op.constraint_name << "'.\n";
                        next_steps.push_back("💡 Name constraint '" + rop.constraint_name + "'.");
                        op_equal = equal = false;
                    }
                }
                // If this is a CHECK constraint, compare normalized signatures
                if (!rop.constraint_signature.empty())
                {
                    if (rop.constraint_signature != op.constraint_signature)
                    {
                        incorrect_parts.push_back("Operation " + idx);
                        message << "● Op " << idx
                                << ": expected CHECK condition '"
                                << rop.constraint_expr << "' but found '"
                                << op.constraint_expr << "'.\n";
                        next_steps.push_back(
                            "Use CHECK (" + rop.constraint_expr + ").");
                        op_equal = equal = false;
                    }
                }
                else if (rop.constraint_expr != op.constraint_expr)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected constraint expr '" << rop.constraint_expr << "' but found '" << op.constraint_expr << "'.\n";
                    next_steps.push_back("💡 Use CHECK/KEY expr '" + rop.constraint_expr + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::DROP_CONSTRAINT:
                if (rop.constraint_name != op.constraint_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected to drop constraint '"
                            << rop.constraint_name << "' but found '"
                            << op.constraint_name << "'.\n";
                    next_steps.push_back("💡 DROP CONSTRAINT '" + rop.constraint_name + "'.");
                    op_equal = equal = false;
                }
                break;

            case alter_clause_info::SET_SCHEMA:
            case alter_clause_info::SET_TABLESPACE:
            case alter_clause_info::OWNER_TO:
                if (rop.new_name != op.new_name)
                {
                    incorrect_parts.push_back("Operation " + idx);
                    message << "● Op " << idx << ": expected argument '"
                            << rop.new_name << "' but found '"
                            << op.new_name << "'.\n";
                    next_steps.push_back("💡 Set to '" + rop.new_name + "'.");
                    op_equal = equal = false;
                }
                break;
            }
        }

        if (op_equal)
            correct_parts.push_back("Operation " + idx);
    }

    comp.equal = equal;
    comp.correct_parts = std::move(correct_parts);
    comp.incorrect_parts = std::move(incorrect_parts);
    comp.next_steps = std::move(next_steps);
    comp.message = message.str();
    return comp;
}

void Alter_clause::print(const alter_clause_info &info)
{
    std::cout << "Alter " << info.object_type << " '" << info.object_name << "':\n";
    for (const auto &op : info.operations)
    {
        switch (op.type)
        {
        case alter_clause_info::ADD_COLUMN:
            std::cout << "  ADD COLUMN " << op.column_name << " " << op.data_type;
            if (op.not_null)
                std::cout << " NOT NULL";
            break;
        case alter_clause_info::DROP_COLUMN:
            std::cout << "  DROP COLUMN " << op.column_name;
            if (!op.behavior.empty())
                std::cout << " " << op.behavior;
            break;
        case alter_clause_info::ALTER_COLUMN_TYPE:
            std::cout << "  ALTER COLUMN " << op.column_name
                      << " TYPE " << op.data_type;
            break;
        case alter_clause_info::SET_NOT_NULL:
            std::cout << "  ALTER COLUMN " << op.column_name
                      << " SET NOT NULL";
            break;
        case alter_clause_info::DROP_NOT_NULL:
            std::cout << "  ALTER COLUMN " << op.column_name
                      << " DROP NOT NULL";
            break;
        case alter_clause_info::RENAME_COLUMN:
            std::cout << "  RENAME COLUMN " << op.column_name
                      << " TO " << op.new_name;
            break;
        case alter_clause_info::RENAME_TABLE:
            std::cout << "  RENAME TO " << op.new_name;
            break;
        case alter_clause_info::ADD_CONSTRAINT:
            std::cout << "  ADD CONSTRAINT ";
            if (!op.constraint_name.empty())
                std::cout << op.constraint_name;
            std::cout << ": " << op.constraint_expr;
            break;
        case alter_clause_info::DROP_CONSTRAINT:
            std::cout << "  DROP CONSTRAINT "
                      << op.constraint_name;
            break;
        case alter_clause_info::SET_SCHEMA:
            std::cout << "  SET SCHEMA "
                      << op.new_name;
            break;
        case alter_clause_info::SET_TABLESPACE:
            std::cout << "  SET TABLESPACE "
                      << op.new_name;
            break;
        case alter_clause_info::OWNER_TO:
            std::cout << "  OWNER TO "
                      << op.new_name;
            break;
        }
        std::cout << "\n";
    }
}
