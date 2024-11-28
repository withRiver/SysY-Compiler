#pragma once
#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include "koopa.h"

#define IN_IMM12(x) (((x) >= -2048) && ((x) <= 2047)) 

// 栈帧 {inst, location}
static std::unordered_map<koopa_raw_value_t, int> stack_frame;
static int sf_size = 0 ;
static int sf_index = 0;


// 访问raw program
void Visit(const koopa_raw_program_t& program);
// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice);
// 访问函数
void Visit(const koopa_raw_function_t &func);
// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb);
// 访问指令
void Visit(const koopa_raw_value_t &value);
// 访问 return 指令
void Visit(const koopa_raw_return_t &ret);
// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer);
// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &dest);
// load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &dest);
// store 指令
void Visit(const koopa_raw_store_t &store);
// branch
void Visit(const koopa_raw_branch_t &branch);
//jump
void Visit(const koopa_raw_jump_t &jump);

// 将栈帧中value的值写到寄存器reg_name中
void write_reg(const koopa_raw_value_t &value, const std::string &reg_name);
// 将寄存器reg_name的值储存到栈帧中的value
void save_reg(const koopa_raw_value_t &value, const std::string &reg_name);

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  if(func->bbs.len == 0) return;
  // 执行一些其他的必要操作
  std::cout << "  .text" << std::endl;
  std::cout << "  .globl " << func->name + 1 << std::endl;
  std::cout << func->name + 1 << ":" << std::endl;

  // 清空栈帧
  sf_size = sf_index = 0;
  stack_frame.clear();
  // 计算该函数的栈帧大小
  for(size_t i = 0; i < func->bbs.len; ++i) {
    auto block = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    sf_size += 4 * (block->insts.len);
    for(size_t j = 0; j < block->insts.len; ++j) {
      auto inst = reinterpret_cast<koopa_raw_value_t>(block->insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT) {
        sf_size -= 4;
      }
    }
  }
  int mod = sf_size % 16;
  sf_size += ((mod == 0) ? 0 : (16 - mod));

  if(sf_size > 0 && sf_size <= 2048) {
    std::cout << "  addi sp, sp, -" << sf_size << std::endl;
  } else if (sf_size > 2048) {
    std::cout << "  li t0, " << -sf_size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }

  // 访问所有基本块
  Visit(func->bbs);
  std::cout << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // 打印基本块入口
  if(strncmp(bb->name + 1, "LHR_entry", 9) != 0) {
    std::cout << bb->name + 1 << ":" << std::endl;
  }

  // 访问所有指令
  Visit(bb->insts);

  std::cout << std::endl;
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  /*
  struct koopa_raw_value_data {
  /// Type of value.
  koopa_raw_type_t ty;
  /// Name of value, null if no name.
  const char *name;
  /// Values that this value is used by.
  koopa_raw_slice_t used_by;
  /// Kind of value.
  koopa_raw_value_kind_t kind;
};
  */
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_INTEGER:
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_ALLOC:
      stack_frame[value] = sf_index; 
      sf_index += 4;
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_BINARY:
      Visit(kind.data.binary, value);
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_RETURN:
      Visit(kind.data.ret);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问integer
void Visit(const koopa_raw_integer_t &integer) {
  int32_t int_val = integer.value;
  std::cout << "  li a0, " << int_val << "\n";
} 

// 访问load
// 处理imm12, 若越界则
// li t3, imm12
// add t3, sp, t3
// sw dest, t3
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &dest) {
  write_reg(load.src, "t0");
  stack_frame[dest] = sf_index; sf_index += 4;
  save_reg(dest, "t0");
}

// 访问store
void Visit(const koopa_raw_store_t &store) {
  write_reg(store.value, "t0");
  save_reg(store.dest, "t0");
}

