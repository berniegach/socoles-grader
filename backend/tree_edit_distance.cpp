#include "tree_edit_distance.h"
#include <iostream>
#include <set>
#include <unordered_set>
#include <climits>

/*
 * Helper function to perform the post - order traversal.
 */
void TreeEditDistance::post_order_traversal_helper(const std::shared_ptr<AbstractSyntaxTree::Node> &node, std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &result)
{
	if (node == nullptr)
	{
		return;
	}
	// Traverse the children first.
	for (const auto &child : node->children)
	{
		post_order_traversal_helper(child, result);
	}
	// Then, visit the current node.
	result.push_back(node);
}

/**
 * Returns the post - order traversal of a parse tree.
 * The process of post order traversal is as follows:
 * 1. Traverse left.
 * 2. Traverse right.
 * 3. Visit node.
 */
std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> TreeEditDistance::post_order_traversal(const std::shared_ptr<AbstractSyntaxTree::Node> &root)
{
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> result;
	post_order_traversal_helper(root, result);
	return result;
}

/*
 * Helper function to find the ancestors of a node.
 */
bool TreeEditDistance::find_ancestors_helper(const std::shared_ptr<AbstractSyntaxTree::Node> &node, const std::shared_ptr<AbstractSyntaxTree::Node> &target, std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &ancestors)
{
	if (node == nullptr)
	{
		return false;
	}

	if (node == target)
	{
		return true;
	}

	ancestors.push_back(node);

	for (const auto &child : node->children)
	{
		if (find_ancestors_helper(child, target, ancestors))
		{
			return true;
		}
	}

	ancestors.pop_back();
	return false;
}

/*
 * Returns the ancestors of a node in a parse tree, or an empty vector if the node is not found.
 */
std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> TreeEditDistance::find_ancestors(const std::shared_ptr<AbstractSyntaxTree::Node> &root, const std::shared_ptr<AbstractSyntaxTree::Node> &target)
{
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> ancestors;
	find_ancestors_helper(root, target, ancestors);
	return ancestors;
}
// Returns the leftmost leaf descendant of a node in the parse tree.
std::shared_ptr<AbstractSyntaxTree::Node> TreeEditDistance::leftmost_leaf_descendant(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
	if (node->children.empty())
	{
		return node;
	}

	return leftmost_leaf_descendant(node->children[0]);
}

// Returns a vector containing the leftmost leaf descendants of all nodes in a parse tree.
std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> TreeEditDistance::find_leftmost_leaf_descendants(const std::shared_ptr<AbstractSyntaxTree::Node> &root)
{
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> leftmost_leaf_descendants;
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> postorder_nodes = post_order_traversal(root);
	// a set to track unique nodes
	std::set<std::shared_ptr<AbstractSyntaxTree::Node>> unique_nodes;

	for (const auto &node : postorder_nodes)
	{
		if (node->children.empty())
		{
			if (auto parentNode = node->parent.lock()) // Check if the current node has a parent
			{
				if (parentNode->children[0] == node) // Check if the first child of the parent is the same as the node
				{
					if (unique_nodes.find(node) == unique_nodes.end())
					{
						leftmost_leaf_descendants.push_back(node);
						unique_nodes.insert(node);
					}
				}
			}
		}
		else
		{
			auto left_mld = leftmost_leaf_descendant(node->children[0]);
			if (unique_nodes.find(left_mld) == unique_nodes.end())
			{
				leftmost_leaf_descendants.push_back(left_mld);
				unique_nodes.insert(left_mld);
			}
		}
	}

	return leftmost_leaf_descendants;
}
// Returns a leftmost leaf descendant of a  node in a parse tree.
std::shared_ptr<AbstractSyntaxTree::Node> TreeEditDistance::find_single_leftmost_leaf_descendant(const std::shared_ptr<AbstractSyntaxTree::Node> &node)
{
	if (node == nullptr || node->children.empty() || node->children[0] == nullptr)
	{
		return node;
	}
	return find_single_leftmost_leaf_descendant(node->children[0]);
}

