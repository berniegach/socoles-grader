#include "my_evosql.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <fstream>
#include "model_query.h"
#include "utils.h"


MyEvoSQL::MyEvoSQL()
{
    //empty constructor
}
MyEvoSQL::MyEvoSQL(std::string model_sql_file, std::string reference_query_sql_file, std::string database, MyDuckDB& my_duckdb)
{
    // Command to run the Java program
    const std::string cmd = "java -jar evosql.jar " + reference_query_sql_file + " junit5 \"jdbc:postgresql://localhost:5432/" + database + "?user=" + postgresql_username + "&password="+ postgresql_password + "\" username pw=hjkl pkg=model cls=MyModel";
    bool success = false;
    int count = 0;
    //run init until the database is created successfully
    while(!success)
    {
        std::cout << "Trying to create a database run: " << ++count << std::endl;
        success = init(cmd, model_sql_file, reference_query_sql_file, database, my_duckdb);
    }
}
bool MyEvoSQL::init(const std::string command, std::string model_sql_file, std::string reference_query_sql_file, std::string database, MyDuckDB& my_duckdb)
{ 
    try 
    {
        // Execute the command and capture the output
        std::vector<std::string> output = execute(command);
        if (!output.empty()) 
        {
            //std::cout << "Captured fixtures:\n";
            for (const auto& line : output)
            {
                //std::cout << line << std::endl;
                //std::cout << "Insert statements:\n";
                std::vector<std::string> insert_statements = extract_insert_statements(line);
                /*for (const auto& statement : insert_statements)
                {
                    std::cout << statement << std::endl;
                }*/
                return create_database(my_duckdb, reference_query_sql_file, insert_statements);
                /*if(success)
                {
                    string label = std::to_string(how_many);
                    write_statements(insert_statements, label);
                    how_many+=1;
                }
                else
                {
                    string label = "F " + std::to_string(how_many);
                    write_statements(insert_statements, label);
                }*/
            }
        } 
        else 
        {
            std::cout << "No fixtures found in output." << std::endl;
            return false;
        }
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        exit(1);
    }
    return false;   
}

std::vector<std::string> MyEvoSQL::execute(std::string command)
{
    std::array<char, 4096> buffer;
    std::vector<std::string> results;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) 
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
    {
        std::string line(buffer.data());
        if (line.find("Generated fixture:") != std::string::npos) 
        {
            results.push_back(line);
        }
    }
    return results;
}

std::vector<std::string> MyEvoSQL::extract_insert_statements(const std::string& fixture_line)
{
    /**
     * A fixture line looks like this:
     * 13:09:55.655 [main] INFO  nl.tudelft.serg.evosql.EvoSQL - Generated fixture: [INSERT INTO "performs" VALUES ('1','856','%I5DjVn}[`3o!_)Uh$2NVO,3LY!H;UWFLDT#'), ('1','865','C!./1&8QQOX35< _KnLTCf#'), INSERT INTO "artist" VALUES ('2','nF6k3JD$WT~h8[v@I9O+xVSd9 m]l5xe9%_','2012-07-11','2020-09-23'), INSERT INTO "track" VALUES ('865','365','VMoa$=/-ELM6suV#[Gh*1S(K01sFLvsKR)','690'), ('856','1','P4qMYCWRt;]E5)e9Jj8Z|0*DSU;:=j?Y"[0E!HP5(s0Gumr*f','3'), ('856','996','"c~[?gr6S','1')]
    */
    std::vector<std::string> insert_statements;
   
   //first split the string using the words "Generated fixture:"
   //boost library is not able to handle the fixture long string therefore we use regex.
   std::regex regex_delimiter(R"(Generated fixture:)");
    std::sregex_token_iterator iter(fixture_line.begin(), fixture_line.end(), regex_delimiter, -1);
    std::sregex_token_iterator end;

    std::vector<std::string> splited_line(iter, end);

    if (splited_line.size() < 2) {
        std::cerr << "Error: Could not split the fixture line." << std::endl;
    }
    
    //the second part contains the insert statements
    std::string insert_section = splited_line[1];

    //remove leading and trailing whitespaces
    boost::trim(insert_section);

    //remove the square brackets at the beginning and end of the string
    insert_section = insert_section.substr(1, insert_section.size() - 2);
    std::regex reg(R"(INSERT INTO.*?(?=,\s*INSERT INTO|$))");
    std::sregex_iterator iter2(insert_section.begin(), insert_section.end(), reg);
    std::sregex_iterator end2;

    for (std::sregex_iterator i = iter2; i != end2; ++i) {
        insert_statements.push_back((*i).str());
    }

    // Remove quotes from table names
    std::regex table_name_quotes(R"(INSERT INTO\s+\"([^\"]+)\"\s+VALUES)");
    for (auto& statement : insert_statements) {
        statement = std::regex_replace(statement, table_name_quotes, "INSERT INTO $1 VALUES");
    }
    
    return insert_statements;
}

bool MyEvoSQL::create_database(MyDuckDB& my_duckdb, const std::string reference_query_sql_file, std::vector<std::string> insert_statements)
{
    Utils my_utils;
    //first the queries that creates the tables. 
    //MyDuckDB my_duckdb(model_sql_file);

    //then the insert statements
    for (const auto& statement : insert_statements)
    {
        std::string error = "";
        my_duckdb.execute_query_cud(statement, error);
        if (error != "")
        {
            std::cerr << "Error: Could not execute the insert statement: " << statement << std::endl;
            std::cerr << "Error: " << error << std::endl;
            //exit(1);
            return false;
        }
    }

    //get the model reference query
    const std::string reference_query = read_model_query(reference_query_sql_file);

    //run the reference query, if it returns the results then the database is good
    ModelQuery model_query("1", reference_query);
    //DEAL WITH THIS LATER
	//model_query.create_output( -1);
    if(model_query.get_output().empty())
    {
        //the insert statements are not correct
        //write an error message
        std::cout << "WRONG!!!!These insert statements do not create a database that resturns results given the query: " << reference_query << std::endl;
        for (const auto& statement : insert_statements)
        {
            std::cout << statement << std::endl;
        }
        std::cout << "The output seen is: \n";
        //my_utils.print_2d_vector(model_query.get_output());
        return false;
    }
    //the insert statements are correct
    std::cout << "These insert statements create a database that returns results given the query: " << reference_query << std::endl;
    for (const auto& statement : insert_statements)
    {
        std::cout << statement << std::endl;
    }
    std::cout << "The output seen is: \n";
    //my_utils.print_2d_vector(model_query.get_output());
    //databases.push_back(my_duckdb);
    return true;
}

const std::string MyEvoSQL::read_model_query(const std::string &model_sql_file)
{
    //open the file
    std::ifstream file(model_sql_file);
    if (!file.is_open())
    {
        std::cerr << "Could not open file: " << model_sql_file << std::endl;
        exit(1);
    }

    //read the contents into a string
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string query = buffer.str();
    return query;
}

void MyEvoSQL::write_statements(const std::vector<std::string> &insert_statements, std::string label)
{
    std::ofstream file;
    //open the file in append mode
    file.open("insert_statements.txt", std::ios_base::app);

    for (const auto& statement : insert_statements)
    {
        file << label << ": " << statement << std::endl;
    }
    file.close();
}
