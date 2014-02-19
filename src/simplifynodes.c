#include "simplifynodes.h"

extern int outputStage; // This variable is located in vslc.c

// Helper function for when we wish to simplify all child nodes
void simplify_children_with_null_check( Node_t *root, int depth ) {
    for (int i=0; i<root->n_children; i++) {
        Node_t *next_node = root->children[i];

        if (next_node != NULL) {
            root->children[i] = next_node->simplify(next_node, depth + 1);
        }
    }
}

Node_t* simplify_default ( Node_t *root, int depth )
{
    simplify_children_with_null_check(root, depth);
    return root;
}


Node_t *simplify_types ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Only simplify if there is something to be simplified
    if (root->data_type.base_type == CLASS_TYPE) { 
        root->data_type.class_name = STRDUP(root->children[0]->label);

        // Clean up the child node
        node_finalize(root->children[0]);
        root->children = NULL;
        root->n_children = 0;
    }

    return root;
}


Node_t *simplify_function ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Simplify all child nodes
    simplify_children_with_null_check(root, depth);

    // Fetch data type and label nodes
    Node_t *data_type_node = root->children[0];
    Node_t *label_node = root->children[1];

    // retreive values
    root->data_type = data_type_node->data_type;
    root->label = STRDUP(label_node->label);

    // Clean up children nodes
    node_finalize(data_type_node);
    node_finalize(label_node);

    // Allocate new array and copy references
    Node_t **children = malloc(sizeof(node_t)*2);
    children[0] = root->children[2]; /* Parameter list */
    children[1] = root->children[3]; /* Statement list */

    // Clear old children list and change reference
    free(root->children);
    root->children = children;
    root->n_children = 2;

    return root;
}


Node_t *simplify_class( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Recurse children, then remove first child
    simplify_children_with_null_check(root, depth);

    // Set label
    Node_t *label_node = root->children[0];
    root->label = STRDUP(label_node->label);

    // Clear first node
    node_finalize(label_node);

    // Allocate new array and copy references. There can be many children so we loop over all
    Node_t **children = malloc(sizeof(node_t) * (root->n_children - 1));
    for (int i=1; i < root->n_children; i++) {
        children[i-1] = root->children[i];
    }

    // Clear old children list and change reference
    free(root->children);
    root->children = children;
    root->n_children = (root->n_children - 1);

    return root;
}


Node_t *simplify_declaration_statement ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Simplify all child nodes
    simplify_children_with_null_check(root, depth);

    // Fetch data type and label nodes
    Node_t *n_data_type = root->children[0];
    Node_t *n_label = root->children[1];

    // Copy label and data type
    root->data_type = n_data_type->data_type;
    root->label = STRDUP(n_label->label);
    
    // Clear up memory
    node_finalize(n_data_type);
    node_finalize(n_label);

    // Clear old children list and set dangling pointers to NULL
    free(root->children);
    root->children = NULL;
    root->n_children = 0;

    return root;
}


Node_t *simplify_single_child ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    if (root == NULL) { // Oh god, abort!
        return root;
    }

    // Simplify all child nodes, even though there only is one :)
    simplify_children_with_null_check(root, depth);

    // Bind child node
    Node_t *child = root->children[0];

    // Clear our own memory footprint
    node_finalize(root);

    // Return our child
    return child;
}

Node_t *simplify_list_with_null ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Simplify all child nodes
    simplify_children_with_null_check(root, depth);

    // Bind child nodes
    Node_t *left_child = root->children[0]; /* Might be another list */
    Node_t *right_child = root->children[1]; /* Guaranteed to be a non-list by the grammar */

    if (left_child == NULL) { /* At bottom of statement tree */
        // Remove null node and create new list
        Node_t ** children = malloc(sizeof(Node_t) * 1);
        children[0] = right_child;

        // Switch the lists and update child count
        free(root->children);
        root->children = children;
        root->n_children = 1;

    } else { /* Left child is a list */
        // Allocate space for new children list
        int n_children_left = left_child->n_children;
        Node_t **children = malloc(sizeof(Node_t) * (n_children_left + 1));

        // Copy over references
        for (int i=0; i<n_children_left; i++) {
            children[i] = left_child->children[i];
        }
        children[n_children_left] = right_child; /* Insert right node */

        // Free left child
        node_finalize(left_child);

        // Swap the lists
        free(root->children);
        root->children = children;
        root->n_children = n_children_left + 1;
    }

    return root;
}


Node_t *simplify_list ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Simplify all child nodes
    simplify_children_with_null_check(root, depth);

    if (root->n_children > 1) { /* Somewhere in the midlle of the tree, time to concat children! */
        // Bind child nodes
        Node_t *left_child = root->children[0];
        Node_t *right_child = root->children[1];

        // Allocate space for new children list
        int n_children_left = left_child->n_children;
        Node_t **children = malloc(sizeof(Node_t) * (n_children_left + 1));

        // Copy over references
        for (int i=0; i<n_children_left; i++) {
            children[i] = left_child->children[i];
        }
        children[n_children_left] = right_child; /* Insert right node */

        // Free left children list
        node_finalize(left_child);

        // Switch the lists
        free(root->children);
        root->children = children;
        root->n_children = n_children_left + 1;
    }

    return root;
}


Node_t *simplify_expression ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s (%s) \n", depth, ' ', root->nodetype.text, root->expression_type.text );

    // Recurse over our single child! :D
    simplify_children_with_null_check(root, depth);

    // If single child, simplify
    if (root->n_children == 1 &&
            root->expression_type.index != NEW_E &&
            root->expression_type.index != UMINUS_E &&
            root->expression_type.index != NOT_E) {

        // Clear our own memory footprint
        Node_t *child = root->children[0];
        node_finalize(root);

        return child;
    }

    return root;
}

