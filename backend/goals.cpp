#include "goals.h"
#include <sstream>
#include <boost/format.hpp>
#include <iostream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <set>
#include "clauses/common.h"
#include "clauses/insert_clause.h"
#include "clauses/select/where_clause.h"
#include "clauses/select/select_clause.h"
#include "clauses/select/group_by_clause.h"
#include "clauses/select/having_clause.h"
#include "clauses/select/order_by_clause.h"
#include "clauses/assertion_clause.h"
#include "clauses/create_clause.h"
#include "clauses/update_clause.h"
#include "clauses/delete_clause.h"
#include "clauses/create_view.h"
#include "clauses/alter_clause.h"

Goals::Goals()
{
    // empty constructor
}
std::vector<Goals::Goal> Goals::process_query(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node)
{
    AbstractSyntaxTree ast;
    std::vector<Goal> goals;
    Insert_clause insert_clause;
    Select_clause select_clause;
    Where_clause where_clause;
    From_clause from_clause;

    if (!root_node)
    {
        return goals;
    }

    // Detect statement type by the key:
    std::string stmt_type = ast.get_statement_type(root_node);
    auto stmt_node = ast.get_statement_node(root_node);

    if (stmt_type == "SelectStmt")
    {
        // Step 1: Process FROM clause
        auto from_result = From_clause::process(root_node);
        if (!from_result.first.empty())
        {
            Goal goal;
            goal.type = "From";
            goal.content = from_result.first;
            goals.push_back(goal);
        }

        // Step 2: Process SELECT clause
        auto select_result = Select_clause::process(root_node, from_result.second);
        if (!select_result.first.empty())
        {
            Goal goal;
            goal.type = "Select";
            goal.content = select_result.first;
            goals.push_back(goal);
        }

        // Step 3: Process WHERE clause
        auto where_result = Where_clause::process(root_node, from_result.second, select_result.second);
        if (!where_result.first.empty())
        {
            Goal goal;
            goal.type = "Where";
            goal.content = where_result.first;
            goals.push_back(goal);
        }

        // Step 4: Process GROUP BY clause
        auto group_by_result = Group_by_clause::process(root_node, from_result.second);
        if (!group_by_result.first.empty())
        {
            Goal goal;
            goal.type = "Group_by";
            goal.content = group_by_result.first;
            goals.push_back(goal);
        }

        // Step 5: Process HAVING clause
        auto having_result = Having_clause::process(root_node, from_result.second, select_result.second);
        if (!having_result.first.empty())
        {
            Goal goal;
            goal.type = "Having";
            goal.content = having_result.first;
            goals.push_back(goal);
        }

        // Step 6: Process ORDER BY clause
        auto order_by_result = Order_by_clause::process(root_node, from_result.second, select_result.second);
        if (!order_by_result.first.empty())
        {
            Goal goal;
            goal.type = "Order_by";
            goal.content = order_by_result.first;
            goals.push_back(goal);
        }
    }
    else if (stmt_type == "CreateStmt")
    {
        // CREATE TABLE or other CREATE variant
        auto create_result = Create_clause::process(stmt_node);
        if (!create_result.first.empty())
        {
            goals.push_back({"Create", create_result.first});
        }
    }
    else if (stmt_type == "UpdateStmt")
    {
        // UPDATE statement
        // For UPDATE, we might still want FROM info (in case needed)
        // but typically UPDATEStmt does not have a fromClause. If needed, parse fromClause if present.
        From_clause::from_clause_info dummy_from;
        auto update_result = Update_clause::process(stmt_node, dummy_from);
        if (!update_result.first.empty())
        {
            goals.push_back({"Update", update_result.first});
        }
    }
    else if (stmt_type == "DeleteStmt")
    {
        // DELETE statement
        From_clause::from_clause_info dummy_from;
        auto delete_result = Delete_clause::process(stmt_node, dummy_from);
        if (!delete_result.first.empty())
        {
            goals.push_back({"Delete", delete_result.first});
        }
    }
    else if (stmt_type == "CreateAssertionStmt")
    {
        // CREATE ASSERTION
        auto assertion_result = Assertion_clause::process(stmt_node);
        if (!assertion_result.first.empty())
        {
            goals.push_back({"Assertion", assertion_result.first});
        }
    }
    else if (stmt_type == "InsertStmt")
    {
        auto insert_result = Insert_clause::process(stmt_node);
        if (!insert_result.first.empty())
        {
            Goal goal;
            goal.type = "Insert";
            goal.content = insert_result.first;
            goals.push_back(goal);
        }
    }
    else if (stmt_type == "ViewStmt")
    {
        auto create_view_result = Create_clause::process(stmt_node);
        if (!create_view_result.first.empty())
        {
            goals.push_back({"CreateView", create_view_result.first});
        }
    }
    else if (stmt_type == "AlterTableStmt")
    {
        auto alter_result = Alter_clause::process(stmt_node);
        if (!alter_result.first.empty())
        {
            goals.push_back({"Alter", alter_result.first});
        }
    }
    else
    {
        // Other statement types (e.g., DROP) can be added similarly
        goals.push_back({"Unknown", "Statement type not handled."});
    }

    // Return the vector of goals
    return goals;
}
Common::comparision_result Goals::compare_single_statement(const std::shared_ptr<AbstractSyntaxTree::Node> &reference_root, const std::shared_ptr<AbstractSyntaxTree::Node> &student_root)
{
    Common::comparision_result info;
    AbstractSyntaxTree ast;

    // Initialize the output string
    std::ostringstream oss;
    std::string success_message;

    if (!reference_root || !student_root)
    {
        info.message = "One or both statements are empty.";
        return info;
    }

    // Extract statement nodes
    std::string ref_type = ast.get_statement_type(reference_root);
    std::string stu_type = ast.get_statement_type(student_root);
    auto ref_stmt_node = ast.get_statement_node(reference_root);
    auto stu_stmt_node = ast.get_statement_node(student_root);

    if (ref_type != stu_type)
    {
        // Different statement types
        oss << "The correct statement is a " << ref_type << " but your statement is a " << stu_type << ".\n";
        info.message = oss.str();
        info.incorrect_parts.push_back("Statement Type");
        return info;
    }

    if (ref_type == "SelectStmt")
    {

        // Process the reference query
        auto reference_goals = process_query(reference_root);
        // Process the student query
        auto student_goals = process_query(student_root);

        // Initialize variables to hold clause info
        Select_clause::select_clause_info ref_select_info, stu_select_info;
        From_clause::from_clause_info ref_from_info, stu_from_info;
        Where_clause::where_clause_info ref_where_info, stu_where_info;
        Group_by_clause::group_by_clause_info ref_group_by_info, stu_group_by_info;
        Where_clause::where_clause_info ref_having_info, stu_having_info;
        Order_by_clause::order_by_clause_info ref_order_by_info, stu_order_by_info;

        // Process the clauses and collect info
        // Reference Query
        auto ref_from_result = From_clause::process(reference_root);
        ref_from_info = ref_from_result.second;

        auto ref_select_result = Select_clause::process(reference_root, ref_from_info);
        ref_select_info = ref_select_result.second;

        auto ref_where_result = Where_clause::process(reference_root, ref_from_info, ref_select_info);
        ref_where_info = ref_where_result.second;

        auto ref_group_by_result = Group_by_clause::process(reference_root, ref_from_info);
        ref_group_by_info = ref_group_by_result.second;

        auto ref_having_result = Having_clause::process(reference_root, ref_from_info, ref_select_info);
        ref_having_info = ref_having_result.second;

        auto ref_order_by_result = Order_by_clause::process(reference_root, ref_from_info, ref_select_info);
        ref_order_by_info = ref_order_by_result.second;

        // Student Query
        auto stu_from_result = From_clause::process(student_root);
        stu_from_info = stu_from_result.second;

        auto stu_select_result = Select_clause::process(student_root, stu_from_info);
        stu_select_info = stu_select_result.second;

        auto stu_where_result = Where_clause::process(student_root, stu_from_info, stu_select_info);
        stu_where_info = stu_where_result.second;

        auto stu_group_by_result = Group_by_clause::process(student_root, stu_from_info);
        stu_group_by_info = stu_group_by_result.second;

        auto stu_having_result = Having_clause::process(student_root, stu_from_info, stu_select_info);
        stu_having_info = stu_having_result.second;

        auto stu_order_by_result = Order_by_clause::process(student_root, stu_from_info, stu_select_info);
        stu_order_by_info = stu_order_by_result.second;

        // Keep track of correctly matched clauses
        std::vector<std::string> correct_clauses;
        // Keep track of clauses with problems
        std::vector<std::string> problem_clauses;
        // Initialize variables to hold suggestions for next steps
        std::vector<std::string> next_steps;
        std::vector<std::string> next_steps_general;

        // Compare SELECT clauses
        auto select_comparison = Select_clause::compare(ref_select_info, stu_select_info, next_steps);
        if (select_comparison.first == 1)
        {
            correct_clauses.push_back("SELECT clause");
        }
        else
        {
            problem_clauses.push_back("SELECT clause");
            // oss << "Problem in SELECT clause: " << select_comparison.second << "\n";
            next_steps_general.push_back("Review the SELECT clause to ensure you are selecting the correct columns, functions, or expressions as specified in the goal.");
        }

        // Compare FROM clauses
        auto from_comparison = From_clause::compare(ref_from_info, stu_from_info);
        if (from_comparison.first == 1)
        {
            correct_clauses.push_back("FROM clause");
        }
        else
        {
            problem_clauses.push_back("FROM clause");
            // oss << "Problem in FROM clause: " << from_comparison.second << "\n";
            next_steps.push_back("Check the FROM clause to ensure you are using the correct tables and joins as required.");
        }

        // Compare WHERE clauses
        auto where_comparison = Where_clause::compare(ref_where_info, stu_where_info, next_steps);
        if (where_comparison.first == 1)
        {
            correct_clauses.push_back("WHERE clause");
        }
        else if (where_comparison.first == -1)
        {
            // Both queries have no WHERE clause
        }
        else
        {
            problem_clauses.push_back("WHERE clause");
            // oss << "Problem in WHERE clause: " << where_comparison.second << "\n";
        }

        // Compare GROUP BY clauses
        auto group_by_comparison = Group_by_clause::compare(ref_group_by_info, stu_group_by_info);
        if (group_by_comparison.first == 1)
        {
            correct_clauses.push_back("GROUP BY clause");
        }
        else if (group_by_comparison.first == -1)
        {
            // Both queries have no GROUP BY clause
        }
        else
        {
            problem_clauses.push_back("GROUP BY clause");
            // oss << "Problem in GROUP BY clause: " << group_by_comparison.second << "\n";
            next_steps.push_back("Review the GROUP BY clause to ensure you are grouping by the correct columns or expressions.");
        }

        // Compare HAVING clauses
        auto having_comparison = Having_clause::compare(ref_having_info, stu_having_info);
        if (having_comparison.first == 1)
        {
            correct_clauses.push_back("HAVING clause");
        }
        else if (having_comparison.first == -1)
        {
            // Both queries have no HAVING clause
        }
        else
        {
            problem_clauses.push_back("HAVING clause");
            // oss << "Problem in HAVING clause: " << having_comparison.second << "\n";
            next_steps.push_back("Check the HAVING clause to make sure the conditions applied to the grouped data are correct.");
        }

        // Compare ORDER BY clauses
        auto order_by_comparison = Order_by_clause::compare(ref_order_by_info, stu_order_by_info);
        if (order_by_comparison.first == 1)
        {
            correct_clauses.push_back("ORDER BY clause");
        }
        else if (order_by_comparison.first == -1)
        {
            // Both queries have no ORDER BY clause
        }
        else
        {
            problem_clauses.push_back("ORDER BY clause");
            // oss << "Problem in ORDER BY clause: " << order_by_comparison.second << "\n";
            next_steps.push_back("Adjust the ORDER BY clause to sort the results as specified.");
        }

        // Build the output string

        // 1. What the student query managed
        // oss << "You have correctly constructed the following clauses:\n";
        oss << "2️⃣ Correct Clauses:\n";
        int count = 0;
        for (const auto &clause : correct_clauses)
        {
            oss << "✅ " << clause << "\n";
            count++;
        }
        if (count == 0)
        {
            oss << "●  None.\n";
        }
        oss << "\n";

        // 2. Where the problem was
        if (!problem_clauses.empty())
        {
            // oss << "There are issues with the following clauses:\n";
            oss << "3️⃣ Issues found in the following clauses:\n";
            for (const auto &clause : problem_clauses)
            {
                oss << "❌ " << clause << "\n";
            }
            oss << "\n";

            // Include detailed problems
            int count = 1;
            oss << "❗ Problem Details:\n";
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "SELECT clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". SELECT clause: " << select_comparison.second << "\n";
            }
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "FROM clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". FROM clause: " << from_comparison.second << "\n";
            }
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "WHERE clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". WHERE clause: " << where_comparison.second << "\n";
            }
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "GROUP BY clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". GROUP BY clause: " << group_by_comparison.second << "\n";
            }
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "HAVING clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". HAVING clause: " << having_comparison.second << "\n";
            }
            if (std::find(problem_clauses.begin(), problem_clauses.end(), "ORDER BY clause") != problem_clauses.end())
            {
                oss << std::to_string(count++) << ". ORDER BY clause: " << order_by_comparison.second << "\n";
            }
            oss << "\n";
        }

        // 3. What to do next
        if (!next_steps.empty())
        {
            oss << "4️⃣ Next steps:\n";
            for (const auto &step : next_steps)
            {
                oss << "➡️ " << step << "\n";
            }
        }
        else
        {
            oss << "Excellent! Your query matches the reference query.\n";
        }

        info.correct_parts = correct_clauses;
        info.incorrect_parts = problem_clauses;
        info.message = oss.str();

        return info;
    }
    else if (ref_type == "CreateStmt")
    {
        auto ref_create_info = Create_clause::get_info(ref_stmt_node);
        auto stu_create_info = Create_clause::get_info(stu_stmt_node);
        auto create_cmp = Create_clause::compare(ref_create_info, stu_create_info);

        info = create_cmp;
        success_message = "Excellent! Your CREATE statement matches the reference.\n";
    }
    else if (ref_type == "ViewStmt")
    {
        auto ref_view_info = Create_view::get_info(ref_stmt_node);
        auto stu_view_info = Create_view::get_info(stu_stmt_node);
        auto view_cmp = Create_view::compare(ref_view_info, stu_view_info);
        info = view_cmp;
        success_message = "Excellent! Your CREATE VIEW statement matches the reference.\n";
    }
    else if (ref_type == "UpdateStmt")
    {
        From_clause::from_clause_info dummy_from;
        auto ref_update_result = Update_clause::process(ref_stmt_node, dummy_from);
        auto stu_update_result = Update_clause::process(stu_stmt_node, dummy_from);
        auto update_cmp = Update_clause::compare(ref_update_result.second, stu_update_result.second);

        info = update_cmp;
        success_message = "Excellent! Your UPDATE statement matches the reference.\n";
    }
    else if (ref_type == "DeleteStmt")
    {
        From_clause::from_clause_info dummy_from;
        auto ref_delete_result = Delete_clause::process(ref_stmt_node, dummy_from);
        auto stu_delete_result = Delete_clause::process(stu_stmt_node, dummy_from);
        auto delete_cmp = Delete_clause::compare(ref_delete_result.second, stu_delete_result.second);

        info = delete_cmp;
        success_message = "Excellent! Your DELETE statement matches the reference.\n";
    }
    else if (ref_type == "CreateAssertionStmt")
    {
        auto ref_assertion_result = Assertion_clause::process(ref_stmt_node);
        auto stu_assertion_result = Assertion_clause::process(stu_stmt_node);
        auto assertion_cmp = Assertion_clause::compare(ref_assertion_result.second, stu_assertion_result.second);

        info = assertion_cmp;
        success_message = "Excellent! Your ASSERTION statement matches the reference.\n";
    }
    else if (ref_type == "InsertStmt")
    {
        auto ref_insert_info = Insert_clause::get_info(ref_stmt_node);
        auto stu_insert_info = Insert_clause::get_info(stu_stmt_node);
        auto insert_cmp = Insert_clause::compare(ref_insert_info, stu_insert_info);

        info = insert_cmp;
        success_message = "Excellent! Your INSERT statement matches the reference.\n";
    }
    else if (ref_type == "AlterTableStmt")
    {
        auto ref_alter_info = Alter_clause::get_info(ref_stmt_node);
        auto stu_alter_info = Alter_clause::get_info(stu_stmt_node);
        auto alter_cmp = Alter_clause::compare(ref_alter_info, stu_alter_info);

        info = alter_cmp;
        success_message = "Excellent! Your ALTER statement matches the reference.\n";
    }
    else if (ref_type == "CreateViewStmt")
    {
        auto ref_create_view_info = Create_view::get_info(ref_stmt_node);
        auto stu_create_view_info = Create_view::get_info(stu_stmt_node);
        auto create_view_cmp = Create_view::compare(ref_create_view_info, stu_create_view_info);

        info = create_view_cmp;
        success_message = "Excellent! Your CREATE VIEW statement matches the reference.\n";
    }
    else
    {
        oss << "Unsupported statement type: " << ref_type << "\n";
        info.message = oss.str();
        return info;
    }

    // Build the final feedback message.
    std::ostringstream final_msg;
    final_msg << "2️⃣ Correct Components:\n";
    if (!info.correct_parts.empty())
    {
        for (const auto &clause : info.correct_parts)
        {
            final_msg << "✅ " << clause << "\n";
        }
    }
    else
    {
        final_msg << "● None.\n";
    }
    final_msg << "\n";

    if (!info.incorrect_parts.empty())
    {
        final_msg << "3️⃣ Issues found in the following components:\n";
        for (const auto &clause : info.incorrect_parts)
        {
            final_msg << "❌ " << clause << "\n";
        }
        if (!info.message.empty())
        {
            final_msg << "\n❗ Problem Details:\n"
                      << info.message << "\n";
        }
    }

    if (!info.next_steps.empty())
    {
        final_msg << "4️⃣ Next steps:\n";
        for (const auto &step : info.next_steps)
        {
            final_msg << step << "\n";
        }
    }
    else
    {
        final_msg << success_message;
    }

    info.message = final_msg.str();

    return info;
}
Common::comparision_result Goals::compare_queries(const std::shared_ptr<AbstractSyntaxTree::Node> &reference_root, const std::shared_ptr<AbstractSyntaxTree::Node> &student_root)
{
    Common::comparision_result overall_info;
    std::ostringstream overall_oss;

    if (!reference_root || !student_root)
    {
        overall_info.message = "One or both statements are empty.";
        return overall_info;
    }

    // Extract all statement nodes from both ASTs.
    auto ref_statements = AbstractSyntaxTree::get_statements(reference_root);
    auto stu_statements = AbstractSyntaxTree::get_statements(student_root);

    // Keep track of which student statements we've used
    std::vector<bool> stu_used(stu_statements.size(), false);

    // 1) For each reference stmt, find a student stmt of the same type
    for (auto &ref_stmt : ref_statements)
    {
        std::string ref_type = AbstractSyntaxTree::get_statement_type(ref_stmt);

        // search for a matching student statement
        int match_idx = -1;
        for (size_t j = 0; j < stu_statements.size(); ++j)
        {
            if (!stu_used[j] &&
                AbstractSyntaxTree::get_statement_type(stu_statements[j]) == ref_type)
            {
                match_idx = (int)j;
                break;
            }
        }

        // get the reference query goal
        auto ref_goal = generate_goal_for_stmt_general(ref_stmt);

        if (match_idx >= 0)
        {
            // found a student stmt of same type → compare them
            auto result = compare_single_statement(ref_stmt, stu_statements[match_idx]);
            overall_oss << "1️⃣ Goal:\n"
                        << ref_goal << "\n\n";
            overall_oss << result.message;
            overall_info.correct_parts.insert(overall_info.correct_parts.end(), result.correct_parts.begin(), result.correct_parts.end());
            overall_info.incorrect_parts.insert(overall_info.incorrect_parts.end(), result.incorrect_parts.begin(), result.incorrect_parts.end());
            overall_info.next_steps.insert(overall_info.next_steps.end(), result.next_steps.begin(), result.next_steps.end());

            stu_used[match_idx] = true;
        }
        else
        {
            // no student statement of this type found
            overall_oss << "Missing " << ref_type << " statement.\n";
            overall_info.incorrect_parts.push_back(ref_type + " statement");
        }
    }

    // 2) Any remaining student statements are “extra”
    for (size_t j = 0; j < stu_statements.size(); ++j)
    {
        if (!stu_used[j])
        {
            std::string extra_type = AbstractSyntaxTree::get_statement_type(stu_statements[j]);
            overall_oss << "Extra " << extra_type << " statement.\n";
            overall_info.incorrect_parts.push_back("Extra " + extra_type);
        }
    }

    overall_info.message = overall_oss.str();
    return overall_info;
}

