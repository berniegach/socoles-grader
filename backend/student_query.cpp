#include "student_query.h"
#include "utils.h"
#include "my_duckdb.h"
#include <iostream>
#include "tree_edit_distance.h"
#include "goals.h"
#include "clauses/common.h"

StudentQuery::StudentQuery(const string &id, const string &value)
{
    set_id(id);
    set_value(value);
    value_changed = false;
    text_edit_distance = 0;
    tree_edit_distance = 0;
    feedback = "";
    set_parseable(false);
}

StudentQuery::StudentQuery(const string &id, const string &value, const string question_number, const int attempt_number)
{
    set_id(id);
    set_value(value);
    this->question_number = question_number;
    this->attempt_number = attempt_number;
    value_changed = false;
    text_edit_distance = 0;
    tree_edit_distance = 0;
    feedback = "";
    // Set the default value for parseable to false so that an query with a syntax error will get a 0 for syntax outcome.
    set_parseable(false);
}

void StudentQuery::set_parseable(bool parseable)
{
    this->parseable = parseable;
}

bool StudentQuery::is_parseable() const
{
    return parseable;
}

void StudentQuery::set_syntax_outcome(Grader::property_state syntax_outcome)
{
    this->syntax_outcome = syntax_outcome;
}

Grader::property_state StudentQuery::get_syntax_outcome() const
{
    return syntax_outcome;
}

void StudentQuery::set_semantics_outcome(Grader::property_state semantics_outcome)
{
    this->semantics_outcome = semantics_outcome;
}

Grader::property_state StudentQuery::get_semantics_outcome() const
{
    return semantics_outcome;
}

void StudentQuery::set_results_outcome(Grader::property_state results_outcome)
{
    this->results_outcome = results_outcome;
}

Grader::property_state StudentQuery::get_results_outcome() const
{
    return results_outcome;
}

void StudentQuery::set_correctness_level(int correctness_level)
{
    this->correctness_level = correctness_level;
}

int StudentQuery::get_correctness_level() const
{
    return correctness_level;
}

void StudentQuery::set_execution_error(const string &execution_error)
{
    this->execution_error = execution_error;
}

string StudentQuery::get_execution_error() const
{
    return execution_error;
}

void StudentQuery::set_feedback(const string &feedback)
{
    this->feedback = feedback;
}

string StudentQuery::get_feedback() const
{
    return feedback;
}

void StudentQuery::add_feedback(const string &feedback)
{
    this->feedback += feedback;
}

void StudentQuery::result_analysis(const Admin &admin, const vector<vector<string>> &expected_output)
{
    Utils my_utils;

    // first check if we are supposed to check the property
    if (admin.get_results_sensitivity() == Grader::property_level::ABSENT)
    {
        set_results_outcome(Grader::property_state::INVALID);
        return;
    }

    Utils::comparison_result comparison_result = my_utils.compare_output(get_output(), expected_output, admin.get_check_order());

    if (comparison_result.is_equal)
    {
        // add positive feedback about semantics .
        add_feedback(" RESULTS: Correct! Well done. ");
        set_results_outcome(Grader::property_state::CORRECT);
    }
    else if (comparison_result.is_subset && comparison_result.difference > 0)
    {
        if (admin.get_results_sensitivity() == Grader::property_level::THREE_LEVELS)
        {
            set_results_outcome(Grader::property_state::MINOR_INCORRECT);
        }
        else
        {
            set_results_outcome(Grader::property_state::INCORRECT);
        }
        add_feedback(" RESULTS: Not correct! Though, the correct results are contained within the query results,  they were not correctly filtered. ");
        set_results_analysis_message("Correct results are contained within the query results.\n");
    }
    else
    {
        // Build structured info
        auto ref_info = MyDuckDB::get_info(expected_output);
        auto stu_info = MyDuckDB::get_info(get_output());

        // Compare and get detailed result
        Common::comparision_result comp = MyDuckDB::compare(ref_info, stu_info);

        // Emit feedback
        set_results_analysis_message(comp.message);
        set_results_outcome(Grader::property_state::INCORRECT);
        add_feedback(" RESULTS: The output of the query is not correct. ");
    }
    print_output();
}

