#include "where_clause.h"
#include "../common.h"
#include "select_clause.h"
#include <iostream>
#include <set>

std::string Where_clause::extract_constant(const std::shared_ptr<AbstractSyntaxTree::Node> &const_node)
{
    std::string constant_value;

    std::function<void(const std::shared_ptr<AbstractSyntaxTree::Node> &)> traverse;
    traverse = [&](const std::shared_ptr<AbstractSyntaxTree::Node> &node)
    {
        if (node->key == "ival" || node->key == "sval" || node->key == "fval")
        {
            constant_value = (node->key == "sval") ? Common::strip_quotes(node->value) : Common::strip_quotes(node->value);
        }
        for (const auto &child : node->children)
        {
            traverse(child);
        }
    };

    traverse(const_node);
    return constant_value;
}
std::string Where_clause::extract_expression(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    if (!expr_node)
        return "(null)";
    if (expr_node->key == "ColumnRef")
    {
        std::string column_name = Select_clause::extract_column_name(expr_node, from_info);

        // Check for alias in select_info
        for (const auto &col_info : select_info.the_columns)
        {
            if (column_name == col_info.alias)
            {
                // Replace alias with the full column name
                column_name = col_info.column_name;
                break;
            }
        }
        for (const auto &func_info : select_info.the_functions)
        {
            if (column_name == func_info.alias)
            {
                // Replace alias with the full function expression
                column_name = func_info.function_name + "()";
                break;
            }
        }
        for (const auto &expr_info : select_info.the_expressions)
        {
            if (column_name == expr_info.alias)
            {
                // Replace alias with the full expression
                column_name = expr_info.expression;
                break;
            }
        }
        return column_name;
        // return select_extract_column_name(expr_node, from_info);
    }
    else if (expr_node->key == "FuncCall")
    {
        return Select_clause::extract_function(expr_node, from_info);
    }
    else if (expr_node->key == "A_Const")
    {
        return Where_clause::extract_constant(expr_node);
    }
    else if (expr_node->key == "A_Expr")
    {
        // Handle nested expressions
        std::string left = "(null)";
        std::string right = "(null)";

        // Get the lexpr node
        auto lexpr_node = expr_node->get_child("lexpr");
        if (lexpr_node && !lexpr_node->children.empty())
        {
            // Process the child of lexpr
            left = extract_expression(lexpr_node->children.front(), from_info, select_info);
        }

        // Get the operator
        std::string op = extract_operator(expr_node->get_child("name"));

        // Get the rexpr node
        auto rexpr_node = expr_node->get_child("rexpr");
        if (rexpr_node && !rexpr_node->children.empty())
        {
            // Process the child of rexpr
            right = extract_expression(rexpr_node->children.front(), from_info, select_info);
        }

        return "(" + left + " " + op + " " + right + ")";
    }
    else if (expr_node->key == "TypeCast")
    {
        // Handle type casting
        std::string arg = extract_expression(expr_node->get_child("arg"), from_info, select_info);
        auto type_name_node = expr_node->get_child("typeName");
        std::string type_name;
        if (type_name_node)
        {
            auto names_node = type_name_node->get_child("names");
            if (names_node && !names_node->children.empty())
            {
                auto last_name_node = names_node->children.back();
                if (last_name_node->key == "String")
                {
                    type_name = Common::strip_quotes(last_name_node->get_child("sval")->value);
                }
            }
        }
        return arg + "::" + type_name;
    }
    else if (expr_node->key == "SubLink")
    {
        // Handle subqueries
        return "(subquery)";
    }
    else if (expr_node->key == "CollateClause")
    {
        // New code to handle collate expressions
        std::string arg = extract_expression(expr_node->get_child("arg")->children.front(), from_info, select_info);
        auto collate_node = expr_node->get_child("collname");
        std::string collate_name;
        if (collate_node)
        {
            // Extract the collation name, which may consist of multiple identifiers
            std::vector<std::string> collate_parts;
            for (const auto &name_part : collate_node->children)
            {
                if (name_part->key == "String")
                {
                    auto sval_node = name_part->get_child("sval");
                    if (sval_node)
                    {
                        collate_parts.push_back(Common::strip_quotes(sval_node->value));
                    }
                }
            }
            // Join the parts with dots if necessary (e.g., "pg_catalog"."en_US")
            collate_name = join_elements_to_str(collate_parts, ".");
        }
        return arg + " COLLATE \"" + collate_name + "\"";
    }
    else if (expr_node->key == "BoolExpr")
    {
        std::string boolop = expr_node->get_value("boolop");
        std::string delim = (boolop == "AND_EXPR") ? " and " : " or ";

        std::vector<std::string> parts;
        auto args_node = expr_node->get_child("args");
        if (args_node)
        {
            for (auto &arg : args_node->children)
                parts.push_back(extract_expression(arg, from_info, select_info));
        }

        std::ostringstream oss;
        oss << "(";
        for (size_t i = 0; i < parts.size(); ++i)
        {
            oss << parts[i];
            if (i + 1 < parts.size())
                oss << delim;
        }
        oss << ")";
        return oss.str();
    }
    // Default case for unexpected node types
    else
    {
        // Optionally, print or log the unexpected node key for debugging
        std::cout << "Unexpected expression node key: " << expr_node->key << std::endl;
        return "(unknown expression)";
    }
}
std::string Where_clause::extract_operator(const std::shared_ptr<AbstractSyntaxTree::Node> &name_node)
{
    if (!name_node)
        return "";
    for (const auto &child : name_node->children)
    {
        if (child->key == "String")
        {
            for (const auto &sval_node : child->children)
            {
                if (sval_node->key == "sval")
                {
                    return Common::strip_quotes(sval_node->value);
                }
            }
        }
    }
    return "";
}
std::string Where_clause::join_elements_to_str(const std::vector<std::string> &elements, const std::string &delimiter)
{
    std::ostringstream oss;
    for (size_t i = 0; i < elements.size(); ++i)
    {
        oss << "'" << elements[i] << "'";
        if (i < elements.size() - 1)
        {
            oss << delimiter << " ";
        }
    }
    return oss.str();
}
std::shared_ptr<Where_clause::ConditionNode> Where_clause::extract_condition(const std::shared_ptr<AbstractSyntaxTree::Node> &expr_node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    if (!expr_node)
    {
        return nullptr;
    }
    if (expr_node->key == "A_Expr")
    {
        // Handle expression operators
        std::string kind = expr_node->get_value("kind");
        if (kind == "AEXPR_OP")
        {
            // Handle simple comparison expressions (e.g., =, <, >)
            auto lexpr_node = expr_node->get_child("lexpr");
            auto name_node = expr_node->get_child("name");
            auto rexpr_node = expr_node->get_child("rexpr");

            std::string left_operand = Where_clause::extract_expression(lexpr_node->children.front(), from_info, select_info);
            std::string operator_ = extract_operator(name_node);
            std::string right_operand = Where_clause::extract_expression(rexpr_node->children.front(), from_info, select_info);

            return std::make_shared<ConditionNode>(left_operand, operator_, right_operand);
        }
        else if (kind == "AEXPR_IN")
        {
            // Handle IN expressions
            std::string left_operand = Where_clause::extract_expression(expr_node->get_child("lexpr"), from_info, select_info);
            auto rexpr_node = expr_node->get_child("rexpr");
            std::string right_operand;
            if (rexpr_node->key == "A_ArrayExpr" || rexpr_node->key == "List")
            {
                // Extract array elements
                std::vector<std::string> elements;
                for (const auto &elem : rexpr_node->children)
                {
                    elements.push_back(Where_clause::extract_expression(elem, from_info, select_info));
                }
                right_operand = "(" + join_elements_to_str(elements, ", ") + ")";
            }
            else
            {
                right_operand = Where_clause::extract_expression(rexpr_node, from_info, select_info);
            }
            return std::make_shared<ConditionNode>(left_operand, "IN", right_operand);
        }
        else if (kind == "AEXPR_LIKE")
        {
            // Handle LIKE expressions
            std::string left_operand = Where_clause::extract_expression(expr_node->get_child("lexpr"), from_info, select_info);
            std::string right_operand = Where_clause::extract_expression(expr_node->get_child("rexpr"), from_info, select_info);
            return std::make_shared<ConditionNode>(left_operand, "LIKE", right_operand);
        }
        else if (kind == "AEXPR_BETWEEN")
        {
            // Handle BETWEEN expressions
            std::string left_operand = Where_clause::extract_expression(expr_node->get_child("lexpr"), from_info, select_info);
            std::string lower_bound = Where_clause::extract_expression(expr_node->get_child("lower"), from_info, select_info);
            std::string upper_bound = Where_clause::extract_expression(expr_node->get_child("upper"), from_info, select_info);
            std::string right_operand = lower_bound + " AND " + upper_bound;
            return std::make_shared<ConditionNode>(left_operand, "BETWEEN", right_operand);
        }
        else if (kind == "AEXPR_NOT")
        {
            // Handle NOT expressions
            auto rexpr_node = expr_node->get_child("rexpr");
            auto condition = extract_condition(rexpr_node, from_info, select_info);
            if (condition)
            {
                std::vector<std::shared_ptr<ConditionNode>> children = {condition};
                return std::make_shared<ConditionNode>(ConditionType::NOT, children);
            }
        }
        // Handle other expression kinds if necessary
    }
    else if (expr_node->key == "BoolExpr")
    {
        // Handle boolean expressions (AND, OR, NOT)
        std::string boolop = expr_node->get_value("boolop");
        ConditionType cond_type;
        if (boolop == "AND_EXPR")
        {
            cond_type = ConditionType::AND;
        }
        else if (boolop == "OR_EXPR")
        {
            cond_type = ConditionType::OR;
        }
        else if (boolop == "NOT_EXPR")
        {
            cond_type = ConditionType::NOT;
        }
        else
        {
            return nullptr; // Unknown boolean operator
        }

        std::vector<std::shared_ptr<ConditionNode>> cond_children;
        auto args_node = expr_node->get_child("args");
        if (args_node)
        {
            for (const auto &arg_node : args_node->children)
            {
                auto condition = extract_condition(arg_node, from_info, select_info);
                if (condition)
                {
                    cond_children.push_back(condition);
                }
            }
        }
        return std::make_shared<ConditionNode>(cond_type, cond_children);
    }
    else if (expr_node->key == "NullTest")
    {
        // Handle IS NULL and IS NOT NULL
        auto arg_node = expr_node->get_child("arg");
        std::string arg = Where_clause::extract_expression(arg_node, from_info, select_info);
        std::string nulltesttype = expr_node->get_value("nulltesttype");
        std::string operator_;
        if (nulltesttype == "IS_NULL")
        {
            operator_ = "IS NULL";
        }
        else if (nulltesttype == "IS_NOT_NULL")
        {
            operator_ = "IS NOT NULL";
        }
        else
        {
            return nullptr; // Unknown null test type
        }
        return std::make_shared<ConditionNode>(arg, operator_, "");
    }
    else if (expr_node->key == "SubLink")
    {
        // Handle subqueries
        std::string sublink_type = expr_node->get_value("subLinkType");
        auto subselect_node = expr_node->get_child("subselect");
        std::string subquery = "(subquery)";
        std::string operator_;
        if (sublink_type == "EXISTS_SUBLINK")
        {
            operator_ = "EXISTS";
            return std::make_shared<ConditionNode>(operator_, "", subquery);
        }
        else if (sublink_type == "ANY_SUBLINK" || sublink_type == "ALL_SUBLINK")
        {
            // For ANY or ALL sublinks, there is a test expression and operator
            std::string left_operand = Where_clause::extract_expression(expr_node->get_child("testexpr"), from_info, select_info);
            std::string operator_name = extract_operator(expr_node->get_child("operName"));
            operator_ = operator_name + " " + (sublink_type == "ANY_SUBLINK" ? "ANY" : "ALL");
            return std::make_shared<ConditionNode>(left_operand, operator_, subquery);
        }
        else if (sublink_type == "EXPR_SUBLINK")
        {
            // Handle expressions that return a single value
            std::string left_operand = Where_clause::extract_expression(expr_node->get_child("testexpr"), from_info, select_info);
            operator_ = "="; // Assuming equality comparison
            return std::make_shared<ConditionNode>(left_operand, operator_, subquery);
        }
        // Handle other sublink types if necessary
    }
    // Handle other expression types if necessary
    return nullptr;
}
Where_clause::where_clause_info Where_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    where_clause_info info;
    if (!node)
    {
        return info;
    }
    if (node->key == "whereClause")
    {
        info.condition_root = extract_condition(node->children.front(), from_info, select_info);
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
std::pair<std::string, Where_clause::where_clause_info> Where_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info)
{
    where_clause_info where_info = get_info(node, from_info, select_info);
    if (where_info.condition_root)
    {
        std::string condition_str = condition_to_string(where_info.condition_root);
        return std::make_pair("To filter data where " + condition_str, where_info);
        // return "To filter data where " + condition_str;
    }
    else
    {
        return std::make_pair("No conditions in WHERE clause", where_info);
        // return "No conditions in WHERE clause";
    }
}
// Function to print the WHERE clause
void Where_clause::print(const where_clause_info &where_info)
{
    if (!where_info.condition_root)
    {
        std::cout << "No WHERE clause." << std::endl;
    }
    else
    {
        std::cout << "WHERE clause conditions:" << std::endl;
        print(where_info.condition_root, 0);
    }
}
// Helper function to recursively print the condition nodes
void Where_clause::print(const std::shared_ptr<Where_clause::ConditionNode> &node, int indent_level)
{
    std::string indent(indent_level * 2, ' ');
    if (node->type == Where_clause::ConditionType::SIMPLE)
    {
        std::cout << indent << "Condition: " << node->left_operand << " " << node->operator_ << " " << node->right_operand << std::endl;
    }
    else if (node->type == Where_clause::ConditionType::AND || node->type == Where_clause::ConditionType::OR)
    {
        std::string condition_type = (node->type == Where_clause::ConditionType::AND) ? "AND" : "OR";
        std::cout << indent << condition_type << " condition:" << std::endl;
        for (const auto &child : node->children)
        {
            print(child, indent_level + 1);
        }
    }
    else if (node->type == Where_clause::ConditionType::NOT)
    {
        std::cout << indent << "NOT condition:" << std::endl;
        if (!node->children.empty())
        {
            print(node->children[0], indent_level + 1);
        }
    }
}
// Function to compare the WHERE clause information
std::pair<int, std::string> Where_clause::compare(const where_clause_info &reference, const where_clause_info &other, std::vector<std::string> &next_steps)
{
    std::string message = "";
    bool equal = true;

    if (!reference.condition_root && !other.condition_root)
    {
        return std::make_pair(-1, "Both queries have no WHERE clause.");
    }

    if ((reference.condition_root && !other.condition_root) || (!reference.condition_root && other.condition_root))
    {
        message = "Mismatch in WHERE clause presence.\n";
        if (reference.condition_root)
            next_steps.push_back("💡 Add a WHERE clause to filter the data as specified.");
        else
            next_steps.push_back("💡 Remove the WHERE clause if it's not required.");
        equal = false;
    }
    else
    {
        if (!compare_condition_nodes(reference.condition_root, other.condition_root, message))
        {
            next_steps.push_back("💡 Review your WHERE clause conditions to match the specified requirements.");
            equal = false;
        }
    }

    return std::make_pair(equal, message);
}
// Helper function to recursively compare condition nodes
bool Where_clause::compare_condition_nodes(const std::shared_ptr<Where_clause::ConditionNode> &node1, const std::shared_ptr<Where_clause::ConditionNode> &node2, std::string &message)
{
    if (node1->type != node2->type)
    {
        message += "\n ● Condition type mismatch: Expected " + condition_type_to_string(node1->type) + ", but found " + condition_type_to_string(node2->type) + ".\n";
        return false;
    }

    if (node1->type == Where_clause::ConditionType::SIMPLE)
    {
        // Compare operators
        if (node1->operator_ != node2->operator_)
        {
            message += "● Operator mismatch: Expected '" + node1->operator_ + "', but found '" + node2->operator_ + "'.\n";
            return false;
        }

        // Compare operands
        if (operators_commutative(node1->operator_))
        {
            // For commutative operators, operands can be in any order
            std::set<std::string> operands1 = {node1->left_operand, node1->right_operand};
            std::set<std::string> operands2 = {node2->left_operand, node2->right_operand};
            if (operands1 != operands2)
            {
                message += "● Operands mismatch in condition '" + node1->left_operand + " " + node1->operator_ + " " + node1->right_operand + "'.\n";
                message += "● Expected operands: '" + node1->left_operand + "' and '" + node1->right_operand + "'.\n";
                message += "● Found operands: '" + node2->left_operand + "' and '" + node2->right_operand + "'.\n";
                return false;
            }
        }
        else
        {
            // For non-commutative operators, order matters
            if (node1->left_operand != node2->left_operand || node1->right_operand != node2->right_operand)
            {
                message += "● Operands mismatch in condition '" + node1->left_operand + " " + node1->operator_ + " " + node1->right_operand + "'.\n";
                message += "● Expected: '" + node1->left_operand + "' and '" + node1->right_operand + "'.\n";
                message += "● Found: '" + node2->left_operand + "' and '" + node2->right_operand + "'.\n";
                return false;
            }
        }
    }
    else if (node1->type == Where_clause::ConditionType::AND || node1->type == Where_clause::ConditionType::OR)
    {
        // For AND and OR, compare children sets

        // First, check if number of children is the same
        if (node1->children.size() != node2->children.size())
        {
            message += "● Number of conditions in " + condition_type_to_string(node1->type) + " clause mismatch. Expected " + std::to_string(node1->children.size()) + ", but found " + std::to_string(node2->children.size()) + ".\n";
            return false;
        }

        // Generate signatures for each child and compare sets
        std::vector<std::string> node1_child_signatures;
        std::vector<std::string> node2_child_signatures;

        for (const auto &child : node1->children)
        {
            node1_child_signatures.push_back(generate_condition_signature(child));
        }

        for (const auto &child : node2->children)
        {
            node2_child_signatures.push_back(generate_condition_signature(child));
        }

        // Sort signatures
        std::sort(node1_child_signatures.begin(), node1_child_signatures.end());
        std::sort(node2_child_signatures.begin(), node2_child_signatures.end());

        if (node1_child_signatures != node2_child_signatures)
        {
            message += "● Mismatch in conditions within " + condition_type_to_string(node1->type) + " clause.\n";
            message += "● Expected conditions: ";
            for (const auto &sig : node1_child_signatures)
                message += sig + "; ";
            message += "\n ● Found conditions: ";
            for (const auto &sig : node2_child_signatures)
                message += sig + "; ";
            message += "\n ● ";
            return false;
        }

        return true;
    }
    else if (node1->type == Where_clause::ConditionType::NOT)
    {
        // NOT should have only one child
        if (node1->children.size() != 1 || node2->children.size() != 1)
        {
            message += "● NOT condition should have exactly one child.\n";
            return false;
        }

        return compare_condition_nodes(node1->children[0], node2->children[0], message);
    }
    return true; // Should not reach here
}
// Helper function to check if an operator is commutative
bool Where_clause::operators_commutative(const std::string &op)
{
    return op == "=" || op == "<>" || op == "!=";
}

