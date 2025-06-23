#include "abstract_syntax_tree.h"
#include <iostream>

AbstractSyntaxTree::Node::Node(const string &key, const string &value, const std::shared_ptr<Node> &parent)
    : key(key), value(value), parent(parent) {}

void AbstractSyntaxTree::Node::add_child(std::shared_ptr<Node> child)
{
    child->parent = shared_from_this();
    children.push_back(child);
}
std::shared_ptr<AbstractSyntaxTree::Node> AbstractSyntaxTree::Node::get_child(const std::string &key) const
{
    for (const auto &child : children)
    {
        if (child->key == key)
        {
            return child;
        }
    }
    return nullptr;
}

std::string AbstractSyntaxTree::Node::get_value(const std::string &key) const
{
    auto child = get_child(key);
    if (child)
    {
        std::string value = child->value;
        // Remove surrounding quotes if present
        if (!value.empty() && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    }
    return "";
}

void AbstractSyntaxTree::build_tree(const nlohmann::json &input, std::shared_ptr<Node> &parent)
{
    using namespace std;

    for (const auto &element : input.items())
    {
        shared_ptr<Node> childNode;

        if (element.value().is_object())
        {
            childNode = make_shared<Node>(element.key(), "");
            build_tree(element.value(), childNode);
        }
        else if (element.value().is_array())
        {
            childNode = make_shared<Node>(element.key(), "");
            for (const auto &array_element : element.value())
            {
                build_tree(array_element, childNode);
            }
        }
        else if (element.value().is_primitive())
        {
            childNode = make_shared<Node>(element.key(), element.value().dump());
        }

        if (childNode)
        {
            parent->add_child(childNode);
        }
    }
    // prune tree to remove unnecessary nodes
    prune_tree(parent);
}

void AbstractSyntaxTree::print_tree(const std::shared_ptr<Node> &node, int level, bool is_last, const string &prefix)
{
    using namespace std;

    string indentation(level * 2, ' ');
    string connector = is_last ? "└──" : "├──";
    string new_prefix = prefix + (is_last ? "    " : "│   ");

    cout << prefix << connector << node->key << ": " << node->value << endl;

    for (size_t i = 0; i < node->children.size(); ++i)
    {
        const auto &child = node->children[i];
        bool last_child = i == node->children.size() - 1;
        print_tree(child, level + 1, last_child, new_prefix);
    }
}

void AbstractSyntaxTree::prune_tree(std::shared_ptr<Node> &root)
{
    // if the root node is nullptr return
    if (!root)
    {
        return;
    }

    // access the children of the current node
    auto &children = root->children;
    for (auto it = children.begin(); it != children.end();)
    {
        auto &child = *it;

        // Check if the child node's key matches any of the keys we want to prune
        if (child->key == "location" || child->key == "stmt_len" ||
            child->key == "version" || child->key == "inh" ||
            child->key == "relpersistence" || child->key == "limitOption" ||
            child->key == "op")
        {

            // Update the parent of each grandchild to point to the current root node
            if (child->children.size() > 0)
            {
                for (auto &grandchild : child->children)
                {
                    grandchild->parent = root;
                }

                // Move the grandchildren of the child node to the same level as the child
                children.insert(it, child->children.begin(), child->children.end());
            }

            // Remove the child node from the tree
            it = children.erase(it);
        }
        else
        {
            // Recursively call the prune_tree function for the child node
            prune_tree(child);
            // Move the iterator to the next child
            ++it;
        }
    }
}
string AbstractSyntaxTree::get_statement_type(const std::shared_ptr<Node> &node)
{
    if (!node)
    {
        return "";
    }

    if (node->key == "stmt" && !node->children.empty())
    {
        // return the key of the first child
        return node->children.front()->key;
    }
    for (const auto &child : node->children)
    {
        string st = get_statement_type(child);
        if (!st.empty())
        {
            return st;
        }
    }
    return "";
}
std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> AbstractSyntaxTree::get_statements(const std::shared_ptr<AbstractSyntaxTree::Node> &root)
{
    std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> statements;
    if (!root)
        return statements;

    // If the node itself is a single statement.
    if (root->key == "stmt" && !root->children.empty())
    {
        statements.push_back(root);
    }
    // If the root represents a collection
    else if (root->key == "stmts")
    {
        for (const auto &child : root->children)
        {
            // Assume each child with key "stmt" is a statement.
            if (child->key == "stmt")
                statements.push_back(child);
        }
    }
    else
    {
        // Otherwise, try to search for statements in the children.
        for (const auto &child : root->children)
        {
            auto sub_statements = get_statements(child);
            statements.insert(statements.end(), sub_statements.begin(), sub_statements.end());
        }
    }
    return statements;
}
std::shared_ptr<AbstractSyntaxTree::Node> AbstractSyntaxTree::get_statement_node(const std::shared_ptr<Node> &node) const
{
    if (!node)
    {
        return nullptr;
    }

    // Check if the current node is a "stmt" node.
    // If it is and it has children, then this is a statement node.
    if (node->key == "stmt" && !node->children.empty())
    {
        return node->children.front();
    }

    // Otherwise, recursively search through the children.
    for (const auto &child : node->children)
    {
        auto stmt_node = get_statement_node(child);
        if (stmt_node)
        {
            return stmt_node;
        }
    }

    // If no statement node was found in this subtree, return nullptr.
    return nullptr;
}