void StudentQuery::syntax_analysis(const Admin &admin, vector<ModelQuery> &model_queries, vector<StudentQuery> &student_queries, Query_Engine &qe)
{
    Utils my_utils;

    // first check if we are supposed to check syntax
    if (admin.get_syntax_sensitivity() == Grader::property_level::ABSENT)
    {
        set_syntax_outcome(Grader::property_state::INVALID);
        return;
    }

    if (is_parseable())
    {
        // add positive feedback about syntax .
        add_feedback(" SYNTAX: Correct! Well done. ");
        set_syntax_outcome(Grader::property_state::CORRECT);
    }
    else
    {
        // create feedeback that the query is not parseable.
        add_feedback(" SYNTAX: The query is not parseable. ");

        // first lets try with model queries.
        // we will use the levenshteins distance to find the closest query

        size_t min_distance = 1000;
        std::string best_query;
        bool is_corrected = false;

        for (size_t i = 0; i < model_queries.size(); i++)
        {
            ModelQuery *query = &model_queries.at(i);
            std::string model_query = query->get_value();
            std::string student_query = get_value();

            // Preprocess the queries to lower case to ensure the distance calculation is accurate
            my_utils.preprocess_query(model_query);
            my_utils.preprocess_query(student_query);

            // Calculate edit distance or perform the correction
            std::tuple<bool, std::string, int> corrected = my_utils.fix_query_syntax_using_another_query(admin, student_query, model_query, get_value());
            if (std::get<0>(corrected))
            {
                size_t current_distance = std::get<2>(corrected);
                if (current_distance < min_distance)
                {
                    min_distance = current_distance;
                    best_query = std::get<1>(corrected);
                    is_corrected = true;
                }
            }
        }
        // create a lambda function that does the same thing as the below check
        auto update_query = [&]()
        {
            add_feedback(" SYNTAX MINOR ERROR: To make the query parseable, it was changed to, '" + best_query + "'. ");
            set_syntax_analysis_message("Syntax minor error. The query was changed to '" + best_query + "' make it parseable.\n");
            old_value = get_value();
            set_value(best_query);
            value_changed = true;
            set_parseable(true);
            create_abstract_syntax_tree();
            create_fingerprint();
            create_output(qe);
            if (admin.get_syntax_sensitivity() == Grader::property_level::THREE_LEVELS)
            {
                set_syntax_outcome(Grader::property_state::MINOR_INCORRECT);
            }
            else
            {
                set_syntax_outcome(Grader::property_state::INCORRECT);
            }
        };
        if (is_corrected)
        {

            update_query();
            return;
        }
        min_distance = 1000;

        // second try to fix the query using sql kewords
        std::tuple<bool, string> corrected = my_utils.fix_query_syntax_using_keywords(get_value());
        best_query = std::get<1>(corrected);
        if (std::get<0>(corrected))
        {
            update_query();
            return;
        }
        min_distance = 1000;

        // third try to fix the query using the student queries
        for (size_t i = 0; i < student_queries.size(); i++)
        {
            StudentQuery *query = &student_queries.at(i);
            // dont compare with itself
            if (query->get_id() == get_id())
            {
                continue;
            }
            if (query->is_parseable())
            {
                string other_query = query->get_value();
                string this_query = get_value();

                my_utils.preprocess_query(other_query);
                my_utils.preprocess_query(this_query);

                std::tuple<bool, string, int> corrected = my_utils.fix_query_syntax_using_another_query(admin, this_query, other_query, get_value());

                if (std::get<0>(corrected))
                {
                    size_t current_distance = std::get<2>(corrected);
                    if (current_distance < min_distance)
                    {
                        min_distance = current_distance;
                        best_query = std::get<1>(corrected);
                        is_corrected = true;
                    }
                }
            }
        }
        if (is_corrected)
        {
            update_query();
            return;
        }
    }
}

