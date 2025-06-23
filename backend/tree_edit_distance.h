/**
 * @file tree_edit_distance.h
 * @brief This is the Zhang-Shasha algorithm,
 * it computes the minimum edit distance between two trees by considering node insertions, deletions, and updates.
 * The algorithm is implemented based on the paper: Zhang, Kaizhong & Shasha, Dennis. (1989). Simple Fast Algorithms for the Editing Distance Between Trees and Related Problems. SIAM J. Comput.. 18. 1245-1262. 10.1137/0218082. 
 * 
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef TREE_EDIT_DISTANCE_H
#define TREE_EDIT_DISTANCE_H

#include "abstract_syntax_tree.h"

class TreeEditDistance
{
public:
    /**
     * This recursive function does the actual post order left to right traversal of a tree.
     * @param node: the current parent node.
     * @param result: the vector containing the nodes of the tree in post-order left-right configuration.
    */
    void post_order_traversal_helper(const std::shared_ptr<AbstractSyntaxTree::Node>& node, std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& result);
    /**
     * This function performs a post-order traversal of a tree.
     * The nodes are arranged from the leftmost leaf descendant to the rightmost leaf descendant.
     * @param root: the root of the tree.
     * @return a vector containing the nodes of the tree in post-order left-right configuration.
    */
    std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> post_order_traversal(const std::shared_ptr<AbstractSyntaxTree::Node>& root);
    /**
     * This recursive helper function does the actual work of finding the ancestors of a node in a tree.
     * @param node: the current child node.
     * @param target: a mini tree containing the target node.
     * @param ancestors: a vector containing the ancestors of the target node.
    */
    bool find_ancestors_helper(const std::shared_ptr<AbstractSyntaxTree::Node>& node, const std::shared_ptr<AbstractSyntaxTree::Node>& target, std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& ancestors);
    /**
     * This function finds the ancestors of a node in a tree.
     * @param root: the root of the tree.
     * @param target: the node whose ancestors are to be found.
     * @return a vector containing the ancestors of the target node.
    */
    std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> find_ancestors(const std::shared_ptr<AbstractSyntaxTree::Node>& root, const std::shared_ptr<AbstractSyntaxTree::Node>& target);
    /**
     * This recursive helper function does the actual work of finding the leftmost leaf descendant of a parent node.
     * @param node: the current parent node.
     * @return a pointer to the left most descendant found.
    */
    std::shared_ptr<AbstractSyntaxTree::Node> leftmost_leaf_descendant(const std::shared_ptr<AbstractSyntaxTree::Node>& node);
    /**
     * This function finds the leftmost leaf descendants of a parent node.
     * @param root: the parent node
     * @return a vector containing the leftmost leaf descendants of the parent node.
    */
    std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> find_leftmost_leaf_descendants(const std::shared_ptr<AbstractSyntaxTree::Node>& root);
    /**
     * This recursive function finds the rightmost leaf descendants of a parent node.
     * @param root: the parent node
     * @return a pointer to the left most descendant found.
    */
    std::shared_ptr<AbstractSyntaxTree::Node> find_single_leftmost_leaf_descendant(const std::shared_ptr<AbstractSyntaxTree::Node>& node);
    /**
     * This function finds the indexes of the keyroots of a tree in a post-order left-right traversal.
     * @param postorder: the post-order left-right traversal of the tree.
     * @return a vector containing the indexes of the keyroots of the tree.
    */
    std::vector<size_t> find_lr_keyroots_index(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& postorder);
    /**
     * This function finds the keyroots of a tree in a post-order left-right traversal.
     * @param postorder: the post-order left-right traversal of the tree.
     * @return a vector containing the keyroots of the tree.
    */
    std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> find_lr_keyroots_node(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& postorder);
    /**
     * This function sets the edit weight between two nodes.
     * @param node1: the first node.
     * @param node2: the second node.
     * @return the edit weight between the two nodes.
    */
    int edit_weight(const std::shared_ptr<AbstractSyntaxTree::Node>& node1, const std::shared_ptr<AbstractSyntaxTree::Node>& node2);
    /**
     * This function calculates the edit distance between two trees.
     * @param tree1: the first tree.
     * @param tree2: the second tree.
     * @return the edit distance between the two trees.
    */
    int zhang_shasha(const std::shared_ptr<AbstractSyntaxTree::Node> &tree1, const std::shared_ptr<AbstractSyntaxTree::Node> &tree2);
    /**
     * This function calculates the edit distance between forests.
     * @param postorder1: the post-order left-right traversal of the first tree.
     * @param postorder2: the post-order left-right traversal of the second tree.
     * @param i: the index of the root node of the first tree.
     * @param j: the index of the root node of the second tree.
     * @param tree_dist: a 2D matrix containing the edit distances between the nodes of the two trees.
    */
    void compute_forest_distance(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& postorder1, const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>>& postorder2, const size_t i, const size_t j, std::vector<std::vector<int>>& tree_dist);
private:
    /**
     * This function returns whether a node should be considered for editing in tree comparisons or not.
     * @param key: the node to be checked.
     * @return true if the node should be considered for editing, false otherwise.
    */
    bool is_irrelevant_key(const std::string& key);
};
#endif // TREE_EDIT_DISTANCE_H


