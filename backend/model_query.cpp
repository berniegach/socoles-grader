#include "model_query.h"
#include "my_duckdb.h"
#include <iostream>
#include "utils.h"
#include "my_evosql.h"

ModelQuery::ModelQuery() {}

ModelQuery::ModelQuery(const string &id, const string &value)
    : id(id), value(value)
{
}

void ModelQuery::set_id(const string &id)
{
    this->id = id;
}

string ModelQuery::get_id() const
{
    return id;
}

void ModelQuery::set_value(const string &value)
{
    this->value = value;
}

string ModelQuery::get_value() const
{
    return value;
}

void ModelQuery::set_output(const vector<vector<string>> &output)
{
    this->output = output;
}

vector<vector<string>> ModelQuery::get_output() const
{
    return output;
}

void ModelQuery::set_fingerprint(const string &fingerprint)
{
    this->fingerprint = fingerprint;
}

string ModelQuery::get_fingerprint() const
{
    return fingerprint;
}

void ModelQuery::set_parse_tree(const std::shared_ptr<AbstractSyntaxTree::Node> &parse_tree)
{
    this->parse_tree = parse_tree;
}

std::shared_ptr<AbstractSyntaxTree::Node> ModelQuery::get_parse_tree() const
{
    return parse_tree;
}

void ModelQuery::create_abstract_syntax_tree()
{
    AbstractSyntaxTree ast;
    PgQueryParseResult result = pg_query_parse(value.c_str());
    nlohmann::json json_parse_tree_current = nlohmann::json::parse(result.parse_tree);
    parse_tree = std::make_shared<AbstractSyntaxTree::Node>("root", "");
    ast.build_tree(json_parse_tree_current, parse_tree);

    PgQueryProtobufParseResult protobuf_result = pg_query_parse_protobuf(value.c_str());
    parse_result = protobuf_result.parse_tree;
}

void ModelQuery::create_fingerprint()
{
    PgQueryFingerprintResult result = pg_query_fingerprint(value.c_str());
    fingerprint = result.fingerprint_str;
}

void ModelQuery::create_output(Query_Engine &qe)
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
                //
            }
        }
    }
    catch (const std::exception &)
    {
        std::cout << "DBMS crashed. Crashed at model id: " << get_id() << std::endl;
    }
}

void ModelQuery::print_output()
{
    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    cout << "Output of query: " << value << endl;

    for (const auto &row : output)
    {
        for (const auto &column : row)
        {
            cout << column << " ";
        }
        cout << endl;
    }
}

PgQueryProtobuf ModelQuery::get_parse_result() const
{
    return parse_result;
}
vector<ModelQuery> ModelQuery::get_model_queries_from_csv(const std::string &filename)
{
    Utils utils;
    vector<ModelQuery> model_queries;
    /**
     * This parameter is used to determine if the csv file been read contains SQL data or not.
     * If it contains SQL data, then the csv file is not in the standard format.
     * An SQL file will contain characters like ;,() etc. in a query string which will be interpreted incorrectly by the csv standard reader.
     * If the csv is not standard then the reader will take this into account.
     */
    bool standard_csv_format = false;
    // read the correct queries that will serve as the basis for grading
    const std::vector<std::vector<std::string>> model_queries_data = utils.read_csv(filename, standard_csv_format);
    int count_model_queries = 1;
    for (const auto &row : model_queries_data)
    {
        // get the correct queries one by one
        string id = std::to_string(count_model_queries++);
        string query = row[0];

        // this will help when comparing text edit distances between queries
        //  remove spaces from the query and change query to lower case
        // my_utils.preprocess_query(query);

        ModelQuery model_query(id, query);
        model_queries.push_back(model_query);
    }

    return model_queries;
}

void ModelQuery::set_goal_general(const std::vector<string> &goal_general)
{
    this->goal_general = goal_general;
}

std::vector<string> ModelQuery::get_goal_general() const
{
    return goal_general;
}

void ModelQuery::set_goal_specific(const std::vector<string> &goal_specific)
{
    this->goal_specific = goal_specific;
}

std::vector<string> ModelQuery::get_goal_specific() const
{
    return goal_specific;
}
