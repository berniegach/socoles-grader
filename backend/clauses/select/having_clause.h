#ifndef HAVING_CLAUSE_H
#define HAVING_CLAUSE_H

#include <string>
#include "where_clause.h"

class Having_clause
{
public:
    /**
     * This function processes the HAVING clause and returns the clause information
     * @param node: This is a pointer to the root node of the HAVING clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A pair containing the clause information as a single string and the Where_clause::where_clause_info structure
     */
    static std::pair<std::string, Where_clause::where_clause_info> process(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function extracts the information about the HAVING clause
     * THis is a helper function for the having_clause_process function
     * @param node: This is a pointer to the root node of the HAVING clause
     * @param from_info: This is a from_clause_info structure containing information about the FROM clause. It is used to resolve column references
     * @param select_info: This is a select_clause_info structure containing information about the SELECT clause. It is used to resolve column references
     * @return: A Where_clause::where_clause_info structure containing the information about the HAVING clause
     */
    static Where_clause::where_clause_info get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const From_clause::from_clause_info &from_info, const Select_clause::select_clause_info &select_info);
    /**
     * This function compares two HAVING clauses and returns the comparison information
     * @param reference: This is a Where_clause::where_clause_info structure containing the reference HAVING clause information
     * @param other: This is a Where_clause::where_clause_info structure containing the student HAVING clause information
     * @return: A pair containing the comparison result as an integer(-1 for both missing, 0 for unequal and 1 for equal) and a string message
     * containing the comparison information as a single string
     */
    static std::pair<int, std::string> compare(const Where_clause::where_clause_info &reference, const Where_clause::where_clause_info &other);
    /**
     * This function prints the HAVING clause information
     * @param having: This is a Where_clause::where_clause_info structure containing the HAVING clause information
     */
    static void print(const Where_clause::where_clause_info &having);
};

#endif // HAVING_CLAUSE_H