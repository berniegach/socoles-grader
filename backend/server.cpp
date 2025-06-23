#include <crow.h>
#include <crow/middlewares/cors.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "goals.h"
#include "abstract_syntax_tree.h"
#include "model_query.h"
#include "student_query.h"
#include "process_queries.h"
#include "admin.h"

Grader::grading_options set_grading_options(int syntax, int semantics, int results, int order_of_importance);
int main()
{
    // Initialize necessary objects
    AbstractSyntaxTree ast;
    Goals goals;
    // crow::SimpleApp app;
    crow::App<crow::CORSHandler> app;

    auto &cors = app.get_middleware<crow::CORSHandler>();

    // Configure CORS settings
    cors
        .global()
        .origin("http://localhost:3000") // Specify allowed origin(s)
        .methods("POST"_method, "OPTIONS"_method)
        .headers("Content-Type", "Authorization")
        .max_age(86400); // Optional: Cache preflight response

    CROW_ROUTE(app, "/parse-queries").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&goals](const crow::request &req)
                                                                                                 {
                                                                                                     std::cout << "Received request with method: " << crow::method_name(req.method) << std::endl;

                                                                                                     crow::response res;

                                                                                                     // Add CORS headers
                                                                                                     /*res.add_header("Access-Control-Allow-Origin", "*"); // Allow all origins. Dont do this in production as it is a security risk
                                                                                                     //res.add_header("Access-Control-Allow-Origin", "http://localhost:3000");
                                                                                                     res.add_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                                                                                                     res.add_header("Access-Control-Allow-Headers", "Content-Type");*/

                                                                                                     if (req.method == "OPTIONS"_method)
                                                                                                     {
                                                                                                         // For preflight requests, we return a response with no content
                                                                                                         res.code = 204; // No Content
                                                                                                         return res;
                                                                                                     }

                                                                                                     // Handle POST request
                                                                                                     auto body = crow::json::load(req.body);
                                                                                                     if (!body)
                                                                                                     {
                                                                                                         res.code = 400; // Bad Request
                                                                                                         res.write("Invalid JSON");
                                                                                                         return res;
                                                                                                     }

                                                                                                     // Extract queries and process them
                                                                                                     std::vector<std::string> queries;
                                                                                                     for (const auto &query : body["queries"])
                                                                                                     {
                                                                                                         queries.push_back(query.s());
                                                                                                     }

                                                                                                     crow::json::wvalue result;
                                                                                                     int index = 0;

                                                                                                     for (const auto &query : queries)
                                                                                                     {
                                                                                                         ModelQuery model_query(std::to_string(index), query);

                                                                                                         model_query.create_abstract_syntax_tree();
                                                                                                         auto root_node = model_query.get_parse_tree();
                                                                                                         std::string goal_general; 
                                                                                                         auto goals = Goals::generate_query_goal_general(root_node);
                                                                                                         if (!goals.empty())
                                                                                                         {
                                                                                                             for (const auto &g : goals)
                                                                                                             {
                                                                                                                 goal_general += "1️⃣ Goal:\n" + g + "\n\n";
                                                                                                                              
                                                                                                             }
                                                                                                         }
                                                                                                         std::string goal_specific; 
                                                                                                         auto goals_specific = Goals::generate_query_goal_specific(root_node);
                                                                                                            if (!goals_specific.empty())
                                                                                                            {
                                                                                                                for (const auto &g : goals_specific)
                                                                                                                {
                                                                                                                    goal_specific += "1️⃣ Goal:\n" + g + "\n\n";
                                                                                                                }
                                                                                                            }
                                                                                                         std::vector<Goals::Goal> gl = Goals::process_query(root_node);

                                                                                                         // Add query and goals to the result
                                                                                                         result[index]["query"] = query;
                                                                                                         result[index]["goal_general"] = goal_general;
                                                                                                         result[index]["goal_specific"] = goal_specific;

                                                                                                         // Add goals to the "goals" array using integer indices
                                                                                                         int goal_index = 0;
                                                                                                         for (const auto &goal : gl)
                                                                                                         {
                                                                                                             result[index]["goals"][goal_index]["type"] = goal.type;
                                                                                                             result[index]["goals"][goal_index]["content"] = goal.content;
                                                                                                             goal_index++;
                                                                                                         }

                                                                                                         index++;
                                                                                                     }

                                                                                                     // Prepare the response
                                                                                                     res.code = 200; // OK
                                                                                                     res.set_header("Content-Type", "application/json");
                                                                                                     res.write(result.dump());

                                                                                                     return res; });

    // **New /grade-queries Endpoint**
    CROW_ROUTE(app, "/grade-queries").methods(crow::HTTPMethod::POST, crow::HTTPMethod::OPTIONS)([&goals](const crow::request &req)
                                                                                                 {
                                                                                                     std::cout << "Received /grade-queries request with method: " << crow::method_name(req.method) << std::endl;

                                                                                                     crow::response res;

                                                                                                     if (req.method == "OPTIONS"_method)
                                                                                                     {
                                                                                                         // For preflight requests, return a response with no content
                                                                                                         res.code = 204; // No Content
                                                                                                         return res;
                                                                                                     }

                                                                                                     try
                                                                                                     {
                                                                                                         // Handle POST request
                                                                                                         auto body = crow::json::load(req.body);
                                                                                                         if (!body)
                                                                                                         {
                                                                                                             res.code = 400; // Bad Request
                                                                                                             res.write("Invalid JSON");
                                                                                                             return res;
                                                                                                         }

                                                                                                         //--sql=290/discography.sql --queries=290/original_results.csv --model=290/correct.csv --syntax=3 --semantics=8 --results=3 --prop_order=5 --edit_dist=4 --tree_dist=4 --check_order=0
                                                                                                         // **Validate Required Fields**
                                                                                                         if (!body.has("sql_data") || !body.has("queries") || !body.has("model_queries") ||
                                                                                                             !body.has("syntax") || !body.has("semantics") || !body.has("results") ||
                                                                                                             !body.has("prop_order") || !body.has("edit_dist") || !body.has("tree_dist") || !body.has("check_order") ||
                                                                                                             !body.has("auto_db") || !body.has("num_db") || !body.has("sql_create_data") || !body.has("dbname") || !body.has("use_postgresql"))
                                                                                                         {
                                                                                                             res.code = 400; // Bad Request
                                                                                                             res.write("Missing one or more required fields: sqlData, queries, options");
                                                                                                             return res;
                                                                                                         }

                                                                                                         // **Extract Model Queries**
                                                                                                         // Expected JSON structure:
                                                                                                         /*
                                                                                                         {
                                                                                                             "model": [
                                                                                                                 "SELECT * FROM students;",
                                                                                                                 "SELECT * FROM courses;"
                                                                                                             ]
                                                                                                         }
                                                                                                         */
                                                                                                         vector<ModelQuery> model_queries;
                                                                                                         int index = 1;
                                                                                                         for (const auto &query : body["model_queries"])
                                                                                                         {
                                                                                                             std::string element = query.s();
                                                                                                             std::string id = std::to_string(index);

                                                                                                             // If the string begins and ends with quotes, remove them
                                                                                                             if (element.size() > 1 && element.front() == '\"' && element.back() == '\"')
                                                                                                             {
                                                                                                                 element.erase(0, 1);               // Remove first character
                                                                                                                 element.erase(element.size() - 1); // Remove last character

                                                                                                                 // Replace "" with "
                                                                                                                 size_t position = element.find("\"\"");
                                                                                                                 while (position != std::string::npos)
                                                                                                                 {
                                                                                                                     element.replace(position, 2, "\"");
                                                                                                                     position = element.find("\"\"", position + 1);
                                                                                                                 }
                                                                                                             }
                                                                                                             ModelQuery model_query(std::to_string(index++), element);
                                                                                                             model_queries.push_back(model_query);
                                                                                                         }

                                                                                                         // **Extract Student Queries** we only care about Org Defined ID, Attempt #, Q # and Answer
                                                                                                         // Expected JSON structure:
                                                                                                         /*
                                                                                                         {
                                                                                                             "queries": [
                                                                                                                 "ID123",
                                                                                                                 1,
                                                                                                                 "Q1",
                                                                                                                 "SELECT * FROM students;"
                                                                                                             ],
                                                                                                             // ... more entries
                                                                                                         }
                                                                                                         */
                                                                                                         std::vector<StudentQuery> student_queries;
                                                                                                         for (const auto &query : body["queries"])
                                                                                                         {
                                                                                                             // StudentQuery student_query(id, query, question_number, attempt_number);
                                                                                                             StudentQuery student_query(query[0].s(), query[3].s(), query[2].s(), query[1].i());
                                                                                                             student_queries.push_back(student_query);
                                                                                                         }

                                                                                                         // **Extract Options**
                                                                                                         // Expected JSON structure:
                                                                                                         /*
                                                                                                         {
                                                                                                             "syntax": 3,
                                                                                                             "semantics": 8,
                                                                                                             "results": 3,
                                                                                                             "prop_order": 5,
                                                                                                             "edit_dist": 4,
                                                                                                             "tree_dist": 4,
                                                                                                             "check_order": 0,
                                                                                                             "auto_db": 1,
                                                                                                             "num_db": 3,
                                                                                                             "sql_create": "create table ...",
                                                                                                             "dbname": "my_database"
                                                                                                         }
                                                                                                         */
                                                                                                         int syntax_sensitivity = body["syntax"].i();
                                                                                                         int semantics_sensitivity = body["semantics"].i();
                                                                                                         int results_sensitivity = body["results"].i();
                                                                                                         int prop_order = body["prop_order"].i();
                                                                                                         int edit_dist = body["edit_dist"].i();
                                                                                                         int tree_dist = body["tree_dist"].i();
                                                                                                         int check_order = body["check_order"].i();
                                                                                                         int auto_db = body["auto_db"].i();
                                                                                                         int num_db = body["num_db"].i();
                                                                                                         std::string sql_create_only = body["sql_create_data"].s();
                                                                                                         std::string dbname = body["dbname"].s();
                                                                                                         dbname = "grader";
                                                                                                         bool use_postgresql = body["use_postgresql"].b();

                                                                                                         // set grading options
                                                                                                         Grader::grading_options grading_options = set_grading_options(syntax_sensitivity, semantics_sensitivity, results_sensitivity, prop_order);

                                                                                                         // Initialize the Admin object
                                                                                                         Admin admin;
                                                                                                         admin.init(grading_options.syntax, grading_options.semantics, grading_options.results, grading_options.order_of_importance, (check_order == 1), edit_dist, tree_dist);

                                                                                                         // Process the queries
                                                                                                         Grader grader;
                                                                                                         // Extract the .sql file path
                                                                                                         std::string sql_create_insert_data = body["sql_data"].s();
                                                                                                         Admin::database_options db_opts = {sql_create_insert_data, auto_db, num_db, sql_create_insert_data, "", dbname, true, use_postgresql};
                                                                                                         // for now skip the evosql part in the argment ""
                                                                                                         ProcessQueries process_queries(model_queries, student_queries, db_opts, admin, grader);

                                                                                                         // Output the results
                                                                                                         std::vector<ProcessQueries::grading_info> grading_info = process_queries.get_grading_info(&student_queries, admin);

                                                                                                         // Prepare the JSON response
                                                                                                         index = 0;
                                                                                                         crow::json::wvalue jsonResults;
                                                                                                         for (const auto &info : grading_info)
                                                                                                         {
                                                                                                             jsonResults[index]["Org Defined ID"] = info.org_defined_id;
                                                                                                             jsonResults[index]["Attempt #"] = info.attempt_number;
                                                                                                             jsonResults[index]["Q #"] = info.question_number;
                                                                                                             jsonResults[index]["Query"] = info.query;
                                                                                                             jsonResults[index]["Grade"] = info.grade;
                                                                                                             jsonResults[index]["Out Of"] = info.out_of;
                                                                                                             jsonResults[index]["Feedback"] = info.feedback;
                                                                                                             index++;
                                                                                                         }

                                                                                                         res.code = 200; // OK
                                                                                                         res.set_header("Content-Type", "application/json");
                                                                                                         res.write(jsonResults.dump());

                                                                                                         return res;
                                                                                                     }
                                                                                                     catch (const std::exception &e)
                                                                                                     {
                                                                                                         std::cerr << "Exception occurred: " << e.what() << std::endl;
                                                                                                         res.code = 500;
                                                                                                         res.write("Internal Server Error: " + std::string(e.what()));
                                                                                                         return res;
                                                                                                     } });

    app.port(5000).multithreaded().run();
}

