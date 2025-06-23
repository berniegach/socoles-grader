#include "grader.h"
#include<iostream>
#include"utils.h"
#include<cmath>
#include "admin.h"
#include "student_query.h"
#include <cstdlib>
#include <iomanip>

vector<Grader::properties> Grader::get_correctness_matrix(property_level results, property_level semantics, property_level syntax, property_order order)
{
    using ps = property_state;
    using pl = property_level;
    std::vector<properties> matrix;
    
    // Lambda function to get the possible states based on the level
    auto get_possible_states = [](property_level level) -> std::vector<property_state> {
        if (level == property_level::ABSENT) 
        {
            return {property_state::INVALID};
        } 
        else if (level == property_level::TWO_LEVELS) 
        {
            return {property_state::INCORRECT, property_state::CORRECT};
        } 
        //This option is only available for semantics, make sure to check this
        else if (level == property_level::SEMATICS_LEVELS_6)
        {
			return { property_state::INCORRECT, 
                property_state::SM_1, property_state::SM_2, property_state::SM_3, property_state::SM_4, property_state::SM_5,
                property_state::MINOR_INCORRECT, property_state::CORRECT };
		}
        else 
        {
            return {property_state::INCORRECT, property_state::MINOR_INCORRECT, property_state::CORRECT};
        }
    };

    // Get possible states for each property
    std::vector<property_state> results_states = get_possible_states(results);
    std::vector<property_state> semantics_states = get_possible_states(semantics);
    std::vector<property_state> syntax_states = get_possible_states(syntax);

    auto get_states = [&](property_order order, int index) -> std::vector<property_state>&
        {
            if (order == property_order::RE_SM_SY)
            {
                if (index == 0) return results_states;
                if (index == 1) return semantics_states;
                return syntax_states;
            }
            else if (order == property_order::SM_SY_RE)
            {
                if (index == 0) return semantics_states;
                if (index == 1) return syntax_states;
                return results_states;
            }
            else if (order == property_order::SY_SM_RE)
            {
                if (index == 0) return syntax_states;
                if (index == 1) return semantics_states;
                return results_states;
            }
            else if (order == property_order::SY_RE_SM)
            {
                if (index == 0) return syntax_states;
                if (index == 1) return results_states;
                return semantics_states;
            }
            else if (order == property_order::SM_RE_SY)
            {
                if (index == 0) return semantics_states;
                if (index == 1) return results_states;
                return syntax_states;
            }
            else
            { // order == property_order::RE_SY_SM
                if (index == 0) return results_states;
                if (index == 1) return syntax_states;
                return semantics_states;
            }
        };


    // Generate all combinations based on the order and levels
    for (auto state1 : get_states(order, 0)) {
        for (auto state2 : get_states(order, 1)) {
            for (auto state3 : get_states(order, 2)) 
            {
                properties props;
                if (order == property_order::RE_SM_SY) {
                    props = {state1, state2, state3};
                } else if (order == property_order::SM_SY_RE) {
                    props = {state3, state1, state2};
                } else if (order == property_order::SY_SM_RE) {
                    props = {state3, state2, state1};
                } else if (order == property_order::SY_RE_SM) {
					props = {state2, state3, state1};
				} else if (order == property_order::SM_RE_SY) {
                    props = { state2, state1, state3 };
                } else { // order == property_order::RE_SY_SM
                    props = { state1, state3, state2 };
                }
                

                //skip the levels that are not possible
                // // Helper function to check if a state is SM_1 to SM_6
                auto is_SM_X = [](property_state state) -> bool
                    {
                        switch (state)
                        {
                            case property_state::SM_1:
                            case property_state::SM_2:
                            case property_state::SM_3:
                            case property_state::SM_4:
                            case property_state::SM_5:
                                return true;
                            default:
                                return false;
                        }
                    };
                // im 8-mag: results = incorrect, semantics = correct, in 27-mag: results = incorrect, semantics = minor incorrect
                if (props.results == ps::INCORRECT && props.semantics == ps::MINOR_INCORRECT )
                    continue;
                // in 27-mag: results = minor incorrect, semantics = incorrect
                if (props.results == ps::MINOR_INCORRECT && props.semantics == ps::INCORRECT && props.syntax == ps::INCORRECT && 
                    results == pl::THREE_LEVELS && semantics == pl::THREE_LEVELS && syntax == pl::THREE_LEVELS)
                    continue;
                // in 27-mag: results = correct, semantics = incorrect
                if (props.results == ps::CORRECT && props.semantics == ps::INCORRECT 
                    && semantics == pl::THREE_LEVELS)
                    continue;
                // in 27-mag: semantics = minor incorrect, syntax = incorrect
                if (props.semantics == ps::MINOR_INCORRECT && props.syntax == ps::INCORRECT && semantics == pl::THREE_LEVELS && syntax == pl::THREE_LEVELS)
                    continue;
                // in 27-mag: results = minor incorrect, semantics = minor incorrect
                if (props.results == ps::MINOR_INCORRECT && props.semantics == ps::MINOR_INCORRECT && results == pl::THREE_LEVELS && semantics == pl::THREE_LEVELS)
                    continue;
                // in 27-mag: semantics = correct, syntax = incorrect
                if (props.semantics == ps::CORRECT && props.syntax == ps::INCORRECT && syntax == pl::THREE_LEVELS)
                    continue;
                //in 27-mag: results = incorrect, semantics = correct
                if (props.results == ps::INCORRECT && props.semantics == ps::CORRECT)
                    continue;
                //in 27-mag: results = minor incorrect, semantics = correct
                if (props.results == ps::MINOR_INCORRECT && props.semantics == ps::CORRECT && results == pl::THREE_LEVELS && semantics == pl::THREE_LEVELS)
                    continue;
                //results = correct , semantics = SM_X  
				if (props.results == ps::CORRECT && is_SM_X(props.semantics))
					continue;
				if (props.results == ps::CORRECT && props.semantics == ps::INCORRECT 
                    && semantics == pl::SEMATICS_LEVELS_6)
					continue;
				//syntax = incorrect, semantics = SM_X
                if (props.syntax == ps::INCORRECT && is_SM_X(props.semantics))
                    continue;

                matrix.push_back({props});
            }
        }
    }
    
    return matrix;
}
double Grader::correctness_level_normalized_value(const vector<properties> &matrix, int correctness_level)
{
    // The minimum correctness level is the level of the first element in the matrix
    int min_level = 1;

    // The maximum correctness level is the level of the last element in the matrix
    int max_level = matrix.size();

    // Use the formula to calculate the normalized value
    double normalized_value = static_cast<double>(correctness_level - min_level) / (max_level - min_level);

    return normalized_value;
}
std::pair<int, double> Grader::calculate_correctness_level(property_level sn_level, property_level sm_level, property_level rs_level, property_order order, property_state results, property_state semantics, property_state syntax)
{
    //initialize the correctness matrix
    vector<properties> correctness_matrix = get_correctness_matrix(rs_level, sm_level, sn_level, order);
    int correctness_level = 1;

    //calculate the correctness level for the rest of the cases
    for (auto& correctness_level_outcomes : correctness_matrix)
    {   
        if (correctness_level_outcomes.results == results &&
            correctness_level_outcomes.semantics == semantics &&
            correctness_level_outcomes.syntax == syntax)
        {
            break;
        }
        correctness_level++;
    }

    //If the correctness level is bigger than the size of the matrix, then something is wrong
    //print a message with property states
    if (correctness_level > correctness_matrix.size())
    {
        std::cerr << "Error: The correctness level is bigger than the size of the correctness matrix." << std::endl;
        std::cerr << "Results: " << property_state_to_string(results) << std::endl;
        std::cerr << "Semantics: " << property_state_to_string(semantics) << std::endl;
        std::cerr << "Syntax: " << property_state_to_string(syntax) << std::endl;
        //return std::make_pair(-1, -1);
    }

    //calculate the normalized value
    double normalized_value = correctness_level_normalized_value(correctness_matrix, correctness_level);
    return std::make_pair(correctness_level, normalized_value);
}

