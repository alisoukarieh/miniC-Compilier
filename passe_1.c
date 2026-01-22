
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

    if (list == NULL) return;

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

            if (decl->opr[1]->type != type) error_rule(decl->opr[1], "1.13", "Type mismatch in initialization of variable '%s'", ident_node->ident);
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

    if (block == NULL) return;

    //Declarations processing
    push_context();
    decls_list(block->opr[0],false);

    //Instructions processing
    instr_list_processing(block->opr[1]);
    pop_context();


}

//expression processing
void expr_list_processing(node_t expr){

    if (expr == NULL) return;

        if (expr->nature == NODE_LIST) {
        expr_list_processing(expr->opr[0]);
        expr_list_processing(expr->opr[1]);
    } else if (expr->nature != NODE_LIST) {
        expr_processing(expr);


    }

}

void expr_processing(node_t expr){

    if (expr == NULL) return;


    node_t left, right;

    switch (expr->nature)
    {

        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
            left = expr->opr[0];
            right =  expr->opr[1];

            

            expr_processing(left);
            expr_processing(right);

            if (left->type != TYPE_INT || right->type != TYPE_INT) {
                error_rule(expr, "1.30", "Operands of arithmetic operators must be of type int");
            }
            
            expr->type = TYPE_INT;

            break;

        case NODE_LT:
        case NODE_GT:
        case NODE_LE:
        case NODE_GE:

            left = expr->opr[0];
            right =  expr->opr[1];

            
            expr_processing(left);
            expr_processing(right);

            if (left->type != TYPE_INT || right->type != TYPE_INT) {
                error_rule(expr, "1.30", "Operands of comparaison operators must be of type int");
            }

            expr->type = TYPE_BOOL;

            break;

        case NODE_EQ:
        case NODE_NE:
            left = expr->opr[0];
            right = expr->opr[1];

            expr_processing(left);
            expr_processing(right);

            if (left->type != right->type) {
                error_rule(expr, "1.30", "Operands of equality operators must have the same type");
            }

            expr->type = TYPE_BOOL;
            break;
        
        case NODE_OR:
        case NODE_AND:
            left = expr->opr[0];
            right =  expr->opr[1];



            expr_processing(left);
            expr_processing(right);

            if (left->type != TYPE_BOOL || right->type != TYPE_BOOL) {
                error_rule(expr, "1.30", "Operands of logical operators must be of type bool");
            }

            expr->type = TYPE_BOOL; 
            break;

        case NODE_BAND:
        case NODE_BOR:
        case NODE_BXOR:
        case NODE_SLL:
        case NODE_SRA:
        case NODE_SRL:
            left = expr->opr[0];
            right =  expr->opr[1];



            expr_processing(left);
            expr_processing(right);

            if (left->type != TYPE_INT || right->type != TYPE_INT) {
                error_rule(expr, "1.30", "Operands of bitwise operators must be of type int");
            }
        
            expr->type = TYPE_INT;
            break;



        
        case NODE_NOT:
            left = expr->opr[0];
    
            expr_processing(left);

            if (left->type != TYPE_BOOL) {
                error_rule(expr, "1.31", "Operand of logical not operator must be of type bool");
            }

            expr->type = TYPE_BOOL;

            break;
        
        case NODE_UMINUS:
            left = expr->opr[0];

            expr_processing(left);

            if (left->type != TYPE_INT) {
                error_rule(expr, "1.31", "Operand of unary minus operator must be of type int");
            }

            expr->type = TYPE_INT;

            break;

        case NODE_BNOT:
            left = expr->opr[0];
    
            

            expr_processing(left);

            if (left->type != TYPE_INT) {
                error_rule(expr, "1.31", "Operand of bitwise not operator must be of type int");
            }

            expr->type = TYPE_INT;

            break;

        case NODE_IDENT:{

            node_t ident_node = get_decl_node(expr->ident);

            if (ident_node == NULL) {
                error_rule(expr, "1.61", "Variable '%s' not declared", expr->ident);
            }
            expr->type = ident_node->type;
            expr->offset = ident_node->offset;
            expr->decl_node = ident_node;
            expr->global_decl = ident_node->global_decl;
            break;
        }
        case NODE_AFFECT:
        
            left = expr->opr[0];
            right = expr->opr[1];

            expr_processing(left);
            expr_processing(right);

            if (left->nature != NODE_IDENT) {
                error_rule(expr, "1.32", "Left operand of assignment must be a variable");
            }

            if (left->type != right->type) {
                error_rule(expr, "1.32", "Type mismatch in assignment to variable '%s'", left->ident);
            }

            expr->type = left->type;

            break;
            


    default:
        break;
    }

}


//instruction processing
void instr_list_processing(node_t instr){
    if (instr == NULL) return;

    if (instr->nature == NODE_LIST) {
        instr_list_processing(instr->opr[0]);
        instr_list_processing(instr->opr[1]);
    } else if (instr->nature != NODE_LIST) {
        instr_processing(instr);

    }

}

void instr_processing(node_t instr){

    if (instr == NULL) return;

    node_t left, right;

   switch (instr->nature) {

    case NODE_IF: {
        left = instr->opr[0];
        right = instr->opr[1];
        node_t else_instr = (instr->nops == 3) ? instr->opr[2] : NULL;


        expr_processing(left);

        if (left->type != TYPE_BOOL) {
            error_rule(left, "1.18", "Condition in if statement must be of type bool");
        }

        instr_processing(right);


        if (else_instr) instr_processing(else_instr);

        break;
    }

    case NODE_WHILE: 
        left = instr->opr[0];
        right = instr->opr[1];

        expr_processing(left);
        if (left->type != TYPE_BOOL) {
            error_rule(left, "1.20", "Condition in while statement must be of type bool");
        }
        instr_processing(right);


        break;

    case NODE_FOR:{
        left = instr->opr[0];   
        node_t cond = instr->opr[1]; 
        node_t increment = instr->opr[2];
        right = instr->opr[3];

        expr_processing(left);
        expr_processing(cond);
        expr_processing(increment);

        if (cond->type != TYPE_BOOL) {
            error_rule(cond, "1.21", "Condition expression in for statement must be of type bool");
        }

        
        instr_processing(right);
        break;
    }
    case NODE_DOWHILE:
        left = instr->opr[0];
        right = instr->opr[1];

        expr_list_processing(right);

        if (right->type != TYPE_BOOL) {
            error_rule(right, "1.22", "Condition in do-while statement must be of type bool");
        }

        instr_processing(left);



        break;

    case NODE_BLOCK:
        block_decl(instr);
        break;

    case NODE_PRINT:
        
        print_list_processing(instr->opr[0]);

        break;

    default:
        expr_list_processing(instr); 
        break;  
    }
}

// Print instruction processing
void print_list_processing(node_t print_node) {
    if (print_node == NULL) return;

    if (print_node->nature == NODE_LIST) {
        print_list_processing(print_node->opr[0]);
        print_list_processing(print_node->opr[1]);
    } else if (print_node->nature != NODE_LIST) {
        print_processing(print_node);
    }
    
}

void print_processing(node_t print_node){
    if (print_node == NULL) return;

    
    if (print_node->nature == NODE_IDENT){
        node_t ident_node = get_decl_node(print_node->ident);
        if (ident_node == NULL) {
            error_rule(print_node, "1.61", "Variable '%s' not declared", print_node->ident);
        }

        print_node->type = ident_node->type;
        print_node->offset = ident_node->offset;
        print_node->decl_node = ident_node;
        print_node->global_decl = ident_node->global_decl;
    }    


    ;
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
  

