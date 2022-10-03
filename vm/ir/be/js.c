
#include "../ir.h"

void vm_ir_be_js_block(FILE *of, vm_ir_block_t *arg) {
    fprintf(of, "return()=>l%li(", arg->id);
    for (size_t i = 0; i < arg->nargs; i++) {
        if (i != 0) {
            fprintf(of, ",");
        }
        fprintf(of, "r%zu", arg->args[i]);
    }
    fprintf(of, ");");
}

void vm_ir_be_js_arg(FILE *of, vm_ir_arg_t arg) {
    switch (arg.type) {
        case VM_IR_ARG_NIL: {
            fprintf(of, "undefined");
            break;
        }
        case VM_IR_ARG_BOOL: {
            if (arg.logic) {
                fprintf(of, "true");
            } else {
                fprintf(of, "false");
            }
            break;
        }
        case VM_IR_ARG_REG: {
            fprintf(of, "r%zu", arg.reg);
            break;
        }
        case VM_IR_ARG_NUM: {
            fprintf(of, "%lf", arg.num);
            break;
        }
        case VM_IR_ARG_STR: {
            __builtin_trap();
        }
        case VM_IR_ARG_EXTERN: {
            __builtin_trap();
        }
        case VM_IR_ARG_FUNC: {
            fprintf(of, "l%zi", arg.func->id);
            break;
        }
    }
}

void vm_ir_be_js(FILE *of, size_t nargs, vm_ir_block_t *blocks) {
    fprintf(of, "const fs=require('fs');\n");
    fprintf(of, "const call=(f)=>{while (f instanceof Function){f=f();}return f;};\n");
    fprintf(of, "const putchar=(c)=>{process.stdout.write(String.fromCharCode(c))};\n");
    fprintf(of, "const getchar=()=>{const buffer=Buffer.alloc(1);fs.readSync(0, buffer, 0, 1);return buffer.toString('utf8').charCodeAt(0);};\n");
    for (size_t nblock = 0; nblock < nargs; nblock++) {
        vm_ir_block_t *block = &blocks[nblock];
        if (block->id < 0) {
            continue;
        }
        uint8_t *known = vm_alloc0(sizeof(size_t) * block->nregs);
        fprintf(of, "const l%zi = (", block->id);
        for (int i = 0; i < block->nargs; i++) {
            if (i != 0) {
                fprintf(of, ",");
            }
            fprintf(of, "r%zu", block->args[i]);
            known[block->args[i]] = 1;
        }
        fprintf(of, ") => {");
        for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
            vm_ir_instr_t *instr = block->instrs[ninstr];
            switch (instr->op) {
                case VM_IR_IOP_NOP: {
                    break;
                }
                case VM_IR_IOP_MOVE: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_ADD: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "+");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_SUB: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "-");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_MUL: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "*");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_DIV: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "/");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_MOD: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "%%");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_CALL: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=call(");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "(");
                    for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                        if (i != 1) {
                            fprintf(of, ",");
                        }
                        vm_ir_be_js_arg(of, instr->args[i]);
                    }
                    fprintf(of, "));");
                    break;
                }
                case VM_IR_IOP_ARR: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "={length:");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "};");
                    break;
                }
                case VM_IR_IOP_TAB: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=Object.create(null);");
                    break;
                }
                case VM_IR_IOP_GET: {
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "[");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, "];");
                    break;
                }
                case VM_IR_IOP_SET: {
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "[");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, "]=");
                    vm_ir_be_js_arg(of, instr->args[2]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_LEN: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, ".length;");
                    break;
                }
                case VM_IR_IOP_TYPE: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=typeof ");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, ".length;");
                    __builtin_trap();
                }
                case VM_IR_IOP_OUT: {
                    fprintf(of, "putchar(");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, ");");
                    break;
                }
                case VM_IR_IOP_IN: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=getchar();");
                    break;
                }
                case VM_IR_IOP_BOR: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "|");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_BAND: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "&");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_BXOR: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "^");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_BSHL: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, ">>");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
                case VM_IR_IOP_BSHR: {
                    fprintf(of, "var ");
                    vm_ir_be_js_arg(of, instr->out);
                    fprintf(of, "=");
                    vm_ir_be_js_arg(of, instr->args[0]);
                    fprintf(of, "<<");
                    vm_ir_be_js_arg(of, instr->args[1]);
                    fprintf(of, ";");
                    break;
                }
            }
        }
        switch (block->branch->op) {
            case VM_IR_BOP_JUMP: {
                vm_ir_be_js_block(of, block->branch->targets[0]);
                break;
            }
            case VM_IR_BOP_BOOL: {
                fprintf(of, "if(");
                vm_ir_be_js_arg(of, block->branch->args[0]);
                fprintf(of, "){");
                vm_ir_be_js_block(of, block->branch->targets[1]);
                fprintf(of, "}else{");
                vm_ir_be_js_block(of, block->branch->targets[0]);
                fprintf(of, "}");
                break;
            }
            case VM_IR_BOP_LESS: {
                fprintf(of, "if(");
                vm_ir_be_js_arg(of, block->branch->args[0]);
                fprintf(of, "<");
                vm_ir_be_js_arg(of, block->branch->args[1]);
                fprintf(of, "){");
                vm_ir_be_js_block(of, block->branch->targets[1]);
                fprintf(of, "}else{");
                vm_ir_be_js_block(of, block->branch->targets[0]);
                fprintf(of, "}");
                break;
            }
            case VM_IR_BOP_EQUAL: {
                fprintf(of, "if(");
                vm_ir_be_js_arg(of, block->branch->args[0]);
                fprintf(of, "===");
                vm_ir_be_js_arg(of, block->branch->args[1]);
                fprintf(of, "){");
                vm_ir_be_js_block(of, block->branch->targets[1]);
                fprintf(of, "}else{");
                vm_ir_be_js_block(of, block->branch->targets[0]);
                fprintf(of, "}");
                break;
            }
            case VM_IR_BOP_RET: {
                fprintf(of, "return ");
                vm_ir_be_js_arg(of, block->branch->args[0]);
                fprintf(of, ";");
                break;
            }
            case VM_IR_BOP_EXIT: {
                fprintf(of, "exit();");
                break;
            }
        }
        fprintf(of, "};\n");
        vm_free(known);
    }
    fprintf(of, "call(()=>l0());");
}