void StudentQuery::semantics_analysis(const Admin &admin, vector<ModelQuery> &model_queries, vector<StudentQuery> &student_queries)
{
    TreeEditDistance ted;
    Utils my_utils;

    // first check if we are supposed to check semantics
    if (admin.get_semantics_sensitivity() == Grader::property_level::ABSENT)
    {
        set_semantics_outcome(Grader::property_state::INVALID);
        return;
    }
    // is it correct?
    // correct
    if (is_correct())
    {
        // add positive feedback about semantics .
        add_feedback(" SEMANTICS: Correct! Well done. ");
        set_semantics_analysis_message("Correct semantics.\n");
        set_semantics_outcome(Grader::property_state::CORRECT);
        return;
    }
    // is it parseable
    if (!is_parseable())
    {
        // add feedback that the query is not parseable.
        add_feedback(" SEMANTICS: Since the query is not parseable, we could not verify the semantics. ");
        set_semantics_outcome(Grader::property_state::INCORRECT);
        set_semantics_analysis_message("The query is not parseable. Therefore, the semantics could not be verified.\n");
        return;
    }
    // first compare with the model queries for minor‐fix suggestions
    int min_tree_edit_dist = 100;
    string closest_correct_query = "";
    bool is_corrected = false;
    bool put_feedback = false;
    for (size_t i = 0; i < model_queries.size(); i++)
    {
        ModelQuery *query = &model_queries.at(i);
        // do the queries have the same fingerprint
        /*if (get_fingerprint() == query->get_fingerprint())
        {
            if(admin.get_num_of_semantics_outcomes() == 3)
            {
                set_semantics_outcome(2);
            }
            else
            {
                set_semantics_outcome(1);
            }
            return;
        }*/
        // calculate tree edit distance
        int current_tree_distance = ted.zhang_shasha(get_parse_tree(), query->get_parse_tree());

        if (current_tree_distance < min_tree_edit_dist)
        {
            // calculate the edit distance between the queries
            // compare using lower case queries
            string current_query = get_value();
            string model_query = query->get_value();
            my_utils.preprocess_query(current_query);
            my_utils.preprocess_query(model_query);
            int edit_distance = my_utils.general_edit_distance(current_query, model_query);

            // first check if the difference is only in the cases.
            if (edit_distance == 0 && !query->get_output().empty())
            {
                if (admin.get_results_sensitivity() == Grader::property_level::ABSENT)
                {
                    // If Grader::property_state::INVALID is set, it means the correct check above will be skipped for correct queries.
                    // Here we cehck that
                    set_semantics_outcome(Grader::property_state::CORRECT);
                    set_semantics_analysis_message("Correct semantics.\n");
                    return;
                }
                is_corrected = true;
                min_tree_edit_dist = 0;
                put_feedback = false;
                break;
            }
            else if (current_tree_distance <= admin.get_semantics_minor_incorrect_ted() && edit_distance <= admin.get_syntax_minor_incorrect_ted())
            {
                put_feedback = true;
                is_corrected = true;
                closest_correct_query = query->get_value();
                min_tree_edit_dist = current_tree_distance;
            }
        }
    }

    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *query = &student_queries.at(i);
        // dont compare with itself
        if (query->get_id() == get_id())
        {
            continue;
        }
        if (!query->is_correct())
        {
            continue;
        }
        // do the queries have the same fingerprint

        // calculate tree edit distance
        int current_tree_distance = ted.zhang_shasha(get_parse_tree(), query->get_parse_tree());

        if (current_tree_distance < min_tree_edit_dist)
        {
            // compare using lower case queries
            string current_query = get_value();
            string model_query = query->get_value();
            my_utils.preprocess_query(current_query);
            my_utils.preprocess_query(model_query);

            int edit_distance = my_utils.general_edit_distance(current_query, model_query);

            // first check if the difference is only in the cases.
            if (edit_distance == 0 && !query->get_output().empty())
            {
                if (admin.get_results_sensitivity() == Grader::property_level::ABSENT)
                {
                    // If Grader::property_state::INVALID is set, it means the correct check above will be skipped for correct queries.
                    // Here we cehck that
                    set_semantics_outcome(Grader::property_state::CORRECT);
                    set_semantics_analysis_message("Correct semantics.\n");
                    return;
                }
                is_corrected = true;
                min_tree_edit_dist = 0;
                put_feedback = false;
                break;
            }
            else if (current_tree_distance <= admin.get_semantics_minor_incorrect_ted() && edit_distance <= admin.get_syntax_minor_incorrect_ted())
            {
                put_feedback = true;
                is_corrected = true;
                closest_correct_query = query->get_value();
                min_tree_edit_dist = current_tree_distance;
            }
        }
    }

    // create a lambda function that does the same thing as the below check
    auto update_query = [&](bool put_feedback)
    {
        if (!put_feedback)
        {
            // the error was only due to the cases
            add_feedback(" SEMANTICS MINOR ERROR: To make the query have correct semantics, it was changed to, '" + closest_correct_query + "'. ");
            set_semantics_analysis_message("Semantics minor error. Maybe you meant '" + closest_correct_query + "'?\n");
            set_semantics_outcome(Grader::property_state::CORRECT);
            set_results_outcome(Grader::property_state::CORRECT);
            return;
        }
        if (admin.get_semantics_sensitivity() == Grader::property_level::THREE_LEVELS || admin.get_semantics_sensitivity() == Grader::property_level::SEMATICS_LEVELS_6)
        {
            // This minor error has points deduction.
            set_semantics_analysis_message("Semantics minor error. Maybe you meant '" + closest_correct_query + "'?\n");
            set_semantics_outcome(Grader::property_state::MINOR_INCORRECT);
            set_results_outcome(Grader::property_state::CORRECT);
        }
        else
        {
            set_semantics_outcome(Grader::property_state::INCORRECT);
            set_results_outcome(Grader::property_state::CORRECT);
        }
    };

    if (is_corrected)
    {
        update_query(put_feedback);
        return;
    }
    // after reaqching this point, the query semantics are incorrect
    // so we carry out further semantics analysis.
    // We will test against all the correct queries and choose the one with the best result.
    Common::comparision_result comparison;
    // get a goal from the first model query
    std::string main_goal;

    // compare only against same‐type model queries
    {
        std::string student_stmt = AbstractSyntaxTree::get_statement_type(get_parse_tree());
        std::vector<ModelQuery *> model_candidates;

        for (auto &mq : model_queries)
        {
            if (AbstractSyntaxTree::get_statement_type(mq.get_parse_tree()) == student_stmt)
                model_candidates.push_back(&mq);
        }
        // if none match type, compare all
        if (model_candidates.empty())
        {
            for (auto &mq : model_queries)
                model_candidates.push_back(&mq);
        }

        if (!model_candidates.empty())
        {
            // main_goal = Goals::generate_query_goal_general(model_candidates.at(0)->get_parse_tree());
        }

        for (auto *mqp : model_candidates)
        {
            auto comp = Goals::compare_queries(mqp->get_parse_tree(), get_parse_tree());
            if (comp.correct_parts.size() > comparison.correct_parts.size() ||
                (comp.correct_parts.size() == comparison.correct_parts.size() && comp.incorrect_parts.size() > comparison.incorrect_parts.size()))
            {
                comparison = comp;
            }
        }
    }

    // compare only against same‐type, correct student queries
    {
        std::string student_stmt = AbstractSyntaxTree::get_statement_type(get_parse_tree());
        std::vector<StudentQuery *> stu_candidates;
        for (auto &sq : student_queries)
        {
            if (sq.get_id() == get_id() || !sq.is_correct())
                continue;
            if (AbstractSyntaxTree::get_statement_type(sq.get_parse_tree()) == student_stmt)
                stu_candidates.push_back(&sq);
        }
        if (stu_candidates.empty())
        {
            for (auto &sq : student_queries)
            {
                if (sq.get_id() == get_id() || !sq.is_correct())
                    continue;
                stu_candidates.push_back(&sq);
            }
        }
        for (auto *other : stu_candidates)
        {
            auto comp = Goals::compare_queries(other->get_parse_tree(), get_parse_tree());
            if (comp.correct_parts.size() > comparison.correct_parts.size() || (comp.correct_parts.size() == comparison.correct_parts.size() && comp.incorrect_parts.size() > comparison.incorrect_parts.size()))
            {
                comparison = comp;
            }
        }
    }

    // Create a message based on the comparison
    std::ostringstream semantics_message;
    if (!comparison.message.empty())
    {
        // detailed feedback
        semantics_message << comparison.message; //<< "\n";
    }
    else
    {
        semantics_message << "2️⃣3️⃣\n"
                          << "No detailed feedback.\n";
    }

    set_semantics_analysis_message(semantics_message.str());
    add_feedback(" SEMANTICS: The query does not have correct semantics. ");

    if (admin.get_semantics_sensitivity() == Grader::property_level::SEMATICS_LEVELS_6)
    {
        int correct_clauses = comparison.correct_parts.size();
        int total_clauses = correct_clauses + comparison.incorrect_parts.size();
        if (total_clauses == 0)
        {
            // If there are no clauses to compare, assume the query is correct
            set_semantics_outcome(Grader::property_state::INCORRECT);
        }
        else if (correct_clauses > 0 && total_clauses == correct_clauses)
        {
            // If all clauses are correct, set to CORRECT
            set_semantics_outcome(Grader::property_state::CORRECT);
            // if results are being checked, set it to correct too
            if (admin.get_results_sensitivity() != Grader::property_level::ABSENT)
            {
                set_results_outcome(Grader::property_state::CORRECT);
            }
        }
        else
        {
            double ratio = static_cast<double>(correct_clauses) / total_clauses;
            int rating = static_cast<int>(std::round(ratio * 5)); // Rounds to nearest integer [0-5]

            // Clamp rating to the valid range in case of rounding issues
            if (rating < 0)
                rating = 0;
            if (rating > 5)
                rating = 5;

            switch (rating)
            {
            case 0:
                set_semantics_outcome(Grader::property_state::INCORRECT);
                break;
            case 1:
                set_semantics_outcome(Grader::property_state::SM_1);
                break;
            case 2:
                set_semantics_outcome(Grader::property_state::SM_2);
                break;
            case 3:
                set_semantics_outcome(Grader::property_state::SM_3);
                break;
            case 4:
                set_semantics_outcome(Grader::property_state::SM_4);
                break;
            case 5:
                set_semantics_outcome(Grader::property_state::SM_5);
                break;
            default:
                set_semantics_outcome(Grader::property_state::INCORRECT);
                break;
            }
        }
    }
    else
    {
        set_semantics_outcome(Grader::property_state::INCORRECT);
    }
}

