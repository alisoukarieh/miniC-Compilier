
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "defs.h"
#include "passe_2.h"
#include "miniccutils.h"
#include "arch.h"

extern int trace_level;


// Pre-pass to collect string literals
static void collect_strings(node_t node) {
    if (node == NULL) return;

    if (node->nature == NODE_STRINGVAL) {
        node->offset = add_string(node->str);
        return;
    }

    for (int i = 0; i < node->nops; i++) {
        collect_strings(node->opr[i]);
    }
}


// Data section generation
static void gen_global_decls(node_t decls) {
    if (decls == NULL) return;

    switch (decls->nature) {
        case NODE_LIST:
            gen_global_decls(decls->opr[0]);
            gen_global_decls(decls->opr[1]);
            break;

        case NODE_DECLS:
            gen_global_decls(decls->opr[1]);
            break;

        case NODE_DECL: {
            node_t ident = decls->opr[0];
            node_t init = decls->opr[1];
            int32_t init_value = 0;

            if (init != NULL && (init->nature == NODE_INTVAL || init->nature == NODE_BOOLVAL)) {
                init_value = (int32_t)init->value;
            }
            create_word_inst(ident->ident, init_value);
            break;
        }

        default:
            break;
    }
}

static void gen_data_section(node_t root) {
    create_data_sec_inst();

    if (root->opr[0] != NULL) {
        gen_global_decls(root->opr[0]);
    }

    int32_t num_strings = get_global_strings_number();
    for (int32_t i = 0; i < num_strings; i++) {
        create_asciiz_inst(NULL, get_global_string(i));
    }
}


