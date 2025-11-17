#include "limit_clause.h"
#include "where_clause.h"

static std::string extract_expr_text(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    // Reuse Where_clause extractor with dummy contexts to stringify expressions
    From_clause::from_clause_info dummy_from;
    Select_clause::select_clause_info dummy_select;
    if (!node)
        return "";
    return Where_clause::extract_expression(node, dummy_from, dummy_select);
}

Limit_clause::limit_clause_info Limit_clause::get_info(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    limit_clause_info info;
    if (!node)
    {
        return info;
    }

    if (node->key == "limitCount")
    {
        if (!node->children.empty())
        {
            info.has_limit = true;
            info.limit_count = extract_expr_text(node->children.front());
        }
        return info;
    }
    if (node->key == "limitOffset")
    {
        if (!node->children.empty())
        {
            info.has_offset = true;
            info.limit_offset = extract_expr_text(node->children.front());
        }
        return info;
    }

    // Recurse through children and merge findings (first non-empty wins)
    for (const auto &child : node->children)
    {
        auto child_info = get_info(child);
        if (child_info.has_limit)
        {
            info.has_limit = true;
            info.limit_count = child_info.limit_count;
        }
        if (child_info.has_offset)
        {
            info.has_offset = true;
            info.limit_offset = child_info.limit_offset;
        }
    }
    return info;
}

std::pair<std::string, Limit_clause::limit_clause_info> Limit_clause::process(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
    auto info = get_info(node);
    if (!info.has_limit && !info.has_offset)
    {
        return {"No LIMIT/OFFSET clause present", info};
    }

    std::ostringstream oss;
    if (info.has_limit)
    {
        oss << "Limit results to " << info.limit_count << " row";
        if (info.limit_count != "1")
            oss << "s";
    }
    if (info.has_offset)
    {
        if (info.has_limit)
            oss << ", ";
        oss << "starting at offset " << info.limit_offset;
    }
    return {oss.str(), info};
}

std::pair<int, std::string> Limit_clause::compare(const limit_clause_info &reference, const limit_clause_info &other)
{
    std::string message;
    int equal = 1;

    if (!reference.has_limit && !reference.has_offset && !other.has_limit && !other.has_offset)
    {
        return {-1, "Both queries have no LIMIT/OFFSET."};
    }

    // Compare presence
    if (reference.has_limit != other.has_limit)
    {
        equal = 0;
        if (reference.has_limit)
            message += "● Missing LIMIT clause.\n";
        else
            message += "● Unexpected LIMIT clause.\n";
    }
    if (reference.has_offset != other.has_offset)
    {
        equal = 0;
        if (reference.has_offset)
            message += "● Missing OFFSET clause.\n";
        else
            message += "● Unexpected OFFSET clause.\n";
    }

    // Compare values when present in both
    if (reference.has_limit && other.has_limit)
    {
        if (reference.limit_count != other.limit_count)
        {
            equal = 0;
            message += "● Mismatch in LIMIT value (expected " + reference.limit_count + ", got " + other.limit_count + ").\n";
        }
    }
    if (reference.has_offset && other.has_offset)
    {
        if (reference.limit_offset != other.limit_offset)
        {
            equal = 0;
            message += "● Mismatch in OFFSET value (expected " + reference.limit_offset + ", got " + other.limit_offset + ").\n";
        }
    }

    return {equal, message};
}
