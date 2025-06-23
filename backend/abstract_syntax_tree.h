/**
 * @file abstract_syntax_tree.h
 *
 * @brief This class defines the mechanisms for building an abstract syntax tree from an SQL query.
 * Each of the elements of the query is represented as a node in the tree.
 *
 * @author Benard Wanjiru
 * Contact: benard.wanjiru@ru.nl
 */
#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <memory>

using std::string;
using std::vector;

class AbstractSyntaxTree
{
public:
    /**
     * This struct represents a node in the abstract syntax tree.
     * enable_shared_from_this allows allocating children pointers to one parent.
     */
    struct Node : public std::enable_shared_from_this<Node>
    {
        string key;
        string value;
        vector<std::shared_ptr<Node>> children;
        std::weak_ptr<Node> parent;

        /**
         * explicit keyword in the constructor prevents implicit conversions of the parameters.
         * @param key: This key identifies the type of node (e.g. relname for relation name, sval for string value, etc.).
         * @param value: This value is the actual value of the node (e.g. "students" for relation name, "id" for string value, etc.).
         * @param parent: This is a pointer to the root node.
         */
        explicit Node(const string &key, const string &value, const std::shared_ptr<Node> &parent = nullptr);
        /**
         * This function adds a child to a parent node.
         * @param child: This is a pointer to the child node.
         */
        void add_child(std::shared_ptr<Node> child);

        std::shared_ptr<Node> get_child(const std::string &key) const;
        std::string get_value(const std::string &key) const;
    };
    /**
     * This function builds an abstract syntax tree in a c++ object oriented structure.
     * @param input: This is the parse tree in JSON format.
     * @param parent: This is a pointer to the root node.
     */
    void build_tree(const nlohmann::json &input, std::shared_ptr<Node> &parent);
    /**
     * This is a recursive function that prints a tree in a human readable format on the console.
     * @param node: This is a pointer to the current node to be printed
     * @param level: This is the indentation value for the current node drawn from the left side of the screen.
     * @param is_last: This is a boolean value that indicates whether the current node is the last child of its parent.
     * @param prefix: This is a string containing beautifying characters for the current node. eg "├──"
     */
    static void print_tree(const std::shared_ptr<Node> &node, int level = 0, bool is_last = true, const std::string &prefix = "");
    /**
     * This recusrive function prunes the tree to remove unnecessary nodes.
     * This helps when carrying out tree distance calculations to avoid calculating distances between nodes that are not relevant.
     * @param root: This is a pointer to the root node.
     */
    void prune_tree(std::shared_ptr<Node> &root);
    /**
     * This function returns the type of statement represented by a parse tree.
     * @param node: This is a pointer to the root node in the parse tree.
     * @return: This is a string representing the type of statement.
     */
    static string get_statement_type(const std::shared_ptr<Node> &node);
    static std::vector<std::shared_ptr<AbstractSyntaxTree::Node>> get_statements(const std::shared_ptr<AbstractSyntaxTree::Node> &root);
    std::shared_ptr<AbstractSyntaxTree::Node> get_statement_node(const std::shared_ptr<Node> &node) const;

private:
    const string statement_types[124] = {
        "AlterEventTrigStmt", "AlterCollationStmt", "AlterDatabaseStmt", "AlterDatabaseSetStmt",
        "AlterDefaultPrivilegesStmt", "AlterDomainStmt", "AlterEnumStmt", "AlterExtensionStmt",
        "AlterExtensionContentsStmt", "AlterFdwStmt", "AlterForeignServerStmt", "AlterFunctionStmt",
        "AlterGroupStmt", "AlterObjectDependsStmt", "AlterObjectSchemaStmt", "AlterOwnerStmt",
        "AlterOperatorStmt", "AlterTypeStmt", "AlterPolicyStmt", "AlterSeqStmt",
        "AlterSystemStmt", "AlterTableStmt", "AlterTblSpcStmt", "AlterCompositeTypeStmt",
        "AlterPublicationStmt", "AlterRoleSetStmt", "AlterRoleStmt", "AlterSubscriptionStmt",
        "AlterStatsStmt", "AlterTSConfigurationStmt", "AlterTSDictionaryStmt", "AlterUserMappingStmt",
        "AnalyzeStmt", "CallStmt", "CheckPointStmt", "ClosePortalStmt",
        "ClusterStmt", "CommentStmt", "ConstraintsSetStmt", "CopyStmt",
        "CreateAmStmt", "CreateAsStmt", "CreateAssertionStmt", "CreateCastStmt",
        "CreateConversionStmt", "CreateDomainStmt", "CreateExtensionStmt", "CreateFdwStmt",
        "CreateForeignServerStmt", "CreateForeignTableStmt", "CreateFunctionStmt", "CreateGroupStmt",
        "CreateMatViewStmt", "CreateOpClassStmt", "CreateOpFamilyStmt", "CreatePublicationStmt",
        "AlterOpFamilyStmt", "CreatePolicyStmt", "CreatePLangStmt", "CreateSchemaStmt",
        "CreateSeqStmt", "CreateStmt", "CreateSubscriptionStmt", "CreateStatsStmt",
        "CreateTableSpaceStmt", "CreateTransformStmt", "CreateTrigStmt", "CreateEventTrigStmt",
        "CreateRoleStmt", "CreateUserStmt", "CreateUserMappingStmt", "CreatedbStmt",
        "DeallocateStmt", "DeclareCursorStmt", "DefineStmt", "DeleteStmt",
        "DiscardStmt", "DoStmt", "DropCastStmt", "DropOpClassStmt",
        "DropOpFamilyStmt", "DropOwnedStmt", "DropStmt", "DropSubscriptionStmt",
        "DropTableSpaceStmt", "DropTransformStmt", "DropRoleStmt", "DropUserMappingStmt",
        "DropdbStmt", "ExecuteStmt", "ExplainStmt", "FetchStmt",
        "GrantStmt", "GrantRoleStmt", "ImportForeignSchemaStmt", "IndexStmt",
        "InsertStmt", "ListenStmt", "RefreshMatViewStmt", "LoadStmt",
        "LockStmt", "MergeStmt", "NotifyStmt", "PrepareStmt",
        "ReassignOwnedStmt", "ReindexStmt", "RemoveAggrStmt", "RemoveFuncStmt",
        "RemoveOperStmt", "RenameStmt", "RevokeStmt", "RevokeRoleStmt",
        "RuleStmt", "SecLabelStmt", "SelectStmt", "TransactionStmt",
        "TruncateStmt", "UnlistenStmt", "UpdateStmt", "VacuumStmt",
        "VariableResetStmt", "VariableSetStmt", "VariableShowStmt", "ViewStmt"};
};
#endif // !ABSTRACT_SYNTAX_TREE_H