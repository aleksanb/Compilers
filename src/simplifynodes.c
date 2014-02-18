#include "simplifynodes.h"

extern int outputStage; // This variable is located in vslc.c

void recurse_with_null_check( Node_t *root, int depth ) {
    for (int i=0; i<root->n_children; i++) {
        Node_t *next_node = root->children[i];

        if (next_node != NULL) {
            root->children[i] = next_node->simplify(next_node, depth + 1);
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

    if (root->data_type.base_type == CLASS_TYPE) { // Yay there are child nodes :D Let's steal their labels
        root->data_type.class_name = STRDUP(root->children[0]->label);

        // Fix le childz
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

    for (int i=0; i<root->n_children; i++) {
        Node_t *next_node = root->children[i];
        if (next_node == NULL) continue;

        // Dealloc individual nodes
        root->children[i] = next_node->simplify(next_node, depth + 1);
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

    // Recurse children, then remove first child
    recurse_with_null_check(root, depth);

    // Set label and clear first node
    root->label = STRDUP(root->children[0]->label);

    node_finalize(root->children[0]);

    // Allocate new array and copy references
    Node_t **children = malloc(sizeof(node_t) * (root->n_children - 1));
    for (int i=1; i < root->n_children; i++) {
        children[i-1] = root->children[i];
    }

    // Change pointers and update total count
    free(root->children);
    root->children = children;
    root->n_children = root->n_children - 1;

    return root;
}


Node_t *simplify_declaration_statement ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    // Bind references
    Node_t *n_data_type = root->children[0];
    Node_t *n_label = root->children[1];

    root->children[0] = n_data_type->simplify(n_data_type, depth + 1);
    root->children[1] = n_label->simplify(n_label, depth + 1);

    // Copy label and data type
    root->data_type = n_data_type->data_type;
    root->label = STRDUP(n_label->label);
    
    // Clear up memory
    node_finalize(n_data_type);
    node_finalize(n_label);

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

    // Single child, replace ourselves with the child
    Node_t *child = root->children[0];
    root->children[0] = child->simplify(child, depth + 1);

    /* TODO:Here there should be memory clearing sale!
        free(root->children);
        free(root);
    */

    // Return our child
    return root->children[0];
}

Node_t *simplify_list_with_null ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    Node_t *left_child = root->children[0];
    Node_t *right_child = root->children[1];

    recurse_with_null_check(root, depth);
    if (left_child == NULL) { // At bottom of statement tree
        //root->children[1] = right_child->simplify(right_child, depth + 1);

        // Remove null node and create new list
        Node_t ** children = malloc(sizeof(Node_t) * 1);
        children[0] = right_child;

        // Switch the lists
        free(root->children);
        root->children = children;
        root->n_children = 1;

    } else { // Left child is a function/declaration list
        // Simplify children

        //root->children[0] = left_child->simplify(left_child, depth + 1);
        //root->children[1] = right_child->simplify(right_child, depth + 1);

        if (root->n_children > 2) {
            printf("omg soo many children %d", root->n_children);
        }

        // Merge child nodes
        int n_children_left = left_child->n_children;
        Node_t **children = malloc(sizeof(Node_t) * (n_children_left + 1));

        // Copy over references
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

        /*if (root->n_children > 2) {
            printf("omg soo many children %d", root->n_children);
        }*/
    }

    return root;
}


Node_t *simplify_list ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s \n", depth, ' ', root->nodetype.text );

    Node_t *left_child = root->children[0];

    recurse_with_null_check(root, depth);
    if (root->n_children == 1) { // At bottom of statement tree
        //root->children[0] = left_child->simplify(left_child, depth + 1);
    } else { // Left child is a statement list
        Node_t *right_child = root->children[1];

        // Simplify children
        //root->children[0] = left_child->simplify(left_child, depth + 1);
        //root->children[1] = right_child->simplify(right_child, depth + 1);

        // Merge child nodes
        int n_children_left = left_child->n_children;
        Node_t **children = malloc(sizeof(Node_t) * (n_children_left + 1));

        // Copy over references
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


Node_t *simplify_expression ( Node_t *root, int depth )
{
	if(outputStage == 4)
		fprintf ( stderr, "%*cSimplify %s (%s) \n", depth, ' ', root->nodetype.text, root->expression_type.text );

    if (root->n_children == 1 &&
            root->expression_type.index != NEW_E &&
            root->expression_type.index != UMINUS_E &&
            root->expression_type.index != NOT_E) {

        Node_t *child = root->children[0];
        root->children[0] = child->simplify(child, depth + 1);

        return root->children[0]; // TODO: Here be magic
    } else {
        recurse_with_null_check(root, depth);

        return root;
    }
}

