/**
 * @file student_query.h
 * @brief This file contains the declaration of the StudentQuery class.
 * The class is used to store the queries and their connected information.
 * The class inherits from the ModelQuery class.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 *
 */
#ifndef STUDENT_QUERY_H
#define STUDENT_QUERY_H

#include "model_query.h"
#include "my_duckdb.h"
#include "admin.h"
#include <atomic>
#include "query_engine.h"

class StudentQuery : public ModelQuery
{
public:
    /**
     * A constructor that assigns the id and the value of a query.
     * @param id: the id of the query.
     * @param value: the query in text form.
     */
    StudentQuery(const string &id, const string &value);
    /**
     * A constructor that assigns the id and the value of a query.
     * @param id: the id of the query.
     * @param value: the query in text form.
     * @param question_number: the number of the question.
     * @param attempt_number: the number of the attempt.
     */
    StudentQuery(const string &id, const string &value, const string question_number, const int attempt_number);
    /**
     * This function sets the parseable property of a query.
     * @param parseable: true if the query is parseable, false otherwise.
     */
    void set_parseable(bool parseable);
    /**
     * This function returns the parseable property of a query.
     * @return: true if the query is parseable, false otherwise.
     */
    bool is_parseable() const;
    /**
     * This function sets the syntax analysis outcome of a query.
     * @param syntax_outcome: the syntax outcome of a query.
     */
    void set_syntax_outcome(Grader::property_state syntax_outcome);
    /**
     * This function returns the syntax analysis outcome of a query.
     * @return: the syntax outcome of a query.
     */
    Grader::property_state get_syntax_outcome() const;
    /**
     * This function sets the semantics analysis outcome of a query.
     * @param semantics_outcome: the semantics outcome of a query.
     */
    void set_semantics_outcome(Grader::property_state semantics_outcome);
    /**
     * This function returns the semantics analysis outcome of a query.
     * @return: the semantics outcome of a query.
     */
    Grader::property_state get_semantics_outcome() const;
    /**
     * This function sets the results analysis outcome of a query.
     * @param results_outcome: the results outcome of a query.
     */
    void set_results_outcome(Grader::property_state results_outcome);
    /**
     * This function returns the results analysis outcome of a query.
     * @return: the results outcome of a query.
     */
    Grader::property_state get_results_outcome() const;
    /**
     * This function sets the correctness level of a query.
     * @param correctness_level: the correctness level of a query.
     */
    void set_correctness_level(int correctness_level);
    /**
     * This function returns the correctness level of a query.
     * @return: the correctness level of a query.
     */
    int get_correctness_level() const;
    /**
     * This function sets the error found after executing the query.
     * @param execution_error: the execution error of a query.
     */
    void set_execution_error(const string &execution_error);
    /**
     * This function returns the error found after executing the query.
     * @return: the execution error of a query.
     */
    string get_execution_error() const;
    /**
     * This function sets the feedback to be forwarded to the student after analyzing and grading the query.
     * @param feedback: the feedback of the query.
     */
    void set_feedback(const string &feedback);
    /**
     * This function returns the feedback to be forwarded to the student after analyzing and grading the query.
     * @return: the feedback of the query.
     */
    string get_feedback() const;
    /**
     * This function adds more feedback to the already set feedback.
     * @param feedback: the feedback to be added.
     */
    void add_feedback(const string &feedback);
    /**
     * This function sets the outcome of result analysis.
     * @param admin: the admin object that contains the grading parameters.
     * @param expected_output: the expected output of the query.
     */
    void result_analysis(const Admin &admin, const vector<vector<string>> &expected_output);
    /**
     * This function sets the outcome of syntax analysis.
     * @param admin: the admin object that contains the grading parameters.
     * @param model_queries: the model queries to be used for syntax analysis.
     * @param student_queries: the student queries to be used for syntax analysis. We will use those correct queries.
     */
    void syntax_analysis(const Admin &admin, vector<ModelQuery> &model_queries, vector<StudentQuery> &student_queries, Query_Engine &qe);
    /**
     * This function sets the outcome of semantics analysis.
     * @param admin: the admin object that contains the grading parameters.
     * @param model_queries: the model queries to be used for semantics analysis.
     * @param student_queries: the student queries to be used for semantics analysis. We will use those correct queries.
     */
    void semantics_analysis(const Admin &admin, vector<ModelQuery> &model_queries, vector<StudentQuery> &student_queries);
    /**
     * This function gets the old value of a query before it was changed to the correct one.
     * This happens when the query is not parseable due to minor syntax mistakes. We edit the query and store the original value.
     * @return: the original value of the query.
     */
    string get_old_value() const;
    /**
     * This function runs the query and stores the output.
     */
    void create_output(Query_Engine &qe);
    /**
     * This function returns whether the query is correct or not. In a binary way.
     * @return: true if the query is correct, false otherwise.
     */
    bool is_correct() const;
    /**
     * THis function gets the closest parse tree to the query.
     * The parse tree is found after tree edit distance calculation.
     * @return: the query with the closest parse tree to the query in question.
     */
    string get_closest_parse_tree() const;
    /**
     * This function sets the normalized value of a query. The value ranges from 0.0 to 1.0.
     * @param normalized_value: the normalized value of a query.
     */
    void set_normalized_value(double normalized_value);
    /**
     * This function returns the normalized value of a query. The value ranges from 0.0 to 1.0.
     * @return: the normalized value of a query.
     */
    double get_normalized_value() const;
    /**
     * This function sets whether the query was edited or not.
     * @param value_changed: true if the value of a query was edited, false otherwise.
     */
    bool is_value_changed() const;
    /**
     * This function gets the closest text edit distance found in syntax analysis.
     * The edit distance is only set if the query was edited as it was found to be minor incorrect in syntax.
     * @return: the closest text edit distance found in syntax analysis.
     */
    int get_text_edit_distance() const;
    /**
     * This function gets the closest tree edit distance found in semantics analysis.
     * The edit distance is only set if the query was found to be minor incorrect in semantics.
     * @return: the closest tree edit distance found in semantics analysis.
     */
    int get_tree_edit_distance() const;
    /**
     * This template function returns the final grade of a query.
     * Since the grade might be of different types like integers from 1 to 10 or characters from A to E, we use a template to handle this.
     * @param worst_grade: the worst grade possible
     * @param best_grade: the best grade possible
     * @param normalized_value: the normalized value of a query being graded.
     */
    template <typename T>
    T final_grade(T worst_grade, T best_grade, double normalized_value);
    /**
     * This function sets the final grade of a query.
     * @param admin: the admin object that contains the grading parameters.
     */
    void set_grade(const Admin &admin);
    /**
     * This function returns the final grade of a query.
     * @return: the final grade of a query.
     */
    double get_grade() const;
    /**
     * This function returns the number of the question.
     * @return: the number of the question.
     */
    string get_question_number() const;
    /**
     * This function returns the number of the attempt that the query belongs to.
     * @return: the number of the attempt that the query belongs to.
     */
    int get_attempt_number() const;
    void post_process(const Admin &admin);
    /**
     * This function sets the message to be forwarded to the student after syntax analysis.
     * @param syntax_analysis_message: the message to be forwarded to the student after syntax analysis.
     */
    void set_syntax_analysis_message(const string &syntax_analysis_message);
    /**
     * This function returns the message to be forwarded to the student after syntax analysis.
     * @return: the message to be forwarded to the student after syntax analysis.
     */
    string get_syntax_analysis_message() const;
    /**
     * This function sets the message to be forwarded to the student after semantics analysis.
     * @param semantics_analysis_message: the message to be forwarded to the student after semantics analysis.
     */
    void set_semantics_analysis_message(const string &semantics_analysis_message);
    /**
     * This function returns the message to be forwarded to the student after semantics analysis.
     * @return: the message to be forwarded to the student after semantics analysis.
     */
    string get_semantics_analysis_message() const;
    /**
     * This function sets the message to be forwarded to the student after results analysis.
     * @param results_analysis_message: the message to be forwarded to the student after results analysis.
     */
    void set_results_analysis_message(const string &results_analysis_message);
    /**
     * This function returns the message to be forwarded to the student after results analysis.
     * @return: the message to be forwarded to the student after results analysis.
     */
    string get_results_analysis_message() const;
    string construct_message() const;

private:
    bool parseable;                           /**< True if the query is parseable, false otherwise. */
    Grader::property_state syntax_outcome;    /**< The syntax analysis outcome of a query. */
    Grader::property_state semantics_outcome; /**< The semantics analysis outcome of a query. */
    Grader::property_state results_outcome;   /**< The results analysis outcome of a query. */
    int correctness_level;                    /**< The correctness level of the query. */
    double normalized_value;                  /**< The normalized value of the correctness level of the query. */
    string execution_error;                   /**< The execution error found when executing the query. */
    string feedback;                          /**< The feedback to be forwarded to the student after analyzing and grading the query. */
    bool value_changed;                       /**< True if the value of a query was edited, false otherwise. */
    string old_value;                         /**< The original value of the query before it was edited. */
    string closest_parse_tree;                /**< The query with the closest parse tree to the query. */
    int text_edit_distance;                   /**< The closest text edit distance found in syntax analysis. */
    int tree_edit_distance;                   /**< The closest tree edit distance found in semantics analysis. */
    double grade;                             /**< The final grade of the query. */
    string question_number;                   /**< The number of the question that the query belongs to. */
    int attempt_number;                       /**< The number of the attempt that the query belongs to. */
    string syntax_analysis_message;           /**< The message to be forwarded to the student after syntax analysis. */
    string semantics_analysis_message;        /**< The message to be forwarded to the student after semantics analysis. */
    string results_analysis_message;          /**< The message to be forwarded to the student after results analysis. */
};

#endif