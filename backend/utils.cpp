#include "utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/tokenizer.hpp>
#include <regex>
#include "sql_keywords.h"
#include <pg_query.h>
#include <boost/algorithm/string.hpp>
#include <unordered_set>
#include <regex>
#include <regex>
#include <set>

// Function to compare two 2D vectors (SQL query results) and calculate their difference
Utils::comparison_result Utils::compare_vectors(std::vector<std::vector<std::string>> query_results, std::vector<std::vector<std::string>> correct_results, bool check_order)
{
	/**
	 * There are different types of array sizes differences that we need to consider.
	 *         A               B
	 *    _____________  _____________
	 * 1. | a | b | c |  | a | b | c |    --> same size
	 *    | d | e | f |  | d | e | f |
	 *    | g | h | i |  | g | h | i |
	 *    -------------  -------------
	 *
	 *         A               B
	 * 2. _____________  _________
	 *    | a | b | c |  | a | b |        --> A has bigger rows and columns than B
	 *    | d | e | f |  | c | d |
	 *    | g | h | i |  ---------
	 *    -------------
	 *
	 *        A               B
	 * 3. _________      _____________
	 *    | a | b |      | a | b | c |    --> A has bigger rows but smaller columns than B
	 *    | d | e |      | d | e | f |
	 *    | g | h |      | g | h | i |
	 *    | j | k |      -------------
	 *    ---------
	 *
	 *      A               B
	 * 4. _____________  _________
	 *    | a | b | c |  | a | b |    --> A has smaller rows and bigger columns than B
	 *    | d | e | f |  | c | d |
	 *    -------------  | e | f |
	 *                   ---------
	 * 5,6,7,8 are the same as 1,2,3,4 but with A and B swapped.
	 */

	int query_rows = query_results.size();
	int correct_rows = correct_results.size();

	comparison_result result{false, false, 0};

	// Handle cases where one or both vectors are empty
	if (query_rows == 0 || correct_rows == 0)
	{
		result.is_equal = false;  //(query_rows == correct_rows);
		result.is_subset = false; //(query_rows == 0);
		result.difference = std::abs(query_rows - correct_rows);
		return result;
	}
	// the columns might be in a different order. We need to sort the columns in ascending order.
	std::vector<std::vector<std::string>> transposed_rows_query = query_results, transposed_rows_correct = correct_results;
	// we arrange the sort the columns in an ascending order. If two vectors had rows in different order then the resulting rows we will be same if they have the same data.
	//  transpose and order the columns
	auto transpose_and_sort = [this](const std::vector<std::vector<std::string>> &vec) -> std::vector<std::vector<std::string>>
	{
		auto transposed = transpose(vec);
		std::sort(transposed.begin(), transposed.end());
		return transpose(transposed);
	};
	transposed_rows_query = transpose_and_sort(query_results);
	transposed_rows_correct = transpose_and_sort(correct_results);

	// Before transposing and sorting logic in `compare_vectors`
	if (transposed_rows_correct.size() <= transposed_rows_query.size() &&
		transposed_rows_correct[0].size() <= transposed_rows_query[0].size())
	{
		if (is_1D_subset(transposed_rows_correct, transposed_rows_query))
		{
			result.is_subset = true;
			result.is_equal = (query_rows == correct_rows);
			result.difference = query_rows - correct_rows;
			// return result;
		}
	}
	// handle cases where order is not important
	if (!check_order)
	{
		// sort the rows in acsneding order based on the first element in the column
		std::sort(transposed_rows_query.begin(), transposed_rows_query.end());
		std::sort(transposed_rows_correct.begin(), transposed_rows_correct.end());
	}

	// lets check for containment
	// we only check if the correct query is smaller or equal to the query. This is a condition for complete containment.
	// this is to avoid to avoid the case where an index does not exist in the query.
	if (transposed_rows_correct.size() <= transposed_rows_query.size() &&
		transposed_rows_correct[0].size() <= transposed_rows_query[0].size())
	{
		// result.is_subset = check_small_array_in_big_array(transposed_rows_correct, transposed_rows_query);
		// result.is_subset = is_1D_subset_ordered(transposed_rows_query, transposed_rows_correct);
		result.is_subset = is_sequence_contained_ordered(transposed_rows_correct, transposed_rows_query);
	}
	result.difference = get_number_of_vector_elements_differences(transposed_rows_query, transposed_rows_correct);
	// if the correct results are contained inside the query results we get only the difference in sizes
	result.is_equal = result.is_subset && result.difference == 0;

	return result;
}
Utils::comparison_result Utils::compare_output(std::vector<std::vector<std::string>> query_results, std::vector<std::vector<std::string>> correct_results, bool check_order)
{
	return compare_vectors(query_results, correct_results, check_order);
}
bool Utils::is_1D_subset(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger)
{
	for (const auto &s_elem : smaller)
	{
		if (std::find(bigger.begin(), bigger.end(), s_elem) == bigger.end())
		{
			return false;
		}
	}
	return true;
}
bool Utils::is_1D_subset_ordered(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger)
{
	if (smaller.empty() || bigger.empty())
	{
		return false;
	}

	auto smaller_outer_iter = smaller.begin();
	auto smaller_inner_iter = smaller_outer_iter->begin();

	for (const auto &b_elem : bigger)
	{
		// Match sub-sequence
		while (smaller_inner_iter != smaller_outer_iter->end() && *smaller_inner_iter == *b_elem.begin())
		{
			++smaller_inner_iter;
			++b_elem.begin();
		}

		// If sub-sequence in the smaller vector is consumed, move to the next sub-vector
		if (smaller_inner_iter == smaller_outer_iter->end())
		{
			++smaller_outer_iter;
			if (smaller_outer_iter == smaller.end())
			{
				return true; // All of smaller vectors are found in bigger vectors in order
			}
			smaller_inner_iter = smaller_outer_iter->begin();
		}
	}

	return false; // Not all smaller vectors were found in bigger vectors in order
}

