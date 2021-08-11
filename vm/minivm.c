#ifdef VM_DEBUG
#include <vm/debug.h>
#endif

#include <vm/vm.h>
#include <vm/vector.h>
#include <vm/gc.h>
#include <vm/gcvec.h>

#define VM_FRAME_NUM ((1 << 16))
#define VM_LOCALS_NUM ((1 << 24))
#define VM_GLOBALS_NUM ((256))

#ifdef __clang__
#define vm_assume(expr) (__builtin_assume(expr))
#else
#define vm_assume(expr) (__builtin_expect(expr, true))
#endif

#ifdef VM_DEBUG
#define next_op (vm_print_opcode(cur_index, basefunc), cur_index += 1, next_op_value)
#else
#define next_op (cur_index += 1, next_op_value)
#endif

#define vm_fetch (next_op_value = ptrs[basefunc[cur_index]])

#define vm_set_frame(frame_arg)          \
  (                                      \
      {                                  \
        stack_frame_t frame = frame_arg; \
        cur_index = frame.index;         \
        cur_func = frame.func;           \
      })

#define run_next_op goto *next_op;
#define cur_bytecode_next(Type)                   \
  (                                               \
      {                                           \
        Type ret = *(Type *)&basefunc[cur_index]; \
        cur_index += sizeof(Type);                \
        ret;                                      \
      })

#define read_reg (cur_bytecode_next(int))
#define read_bool (cur_bytecode_next(bool))
#define read_int (cur_bytecode_next(int))
#define read_num (cur_bytecode_next(int))
#define read_loc (cur_bytecode_next(int))

void vm_putn(long n)
{
  if (n < 0)
  {
    vm_putchar('-');
    vm_putn(-n);
  }
  else
  {
    if (n >= 10)
    {
      vm_putn(n / 10);
    }
    vm_putchar(n % 10 + '0');
  }
}

void vm_puts(const char *ptr)
{
  while (*ptr)
  {
    vm_putchar(*ptr);
    ptr += 1;
  }
}

void vm_putf(double num)
{
  if (vm_fmod(num, 1) == 0)
  {
    vm_putn((long)num);
  }
}

void vm_print(vm_gc_t *gc, nanbox_t val)
{
  if (nanbox_is_boolean(val))
  {
    bool log = nanbox_to_boolean(val);
    if (log)
    {
      vm_puts("true");
    }
    else
    {
      vm_puts("false");
    }
  }
  else if (nanbox_is_double(val))
  {
    number_t num = nanbox_to_double(val);
    vm_putf(num);
  }
  else
  {
    bool first = true;
    vm_putchar('[');
    int len = gcvec_size(gc, val);
    for (int i = 0; i < len; i++)
    {
      if (i != 0)
      {
        vm_putchar(',');
        vm_putchar(' ');
      }
      vm_print(gc, gcvec_get(gc, val, i));
    }
    vm_putchar(']');
  }
}