/*
 * This function finds the indices of the left most leaf descendants.
 * The indices will be used for indexing later on
 * LR_keyroots(T) = {k|there exists no k' such that l(k) = l(k')}
 * That is if k is in LR_keyroots then either k is the root of T or l(k) / = l(p(k)) i.e, k has a left sibling
 */
std::vector<size_t> TreeEditDistance::find_lr_keyroots_index(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &postorder)
{
	std::vector<size_t> lr_keyroots;
	for (size_t index = 0; index < postorder.size(); index++)
	{
		const auto &node = postorder[index];
		if (find_single_leftmost_leaf_descendant(node) != find_single_leftmost_leaf_descendant(node->parent.lock()))
		{
			lr_keyroots.push_back(index);
		}
	}
	return lr_keyroots;
}
std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> TreeEditDistance::find_lr_keyroots_node(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &postorder)
{
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> lr_keyroots;
	for (size_t index = 0; index < postorder.size(); index++)
	{
		const auto &node = postorder[index];
		if (find_single_leftmost_leaf_descendant(node) != find_single_leftmost_leaf_descendant(node->parent.lock()))
		{
			lr_keyroots.push_back(node);
		}
	}
	return lr_keyroots;
}
int TreeEditDistance::edit_weight(const std::shared_ptr<AbstractSyntaxTree::Node> &node1, const std::shared_ptr<AbstractSyntaxTree::Node> &node2)
{
	// both the nodes are empty null pointers
	if (!node1 && !node2)
	{
		return 0;
	}
	// one node is empty null pointer
	if (!node1 || !node2)
	{
		return 1;
	}
	// the nodes are the same
	if (node1->key == node2->key && node1->value == node2->value)
	{
		return 0;
	}
	// the nodes are different
	else
	{
		return 1;
	}
}
/*
 * Zhang - Shasha algorithm for tree edit distance.
 * The following are the steps of the algorithm.
 *	1. Preprocessing;
 *		a) Get the most left node for each node.
 *		b) Get the key root for each node.
 *	2. For s:=1 to |LR_keyroots(t1)|
 *		 For t:=1 to |LR_keyroots(t2)|
 *			i = keyroots(t1)[s]
 *			j = keyroots(t2)[t]
 *			Treedist(i,j)
 *		 end
 *	3. Return tdist[i,j]
 * s,t represents the index of left most node in the postorder
 */