bool Utils::is_sequence_contained_ordered(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger)
{
	auto smaller_iter = smaller.begin();

	for (const auto &b_elem : bigger)
	{
		if (check_small_array_in_big_array({*smaller_iter}, {b_elem}))
		{
			++smaller_iter;
			if (smaller_iter == smaller.end())
			{
				return true; // All of smaller sequences are found in bigger sequences in order
			}
		}
	}
	return false;
}

std::vector<std::vector<std::string>> Utils::transpose(const std::vector<std::vector<std::string>> &original)
{
	int rows = original.size();
	int cols = original[0].size();

	std::vector<std::vector<std::string>> transposed(cols, std::vector<std::string>(rows));

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			transposed[j][i] = original[i][j];
		}
	}

	return transposed;
}
/**
 * This function check if there is a mismatch between the two vectors.
 */
bool Utils::is_matched(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger, int row_offset, int col_offset)
{
	for (int i = 0; i < smaller.size(); ++i)
	{
		for (int j = 0; j < smaller[i].size(); ++j)
		{
			// there is a mismatch
			if (smaller[i][j] != bigger[i + row_offset][j + col_offset])
			{
				return false;
			}
		}
	}
	// if the function reaches this point then there is no mismatch
	return true;
}

bool Utils::check_small_array_in_big_array(const std::vector<std::vector<std::string>> &smaller, const std::vector<std::vector<std::string>> &bigger)
{

	// check for 2D vectors
	for (int row = 0; row <= bigger.size() - smaller.size(); ++row)
	{
		for (int col = 0; col <= bigger[0].size() - smaller[0].size(); ++col)
		{
			if (is_matched(smaller, bigger, row, col))
			{
				return true;
			}
		}
	}
	return false;
}
int Utils::get_number_of_vector_elements_differences(const std::vector<std::vector<std::string>> &vec_1, const std::vector<std::vector<std::string>> &vec_2)
{
	int vec_1_rows = vec_1.size();
	int vec_1_cols = vec_1[0].size();
	int vec_2_rows = vec_2.size();
	int vec_2_cols = vec_2[0].size();

	int differences = 0;

	// Loop through all possible translations.
	for (int row_offset = -vec_1_rows + 1; row_offset < vec_2_rows; ++row_offset)
	{
		for (int col_offset = -vec_1_cols + 1; col_offset < vec_2_cols; ++col_offset)
		{

			int min_differences = 0;
			for (int i = 0; i < vec_1_rows; ++i)
			{
				for (int j = 0; j < vec_1_cols; ++j)
				{
					int translated_i = i + row_offset;
					int translated_j = j + col_offset;

					if (translated_i >= 0 && translated_i < vec_2_rows && translated_j >= 0 && translated_j < vec_2_cols)
					{
						// check if the two elements are equal
						if (vec_1[i][j] == vec_2[translated_i][translated_j])
						{
							min_differences++;
						}
					}
				}
			}
			differences = std::max(differences, min_differences);
		}
	}
	// get the total number of elements in the two vectors
	int total_elements = vec_1_rows * vec_1_cols + vec_2_rows * vec_2_cols;
	return total_elements - (differences * 2);
}

