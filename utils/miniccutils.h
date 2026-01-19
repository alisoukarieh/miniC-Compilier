#ifndef _MINICCUTILS_H_
#define _MINICCUTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "../defs.h"



/* For checking the tree constructed */

bool check_program_tree(node_t n);


/* Context related functions */

typedef struct _context_s context_s;
typedef context_s * context_t;

context_t create_context();
void free_context(context_t context);
bool context_add_element(context_t context, char * idf, void * data);
void * get_data(context_t context, char * idf);


/* Environment related functions */

void push_global_context();
void push_context();
void pop_context();
void * get_decl_node(char * ident);
int32_t env_add_element(char * ident, void * node);
void reset_env_current_offset();
int32_t get_env_current_offset();
int32_t add_string(char * str);
int32_t get_global_strings_number();
char * get_global_string(int32_t index);
void free_global_strings();


/* Register allocation related functions */

void push_temporary(int32_t reg);
void pop_temporary(int32_t reg);
bool reg_available();
int32_t get_current_reg();
int32_t get_restore_reg();
void allocate_reg();
void release_reg();
int32_t get_new_label();
void set_temporary_start_offset(int32_t offset);
void set_max_registers(int32_t n);
void reset_temporary_max_offset();
int32_t get_temporary_max_offset();
int32_t get_temporary_curr_offset(); // for debug


/* Program creation related functions */

void create_data_sec_inst();
void create_text_sec_inst();
void create_word_inst(char * label, int32_t init_value);
void create_asciiz_inst(char * label_str, char * str);
void create_label_inst(int32_t label);
void create_label_str_inst(char * label);
void create_comment_inst(char * comment);
void create_lui_inst(int32_t r_dest, int32_t imm);
void create_addu_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_subu_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_slt_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_sltu_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_and_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_or_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_xor_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_nor_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_mult_inst(int32_t r_src_1, int32_t r_src_2);
void create_div_inst(int32_t r_src_1, int32_t r_src_2);
void create_sllv_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_srav_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_srlv_inst(int32_t r_dest, int32_t r_src_1, int32_t r_src_2);
void create_addiu_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_andi_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_ori_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_xori_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_slti_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_sltiu_inst(int32_t r_dest, int32_t r_src_1, int32_t imm);
void create_lw_inst(int32_t r_dest, int32_t imm, int32_t r_src_1);
void create_sw_inst(int32_t r_src_1, int32_t imm, int32_t r_src_2);
void create_beq_inst(int32_t r_src_1, int32_t r_src_2, int32_t label);
void create_bne_inst(int32_t r_src_1, int32_t r_src_2, int32_t label);
void create_mflo_inst(int32_t r_dest);
void create_mfhi_inst(int32_t r_dest);
void create_j_inst(int32_t label);
void create_teq_inst(int32_t r_src_1, int32_t r_src_2);
void create_syscall_inst();
void create_stack_allocation_inst();
void create_stack_deallocation_inst(int32_t val);

void create_program();
void free_program();
void dump_mips_program(char * filename);




#endif

