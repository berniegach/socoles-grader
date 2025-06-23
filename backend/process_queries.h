/**
 * @file process_queries.h
 * This file contains the declaration of the ProcessQueries class.
 * The class encapsulates the main processing of queries. INsteading of placing all the code in the main function, we encapsulate it in a class.
 * Thus allows for testing the grading capabilities of the program.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#include "student_query.h"
#include "model_query.h"
#include "admin.h"
#include "grader.h"
#include "query_engine.h"

class ProcessQueries
{
public:
    struct grading_info
    {
        std::string org_defined_id;
        int attempt_number;
        std::string question_number;
        std::string query;
        double grade;
        double out_of;
        std::string feedback;
    };
    ProcessQueries();
    ProcessQueries(vector<ModelQuery> &model_queries, vector<StudentQuery> &student_queries, Admin::database_options db_opts, const Admin &admin, Grader &grader);
    /**
     * This function outputs the results of the grading to a csv file.
     * In this function we only output the necessary information for the student and the instructor.
     * @param queries: the queries to be graded.
     * @param admin: the admin object that contains the grading parameters.
     */
    /**
     * This template function returns the final grade of a query.
     * Since the grade might be of different types like integers from 1 to 10 or characters from A to E, we use a template to handle this.
     * @param worst_grade: the worst grade possible
     * @param best_grade: the best grade possible
     * @param normalized_value: the normalized value of a query being graded.
     */
    template <typename T>
    T final_grade(T worst_grade, T best_grade, double normalized_value);
    void output_results(const vector<StudentQuery> *queries, const Admin &admin);
    /**
     * This function outputs the results of the grading to a csv file.
     * In this function we output all the information for debugging andvalidation purposes.
     * @param queries: the queries to be graded.
     * @param admin: the admin object that contains the grading parameters.
     */
    void output_results_more(const vector<StudentQuery> *queries, const Admin &admin);
    std::vector<grading_info> get_grading_info(const vector<StudentQuery> *queries, const Admin &admin);
    void pre_process_student_query(StudentQuery *student_query, Admin::database_options db_opts);
    static void pre_process_model_query(ModelQuery *model_query, Query_Engine &qe);

private:
    static void analyze_syntax(StudentQuery *student_query, const Admin &admin, vector<ModelQuery> model_queries, vector<StudentQuery> student_queries, Query_Engine &qe);
    static void analyze_results(StudentQuery *student_query, const Admin &admin, vector<vector<string>> output);
    static void analyze_semantics(StudentQuery *student_query, const Admin &admin, vector<ModelQuery> model_queries, vector<StudentQuery> student_queries);
    Query_Engine query_engine;

    // dummy storage for delegating zero‐arg ctor
    static std::vector<ModelQuery> dummy_model_queries_;
    static std::vector<StudentQuery> dummy_student_queries_;
    static Admin dummy_admin_;
    static Grader dummy_grader_;
};