std::pair<std::string, bool> Utils::replace_double_quotes_with_single_quotes(const std::string &query)
{
	std::string corrected_query;
	bool in_quotes = false;
	bool query_altered = false;
	bool found_equal_sign = false;

	for (size_t i = 0; i < query.size(); ++i)
	{
		const char c = query[i];
		if (c == '=')
		{
			found_equal_sign = true;
			corrected_query.push_back(c);
		}
		else if (c == ' ' && found_equal_sign)
		{
			corrected_query.push_back(c);
		}
		else if (c == '\"' && found_equal_sign)
		{
			in_quotes = true;
			query_altered = true;
			found_equal_sign = false;
			corrected_query.push_back('\'');
		}
		else if (c == '\"' && in_quotes)
		{
			in_quotes = false;
			corrected_query.push_back('\'');
		}
		else
		{
			corrected_query.push_back(c);
		}
	}

	return std::make_pair(corrected_query, query_altered);
}
void Utils::draw_line_separator()
{
	for (int i = 1; i < 20; i++)
		std::cout << '-';
	std::cout << std::endl;
}
// Function to print a vector of strings
void Utils::print_vector(const std::vector<std::string> &v)
{
	std::cout << "[";
	for (size_t i = 0; i < v.size(); ++i)
	{
		std::cout << v[i];
		if (i != v.size() - 1)
		{
			std::cout << ", ";
		}
	}
	std::cout << "]";
}

// Function to print a vector of vector of strings
void Utils::print_2d_vector(const std::vector<std::vector<std::string>> &v)
{
	std::cout << "[\n";
	for (size_t i = 0; i < v.size(); ++i)
	{
		std::cout << "  ";
		print_vector(v[i]);
		if (i != v.size() - 1)
		{
			std::cout << ",";
		}
		std::cout << "\n";
	}
	std::cout << "]";
}

using t_size_type = std::vector<std::string>::size_type;
using edit_path = std::vector<std::tuple<std::string, std::string, std::string>>;

std::pair<int, edit_path> Utils::general_edit_distance_words(const std::vector<std::string> &source, const std::vector<std::string> &target,
															 int insert_cost, int delete_cost, int replace_cost)
{
	t_size_type m = source.size();
	t_size_type n = target.size();

	std::vector<std::vector<t_size_type>> dp(m + 1, std::vector<t_size_type>(n + 1));
	edit_path path;

	for (t_size_type i = 0; i <= m; ++i)
	{
		for (t_size_type j = 0; j <= n; ++j)
		{
			if (i == 0)
				dp[i][j] = j * insert_cost;
			else if (j == 0)
				dp[i][j] = i * delete_cost;
			else if (source[i - 1] == target[j - 1])
				dp[i][j] = dp[i - 1][j - 1];
			else
				dp[i][j] = std::min({dp[i - 1][j] + delete_cost,
									 dp[i][j - 1] + insert_cost,
									 dp[i - 1][j - 1] + replace_cost});
		}
	}

	t_size_type i = m, j = n;
	while (i > 0 || j > 0)
	{
		if (i > 0 && dp[i][j] == dp[i - 1][j] + delete_cost)
		{
			path.push_back({"delete", source[i - 1], ""});
			--i;
		}
		else if (j > 0 && dp[i][j] == dp[i][j - 1] + insert_cost)
		{
			path.push_back({"insert", "", target[j - 1]});
			--j;
		}
		else
		{
			if (source[i - 1] == target[j - 1])
				path.push_back({"equal", source[i - 1], target[j - 1]});
			else
				path.push_back({"replace", source[i - 1], target[j - 1]});
			--i;
			--j;
		}
	}

	// backtracking the path has pushed the edits in reverse order therefore we take care of that
	std::reverse(path.begin(), path.end());
	return {dp[m][n], path};
}

