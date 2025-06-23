#include "process_queries.h"
#include <iostream>
#include <pg_query.h>
#include "utils.h"
#include <future>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "my_evosql.h"
#include "goals.h"
#include <mutex>
#include <exception>
#include <thread>
#include <semaphore>
#include <queue>
#include <atomic>
#include "thread_pool.h"

using namespace std;

// define the static dummies:
std::vector<ModelQuery> ProcessQueries::dummy_model_queries_;
std::vector<StudentQuery> ProcessQueries::dummy_student_queries_;
Admin ProcessQueries::dummy_admin_;
Grader ProcessQueries::dummy_grader_;

// zero‐arg ctor (delegating):
ProcessQueries::ProcessQueries()
    : ProcessQueries(
          dummy_model_queries_,
          dummy_student_queries_,
          Admin::database_options{}, // default‐constructed options
          dummy_admin_,
          dummy_grader_)
{
}

ProcessQueries::ProcessQueries(std::vector<ModelQuery> &model_queries, std::vector<StudentQuery> &student_queries, Admin::database_options db_opts,
                               const Admin &admin, Grader &grader)
    : query_engine(db_opts) // ← initialize our engine
{
    query_engine.initialize();
    Goals goals;
    // Initialize the worker threads
    std::vector<std::thread> workers;
    // Create a thread pool with 10 threads
    ThreadPool pool(1);

    // Mutex for thread-safe console output
    std::mutex cout_mutex;

    // Pre-processing model queries
    for (size_t i = 0; i < model_queries.size(); i++)
    {
        ModelQuery *model_query = &model_queries.at(i);
        pre_process_model_query(model_query, query_engine);
    }

    // Pre-processing student queries
    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *student_query = &student_queries.at(i);

        pool.enqueue([this, student_query, &cout_mutex, &db_opts]()
                     {
            {
                std::cout << "Pre-processing student query " << student_query->get_id() << std::endl;
            }

            this->pre_process_student_query(student_query, db_opts); });
    }

    // Wait for tasks to complete
    pool.wait_until_empty();

    // Syntax analysis
    std::cout << "Syntax analysis started..." << std::endl;
    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *student_query = &student_queries.at(i);
        std::cout << "Syntax: processing student query " << student_query->get_id() << std::endl;
        analyze_syntax(student_query, admin, model_queries, student_queries, query_engine);
    }

    // Wait for tasks to complete
    pool.wait_until_empty();
    std::cout << "Syntax analysis finished" << std::endl;

    // Results analysis
    std::cout << "Results analysis started..." << std::endl;
    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *student_query = &student_queries.at(i);
        int sz = student_queries.size();

        pool.enqueue([student_query, i, &admin, &model_queries, &cout_mutex, &sz]()
                     {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Results: Processing student query " << i + 1 << " of " << sz << std::endl;
            }

            analyze_results(student_query, admin, model_queries.at(0).get_output()); });
    }

    // Wait for tasks to complete
    pool.wait_until_empty();
    std::cout << "Results analysis finished" << std::endl;

    // Semantics analysis
    std::cout << "Semantics analysis started..." << std::endl;
    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *student_query = &student_queries.at(i);

        pool.enqueue([student_query, i, &admin, &model_queries, &student_queries, &cout_mutex]()
                     {
            {
                std::lock_guard<std::mutex> lock(cout_mutex); 
                std::cout << "Semantics: Processing student query " << i + 1 << " of " << student_queries.size() << std::endl;
            }

            analyze_semantics(student_query, admin, model_queries, student_queries); });
    }

    // Wait for tasks to complete
    pool.wait_until_empty();
    std::cout << "Semantics analysis finished" << std::endl;

    // Grading the queries
    std::cout << "Grading the queries started..." << std::endl;
    for (size_t i = 0; i < student_queries.size(); i++)
    {
        StudentQuery *student_query = &student_queries.at(i);
        student_query->post_process(admin);

        // Get the correctness level and the normalized value
        std::pair<int, double> correctness_level = grader.calculate_correctness_level(
            admin.get_syntax_sensitivity(),
            admin.get_semantics_sensitivity(),
            admin.get_results_sensitivity(),
            admin.get_property_order(),
            student_query->get_results_outcome(),
            student_query->get_semantics_outcome(),
            student_query->get_syntax_outcome());

        student_query->set_correctness_level(correctness_level.first);
        student_query->set_normalized_value(correctness_level.second);
        student_query->set_grade(admin);
    }

    std::cout << "Grading the queries finished." << std::endl;
    // clear the engine
    query_engine.clear();
}