// Expression generation
static void gen_expr(node_t expr) {
    if (expr == NULL) return;

    int32_t reg, reg_left, reg_right;

    switch (expr->nature) {
        case NODE_INTVAL:
        case NODE_BOOLVAL:
            reg = get_current_reg();
            if (expr->value >= 0 && expr->value <= 0xFFFF) {
                // Small positive value: single ori instruction
                create_ori_inst(reg, get_r0(), (int32_t)(expr->value & 0xFFFF));
            } else {
                // Full 32-bit value: lui + ori
                create_lui_inst(reg, (int32_t)((expr->value >> 16) & 0xFFFF));
                create_ori_inst(reg, reg, (int32_t)(expr->value & 0xFFFF));
            }
            break;

        case NODE_IDENT: {
            node_t decl = expr->decl_node;
            reg = get_current_reg();

            if (decl != NULL && decl->global_decl) {
                create_lui_inst(reg, 0x1001);
                create_lw_inst(reg, decl->offset, reg);
            } else {
                create_lw_inst(reg, expr->offset, get_stack_reg());
            }
            break;
        }

        case NODE_AFFECT: {
            gen_expr(expr->opr[1]);
            reg = get_current_reg();

            node_t left = expr->opr[0];
            node_t decl = left->decl_node;

            if (decl != NULL && decl->global_decl) {
                allocate_reg();
                int32_t tmp_reg = get_current_reg();
                create_lui_inst(tmp_reg, 0x1001);
                create_sw_inst(reg, decl->offset, tmp_reg);
                release_reg();
            } else {
                create_sw_inst(reg, left->offset, get_stack_reg());
            }
            break;
        }

        case NODE_PLUS: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_addu_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_addu_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_MINUS: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_subu_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_subu_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_MUL: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_mult_inst(get_restore_reg(), reg_right);
                create_mflo_inst(reg_right);
            } else {
                create_mult_inst(reg_left, reg_right);
                create_mflo_inst(reg_left);
                release_reg();
            }
            break;
        }

        case NODE_DIV: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_div_inst(get_restore_reg(), reg_right);
                create_teq_inst(reg_right, get_r0());
                create_mflo_inst(reg_right);
            } else {
                create_div_inst(reg_left, reg_right);
                create_teq_inst(reg_right, get_r0());
                create_mflo_inst(reg_left);
                release_reg();
            }
            break;
        }

        case NODE_MOD: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_div_inst(get_restore_reg(), reg_right);
                create_teq_inst(reg_right, get_r0());
                create_mfhi_inst(reg_right);
            } else {
                create_div_inst(reg_left, reg_right);
                create_teq_inst(reg_right, get_r0());
                create_mfhi_inst(reg_left);
                release_reg();
            }
            break;
        }

        case NODE_LT: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_slt_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_slt_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_GT: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_slt_inst(reg_right, reg_right, get_restore_reg());
            } else {
                create_slt_inst(reg_left, reg_right, reg_left);
                release_reg();
            }
            break;
        }

        case NODE_LE: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_slt_inst(reg_right, reg_right, get_restore_reg());
                create_xori_inst(reg_right, reg_right, 1);
            } else {
                create_slt_inst(reg_left, reg_right, reg_left);
                create_xori_inst(reg_left, reg_left, 1);
                release_reg();
            }
            break;
        }

        case NODE_GE: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_slt_inst(reg_right, get_restore_reg(), reg_right);
                create_xori_inst(reg_right, reg_right, 1);
            } else {
                create_slt_inst(reg_left, reg_left, reg_right);
                create_xori_inst(reg_left, reg_left, 1);
                release_reg();
            }
            break;
        }

        case NODE_EQ: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_xor_inst(reg_right, get_restore_reg(), reg_right);
                create_sltiu_inst(reg_right, reg_right, 1);
            } else {
                create_xor_inst(reg_left, reg_left, reg_right);
                create_sltiu_inst(reg_left, reg_left, 1);
                release_reg();
            }
            break;
        }

        case NODE_NE: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_xor_inst(reg_right, get_restore_reg(), reg_right);
                create_sltu_inst(reg_right, get_r0(), reg_right);
            } else {
                create_xor_inst(reg_left, reg_left, reg_right);
                create_sltu_inst(reg_left, get_r0(), reg_left);
                release_reg();
            }
            break;
        }

        case NODE_AND: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_and_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_and_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_OR: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_or_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_or_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_NOT:
            gen_expr(expr->opr[0]);
            reg = get_current_reg();
            create_xori_inst(reg, reg, 1);
            break;

        case NODE_BAND: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_and_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_and_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_BOR: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_or_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_or_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_BXOR: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_xor_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_xor_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_BNOT:
            gen_expr(expr->opr[0]);
            reg = get_current_reg();
            create_nor_inst(reg, get_r0(), reg);
            break;

        case NODE_SLL: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_sllv_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_sllv_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_SRA: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_srav_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_srav_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_SRL: {
            gen_expr(expr->opr[0]);
            reg_left = get_current_reg();
            bool spilled = !reg_available();
            if (spilled) {
                push_temporary(reg_left);
            }
            allocate_reg();
            gen_expr(expr->opr[1]);
            reg_right = get_current_reg();
            if (spilled) {
                pop_temporary(get_restore_reg());
                create_srlv_inst(reg_right, get_restore_reg(), reg_right);
            } else {
                create_srlv_inst(reg_left, reg_left, reg_right);
                release_reg();
            }
            break;
        }

        case NODE_UMINUS:
            gen_expr(expr->opr[0]);
            reg = get_current_reg();
            create_subu_inst(reg, get_r0(), reg);
            break;

        default:
            break;
    }
}


// Print generation
static void gen_print_item(node_t item) {
    if (item == NULL) return;

    if (item->nature == NODE_STRINGVAL) {
        create_lui_inst(4, 0x1001);
        create_ori_inst(4, 4, item->offset);
        create_ori_inst(2, get_r0(), 0x4);
        create_syscall_inst();
    } else {
        if (item->nature == NODE_IDENT) {
            node_t decl = item->decl_node;
            if (decl != NULL && decl->global_decl) {
                create_lui_inst(4, 0x1001);
                create_lw_inst(4, decl->offset, 4);
            } else {
                create_lw_inst(4, item->offset, get_stack_reg());
            }
        } else {
            gen_expr(item);
            create_addu_inst(4, get_current_reg(), get_r0());
        }
        create_ori_inst(2, get_r0(), 0x1);
        create_syscall_inst();
    }
}

static void gen_print_list(node_t list) {
    if (list == NULL) return;

    if (list->nature == NODE_LIST) {
        gen_print_list(list->opr[0]);
        gen_print_item(list->opr[1]);
    } else {
        gen_print_item(list);
    }
}

static void gen_print(node_t node) {
    gen_print_list(node->opr[0]);
}


// Control flow generation
static void gen_if(node_t node);
static void gen_while(node_t node);
static void gen_for(node_t node);
static void gen_dowhile(node_t node);
static void gen_block(node_t block);