std::string::size_type Utils::general_edit_distance(const std::string &source, const std::string &target, std::string::size_type insert_cost, std::string::size_type delete_cost, std::string::size_type replace_cost)
{
	if (source.size() > target.size())
		return general_edit_distance(target, source, insert_cost, delete_cost, replace_cost);

	using t_size_type = std::string::size_type;
	const t_size_type min_size = source.size(), max_size = target.size();
	std::vector<t_size_type> lev_dist(min_size + 1);

	lev_dist[0] = 0;
	for (t_size_type i = 1; i <= min_size; ++i)
		lev_dist[i] = lev_dist[i - 1] + delete_cost;

	for (t_size_type j = 1; j <= max_size; ++j)
	{
		t_size_type previous_diagonal = lev_dist[0], previous_diagonal_save;
		lev_dist[0] += insert_cost;

		for (t_size_type i = 1; i <= min_size; ++i)
		{
			previous_diagonal_save = lev_dist[i];
			if (source[i - 1] == target[j - 1])
				lev_dist[i] = previous_diagonal;
			else
				lev_dist[i] = std::min(std::min(lev_dist[i - 1] + delete_cost, lev_dist[i] + insert_cost), previous_diagonal + replace_cost);
			previous_diagonal = previous_diagonal_save;
		}
	}
	// t_size_type alpha = alpha_measure(insert_cost, delete_cost);
	// float normalized_ld = normalized_levenshtein_distance(source, target, alpha, lev_dist[min_size]);
	return lev_dist[min_size];
}
// a general edit distance function that uses wide multibyte characters
std::wstring::size_type Utils::general_edit_distance(const std::wstring &source, const std::wstring &target,
													 std::wstring::size_type insert_cost, std::wstring::size_type delete_cost, std::wstring::size_type replace_cost)
{
	if (source.size() > target.size())
		return general_edit_distance(target, source, insert_cost, delete_cost, replace_cost);

	using t_size_type = std::wstring::size_type;
	const t_size_type min_size = source.size(), max_size = target.size();
	std::vector<t_size_type> lev_dist(min_size + 1);

	lev_dist[0] = 0;
	for (t_size_type i = 1; i <= min_size; ++i)
		lev_dist[i] = lev_dist[i - 1] + delete_cost;

	for (t_size_type j = 1; j <= max_size; ++j)
	{
		t_size_type previous_diagonal = lev_dist[0], previous_diagonal_save;
		lev_dist[0] += insert_cost;

		for (t_size_type i = 1; i <= min_size; ++i)
		{
			previous_diagonal_save = lev_dist[i];
			if (source[i - 1] == target[j - 1])
				lev_dist[i] = previous_diagonal;
			else
				lev_dist[i] = std::min(std::min(lev_dist[i - 1] + delete_cost, lev_dist[i] + insert_cost), previous_diagonal + replace_cost);
			previous_diagonal = previous_diagonal_save;
		}
	}
	return lev_dist[min_size];
}
std::string::size_type Utils::alpha_measure(std::string::size_type insert_cost, std::string::size_type delete_cost)
{
	return std::max(insert_cost, delete_cost);
}

/*std::string::size_type Utils::generalized_levenshtein_similarity(const std::string &source, const std::string &target, std::string::size_type alpha, std::string::size_type g_edit_distance)
{
	using t_size_type = std::string::size_type;
	t_size_type source_size = source.size();
	t_size_type target_size = target.size();

	return ((alpha * (source_size + target_size)) - g_edit_distance) / 2;
}*/