void Grader::display_correctness_matrix(std::vector<properties> matrix)
{
   std::cout << "Correctness matrix:" << std::endl;
    std::string divider = "+-------+--------+------+";
    std::string header =  "|results|semantic|syntax|";

    std::cout << divider << std::endl;
    std::cout << header << std::endl;
    std::cout << divider << std::endl;

    for (auto& correctness_level_outcomes : matrix) 
    {
        std::cout << "|"
            << std::setw(7) << std::setfill(' ') << property_state_to_string(correctness_level_outcomes.results) << "|"
            << std::setw(8) << std::setfill(' ') << property_state_to_string(correctness_level_outcomes.semantics) << "|"
            << std::setw(6) << std::setfill(' ') << property_state_to_string(correctness_level_outcomes.syntax) << "|"
            << std::endl;
        std::cout << divider << std::endl;
    }
   
}

void Grader::format_output(string original_file)
{
    std::string original_csv_path = original_file;
    std::string graded_csv_path = "quiz_graded.csv";

    // Construct the command
    string command = "python3 ../quizzes/output_formater.py \"" + original_file + "\" \"" + graded_csv_path + "\"";

    // Execute the Python script
    int result = system(command.c_str());

    if (result != 0)
    {
        // Handle the error
        std::cerr << "Error executing the Python script" << std::endl;
    }
}

std::string Grader::property_state_to_string(property_state state)
{
    switch(state) {
        case property_state::INVALID: return "invalid";
        case property_state::INCORRECT: return "Incorrect";
		case property_state::SM_1: return "SM_1";
		case property_state::SM_2: return "SM_2";
		case property_state::SM_3: return "SM_3";
		case property_state::SM_4: return "SM_4";
		case property_state::SM_5: return "SM_5";
        case property_state::MINOR_INCORRECT: return "Minor incorrect";
        case property_state::CORRECT: return "correct";
        default: return "unknown";
    }
}