static void gen_instr(node_t instr) {
    if (instr == NULL) return;

    switch (instr->nature) {
        case NODE_IF:
            gen_if(instr);
            break;
        case NODE_WHILE:
            gen_while(instr);
            break;
        case NODE_FOR:
            gen_for(instr);
            break;
        case NODE_DOWHILE:
            gen_dowhile(instr);
            break;
        case NODE_BLOCK:
            gen_block(instr);
            break;
        case NODE_PRINT:
            gen_print(instr);
            break;
        default:
            gen_expr(instr);
            break;
    }
}

static void gen_instr_list(node_t instr) {
    if (instr == NULL) return;

    if (instr->nature == NODE_LIST) {
        gen_instr_list(instr->opr[0]);
        gen_instr_list(instr->opr[1]);
    } else {
        gen_instr(instr);
    }
}

static void gen_if(node_t node) {
    int32_t label_else = get_new_label();

    gen_expr(node->opr[0]);
    create_beq_inst(get_current_reg(), get_r0(), label_else);

    gen_instr(node->opr[1]);

    if (node->opr[2] != NULL) {
        // Only allocate label_end when there's an else clause
        int32_t label_end = get_new_label();
        create_j_inst(label_end);
        create_label_inst(label_else);
        gen_instr(node->opr[2]);
        create_label_inst(label_end);
    } else {
        create_label_inst(label_else);
    }
}

static void gen_while(node_t node) {
    int32_t label_start = get_new_label();
    int32_t label_end = get_new_label();

    create_label_inst(label_start);

    gen_expr(node->opr[0]);
    create_beq_inst(get_current_reg(), get_r0(), label_end);

    gen_instr(node->opr[1]);

    create_j_inst(label_start);
    create_label_inst(label_end);
}

static void gen_for(node_t node) {
    int32_t label_start = get_new_label();
    int32_t label_end = get_new_label();

    if (node->opr[0] != NULL) {
        gen_expr(node->opr[0]);
    }

    create_label_inst(label_start);

    if (node->opr[1] != NULL) {
        gen_expr(node->opr[1]);
        create_beq_inst(get_current_reg(), get_r0(), label_end);
    }

    gen_instr(node->opr[3]);

    if (node->opr[2] != NULL) {
        gen_expr(node->opr[2]);
    }

    create_j_inst(label_start);
    create_label_inst(label_end);
}

static void gen_dowhile(node_t node) {
    int32_t label_start = get_new_label();

    create_label_inst(label_start);
    gen_instr(node->opr[0]);

    gen_expr(node->opr[1]);
    create_bne_inst(get_current_reg(), get_r0(), label_start);
}


// Block and local declarations
static void gen_local_decls(node_t decls) {
    if (decls == NULL) return;

    switch (decls->nature) {
        case NODE_LIST:
            gen_local_decls(decls->opr[0]);
            gen_local_decls(decls->opr[1]);
            break;

        case NODE_DECLS:
            gen_local_decls(decls->opr[1]);
            break;

        case NODE_DECL: {
            node_t ident = decls->opr[0];
            node_t init = decls->opr[1];

            if (init != NULL) {
                gen_expr(init);
                create_sw_inst(get_current_reg(), ident->offset, get_stack_reg());
            }
            break;
        }

        default:
            break;
    }
}

static void gen_block(node_t block) {
    if (block == NULL) return;

    if (block->opr[0] != NULL) {
        gen_local_decls(block->opr[0]);
    }

    if (block->opr[1] != NULL) {
        gen_instr_list(block->opr[1]);
    }
}


// Text section generation
static void gen_text_section(node_t func) {
    create_text_sec_inst();
    create_label_str_inst("main");

    // Prologue
    set_temporary_start_offset(func->offset);
    create_stack_allocation_inst();

    if (func->opr[2] != NULL) {
        gen_block(func->opr[2]);
    }

    // Epilogue - also updates the allocation instruction with correct size
    // Use max of local vars (func->offset) and temporaries (get_temporary_max_offset())
    int32_t stack_size = get_temporary_max_offset();
    if (func->offset > stack_size) {
        stack_size = func->offset;
    }
    create_stack_deallocation_inst(stack_size);
    create_ori_inst(2, get_r0(), 0xa);
    create_syscall_inst();
}


// Main entry point
void gen_code_passe_2(node_t root) {
    collect_strings(root);

    set_max_registers(get_num_registers());
    reset_temporary_max_offset();

    gen_data_section(root);

    if (root->opr[1] != NULL) {
        gen_text_section(root->opr[1]);
    }
}