float Utils::normalized_levenshtein_distance(const std::string &source, const std::string &target,
											 std::string::size_type insert_cost,
											 std::string::size_type delete_cost,
											 std::string::size_type replace_cost)
{
	using t_size_type = std::string::size_type;
	std::string::size_type g_edit_distance = general_edit_distance(source, target, insert_cost, delete_cost, replace_cost);
	t_size_type alpha = alpha_measure(insert_cost, delete_cost);
	// here we cast 2 to float so that the other integers are changed to floating type as well
	// during arithmetic
	return (1.0f * 2 * g_edit_distance) / (alpha * (source.size() + target.size()) + g_edit_distance);
}
/*std::map<std::string, std::string> Utils::read_config(const std::string& filename)
{
	std::ifstream config_file(filename);
	std::map<std::string, std::string> config_map;

	if (config_file.is_open())
	{
		std::string line;
		while (std::getline(config_file, line))
		{
			std::istringstream line_stream(line);
			std::string key, value;
			std::getline(line_stream, key, '=');
			std::getline(line_stream, value);
			config_map[key] = value;
		}
		config_file.close();
	}
	else
	{
		std::cerr << "Error: Unable to open config file" << std::endl;
		exit(1);
	}

	return config_map;
}*/
std::vector<std::vector<std::string>> Utils::read_csv(const std::string &filename, bool standard_csv)
{
	if (standard_csv)
	{
		return read_other_csv(filename);
	}
	else
	{
		return read_sql_csv(filename);
	}
}
std::vector<std::vector<std::string>> Utils::read_other_csv(const std::string &filename)
{
	using namespace boost;
	typedef tokenizer<escaped_list_separator<char>> Tokenizer;
	std::vector<std::vector<std::string>> data;
	std::ifstream file(filename);

	if (file.is_open())
	{
		std::string line;
		bool first_line = true;
		while (std::getline(file, line))
		{
			if (first_line)
			{
				// Check for and remove UTF-8 BOM if present
				if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
					static_cast<unsigned char>(line[1]) == 0xBB &&
					static_cast<unsigned char>(line[2]) == 0xBF)
				{
					line.erase(0, 3);
				}
				first_line = false;
			}

			Tokenizer tok(line);
			std::vector<std::string> row(tok.begin(), tok.end());
			// Remove newline character from the last element of the row if present
			// This will prevent the character from displaying in the output csv file.
			if (!row.empty())
			{
				std::string &last_element = row.back();
				last_element.erase(std::remove(last_element.begin(), last_element.end(), '\n'), last_element.end());
				last_element.erase(std::remove(last_element.begin(), last_element.end(), '\r'), last_element.end());
			}
			data.push_back(row);
		}
		file.close();
	}
	else
	{
		std::cerr << "Error: Unable to open CSV file: " << filename << std::endl;
		exit(1);
	}

	return data;
}
std::vector<std::vector<std::string>> Utils::read_sql_csv(const std::string &filename)
{
	std::vector<std::vector<std::string>> data;
	std::ifstream file(filename);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			std::vector<std::string> row;
			std::regex regex(",(?=(?:[^\"]*\"[^\"]*\")*(?![^\"]*\"))");
			std::sregex_token_iterator it(line.begin(), line.end(), regex, -1);
			std::sregex_token_iterator reg_end;
			for (; it != reg_end; ++it)
			{
				std::string element = it->str();
				// Remove leading and trailing whitespaces
				element.erase(0, element.find_first_not_of(' ')); // prefixing spaces
				// this only removes trailing spaces and not other types of whitespace characters like tabs or newlines.
				element.erase(element.find_last_not_of(' ') + 1); // suffixing spaces

				// Check for and remove UTF-8 BOM if present
				if (element.size() >= 3 && static_cast<unsigned char>(element[0]) == 0xEF &&
					static_cast<unsigned char>(element[1]) == 0xBB &&
					static_cast<unsigned char>(element[2]) == 0xBF)
				{
					element.erase(0, 3);
				}
				// Remove trailing whitespaces
				// removes all types of trailing whitespace characters, not just spaces.
				auto end = std::find_if(element.rbegin(), element.rend(), [](unsigned char ch)
										{ return !std::isspace(ch); })
							   .base();
				element.erase(end, element.end());

				// If the string begins and ends with quotes, remove them
				if (element.size() > 1 && element.front() == '\"' && element.back() == '\"')
				{
					element.erase(0, 1);			   // Remove first character
					element.erase(element.size() - 1); // Remove last character

					// Replace "" with "
					size_t position = element.find("\"\"");
					while (position != std::string::npos)
					{
						element.replace(position, 2, "\"");
						position = element.find("\"\"", position + 1);
					}
				}
				row.push_back(element);
			}
			data.push_back(row);
		}
		file.close();
	}
	else
	{
		std::cerr << "Error: Unable to open CSV file: " << filename << std::endl;
		exit(1);
	}

	return data;
}

