/**
 * @file admin.h
 *
 * @brief This class is used to initialize the credentials needed to connect to the database.
 * It also sets administrative global parameters. that are used in various stages of grading.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef ADMIN_H
#define ADMIN_H
#include <string>
#include "grader.h"

using std::string;

class Admin
{
public:
    struct database_options
    {
        string sql_file;
        int auto_db;
        int num_db;
        string sql_create;
        string correct_queries_csv;
        string postgresql_dbname;
        bool is_front_end;
        bool use_postgresql;
    };
    /**
     * The arguments for these constructor are given by the admin/instructor. For now they are command line arguments.
     * @param syntax_level: The level of sensitivity of the syntax analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param semantics_level: The level of sensitivity of the semantics analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param results_level: The level of sensitivity of the results analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param propert_order: The order of importance of the grading properties. RE_SM_SY - results most important, SM_SY_RE - semantics most important, SY_SM_RE - syntax most important
     * @param check_order: If true, the order of the results is also checked. If false, the order of the results is not checked.
     */
    Admin(Grader::property_level syntax_level = Grader::property_level::THREE_LEVELS,
          Grader::property_level semantics_level = Grader::property_level::THREE_LEVELS,
          Grader::property_level results_level = Grader::property_level::THREE_LEVELS,
          Grader::property_order propert_order = Grader::property_order::RE_SM_SY, bool check_order = false);
    /**
     * this function initializes the grading parameters.
     * @param syntax_level: The level of sensitivity of the syntax analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param semantics_level: The level of sensitivity of the semantics analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param results_level: The level of sensitivity of the results analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param propert_order: The order of importance of the grading properties. RE_SM_SY - results most important, SM_SY_RE - semantics most important, SY_SM_RE - syntax most important
     * @param check_order: If true, the order of the results is also checked. If false, the order of the results is not checked.
     */

    void init(Grader::property_level syntax_level, Grader::property_level semantics_level, Grader::property_level results_level, Grader::property_order propert_order, bool check_order);
    /**
     * this function initializes the grading parameters.
     * @param syntax_level: The level of sensitivity of the syntax analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param semantics_level: The level of sensitivity of the semantics analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param results_level: The level of sensitivity of the results analysis from 1 to 3. The higher the number, the more sensitive the analysis.
     * @param propert_order: The order of importance of the grading properties. RE_SM_SY - results most important, SM_SY_RE - semantics most important, SY_SM_RE - syntax most important
     * @param check_order: If true, the order of the results is also checked. If false, the order of the results is not checked.
     * @param text_edit_distance: The boundary value for determining how many wrong characters a required to classify the syntax of a query as minor incorrect. It is used in levenstein distance calculation.
     * @param tree_edit_distance: The boundary value for determining how many wrong characters a required to classify the semantics of a query as minor incorrect. It is used in levenstein distance calculation.
     */
    void init(Grader::property_level syntax_level, Grader::property_level semantics_level, Grader::property_level results_level, Grader::property_order propert_order, bool check_order, int text_edit_distance, int tree_edit_distance);
    /**
     * This function returns the connection string that is used to connect to the database.
     * @return: the connection string.
     */
    string get_connection_string() const;
    /**
     * This function returns the sensitivity level of the syntax analysis.
     * @return: the sensitivity level of the syntax analysis.
     */
    Grader::property_level get_syntax_sensitivity() const;
    /**
     * This function returns the sensitivity level of the semantics analysis.
     * @return: the sensitivity level of the semantics analysis.
     */
    Grader::property_level get_semantics_sensitivity() const;
    /**
     * This function returns the sensitivity level of the results analysis.
     * @return: the sensitivity level of the results analysis.
     */
    Grader::property_level get_results_sensitivity() const;
    /**
     * This function returns the order of importance of the grading properties.
     * @return: the order of importance of the grading properties.
     */
    Grader::property_order get_property_order() const;
    /**
     * This function returns whether the order of the results is checked or not.
     * @return: true if the order of the results is checked, false otherwise.
     */
    bool get_check_order() const;
    /**
     * This function gets the boundary value for determining how many wrong characters a required to classify the syntax of a query as minor incorrect.
     * It is used in levenstein distance calculation.
     * @param num_of_syntax_outcomes: the boundary value for minor incorrect syntax.
     */
    int get_syntax_minor_incorrect_ted() const;
    /**
     * This function gets the boundary value for determining how many wrong characters a required to classify the semantics of a query as minor incorrect.
     * It is used in levenstein distance calculation.
     * @param num_of_syntax_outcomes: the boundary value for minor incorrect semantics.
     */
    int get_semantics_minor_incorrect_ted() const;

private:
    string connection_string; /**< The connection string used to connect to the database. contructed from host, database, user and password*/
    // grading parameters
    Grader::property_level syntax_level;    /**< The number of syntax outcomes to use in grading. */
    Grader::property_level semantics_level; /**< The number of semantics outcomes to use in grading. */
    Grader::property_level results_level;   /**< The number of results outcomes to use in grading. */
    Grader::property_order property_order;  /**< The order of importance of the grading properties. 1 - syntax most important, 2 - semantics most important, 3 - results most important */
    bool check_order;                       /**< If true, the order of the results is also checked. If false, the order of the results is not checked. */
    int syntax_minor_incorrect_ted;         /**< The boundary value for determining how many wrong characters a required to classify the syntax of a query as minor incorrect. It is used in levenstein distance calculation. */
    int semantics_minor_incorrect_ted;      /**< The boundary value for determining how many wrong characters a required to classify the semantics of a query as minor incorrect. It is used in levenstein distance calculation. */
};
#endif // !ADMIN_H