string StudentQuery::get_old_value() const
{
    return old_value;
}

void StudentQuery::create_output(Query_Engine &qe)
{
    try
    {
        //  Determine the type of statement from the parse tree.
        std::string stmt_type = AbstractSyntaxTree::get_statement_type(get_parse_tree());
        if (stmt_type == "SelectStmt")
        {
            // For SELECT queries, generate outputs as usual.
            auto out = qe.execute_select(get_value());
            set_output(out);
        }
        else
        {
            // For non-select queries, capture table differences.
            std::string run_error;
            auto diff = qe.execute_non_select(get_value(), run_error);
            set_output(diff);
            if (!run_error.empty())
            {
                set_feedback(get_feedback() + " Query error: " + run_error);
            }
        }
    }
    catch (const std::exception &)
    {
        // somehow the dbms crashed therefore we restore it from the copy.
        set_feedback(get_feedback() + "DBMS crashed.");
        std::cout << "DBMS crashed. Crashed at student id: " << get_id() << std::endl;
    }
}

bool StudentQuery::is_correct() const
{
    // is it correct?
    return get_results_outcome() == Grader::property_state::CORRECT;
}

string StudentQuery::get_closest_parse_tree() const
{
    return closest_parse_tree;
}
void StudentQuery::set_normalized_value(double normalized_value)
{
    this->normalized_value = normalized_value;
}