void Utils::preprocess_query(std::string &query)
{
	// remove spaces from the query
	std::string::iterator new_end = std::unique(query.begin(), query.end(), both_are_spaces);
	query.erase(new_end, query.end());
	// change query to lower case
	std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c)
				   { return std::tolower(c); });
	// replace the special character ´ with `
	// THis allows for correctly calculating the edit distance between queries. It is just a placeholder since the query did not use the correct character.
	// WE do this because character ´ is not multi-byte character and therefore its edit distance to ' would be 2.
	// In our processing we want to treat it as a single character.
	// boost::replace_all(query, "´", "`");
}

bool Utils::both_are_spaces(char lhs, char rhs)
{
	return (lhs == rhs) && (lhs == ' ');
}

std::tuple<bool, std::string> Utils::fix_query_syntax_using_keywords(std::string query)
{
	std::string query_copy = query;
	/*std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c)
		{
			return std::toupper(c);
		});*/
	// the query passed cannot be parsed by pg_query
	bool changed = false;

	std::vector<std::string> tokens;
	boost::split(tokens, query, boost::is_any_of(" \t\n;,"), boost::token_compress_on);
	for (int c = 0; c < tokens.size(); c++)
	{
		auto token = tokens.at(c);
		// transform token to uppercase
		std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c)
					   { return std::toupper(c); });
		if (sql_keywords_2016.find(token) == sql_keywords_2016.end())
		{
			// let us iterate through the valid keywords and find the one that is closest to the token
			int min_edit_distance = 2;
			for (const auto &keyword : sql_keywords_2016)
			{
				int edit_distance = general_edit_distance(token, keyword);
				double norm_edit_distance = normalized_levenshtein_distance(token, keyword);
				if (norm_edit_distance < 0.5)
					// std::cout << "keyword " << keyword << " token " << token << "dist " << edit_distance << " norm " << norm_edit_distance << std::endl;
					if (edit_distance <= min_edit_distance && norm_edit_distance <= 0.27)
					{
						query = std::regex_replace(query, std::regex(token, std::regex_constants::icase), keyword);
						changed = true;
					}
			}
		}
	}
	// change the origial string to lowercase
	/*std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c)
		{
			return std::tolower(c);
		});*/
	// check if the query is parseable.
	PgQueryParseResult result = pg_query_parse(query.c_str());
	if (result.error)
	{
		return {false, ""};
		exit(1);
	}
	boost::trim(query);
	return {changed, query};
}

