/**
 * @file model_query.h
 * @brief This file contains the declaration of the ModelQuery class.
 * The class is used to store the queries and their properties. It also serves as a base class for the StudentQuery class.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef MODEL_QUERY_H
#define MODEL_QUERY_H

#include <string>
#include <vector>
#include "abstract_syntax_tree.h"
#include "my_duckdb.h"
extern "C"
{
#include <pg_query.h>
}
#include "admin.h"
#include "query_engine.h"

class ModelQuery
{
public:
    /**
     * Default constructor.
     */
    ModelQuery();
    /**
     * Constructor that takes the id and the value of the query.
     * @param id: the id of the query.
     * @param value: the query in text form.
     */
    ModelQuery(const string &id, const string &value);
    /**
     * This function sets the id of a query.
     * @param id: the id of the query.
     */
    void set_id(const string &id);
    /**
     * This function returns the id of a query.
     * @return: the id of the query.
     */
    string get_id() const;
    /**
     * This function sets the value of a query.
     * @param value: the query in text form.
     */
    void set_value(const string &value);
    /**
     * This function returns the value of a query.
     * @return: the query in text form.
     */
    string get_value() const;
    /**
     * This function sets the output of a query when it is executed.
     * @param output: the output of the query in a 2D matrix form.
     */
    void set_output(const vector<vector<string>> &output);
    /**
     * This function returns the output of a query when it is executed.
     * @return: the output of the query in a 2D matrix form.
     */
    vector<vector<string>> get_output() const;
    /**
     * This function sets the fingerprint of a query.
     * The fingerprint is a hash of the parse tree of the query
     * @param fingerprint: the fingerprint of the query.
     */
    void set_fingerprint(const string &fingerprint);
    /**
     * This function returns the fingerprint of a query.
     * @return: the fingerprint of the query as hash string.
     */
    string get_fingerprint() const;
    /**
     * This function sets the parse tree of a query.
     * @param parse_tree: the root node of the parse tree.
     */
    void set_parse_tree(const std::shared_ptr<AbstractSyntaxTree::Node> &parse_tree);
    /**
     * This function returns the parse tree of a query.
     * @return: the root node of the parse tree.
     */
    std::shared_ptr<AbstractSyntaxTree::Node> get_parse_tree() const;
    /**
     * This function an abstract syntax tree of a query.
     */
    void create_abstract_syntax_tree();
    /**
     * This function creates a fingerprint of a query.
     */
    void create_fingerprint();
    /**
     * This function creates the output of a query when it is executed.
     * The function is virtual because the StudentQuery class overrides it.
     * @param qe: database engine.
     */
    virtual void create_output(Query_Engine &qe);
    /**
     * This function prints the output of a query when it is executed.
     */
    void print_output();
    /**
     * This function returns the parse tree in PgQueryProtobuf format.
     */
    PgQueryProtobuf get_parse_result() const;
    /**
     * This function is used to get the model queries from a csv file.
     * @param filename: the name of the csv file.
     * @return a 2d vector containing the model queries.
     */
    static vector<ModelQuery> get_model_queries_from_csv(const std::string &filename);
    void set_goal_general(const std::vector<string> &goal_general);
    std::vector<string> get_goal_general() const;
    void set_goal_specific(const std::vector<string> &goal_specific);
    std::vector<string> get_goal_specific() const;

private:
    string id;                                            /**< The id of the query. */
    string value;                                         /**< The query in text form. */
    vector<vector<string>> output;                        /**< The output of the query when it is executed. */
    string fingerprint;                                   /**< The fingerprint of the query as a hash string. */
    std::shared_ptr<AbstractSyntaxTree::Node> parse_tree; /**< The root node of the parse tree of the query. */
    PgQueryProtobuf parse_result;                         /**< The parse tree of the query. */
    std::vector<string> goal_general;                     /**< The general goal of the query. */
    std::vector<string> goal_specific;                    /**< The specific goal of the query. */
};
#endif // !MODEL_QUERY_H