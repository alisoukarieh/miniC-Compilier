
#ifndef _PASSE_1_
#define _PASSE_1_

#include "defs.h"


void analyse_passe_1(node_t root);

void decl_list(node_t decl, node_type type, bool is_global) ;
void block_decl(node_t block);
void expr_list_processing(node_t expr);
void instr_list_processing(node_t instr);
void main_decl(node_t func_node);
void expr_processing(node_t expr);
void instr_processing(node_t instr);


#endif