double StudentQuery::get_normalized_value() const
{
    return normalized_value;
}

bool StudentQuery::is_value_changed() const
{
    return value_changed;
}

int StudentQuery::get_text_edit_distance() const
{
    return text_edit_distance;
}

int StudentQuery::get_tree_edit_distance() const
{
    return tree_edit_distance;
}

template <typename T>
T StudentQuery::final_grade(T worst_grade, T best_grade, double normalized_value)
{
    return worst_grade + normalized_value * (best_grade - worst_grade);
}

template <>
char StudentQuery::final_grade<char>(char worst_grade, char best_grade, double normalized_value)
{
    int worst = worst_grade - 'A';
    int best = best_grade - 'A';

    int grade = worst + std::round(normalized_value * (best - worst));
    return 'A' + grade;
}
void StudentQuery::set_grade(const Admin &admin)
{

    double norm_value = get_normalized_value();
    grade = final_grade(0.0, 1.0, norm_value);
}

double StudentQuery::get_grade() const
{
    return grade;
}

string StudentQuery::get_question_number() const
{
    return question_number;
}

int StudentQuery::get_attempt_number() const
{
    return attempt_number;
}

void StudentQuery::post_process(const Admin &admin)
{
    if (admin.get_results_sensitivity() == Grader::property_level::ABSENT)
    {
        set_results_outcome(Grader::property_state::INVALID);
    }
}

void StudentQuery::set_syntax_analysis_message(const string &syntax_analysis_message)
{
    this->syntax_analysis_message = syntax_analysis_message;
}

string StudentQuery::get_syntax_analysis_message() const
{
    return syntax_analysis_message;
}

void StudentQuery::set_semantics_analysis_message(const string &semantics_analysis_message)
{
    this->semantics_analysis_message = semantics_analysis_message;
}

string StudentQuery::get_semantics_analysis_message() const
{
    return semantics_analysis_message;
}

void StudentQuery::set_results_analysis_message(const string &results_analysis_message)
{
    this->results_analysis_message = results_analysis_message;
}

string StudentQuery::get_results_analysis_message() const
{
    return results_analysis_message;
}

string StudentQuery::construct_message() const
{
    std::ostringstream message;

    // If the query is correct, return a message that the query is correct only
    if (get_grade() == 1.0)
    {
        message << "🟢 Correct.";
        return message.str();
    }

    // semantics message
    message << get_semantics_analysis_message();

    std::string syntax_message = get_syntax_analysis_message();
    std::string result_analysis = get_results_analysis_message();
    // do we have extra information?
    if (!syntax_message.empty() || !result_analysis.empty())
    {
        // begin with the syntax message

        message << "5️⃣ Additional Information:\n";
        if (!syntax_message.empty())
        {
            message << "● Syntax Analysis: " << syntax_message << "\n";
        }
        if (!result_analysis.empty())
        {
            message << "● Results Analysis: " << result_analysis;
        }
    }

    return message.str();
}
