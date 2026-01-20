
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"


extern int trace_level;

static void error_rule(node_t n, const char *rule, const char *fmt, ...) {
    va_list ap;
    int line = n ? n->lineno : -1;

    fprintf(stderr, "Error line %d: ", line);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (rule && rule[0]) {
        fprintf(stderr, " (rule %s)", rule);
    }

    fprintf(stderr, "\n");
    exit(1);
}

// DECLARATIONS PASS
void decls_list(node_t list, bool is_global){

    if (list->nature == NODE_LIST) {
        decls_list(list->opr[0],is_global);
        decls_list(list->opr[1],is_global);
    } else if (list->nature == NODE_DECLS) {
        
        node_t type = list->opr[0];
        node_t decl = list->opr[1];

        decl_list(decl, type->type,is_global);


    }

}

void decl_list(node_t decl, node_type type, bool is_global) {

    int32_t offset;

    if (decl->nature == NODE_LIST) {
        decl_list(decl->opr[0], type, is_global);
        decl_list(decl->opr[1], type, is_global);
    }

    else if (decl->nature == NODE_DECL) {
        node_t ident_node = decl->opr[0];

        offset = env_add_element(ident_node->ident, ident_node);

        if (type == TYPE_VOID) error_rule(ident_node, "1.8", "Variable '%s' cannot be of type void", ident_node->ident); 

        if (offset == -1) error_rule(ident_node, "1.11", "Variable '%s' already declared", ident_node->ident);
        
        if (decl->nops == 2 && is_global){ 
            if (!(decl->opr[1]->nature == NODE_INTVAL || decl->opr[1]->nature == NODE_BOOLVAL)) error_rule(decl->opr[1], "1.12", "Expression are not allowed in initialization of global variable '%s'", ident_node->ident);
            if (decl->opr[1]->type != type) error_rule(decl->opr[1], "1.12", "Type mismatch in initialization of variable '%s'", ident_node->ident);
      }

        if (decl->nops == 2 && !is_global){ 

            //expression processing
            expr_list_processing(decl->opr[1]);

            if (decl->opr[1]->type != type) error_rule(decl->opr[1], "1.12", "Type mismatch in initialization of variable '%s'", ident_node->ident);
        }


        ident_node->type = type;
        ident_node->global_decl = is_global;
        ident_node->offset = offset;

    }

}


//Main function
void main_decl(node_t func_node){

    reset_env_current_offset();

    node_t type = func_node->opr[0];
    node_t name = func_node->opr[1];
    node_t block = func_node->opr[2];

    //name must be main
    if (strcmp(name->ident,"main") != 0) error_rule(name, "1.4", "The main function must be named 'main'");

    // Type must be void
    if (type->type != TYPE_VOID) error_rule(type, "1.4", "The main function must return 'void'");

    // Block processing
    block_decl(block);

    func_node->offset = get_env_current_offset();
}

// Block processing
void block_decl(node_t block){

    //Declarations processing
    push_context();
    decls_list(block->opr[0],false);

    //Instructions processing
    instr_list_processing(block->opr[1]);
    pop_context();


}

//expression processing
void expr_list_processing(node_t expr){

        if (expr->nature == NODE_LIST) {
        expr_list_processing(expr->opr[0]);
        expr_list_processing(expr->opr[1]);
    } else if (expr->nature != NODE_LIST) {
        



    }

}

void expr_processing(node_t expr){

    //to be implemented

}


//instruction processing
void instr_list_processing(node_t instr){

    if (instr->nature == NODE_LIST) {
        instr_list_processing(instr->opr[0]);
        instr_list_processing(instr->opr[1]);
    } else if (instr->nature != NODE_LIST) {
        



    }

}

void instr_processing(node_t instr){

    //to be implemented

}
void analyse_passe_1(node_t root) {



    node_t globals = root->opr[0];
    node_t mainf   = root->opr[1];

    // Process global declarations
    push_global_context();
    decls_list(globals,true);

    // Process main function
    main_decl(mainf);

    
    pop_context();
}
  