template <typename T>
T ProcessQueries::final_grade(T worst_grade, T best_grade, double normalized_value)
{
    return worst_grade + normalized_value * (best_grade - worst_grade);
}

template <>
char ProcessQueries::final_grade<char>(char worst_grade, char best_grade, double normalized_value)
{
    int worst = worst_grade - 'A';
    int best = best_grade - 'A';

    int grade = worst + std::round(normalized_value * (best - worst));
    return 'A' + grade;
}
void ProcessQueries::output_results(const vector<StudentQuery> *queries, const Admin &admin)
{
    using namespace std;

    ofstream file_scanned("quiz_graded.csv");
    if (!file_scanned)
    {
        std::cerr << "File could not be opened" << std::endl;
        return;
    }
    // write the file header
    file_scanned << "Org Defined ID"
                 << ","
                 << "Attempt #"
                 << ","
                 << "Q #"
                 << ","
                 << "Answer"
                 << ","
                 << "Score"
                 << ","
                 << "Out Of"
                 << ","
                 << "Feedback"
                 << '\n';
    // write the queries
    vector<grading_info> info = get_grading_info(queries, admin);
    for (size_t i = 0; i < info.size(); i++)
    {
        file_scanned << info.at(i).org_defined_id
                     << ","
                     << info.at(i).attempt_number
                     << ","
                     << info.at(i).question_number
                     << ","
                     << info.at(i).query
                     << ","
                     << info.at(i).grade
                     << ","
                     << info.at(i).out_of
                     << ","
                     << info.at(i).feedback
                     << '\n';
    }
}
void ProcessQueries::output_results_more(const vector<StudentQuery> *queries, const Admin &admin)
{
    using namespace std;
    Grader grader;

    ofstream file_scanned("quiz_graded.csv");
    if (!file_scanned)
    {
        std::cerr << "File could not be opened" << std::endl;
        return;
    }
    // sort the queries
    // std::multimap<string, string> m_queries_flipped_and_sorted = Utils::flip_map(queries);
    // write the file header
    file_scanned << "id"
                 << ","
                 << "query"
                 << ","
                 << "syntax"
                 << ","
                 << "semantic"
                 << ","
                 << "results"
                 << ","
                 << "correctness level"
                 << ","
                 << "normalized value"
                 << ","
                 << "grade"
                 << ","
                 << "fingerprint"
                 << ","
                 << "text edit distance"
                 << ","
                 << "corrected query"
                 << ","
                 << "tree edit distance"
                 << ","
                 << "error"
                 << '\n';
    // write the queries
    for (size_t i = 0; i < queries->size(); i++)
    {
        // use the original query if the query was edited.
        string query = queries->at(i).is_value_changed() ? queries->at(i).get_old_value() : queries->at(i).get_value();

        // calculate final grade
        double grade;
        double normalized_value = queries->at(i).get_normalized_value();
        /*if(admin.get_num_of_syntax_outcomes() == 1 && admin.get_num_of_semantics_outcomes() == 1 && admin.get_num_of_results_outcomes() == 1)
        {
            normalized_value = normalized_value == 1 ? 1 : 0;
            grade = final_grade(0.0, 1.0, normalized_value);
        }
        else
        {*/
        grade = final_grade(0.0, 1.0, normalized_value);
        //}

        // surround the queries with quotes if it contains any commas.
        // this is because we are using commas as separator in the csv file
        if (query.find(',') != string::npos)
        {
            query = '"' + query + '"';
        }
        string corrected_query = queries->at(i).is_value_changed() ? queries->at(i).get_value() : "";
        if (corrected_query.find(',') != string::npos)
        {
            corrected_query = '"' + corrected_query + '"';
        }

        // surround the error message with quotes if it contains any commas.
        string execution_error = queries->at(i).get_execution_error();
        if (execution_error.find(',') != string::npos)
        {
            execution_error = '"' + execution_error + '"';
        }
        // remove newline characters from the error message
        execution_error.erase(std::remove(execution_error.begin(), execution_error.end(), '\n'), execution_error.end());
        // replace " with ' in the error message
        std::replace(execution_error.begin(), execution_error.end(), '"', '\'');

        // write the grading results to the file
        file_scanned << queries->at(i).get_id()
                     << ","
                     << query
                     << ","
                     << grader.property_state_to_string(queries->at(i).get_syntax_outcome())
                     << ","
                     << grader.property_state_to_string(queries->at(i).get_semantics_outcome())
                     << ","
                     << grader.property_state_to_string(queries->at(i).get_results_outcome())
                     << ","
                     << queries->at(i).get_correctness_level()
                     << "," << fixed << setprecision(2)
                     << queries->at(i).get_normalized_value()
                     << ","
                     << grade
                     << ","
                     << queries->at(i).get_fingerprint()
                     << ","
                     << queries->at(i).get_text_edit_distance()
                     << ","
                     << corrected_query
                     << ","
                     << queries->at(i).get_tree_edit_distance()
                     << ","
                     << execution_error
                     << '\n';
    }
}

