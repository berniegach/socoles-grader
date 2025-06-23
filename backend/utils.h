/**
 * @file utils.h
 * @brief This file contains the declaration of the Utils class.
 * The class is used to store utility functions.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef MY_UTILS_H
#define MY_UTILS_H
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <algorithm>
#include <tuple>
#include "admin.h"

class Utils
{
private:
public:
    /**
     * Custom struct to store comparison results of two 2D string vectors
     * is_equal: true if the two vectors are equal, false otherwise
     * is_subset: true if the first vector is a subset of the second vector, false otherwise
     * difference: the number of differences between the two vectors
     */
    struct comparison_result
    {
        /**
         * True if the two vectors are equal, false otherwise.
         */
        bool is_equal;
        /**
         * True if the first vector is a subset of the second vector, false otherwise.
         */
        bool is_subset;
        /**
         * The number of differences between two vectors. It counts the total number of elements in both vectors that have not been matched.
         */
        int difference; /**< the number of differences between the two vectors */
    };
    /**
     * Overloaded operator function to compare two comparison_result objects
     * @param a: the first comparison_result object
     * @param b: the second comparison_result object
     * @return true if the two objects are equal, false otherwise
     */
    friend bool operator==(const comparison_result &a, const comparison_result &b)
    {
        return a.is_equal == b.is_equal && a.is_subset == b.is_subset && a.difference == b.difference;
    }
    /**
     * Function to compare two 2D vectors (SQL query results) and calculate their differences.
     * @param query_results: the results of the first query in 2D form.
     * @param correct_results: the results of the second query in 2D form.
     * @param check_order: if true, the order of the results is also checked. If false, the order of the results is not checked.
     * @return a comparison_result object containing the comparison results.
     */
    comparison_result compare_vectors(std::vector<std::vector<std::string>> query_results, std::vector<std::vector<std::string>> correct_results, bool check_order = false);
    comparison_result compare_output(std::vector<std::vector<std::string>> query_results, std::vector<std::vector<std::string>> correct_results, bool check_order = false);
    bool is_1D_subset(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger);
    bool is_1D_subset_ordered(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger);
    bool is_sequence_contained_ordered(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger);
    /**
     * THis function is used to replace double quotes with single quotes in a query.
     * @param query: the query to be processed.
     * @return a pair containing the processed query and a boolean indicating whether the query was processed or not.
     */
    std::pair<std::string, bool> replace_double_quotes_with_single_quotes(const std::string &query);
    /**
     * This function is used to draw (-----) separator line in the terminal.
     * It is used to draw a table.
     */
    void draw_line_separator();
    /**
     * This function is used to draw a 1d vector in the terminal.
     * @param v: the 1d vector to be drawn.
     */
    void print_vector(const std::vector<std::string> &v);
    /**
     * This function is used to draw a 2d vector in the terminal.
     * @param v: the 2d vector to be drawn.
     */
    void print_2d_vector(const std::vector<std::vector<std::string>> &v);
    /**
     * This function is used to tranpose a 2d vector.
     * @param original: the 2d vector to be transposed.
     * @return the transposed 2d vector.
     */
    std::vector<std::vector<std::string>> transpose(const std::vector<std::vector<std::string>> &original);
    /**
     * This function checks if there is any mismatch between two 2d vectors.
     * This is a helper function for check_small_array_in_big_array.
     * @param smaller: the smaller 2d vector.
     * @param bigger: the bigger 2d vector.
     * @param row_offset: the row offset to be used when comparing the two vectors.
     * @param col_offset: the column offset to be used when comparing the two vectors.
     * @return true if there is no mismatch, false otherwise.
     */
    bool is_matched(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger, int row_offset, int col_offset);
    /**
     * This function checks if a smaller 2d vector is contained inside a bigger 2d vector.
     * @param smaller: the smaller 2d vector.
     * @param bigger: the bigger 2d vector.
     * @return true if the smaller vector is contained inside the bigger vector, false otherwise.
     */
    bool check_small_array_in_big_array(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger);
    /**
     * This function is used to get the number of elements that are different between two 2d vectors.
     * @param vec_1: the first 2d vector.
     * @param vec_2: the second 2d vector.
     * @return the number of elements that are different between the two vectors.
     */
    int get_number_of_vector_elements_differences(const std::vector<std::vector<std::string>> &vec_1, const std::vector<std::vector<std::string>> &vec_2);
    std::pair<int, std::vector<std::tuple<std::string, std::string, std::string>>> general_edit_distance_words(const std::vector<std::string> &source, const std::vector<std::string> &target, int insert_cost = 1, int delete_cost = 1, int replace_cost = 1);
    /**
     * This function gets the general edit distance between two strings.
     * @param source: the first string.
     * @param target: the second string.
     * @param insert_cost: the cost of inserting a character.
     * @param delete_cost: the cost of deleting a character.
     * @param replace_cost: the cost of replacing a character.
     * @return the general edit distance between the two strings.
     */
    std::string::size_type general_edit_distance(const std::string &source, const std::string &target, std::string::size_type insert_cost = 1, std::string::size_type delete_cost = 1, std::string::size_type replace_cost = 1);
    /**
     * Multibyte version of the general edit distance function.
     */
    std::wstring::size_type general_edit_distance(const std::wstring &source, const std::wstring &target, std::wstring::size_type insert_cost = 1, std::wstring::size_type delete_cost = 1, std::wstring::size_type replace_cost = 1);
    /**
     * THis is a helper function for normalized_levenshtein_distance.
     * @param insert_cost: the cost of inserting a character.
     * @param delete_cost: the cost of deleting a character.
     * @return the alpha measure.
     */
    std::string::size_type alpha_measure(std::string::size_type insert_cost, std::string::size_type delete_cost);
    // std::string::size_type generalized_levenshtein_similarity(const std::string &source, const std::string &target, std::string::size_type alpha, std::string::size_type g_edit_distance);
    /**
     * This function gets the normalized levenshtein distance between two strings.I.e from 0 to 1. with 0 being the same and 1 being completely different.
     * @param source: the first string.
     * @param target: the second string.
     * @param insert_cost: the cost of inserting a character.
     * @param delete_cost: the cost of deleting a character.
     * @param replace_cost: the cost of replacing a character.
     * @return the normalized levenshtein distance between the two strings.
     */
    float normalized_levenshtein_distance(const std::string &source, const std::string &target, std::string::size_type insert_cost = 1, std::string::size_type delete_cost = 1, std::string::size_type replace_cost = 1);
    std::map<std::string, std::string> read_config(const std::string &filename);
    /**
     * This function is used to read a csv file.
     * @param filename: the name of the csv file.
     * @param standard_csv: true if the csv file is a standard csv file i.e not an file containing sql statements, false otherwise.
     * @return a 2d vector containing the contents of the csv file.
     */
    std::vector<std::vector<std::string>> read_csv(const std::string &filename, bool standard_csv);
    /**
     * This function is used to read a normal csv file. This file only uses comma as a separator.
     * It is called by read_csv.
     * @param filename: the name of the csv file.
     * @return a 2d vector containing the contents of the csv file.
     */
    std::vector<std::vector<std::string>> read_other_csv(const std::string &filename);
    /**
     * This function is used to read a csv file containing sql statements. The file has comma as separtors and also inside sql statements.
     * It is called by read_csv.
     * @param filename: the name of the csv file.
     * @return a 2d vector containing the contents of the csv file.
     */
    std::vector<std::vector<std::string>> read_sql_csv(const std::string &filename);
    /**
     * This function is used to preprocess a query before it is executed.
     * It first, removes all the spaces at the beginning and end of the query.
     * It then, removes changes the query to lower case.
     * @param query: the query to be preprocessed.
     */
    void preprocess_query(std::string &query);
    /**
     * This functions checks if two characters are spaces.
     * THis is a helper function for preprocess_query.
     * @param lhs: the first character.
     * @param rhs: the second character.
     * @return true if the two characters are spaces, false otherwise.
     */
    static bool both_are_spaces(char lhs, char rhs);
    /**
     * This function is used to fix a query using SQL keywords.
     * @param query: the query to be fixed.
     * @return a pair containing a boolean indicating whether the query was fixed or not and the fixed query.
     * The boolean is true if the query was fixed, false otherwise.
     */
    std::tuple<bool, std::string> fix_query_syntax_using_keywords(std::string query);
    /**
     * This function is used to fix a query using another query.
     * @param query: the query to be fixed.
     * @return a pair containing a boolean indicating whether the query was fixed or not and the fixed query.
     * The boolean is true if the query was fixed, false otherwise.
     */
    std::tuple<bool, std::string, int> fix_query_syntax_using_another_query(const Admin admin, std::string query, std::string query_model, std::string query_untouched);

    bool check_if_word_is_keyword(std::string word);
};

#endif