std::tuple<bool, std::string, int> Utils::fix_query_syntax_using_another_query(const Admin admin, std::string query, std::string query_model, std::string query_untouched)
{
	int minimum_words = 20;
	int minimum_chars = admin.get_syntax_minor_incorrect_ted();

	// splits the sentences into words using boost library
	std::vector<std::string> sentence1, sentence2;
	boost::split(sentence1, query, boost::is_any_of(" \t\n;,"));
	// delete empty words from sentence1
	sentence1.erase(std::remove_if(sentence1.begin(), sentence1.end(), [](const std::string &s)
								   { return s.empty(); }),
					sentence1.end());
	boost::split(sentence2, query_model, boost::is_any_of(" \t\n;,"));
	// delete empty words from sentence2
	sentence2.erase(std::remove_if(sentence2.begin(), sentence2.end(), [](const std::string &s)
								   { return s.empty(); }),
					sentence2.end());

	// carry out words level edit distance
	auto [distance, edit_path] = general_edit_distance_words(sentence1, sentence2);

	// copy the sentence1 and sentence2 to a new vector
	std::string query_copy = query;
	std::string model_copy = query_model;

	// let's check individual operations to defferentiate betwween syntax and semantic errors
	int total_char_operations = 0;
	for (const auto &[operation, src_word, tgt_word] : edit_path)
	{
		if (operation == "equal")
		{
			// do nothing
		}
		else if (operation == "delete")
		{
			// for now we will assume all delete operations are syntactic.
			// we just count the number of characters to be deleted.
			total_char_operations += src_word.length();
			// delete the characters in src_word from query_copy
		}
		else if (operation == "insert")
		{
			// check if the insert word is a keyword
			if (check_if_word_is_keyword(tgt_word))
			{
				// the word to be inserted is a keyword, therefore we just count the number of characters in the word in model_copy
				// insert the word in sentence1_copy
				total_char_operations += tgt_word.length();
			}
			else
			{
				// ignore the insert
				// its is column, table or database name
				// we therefore remove the word from the model_copy, wwe need to take care of the case where there is a comma after or before the word.
				// first check if the word has a comma after it
				std::string word_with_comma_after = tgt_word + ",";
				std::string word_with_comma_before = "," + tgt_word;
				if (model_copy.find(word_with_comma_after) != std::string::npos)
				{
					// remove the word with comma after it
					boost::replace_all(model_copy, word_with_comma_after, "");
				}
				else if (model_copy.find(word_with_comma_before) != std::string::npos)
				{
					// remove the word with comma before it
					boost::replace_all(model_copy, word_with_comma_before, "");
				}
				else
				{
					// remove the word
					boost::replace_all(model_copy, tgt_word, "");
				}
			}
		}
		else if (operation == "replace")
		{
			// check if the tgt_word is a keyword
			if (check_if_word_is_keyword(tgt_word))
			{
				// this is syntax.
				// check how many char edit operations are required to change the word
				int edit_distance = general_edit_distance(src_word, tgt_word);
				total_char_operations += edit_distance;
			}
			else
			{
				// Lambda expression to extract unquoted words into a set
				auto extract_unquoted_words_set = [](const std::string &str)
				{
					std::set<std::string> wordSet;
					std::regex word_regex(R"(\b(['"`´]?\w+['"`´]?)\b)");
					auto words_begin = std::sregex_iterator(str.begin(), str.end(), word_regex);
					auto words_end = std::sregex_iterator();

					for (std::sregex_iterator i = words_begin; i != words_end; ++i)
					{
						std::smatch match = *i;
						wordSet.insert(match.str());
					}

					return wordSet;
				};
				std::set<std::string> src_core = extract_unquoted_words_set(src_word);
				std::set<std::string> tgt_core = extract_unquoted_words_set(tgt_word);

				// check if the two words are the same after removing quotes
				if (src_core == tgt_core)
				{
					// ignore the change
					// since a word in the student query would be replaced with the correct word from the model query, we exchange this words in the model_copy
					// this is because there is no need to change a word that is not syntax related, we wil do this in semantic analysis.
					// replace the word in model
					// boost::replace_all(model_copy, tgt_word, src_word);
				}
				else
				{
					// ignore the change
					// since a word in the student query would be replaced with the correct word from the model query, we exchange this words in the model_copy
					// this is because there is no need to change a word that is not syntax related, we wil do this in semantic analysis.
					// replace the word in model
					boost::replace_all(model_copy, tgt_word, src_word);
				}
			}
		}
	}

	if (total_char_operations > minimum_chars)
	{
		return {false, "", distance};
	}
	// check if the query is parseable.
	PgQueryParseResult result = pg_query_parse(model_copy.c_str());
	if (result.error)
	{
		return {false, "", distance};
	}

	return {true, model_copy, distance};
}
bool Utils::check_if_word_is_keyword(std::string word)
{
	// transform word to uppercase
	std::transform(word.begin(), word.end(), word.begin(), ::toupper);

	// let us iterate through the valid keywords and find the one that matches it completely
	int min_edit_distance = 2;
	for (const auto &keyword : sql_keywords_2016)
	{
		int edit_distance = general_edit_distance(word, keyword);
		double norm_edit_distance = normalized_levenshtein_distance(word, keyword);
		if (edit_distance == 0 && norm_edit_distance == 0)
			return true;
	}
	return false;
}