// Helper function to generate a normalized signature of a condition node
std::string Where_clause::generate_condition_signature(const std::shared_ptr<Where_clause::ConditionNode> &node)
{
    if (node->type == Where_clause::ConditionType::SIMPLE)
    {
        std::string op = node->operator_;
        std::string left = node->left_operand;
        std::string right = node->right_operand;

        if ((op == "<" || op == ">" ||
             op == "<=" || op == ">=") &&
            !left.empty() && !right.empty()
            // left looks like a literal (digit or quote) and right like an identifier
            && (std::isdigit(left.front()) || left.front() == '\'') && (std::isalpha(right.front()) || right.front() == '_'))
        {
            std::swap(left, right);
            if (op == "<")
                op = ">";
            else if (op == ">")
                op = "<";
            else if (op == "<=")
                op = ">=";
            else if (op == ">=")
                op = "<=";
        }
        if (operators_commutative(op))
        {
            // For commutative operators, sort the operands
            if (left > right)
                std::swap(left, right);
            return "SIMPLE:" + left + op + right;
        }
        else
        {
            return "SIMPLE:" + left + op + right;
        }
    }
    else if (node->type == Where_clause::ConditionType::AND || node->type == Where_clause::ConditionType::OR)
    {
        // For AND and OR, generate a signature based on child signatures
        std::vector<std::string> child_signatures;
        for (const auto &child : node->children)
        {
            child_signatures.push_back(generate_condition_signature(child));
        }
        // Sort the child signatures
        std::sort(child_signatures.begin(), child_signatures.end());
        std::string combined = (node->type == Where_clause::ConditionType::AND) ? "AND:" : "OR:";
        for (const auto &sig : child_signatures)
        {
            combined += sig + ";";
        }
        return combined;
    }
    else if (node->type == Where_clause::ConditionType::NOT)
    {
        // NOT should have one child
        if (!node->children.empty())
        {
            return "NOT:" + generate_condition_signature(node->children[0]);
        }
        else
        {
            return "NOT:EMPTY";
        }
    }
    return "";
}
std::string Where_clause::condition_type_to_string(Where_clause::ConditionType type)
{
    switch (type)
    {
    case Where_clause::ConditionType::SIMPLE:
        return "SIMPLE";
    case Where_clause::ConditionType::AND:
        return "AND";
    case Where_clause::ConditionType::OR:
        return "OR";
    case Where_clause::ConditionType::NOT:
        return "NOT";
    default:
        return "UNKNOWN";
    }
}
std::string Where_clause::condition_to_string(const std::shared_ptr<Where_clause::ConditionNode> &condition)
{
    if (!condition)
    {
        return "";
    }
    if (condition->type == Where_clause::ConditionType::SIMPLE)
    {
        return condition->left_operand + " " + condition->operator_ + " " + condition->right_operand;
    }
    else if (condition->type == Where_clause::ConditionType::AND || condition->type == Where_clause::ConditionType::OR)
    {
        std::string op = (condition->type == Where_clause::ConditionType::AND) ? " and " : " or ";
        std::string result = "(";
        for (size_t i = 0; i < condition->children.size(); ++i)
        {
            result += condition_to_string(condition->children[i]);
            if (i < condition->children.size() - 1)
            {
                result += op;
            }
        }
        result += ")";
        return result;
    }
    else if (condition->type == Where_clause::ConditionType::NOT)
    {
        return "not (" + condition_to_string(condition->children.front()) + ")";
    }
    return "";
}