std::vector<ProcessQueries::grading_info> ProcessQueries::get_grading_info(const vector<StudentQuery> *queries, const Admin &admin)
{
    using namespace std;

    vector<grading_info> grading_info_vector;

    // write the queries
    for (size_t i = 0; i < queries->size(); i++)
    {
        grading_info info;
        // use the original query if the query was edited.
        string query = queries->at(i).is_value_changed() ? queries->at(i).get_old_value() : queries->at(i).get_value();

        // calculate final grade
        double grade;
        double normalized_value = queries->at(i).get_normalized_value();
        /*if (admin.get_num_of_syntax_outcomes() == 1 && admin.get_num_of_semantics_outcomes() == 1 && admin.get_num_of_results_outcomes() == 1)
        {
            normalized_value = normalized_value == 1 ? 1 : 0;
            grade = final_grade(0.0, 1.0, normalized_value);
        }
        else
        {*/
        grade = final_grade(0.0, 1.0, normalized_value);
        //}

        // Escape double quotes
        size_t pos = 0;
        bool query_escaped = false;
        while ((pos = query.find('"', pos)) != std::string::npos)
        {
            query.replace(pos, 1, "\"\"");
            pos += 2; // move past the inserted characters
            query_escaped = true;
        }

        // surround the queries with quotes if it contains any commas.
        // this is because we are using commas as separator in the csv file
        if (query.find(',') != string::npos || query_escaped)
        {
            query = '"' + query + '"';
        }
        string corrected_query = queries->at(i).is_value_changed() ? queries->at(i).get_value() : "";
        if (corrected_query.find(',') != string::npos)
        {
            corrected_query = '"' + corrected_query + '"';
        }

        // surround the error message with quotes if it contains any commas.
        string execution_error = queries->at(i).get_execution_error();
        // remove newline characters from the error message
        execution_error.erase(std::remove(execution_error.begin(), execution_error.end(), '\n'), execution_error.end());
        execution_error.erase(std::remove(execution_error.begin(), execution_error.end(), '\r'), execution_error.end());
        // replace " with ' in the error message
        // std::replace(execution_error.begin(), execution_error.end(), '"', '\'');
        /*if (execution_error.find(',') != string::npos)
        {
            execution_error = '"' + execution_error + '"';
        }*/
        // get the feedback
        string feedback = queries->at(i).get_feedback();
        // create a long text containing the feedback and the error message
        if (execution_error != "")
        {
            feedback = feedback + ". " + execution_error;
        }
        // construct grader comments based on the order of importance
        string grader_comments = "";
        switch (admin.get_property_order())
        {
        case Grader::property_order::SY_SM_RE:
            grader_comments = "This quiz was graded as an introductory quiz. We placed more weight on syntax, then semantics and finally results.";
            break;
        case Grader::property_order::SM_SY_RE:
            grader_comments = "This quiz was graded as an intermediate quiz. We placed more weight on semantics, then syntax and finally results.";
            break;
        case Grader::property_order::RE_SM_SY:
            grader_comments = "This quiz was graded as an advanced quiz. We placed more weight on results, then semantics and finally syntax.";
            break;
        default:
            grader_comments = "There was an error in grading this quiz.";
            break;
        }

        // add grader comments to the feedback
        feedback = feedback + ". " + grader_comments;

        // Escape double quotes
        /*pos = 0;
        bool feedback_escaped = false;
        while ((pos = feedback.find('"', pos)) != std::string::npos)
        {
            feedback.replace(pos, 1, "\"\"");
            pos += 2; // move past the inserted characters
            feedback_escaped = true;
        }

        //surround the feedback with quotes if it contains any commas.
        if (feedback.find(',') != string::npos || feedback_escaped)
        {
            feedback = '"' + feedback + '"';
        }*/

        std::string message = queries->at(i).construct_message();

        // Escape double quotes
        pos = 0;
        bool message_escaped = false;
        while ((pos = message.find('"', pos)) != std::string::npos)
        {
            message.replace(pos, 1, "\"\"");
            pos += 2; // move past the inserted characters
            message_escaped = true;
        }

        // surround the message with quotes if it contains any commas.
        // if (message.find(',') != std::string::npos || message_escaped)
        {
            message = '"' + message + '"';
        }

        info.org_defined_id = queries->at(i).get_id();
        info.attempt_number = queries->at(i).get_attempt_number();
        info.question_number = queries->at(i).get_question_number();
        info.query = query;
        info.grade = grade;
        info.out_of = 1;
        info.feedback = message;

        grading_info_vector.push_back(info);
    }
    return grading_info_vector;
}