Grader::grading_options set_grading_options(int syntax, int semantics, int results, int order_of_importance)
{
    Grader::grading_options options;
    switch (syntax)
    {
    case 0:
        options.syntax = Grader::property_level::ABSENT;
        break;
    case 2:
        options.syntax = Grader::property_level::TWO_LEVELS;
        break;
    case 3:
        options.syntax = Grader::property_level::THREE_LEVELS;
        break;
    default:
        options.syntax = Grader::property_level::THREE_LEVELS;
        break;
    }

    switch (semantics)
    {
    case 0:
        options.semantics = Grader::property_level::ABSENT;
        break;
    case 2:
        options.semantics = Grader::property_level::TWO_LEVELS;
        break;
    case 3:
        options.semantics = Grader::property_level::THREE_LEVELS;
        break;
    case 8:
        options.semantics = Grader::property_level::SEMATICS_LEVELS_6;
        break;
    default:
        options.semantics = Grader::property_level::SEMATICS_LEVELS_6;
        break;
    }

    switch (results)
    {
    case 0:
        options.results = Grader::property_level::ABSENT;
        break;
    case 2:
        options.results = Grader::property_level::TWO_LEVELS;
        break;
    case 3:
        options.results = Grader::property_level::THREE_LEVELS;
        break;
    default:
        options.results = Grader::property_level::THREE_LEVELS;
        break;
    }

    switch (order_of_importance)
    {
    case 1:
        options.order_of_importance = Grader::property_order::SY_SM_RE;
        break;
    case 2:
        options.order_of_importance = Grader::property_order::SM_SY_RE;
        break;
    case 3:
        options.order_of_importance = Grader::property_order::RE_SM_SY;
        break;
    case 4:
        options.order_of_importance = Grader::property_order::SY_RE_SM;
        break;
    case 5:
        options.order_of_importance = Grader::property_order::SM_RE_SY;
        break;
    case 6:
        options.order_of_importance = Grader::property_order::RE_SY_SM;
        break;
    default:
        options.order_of_importance = Grader::property_order::SM_RE_SY;
        break;
    }
    return options;
}