std::vector<std::string>
Goals::generate_query_goal_general(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node)
{
    std::vector<std::string> goals;
    AbstractSyntaxTree ast;

    if (!root_node)
    {
        goals.emplace_back("No query provided.");
        return goals;
    }

    // 1) extract all statements
    auto stmts = ast.get_statements(root_node);
    if (stmts.empty())
    {
        goals.emplace_back("No valid statement found in the given query.");
        return goals;
    }

    // 2) generate a goal for each
    for (auto &stmt_node : stmts)
    {
        goals.push_back(generate_goal_for_stmt_general(stmt_node));
    }

    return goals;
}
std::string Goals::generate_goal_for_stmt_general(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node)
{
    AbstractSyntaxTree ast;
    Insert_clause insert_clause;
    if (!root_node)
    {
        return "No query provided.";
    }

    // Retrieve the statement node
    auto stmt_node = ast.get_statement_node(root_node);
    if (!stmt_node)
    {
        return "No valid statement found in the given query.";
    }

    // Get the statement type
    std::string stmt_type = ast.get_statement_type(root_node);
    if (stmt_type.empty())
    {
        return "Cannot determine the statement type.";
    }

    // Initialize a string stream to build the goal sentence
    std::ostringstream oss;
    oss << "The query is supposed to:";

    if (stmt_type == "SelectStmt")
    {
        // Process each clause of the query to extract information
        auto from_result = From_clause::process(root_node);
        auto select_result = Select_clause::process(root_node, from_result.second);
        auto where_result = Where_clause::process(root_node, from_result.second, select_result.second);
        auto group_by_result = Group_by_clause::process(root_node, from_result.second);
        auto having_result = Having_clause::process(root_node, from_result.second, select_result.second);
        auto order_by_result = Order_by_clause::process(root_node, from_result.second, select_result.second);

        // FROM clause: Identify data sources
        size_t table_count = from_result.second.tables.size();
        if (table_count > 0)
        {
            oss << "\n ● Retrieve data from " << table_count << " table" << (table_count > 1 ? "s" : "") << "; ";
        }

        // SELECT clause: Specify columns or expressions to select
        size_t select_item_count = select_result.second.the_columns.size() +
                                   select_result.second.the_functions.size() +
                                   select_result.second.the_expressions.size();
        if (select_item_count > 0)
        {
            oss << "\n ● Select " << select_item_count << " column" << (select_item_count > 1 ? "s" : "") << "; ";
        }

        // WHERE clause: Apply filters to data
        if (where_result.second.condition_root)
        {
            oss << "\n ● Filter data based on certain conditions; ";
        }

        // GROUP BY clause: Group data
        size_t group_by_item_count = group_by_result.second.the_columns.size() +
                                     group_by_result.second.the_functions.size() +
                                     group_by_result.second.the_expressions.size();
        if (group_by_item_count > 0)
        {
            oss << "\n ● Group the results by " << group_by_item_count << " element" << (group_by_item_count > 1 ? "s" : "") << "; ";
        }

        // HAVING clause: Filter grouped data
        if (having_result.second.condition_root)
        {
            oss << "\n ● Filter grouped data based on certain conditions; ";
        }

        // ORDER BY clause: Sort the results
        size_t order_by_item_count = order_by_result.second.order_items.size();
        if (order_by_item_count > 0)
        {
            oss << "\n ● Sort the output data by " << order_by_item_count << " column" << (order_by_item_count > 1 ? "s" : "") << "; ";
        }
    }
    else if (stmt_type == "CreateStmt")
    {
        // For CREATE statements, you'd implement Create_clause::create_clause_process if not already done
        auto create_result = Create_clause::process(stmt_node);
        oss << "\n ● " << create_result.first;
    }
    else if (stmt_type == "UpdateStmt")
    {
        // For UPDATE statements
        From_clause::from_clause_info dummy_from;
        auto update_result = Update_clause::process(stmt_node, dummy_from);
        oss << "\n ● " << update_result.first;
    }
    else if (stmt_type == "DeleteStmt")
    {
        // For DELETE statements
        From_clause::from_clause_info dummy_from;
        auto delete_result = Delete_clause::process(stmt_node, dummy_from);
        oss << "\n ● " << delete_result.first;
    }
    else if (stmt_type == "CreateAssertionStmt")
    {
        // For ASSERTION statements
        auto assertion_result = Assertion_clause::process(stmt_node);
        oss << "\n ● " << assertion_result.first;
    }
    else if (stmt_type == "InsertStmt")
    {
        // NEW: Generate a general goal for INSERT statements
        auto insert_result = Insert_clause::process(stmt_node);
        oss << "\n ● " << insert_result.first;
    }
    else if (stmt_type == "ViewStmt")
    {
        // For CREATE VIEW statements
        auto create_view_result = Create_view::process(stmt_node);
        oss << "\n ● " << create_view_result.first;
    }
    else if (stmt_type == "AlterTableStmt")
    {
        // For ALTER statements
        auto alter_result = Alter_clause::process(stmt_node);
        oss << "\n ● " << alter_result.first;
    }
    else
    {
        oss << "\n ● This query type is not supported yet.";
    }

    // Return the constructed goal sentence
    return oss.str();
}

