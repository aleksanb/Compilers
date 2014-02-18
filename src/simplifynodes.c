#include "simplifynodes.h"

extern int outputStage; // This variable is located in vslc.c

void recurse_with_null_check( Node_t *root, int depth ) {
    for (int i=0; i<root->n_children; i++) {
        Node_t *next_node = root->children[i];

        if (next_node != NULL) {
            next_node->simplify(next_node, depth + 1);
        }
    }
}

Node_t* simplify_default ( Node_t *root, int depth )
{
    recurse_with_null_check(root, depth);

    return root;
}


Node_t *simplify_types ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

}


Node_t *simplify_function ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    for (int i=0; i<root->n_children; i++) {
        Node_t *next_node = root->children[i];
        if (next_node == NULL) continue;

        // Dealloc individual nodes
        next_node->simplify(next_node, depth + 1);
        switch(next_node->nodetype.index) {
            case TYPE:
                root->data_type = next_node->data_type;
                node_finalize(next_node);
                break;

            case VARIABLE:
                root->label = STRDUP(next_node->label);
                node_finalize(next_node);
                break;
        }
    }

    /* Fix the array. Here's where the magic happens */

    // Allocate new array and copy references
    Node_t **children = malloc(sizeof(node_t)*2);
    children[0] = root->children[2];
    children[1] = root->children[3];

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

    recurse_with_null_check(root, depth);

    return root;
}


Node_t *simplify_declaration_statement ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    recurse_with_null_check(root, depth);

    return root;
}


Node_t *simplify_single_child ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    recurse_with_null_check(root, depth);

    return root;
}

Node_t *simplify_list_with_null ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    Node_t *left_child = root->children[0];
    Node_t *right_child = root->children[1];

    if (left_child == NULL) { // At bottom of statement tree
        right_child->simplify(right_child, depth + 1);

        // Remove null node and create new list
        Node_t ** children = malloc(sizeof(Node_t) * 1);
        children[0] = right_child;

        // Switch the lists
        free(root->children);
        root->children = children;
        root->n_children = 1;

    } else { // Left child is a statement list
        // Simplify children
        left_child->simplify(left_child, depth + 1);
        right_child->simplify(right_child, depth + 1);

        // Merge child nodes
        int n_children_left = left_child->n_children;
        Node_t **children = malloc(sizeof(Node_t) * (n_children_left + 1));

        for (int i=0; i<n_children_left; i++) {
            children[i] = left_child->children[i];
        }

        // Insert right node
        children[n_children_left] = right_child;

        // Free left children list
        free(left_child->children);

        // Switch the lists
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

    recurse_with_null_check(root, depth);

    return root;
}


Node_t *simplify_expression ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s (%s) \n", depth, ' ', root->nodetype.text, root->expression_type.text );
		
    recurse_with_null_check(root, depth);

    return root;
}