void ProcessQueries::pre_process_student_query(StudentQuery *student_query, Admin::database_options db_opts)
{
    PgQueryParseResult result = pg_query_parse(student_query->get_value().c_str());

    if (result.error)
    {
        student_query->set_parseable(false);
        student_query->set_syntax_outcome(Grader::property_state::INCORRECT);
        return;
    }
    // take care of empty queries and queries with only the keyword SELECT.
    if (student_query->get_value().empty() || student_query->get_value() == "SELECT")
    {
        student_query->set_parseable(false);
        student_query->set_syntax_outcome(Grader::property_state::INCORRECT);
        return;
    }

    // the query is parseable.
    student_query->set_parseable(true);
    student_query->create_abstract_syntax_tree();
    student_query->create_fingerprint();
    student_query->create_output(query_engine);
}

void ProcessQueries::pre_process_model_query(ModelQuery *model_query, Query_Engine &qe)
{
    // Check if the query is parseable
    PgQueryParseResult result = pg_query_parse(model_query->get_value().c_str());
    if (result.error)
    {
        // std::lock_guard<std::mutex> lock(cout_mutex);
        std::cerr << "The query " << model_query->get_value() << " is not parseable." << std::endl;
        exit(1);
    }

    // The query is parseable
    model_query->create_abstract_syntax_tree();
    model_query->create_fingerprint();
    model_query->create_output(qe);

    // Set the goal of the exercise
    auto root_node = model_query->get_parse_tree();
    std::vector<std::string> goal_general = Goals::generate_query_goal_general(root_node);
    std::vector<std::string> goal_specific = Goals::generate_query_goal_specific(root_node);
    model_query->set_goal_general(goal_general);
    model_query->set_goal_specific(goal_specific);
}

void ProcessQueries::analyze_syntax(StudentQuery *student_query, const Admin &admin, vector<ModelQuery> model_queries, vector<StudentQuery> student_queries, Query_Engine &qe)
{
    try
    {
        student_query->syntax_analysis(admin, model_queries, student_queries, qe);
    }
    catch (const std::exception &)
    {
        // Alter the admin.
        // cout << "Error: The query " << student_query->get_id() << " : " << student_query->get_value() << " Failed to process" << endl;
        student_query->set_feedback(student_query->get_feedback() + " The query failed to process in syntax analysis.");
    }
}

void ProcessQueries::analyze_results(StudentQuery *student_query, const Admin &admin, vector<vector<string>> output)
{
    if (student_query->is_parseable())
    {
        student_query->result_analysis(admin, output);
    }
    else
    {
        student_query->set_results_outcome(Grader::property_state::INCORRECT);
        // student_query->set_feedback("The query is not parseable.");
    }
}

void ProcessQueries::analyze_semantics(StudentQuery *student_query, const Admin &admin, vector<ModelQuery> model_queries, vector<StudentQuery> student_queries)
{
    try
    {
        student_query->semantics_analysis(admin, model_queries, student_queries);
    }
    catch (exception &e)
    {
        std::cerr << "Semantics exception: " << e.what() << std::endl;
    }
}