void vm_run(opcode_t *basefunc)
{

  int allocn = VM_FRAME_NUM;
  stack_frame_t *frames_base = vm_mem_alloc(sizeof(stack_frame_t) * allocn);
  int locals_allocated = VM_LOCALS_NUM;
  nanbox_t *locals_base = vm_mem_alloc(sizeof(nanbox_t) * locals_allocated);

  stack_frame_t *cur_frame = frames_base;
  nanbox_t *cur_locals = locals_base;
  int cur_index = 0;
  int cur_func = 0;

  vm_gc_t *gc = vm_gc_start(locals_base);

  void *next_op_value;
  void *ptrs[OPCODE_MAX2P] = {NULL};
  ptrs[OPCODE_EXIT] = &&do_exit;
  ptrs[OPCODE_STORE_REG] = &&do_store_reg;
  ptrs[OPCODE_STORE_LOG] = &&do_store_log;
  ptrs[OPCODE_STORE_NUM] = &&do_store_num;
  ptrs[OPCODE_STORE_FUN] = &&do_store_fun;
  ptrs[OPCODE_EQUAL] = &&do_equal;
  ptrs[OPCODE_EQUAL_NUM] = &&do_equal_num;
  ptrs[OPCODE_NOT_EQUAL] = &&do_not_equal;
  ptrs[OPCODE_NOT_EQUAL_NUM] = &&do_not_equal_num;
  ptrs[OPCODE_LESS] = &&do_less;
  ptrs[OPCODE_LESS_NUM] = &&do_less_num;
  ptrs[OPCODE_GREATER] = &&do_greater;
  ptrs[OPCODE_GREATER_NUM] = &&do_greater_num;
  ptrs[OPCODE_LESS_THAN_EQUAL] = &&do_less_than_equal;
  ptrs[OPCODE_LESS_THAN_EQUAL_NUM] = &&do_less_than_equal_num;
  ptrs[OPCODE_GREATER_THAN_EQUAL] = &&do_greater_than_equal;
  ptrs[OPCODE_GREATER_THAN_EQUAL_NUM] = &&do_greater_than_equal_num;
  ptrs[OPCODE_JUMP_ALWAYS] = &&do_jump_always;
  ptrs[OPCODE_JUMP_IF_FALSE] = &&do_jump_if_false;
  ptrs[OPCODE_JUMP_IF_TRUE] = &&do_jump_if_true;
  ptrs[OPCODE_JUMP_IF_EQUAL] = &&do_jump_if_equal;
  ptrs[OPCODE_JUMP_IF_EQUAL_NUM] = &&do_jump_if_equal_num;
  ptrs[OPCODE_JUMP_IF_NOT_EQUAL] = &&do_jump_if_not_equal;
  ptrs[OPCODE_JUMP_IF_NOT_EQUAL_NUM] = &&do_jump_if_not_equal_num;
  ptrs[OPCODE_JUMP_IF_LESS] = &&do_jump_if_less;
  ptrs[OPCODE_JUMP_IF_LESS_NUM] = &&do_jump_if_less_num;
  ptrs[OPCODE_JUMP_IF_GREATER] = &&do_jump_if_greater;
  ptrs[OPCODE_JUMP_IF_GREATER_NUM] = &&do_jump_if_greater_num;
  ptrs[OPCODE_JUMP_IF_LESS_THAN_EQUAL] = &&do_jump_if_less_than_equal;
  ptrs[OPCODE_JUMP_IF_LESS_THAN_EQUAL_NUM] = &&do_jump_if_less_than_equal_num;
  ptrs[OPCODE_JUMP_IF_GREATER_THAN_EQUAL] = &&do_jump_if_greater_than_equal;
  ptrs[OPCODE_JUMP_IF_GREATER_THAN_EQUAL_NUM] = &&do_jump_if_greater_than_equal_num;
  ptrs[OPCODE_INC] = &&do_inc;
  ptrs[OPCODE_INC_NUM] = &&do_inc_num;
  ptrs[OPCODE_DEC] = &&do_dec;
  ptrs[OPCODE_DEC_NUM] = &&do_dec_num;
  ptrs[OPCODE_ADD] = &&do_add;
  ptrs[OPCODE_ADD_NUM] = &&do_add_num;
  ptrs[OPCODE_SUB] = &&do_sub;
  ptrs[OPCODE_SUB_NUM] = &&do_sub_num;
  ptrs[OPCODE_MUL] = &&do_mul;
  ptrs[OPCODE_MUL_NUM] = &&do_mul_num;
  ptrs[OPCODE_DIV] = &&do_div;
  ptrs[OPCODE_DIV_NUM] = &&do_div_num;
  ptrs[OPCODE_MOD] = &&do_mod;
  ptrs[OPCODE_MOD_NUM] = &&do_mod_num;
  ptrs[OPCODE_CALL] = &&do_call;
  ptrs[OPCODE_STATIC_CALL] = &&do_static_call;
  ptrs[OPCODE_REC] = &&do_rec;
  ptrs[OPCODE_RETURN] = &&do_return;
  ptrs[OPCODE_PRINTLN] = &&do_println;
  ptrs[OPCODE_PUTCHAR] = &&do_putchar;
  ptrs[OPCODE_ARRAY] = &&do_array;
  ptrs[OPCODE_LENGTH] = &&do_length;
  ptrs[OPCODE_INDEX] = &&do_index;
  ptrs[OPCODE_INDEX_NUM] = &&do_index_num;
  cur_frame->nlocals = VM_GLOBALS_NUM;
  vm_fetch;
  run_next_op;
do_exit:
{
  vm_mem_free(frames_base);
  vm_mem_free(locals_base);
  vm_gc_stop(gc);
  return;
}
do_return:
{
  reg_t from = read_reg;
  nanbox_t val = cur_locals[from];
  cur_frame--;
  cur_locals -= cur_frame->nlocals;
  cur_func = cur_frame->func;
  cur_index = cur_frame->index;
  int outreg = cur_frame->outreg;
  cur_locals[outreg] = val;
  vm_fetch;
  run_next_op;
}
do_array:
{
  if (vec_size(gc->ptrs) >= gc->maxlen)
  {
    gc->stackptr = cur_locals + cur_frame->nlocals;
    vm_gc_run(gc);
    gc->maxlen = 2 + vec_size(gc->ptrs) * 8;
  }
  reg_t outreg = read_reg;
  int nargs = read_int;
  nanbox_t vec = gcvec_new(gc, nargs);
  for (int i = 0; i < nargs; i++)
  {
    reg_t reg = read_reg;
    gcvec_set(gc, vec, i, cur_locals[reg]);
  }
  vm_fetch;
  cur_locals[outreg] = vec;
  run_next_op;
}
do_length:
{
  reg_t outreg = read_reg;
  reg_t reg = read_reg;
  vm_fetch;
  nanbox_t vec = cur_locals[reg];
  cur_locals[outreg] = nanbox_from_double((double)gcvec_size(gc, vec));
  run_next_op;
}
do_index:
{
  reg_t outreg = read_reg;
  reg_t reg = read_reg;
  reg_t ind = read_reg;
  vm_fetch;
  number_t index = nanbox_to_double(cur_locals[ind]);
  nanbox_t vec = cur_locals[reg];
  cur_locals[outreg] = gcvec_get(gc, vec, (long)index);
  run_next_op;
}
do_index_num:
{
  reg_t outreg = read_reg;
  reg_t reg = read_reg;
  number_t index = read_num;
  vm_fetch;
  nanbox_t vec = cur_locals[reg];
  cur_locals[outreg] = gcvec_get(gc, vec, (long)index);
  run_next_op;
}
do_call:
{
  reg_t outreg = read_reg;
  reg_t func = read_reg;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals + cur_frame->nlocals;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t regno = read_reg;
    next_locals[argno] = cur_locals[regno];
  }
  int next_func = nanbox_to_int(cur_locals[func]);
  cur_locals = next_locals;
  cur_frame->index = cur_index;
  cur_frame->func = cur_func;
  cur_frame->outreg = outreg;
  cur_frame++;
  cur_index = next_func;
  cur_func = next_func;
  cur_frame->nlocals = read_int;
  vm_fetch;
  run_next_op;
}
do_static_call:
{
  reg_t outreg = read_reg;
  int next_func = read_loc;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals + cur_frame->nlocals;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t regno = read_reg;
    next_locals[argno] = cur_locals[regno];
  }
  cur_locals = next_locals;
  cur_frame->index = cur_index;
  cur_frame->func = cur_func;
  cur_frame->outreg = outreg;
  cur_frame++;
  cur_index = next_func;
  cur_func = next_func;
  cur_frame->nlocals = read_int;
  vm_fetch;
  run_next_op;
}
do_rec:
{
  reg_t outreg = read_reg;
  reg_t nargs = read_reg;
  nanbox_t *next_locals = cur_locals + cur_frame->nlocals;
  for (int argno = 0; argno < nargs; argno++)
  {
    reg_t regno = read_reg;
    next_locals[argno] = cur_locals[regno];
  }
  cur_locals = next_locals;
  cur_frame->index = cur_index;
  cur_frame->func = cur_func;
  cur_frame->outreg = outreg;
  cur_frame++;
  cur_index = cur_func;
  cur_frame->nlocals = read_int;
  vm_gc_stack_end_set(gc, cur_locals + cur_frame->nlocals);
  vm_fetch;
  run_next_op;
}
do_store_reg:
{
  reg_t to = read_reg;
  reg_t from = read_reg;
  vm_fetch;
  cur_locals[to] = cur_locals[from];
  run_next_op;
}
do_store_num:
{
  reg_t to = read_reg;
  number_t from = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(from);
  run_next_op;
}
do_store_log:
{
  reg_t to = read_reg;
  bool from = read_bool;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(from);
  run_next_op;
}
do_store_fun:
{
  reg_t to = read_reg;
  int func_end = read_loc;
  int head = cur_index;
  cur_index = func_end;
  vm_fetch;
  cur_locals[to] = nanbox_from_int(head);
  run_next_op;
}
do_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) == nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) == rhs);
  run_next_op;
}
do_not_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) != nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_not_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) != rhs);
  run_next_op;
}
do_less:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) < nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_less_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) < rhs);
  run_next_op;
}
do_greater:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) > nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_greater_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) > rhs);
  run_next_op;
}
do_less_than_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) <= nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_less_than_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) <= rhs);
  run_next_op;
}
do_greater_than_equal:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) >= nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_greater_than_equal_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_boolean(nanbox_to_double(cur_locals[lhs]) >= rhs);
  run_next_op;
}
do_jump_always:
{
  int to = read_loc;
  cur_index = to;
  vm_fetch;
  run_next_op;
}
do_jump_if_false:
{
  int to = read_loc;
  reg_t from = read_reg;
  if (nanbox_to_boolean(cur_locals[from]) == false)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_true:
{
  int to = read_loc;
  reg_t from = read_reg;
  if (nanbox_to_boolean(cur_locals[from]) == true)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) == nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) == rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_not_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) != nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_not_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) != rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) < nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) < rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_than_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) <= nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_less_than_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) <= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) > nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) >= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_than_equal:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  if (nanbox_to_double(cur_locals[lhs]) >= nanbox_to_double(cur_locals[rhs]))
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_jump_if_greater_than_equal_num:
{
  int to = read_loc;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  if (nanbox_to_double(cur_locals[lhs]) >= rhs)
  {
    cur_index = to;
  }
  vm_fetch;
  run_next_op;
}
do_inc:
{
  reg_t target = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) + nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_inc_num:
{
  reg_t target = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) + rhs);
  run_next_op;
}
do_dec:
{
  reg_t target = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) - nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_dec_num:
{
  reg_t target = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[target] = nanbox_from_double(nanbox_to_double(cur_locals[target]) - rhs);
  run_next_op;
}
do_add:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) + nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_add_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) + rhs);
  run_next_op;
}
do_mul:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) * nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_mul_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) * rhs);
  run_next_op;
}
do_sub:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) - nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_sub_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) - rhs);
  run_next_op;
}
do_div:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) / nanbox_to_double(cur_locals[rhs]));
  run_next_op;
}
do_div_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(nanbox_to_double(cur_locals[lhs]) / rhs);
  run_next_op;
}
do_mod:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  reg_t rhs = read_reg;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(vm_fmod(nanbox_to_double(cur_locals[lhs]), nanbox_to_double(cur_locals[rhs])));
  run_next_op;
}
do_mod_num:
{
  reg_t to = read_reg;
  reg_t lhs = read_reg;
  number_t rhs = read_num;
  vm_fetch;
  cur_locals[to] = nanbox_from_double(vm_fmod(nanbox_to_double(cur_locals[lhs]), rhs));
  run_next_op;
}
do_println:
{
  reg_t from = read_reg;
  vm_fetch;
  nanbox_t val = cur_locals[from];
  vm_print(gc, val);
  vm_putchar('\n');
  run_next_op;
}
do_putchar:
{
  reg_t from = read_reg;
  vm_fetch;
  char val = (char)nanbox_to_double(cur_locals[from]);
  vm_putchar(val);
  run_next_op;
}
}