std::vector<std::string>
Goals::generate_query_goal_specific(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node)
{
    std::vector<std::string> goals;
    AbstractSyntaxTree ast;

    if (!root_node)
    {
        goals.emplace_back("No query provided.");
        return goals;
    }

    auto stmts = ast.get_statements(root_node);
    if (stmts.empty())
    {
        goals.emplace_back("No valid statement found in the given query.");
        return goals;
    }

    for (auto &stmt_node : stmts)
    {
        goals.push_back(generate_goal_for_stmt_specific(stmt_node));
    }

    return goals;
}

std::string Goals::generate_goal_for_stmt_specific(const std::shared_ptr<AbstractSyntaxTree::Node> &root_node)
{
    AbstractSyntaxTree ast;
    Insert_clause insert_clause;
    if (!root_node)
    {
        return "No query provided.";
    }

    // Retrieve the statement node
    auto stmt_node = ast.get_statement_node(root_node);
    if (!stmt_node)
    {
        return "No valid statement found in the given query.";
    }

    // Get the statement type
    std::string stmt_type = ast.get_statement_type(root_node);
    if (stmt_type.empty())
    {
        return "Cannot determine the statement type.";
    }

    // Initialize a string stream to build the goal sentence
    std::ostringstream oss;
    oss << "The query is supposed to: ";

    if (stmt_type == "SelectStmt")
    {
        // Process each clause of the query to extract information
        auto from_result = From_clause::process(root_node);
        auto select_result = Select_clause::process(root_node, from_result.second);
        auto where_result = Where_clause::process(root_node, from_result.second, select_result.second);
        auto group_by_result = Group_by_clause::process(root_node, from_result.second);
        auto having_result = Having_clause::process(root_node, from_result.second, select_result.second);
        auto order_by_result = Order_by_clause::process(root_node, from_result.second, select_result.second);

        // FROM clause: Identify data sources
        if (!from_result.second.tables.empty())
        {
            oss << "\n ● Retrieve data from ";
            for (size_t i = 0; i < from_result.second.tables.size(); ++i)
            {
                oss << "'" << from_result.second.tables[i].table_name << "'";
                if (i < from_result.second.tables.size() - 2)
                    oss << ", ";
                else if (i == from_result.second.tables.size() - 2)
                    oss << " and ";
            }
        }

        // SELECT clause: Specify columns or expressions to select
        if (!select_result.second.the_columns.empty() || !select_result.second.the_functions.empty() || !select_result.second.the_expressions.empty())
        {
            oss << "\n ● Select ";
            std::vector<std::string> select_items;
            for (const auto &col : select_result.second.the_columns)
                select_items.push_back(col.column_name);
            for (const auto &func : select_result.second.the_functions)
                select_items.push_back(func.function_name);
            for (const auto &expr : select_result.second.the_expressions)
                select_items.push_back(expr.expression);

            for (size_t i = 0; i < select_items.size(); ++i)
            {
                oss << "'" << select_items[i] << "'";
                if (i < select_items.size() - 2)
                    oss << ", ";
                else if (i == select_items.size() - 2)
                    oss << " and ";
            }
        }

        // WHERE clause: Apply filters to data
        if (where_result.second.condition_root)
        {
            oss << "\n ● Filter data where " << Where_clause::condition_to_string(where_result.second.condition_root);
        }

        // GROUP BY clause: Group data
        if (!group_by_result.second.the_columns.empty() || !group_by_result.second.the_functions.empty() || !group_by_result.second.the_expressions.empty())
        {
            oss << "\n ●  Group the results by ";
            std::vector<std::string> group_by_items;
            for (const auto &col : group_by_result.second.the_columns)
                group_by_items.push_back(col.column_name);
            for (const auto &func : group_by_result.second.the_functions)
                group_by_items.push_back(func.function_name);
            for (const auto &expr : group_by_result.second.the_expressions)
                group_by_items.push_back(expr.expression);

            for (size_t i = 0; i < group_by_items.size(); ++i)
            {
                oss << "'" << group_by_items[i] << "'";
                if (i < group_by_items.size() - 2)
                    oss << ", ";
                else if (i == group_by_items.size() - 2)
                    oss << " and ";
            }
        }

        // HAVING clause: Filter grouped data
        if (having_result.second.condition_root)
        {
            oss << "\n ● Filter grouped data having " << Where_clause::condition_to_string(having_result.second.condition_root);
        }

        // ORDER BY clause: Sort the results
        if (!order_by_result.second.order_items.empty())
        {
            oss << "\n ● Sort the output data by ";
            for (size_t i = 0; i < order_by_result.second.order_items.size(); ++i)
            {
                oss << "'" << order_by_result.second.order_items[i].expression << "'";
                if (order_by_result.second.order_items[i].direction == "SORTBY_ASC")
                    oss << " ascending";
                else if (order_by_result.second.order_items[i].direction == "SORTBY_DESC")
                    oss << " descending";

                if (i < order_by_result.second.order_items.size() - 2)
                    oss << ", ";
                else if (i == order_by_result.second.order_items.size() - 2)
                    oss << " and ";
            }
        }
    }
    else if (stmt_type == "CreateStmt")
    {
        // CREATE
        auto create_result = Create_clause::process(stmt_node);
        oss << "\n ● " << create_result.first;
    }
    else if (stmt_type == "UpdateStmt")
    {
        // UPDATE
        From_clause::from_clause_info dummy_from;
        auto update_result = Update_clause::process(stmt_node, dummy_from);
        oss << "\n ● " << update_result.first;
    }
    else if (stmt_type == "DeleteStmt")
    {
        // DELETE
        From_clause::from_clause_info dummy_from;
        auto delete_result = Delete_clause::process(stmt_node, dummy_from);
        oss << "\n ● " << delete_result.first;
    }
    else if (stmt_type == "CreateAssertionStmt")
    {
        // ASSERTION
        auto assertion_result = Assertion_clause::process(stmt_node);
        oss << "\n ● " << assertion_result.first;
    }
    else if (stmt_type == "InsertStmt")
    {
        auto insert_result = Insert_clause::process(stmt_node);
        oss << "\n ● " << insert_result.first;
    }
    else if (stmt_type == "ViewStmt")
    {
        // CREATE VIEW
        auto create_view_result = Create_view::process(stmt_node);
        oss << "\n ● " << create_view_result.first;
    }
    else if (stmt_type == "AlterTableStmt")
    {
        // ALTER
        auto alter_result = Alter_clause::process(stmt_node);
        oss << "\n ● " << alter_result.first;
    }
    else
    {
        oss << "\n ● This query type is not supported yet.";
    }

    // Return the constructed goal sentence
    return oss.str();
}