int TreeEditDistance::zhang_shasha(const std::shared_ptr<AbstractSyntaxTree::Node> &tree1, const std::shared_ptr<AbstractSyntaxTree::Node> &tree2)
{
	// Get postorder traversals and leftmost leaf descendants.
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> postorder1 = post_order_traversal(tree1);
	std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> postorder2 = post_order_traversal(tree2);

	// find the leftmost descendants of the trees
	auto leftmost_descendants1 = find_leftmost_leaf_descendants(tree1);
	auto leftmost_descendants2 = find_leftmost_leaf_descendants(tree2);
	// this is a matrix containing all combinations of postorder lists for both trees.
	// we add two cells for empty nodes
	std::vector<std::vector<int>> tree_dist(postorder1.size(), std::vector<int>(postorder2.size(), 0));
	// get the indices of the left most nodes in the postorder traversal list
	auto lr_keyroots1 = find_lr_keyroots_index(postorder1);
	auto lr_keyroots2 = find_lr_keyroots_index(postorder2);
	// compute the forest distance for each sub trees
	for (size_t i_prime = 0; i_prime < lr_keyroots1.size(); i_prime++)
	{
		for (size_t j_prime = 0; j_prime < lr_keyroots2.size(); j_prime++)
		{
			size_t i = lr_keyroots1[i_prime];
			size_t j = lr_keyroots2[j_prime];
			compute_forest_distance(postorder1, postorder2, i, j, tree_dist);
		}
	}
	return tree_dist[postorder1.size() - 1][postorder2.size() - 1];
}
void TreeEditDistance::compute_forest_distance(const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &postorder1, const std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> &postorder2, const size_t i, const size_t j, std::vector<std::vector<int>> &tree_dist)
{
	// find the index leftmost descendant of the node at i
	std::shared_ptr<AbstractSyntaxTree::Node> l_i_node = find_single_leftmost_leaf_descendant(postorder1.at(i));
	size_t l_i_index = -1;
	for (size_t index = 0; index < postorder1.size(); index++)
	{
		if (postorder1.at(index) == l_i_node)
		{
			l_i_index = index;
			break;
		}
	}
	if (l_i_index == -1)
	{
		std::cerr << "l_i_index not found." << std::endl;
		exit(1);
	}
	// find the index leftmost descendant of the node at j
	std::shared_ptr<AbstractSyntaxTree::Node> l_j_node = find_single_leftmost_leaf_descendant(postorder2.at(j));
	size_t l_j_index = -1;
	for (size_t index = 0; index < postorder2.size(); index++)
	{
		if (postorder2.at(index) == l_j_node)
		{
			l_j_index = index;
			break;
		}
	}
	if (l_j_index == -1)
	{
		std::cerr << "l_j_index not found." << std::endl;
		exit(1);
	}
	// lets create a matrix that holds the single tree from l(i) to i and l(j) to j elements including space for empty cells
	size_t bound_1 = i - l_i_index + 2;
	size_t bound_2 = j - l_j_index + 2;

	std::vector<std::vector<int>> forest_dist(bound_1, std::vector<int>(bound_2, 0));
	for (size_t i1 = 1, i2 = l_i_index; i1 < bound_1; i1++, i2++)
	{
		forest_dist[i1][0] = forest_dist[i1 - 1][0] + edit_weight(postorder1[i2], nullptr);
	}
	for (size_t j1 = 1, j2 = l_j_index; j1 < bound_2; j1++, j2++)
	{
		forest_dist[0][j1] = forest_dist[0][j1 - 1] + edit_weight(nullptr, postorder2[j2]);
	}
	for (size_t i1 = l_i_index, k = 1; i1 <= i; i1++, k++)
	{
		for (size_t j1 = l_j_index, l = 1; j1 <= j; j1++, l++)
		{
			if (find_single_leftmost_leaf_descendant(postorder1.at(i1)) == find_single_leftmost_leaf_descendant(postorder1.at(i)) &&
				find_single_leftmost_leaf_descendant(postorder2.at(j1)) == find_single_leftmost_leaf_descendant(postorder2.at(j)))
			{
				int delete_cost = forest_dist[k - 1][l] + edit_weight(postorder1[i1], nullptr);
				int insert_cost = forest_dist[k][l - 1] + edit_weight(nullptr, postorder2[j1]);
				int substitute_cost = forest_dist[k - 1][l - 1] + edit_weight(postorder1[i1], postorder2[j1]);

				forest_dist[k][l] = std::min({delete_cost, insert_cost, substitute_cost});
				tree_dist[i1][j1] = forest_dist[k][l];
			}
			else
			{
				int delete_cost = forest_dist[k - 1][l] + edit_weight(postorder1[i1], nullptr);
				int insert_cost = forest_dist[k][l - 1] + edit_weight(nullptr, postorder2[j1]);
				int substitute_cost = forest_dist[k - 1][l - 1] + edit_weight(postorder1[i1], postorder2[j1]);

				forest_dist[k][l] = std::min({delete_cost, insert_cost, substitute_cost});
			}
		}
	}
}
bool TreeEditDistance::is_irrelevant_key(const std::string &key)
{
	return key == "ResTarget" || key == "val" || key == "ColumnRef";
}