// 访问binary
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &dest) {
  write_reg(binary.lhs, "t0"); write_reg(binary.rhs, "t1");
  switch(binary.op) {
    /// Not equal to. (xor, snez)
    case KOOPA_RBO_NOT_EQ:
      std::cout << "  xor t0, t0, t1" << std::endl;
      std::cout << "  snez t0, t0" << std::endl;
      break;
  /// Equal to. (xor, seqz)
    case KOOPA_RBO_EQ:
      std::cout << "  xor t0, t0, t1" << std::endl;
      std::cout << "  seqz t0, t0" << std::endl;
      break;
  /// Greater than. (sgt)
    case KOOPA_RBO_GT:
      std::cout << "  sgt t0, t0, t1" << std::endl;
      break;
  /// Less than. (slt)
    case KOOPA_RBO_LT: 
      std::cout << "  slt t0, t0, t1" << std::endl;
      break;
  /// Greater than or equal to. (slt, seqz)
    case KOOPA_RBO_GE:
      std::cout << "  slt t0, t0, t1" << std::endl;
      std::cout << "  seqz t0, t0" << std::endl;
      break;      
  /// Less than or equal to. (sgt, seqz)
    case KOOPA_RBO_LE:
      std::cout << "  sgt t0, t0, t1" << std::endl;
      std::cout << "  seqz t0, t0" << std::endl;
      break;     
  /// Addition. (add/addi)
    case KOOPA_RBO_ADD:
      std::cout << "  add t0, t0, t1" << std::endl;
      break;
  /// Subtraction. (sub)
    case KOOPA_RBO_SUB:
      std::cout << "  sub t0, t0, t1" << std::endl;  
      break;
  /// Multiplication. (mul)
    case KOOPA_RBO_MUL:
      std::cout << "  mul t0, t0, t1" << std::endl;
      break;
  /// Division. (div)
    case KOOPA_RBO_DIV:
      std::cout << "  div t0, t0, t1" << std::endl;
      break;
  /// Modulo. (rem)
    case KOOPA_RBO_MOD:
      std::cout << "  rem t0, t0, t1" << std::endl;
      break;
  /// Bitwise AND. (and/andi)
    case KOOPA_RBO_AND:
      std::cout << "  and t0, t0, t1" << std::endl;
      break;    
  /// Bitwise OR. (or/ori)
    case KOOPA_RBO_OR:
      std::cout << "  or t0, t0, t1" << std::endl;
      break;      
  /// Bitwise XOR. (xor/xori)
    case KOOPA_RBO_XOR:
      std::cout << "  xor t0, t0, t1" << std::endl;
      break;
  /// Shift left logical (sll)
    case KOOPA_RBO_SHL:
      assert(0); break;
  /// Shift right logical. (srl)
    case KOOPA_RBO_SHR:
      assert(0); break;
  /// Shift right arithmetic. (sra)
    case KOOPA_RBO_SAR: break;
    default: break;
  }
  stack_frame[dest] = sf_index; sf_index += 4;
  save_reg(dest, "t0");
}

// 访问return
void Visit(const koopa_raw_return_t &ret) {
  write_reg(ret.value, "a0");
  if(sf_size > 0 && sf_size <= 2048) {
    std::cout << "  addi sp, sp, " << sf_size << std::endl;
  } else if(sf_size > 2048) {
    std::cout << "  li t0, " << sf_size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;  
  }
  std::cout << "  ret\n";
}

// branch
void Visit(const koopa_raw_branch_t &branch) {
  write_reg(branch.cond, "t0");
  std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
  std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
}

// jump
void Visit(const koopa_raw_jump_t &jump) {
  std::cout << "  j " << jump.target->name + 1 << std::endl;
}


void write_reg(const koopa_raw_value_t &value, const std::string &reg_name) {
  const auto& kind = value->kind;
  int offset;
  switch(kind.tag) {
    case KOOPA_RVT_INTEGER:
      std::cout << "  li " << reg_name << ", " << kind.data.integer.value << std::endl;
      break;
    default:
      offset = stack_frame[value];
      if(IN_IMM12(offset)) {
        std::cout << "  lw " << reg_name << ", " << offset << "(sp)\n";
      } else {
        std::cout << "  li t3, " << offset << std::endl;
        std::cout << "  add t3, sp, t3" << std::endl;
        std::cout << "  lw " << reg_name << ", 0(t3)" << std::endl; 
      }
      break;
  }
} 

void save_reg(const koopa_raw_value_t &value, const std::string &reg_name) {
  int offset = stack_frame[value];
  if(IN_IMM12(offset)) {
    std::cout << "  sw " << reg_name << ", " << offset << "(sp)\n";
  } else {
    std::cout << "  li t3, " << offset << std::endl;
    std::cout << "  add t3, sp, t3" << std::endl;
    std::cout << "  sw " << reg_name << ", 0(t3)" << std::endl; 
  }
}