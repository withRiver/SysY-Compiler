#pragma once
#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include <vector>
#include <unordered_map>
#include "koopa.h"

#define IN_IMM12(x) (((x) >= -2048) && ((x) <= 2047)) 
#define ALIGN_TO_16(x) (((x) + 15) & (~15))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// 栈帧 {inst, location}
static std::unordered_map<koopa_raw_value_t, int> stack_frame;
static int sf_size = 0 ;
static int sf_index = 0;

// 函数是否需要保存ra
static int save_ra = 0;

// 数组或指针的维度长度
static std::unordered_map<koopa_raw_value_t, std::vector<int>> array_dims;
// 每个value对应的维度区间
typedef std::pair<std::vector<int>::iterator, std::vector<int>::iterator> range_t;
static std::unordered_map<koopa_raw_value_t, range_t> array_ranges;

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
// 访问 integer 指令
void Visit(const koopa_raw_integer_t &integer);
// 访问 alloc 指令
void VisitAlloc(const koopa_raw_value_t &alloc);
// 访问 global_alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value);
// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &dest);
// 访问 store 指令
void Visit(const koopa_raw_store_t &store);
// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t& dest);
// 访问 getelemptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t& dest);
// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &dest);
// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch);
// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump);
// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value);
// 访问 return 指令
void Visit(const koopa_raw_return_t &ret);

// utilities
// 将栈帧中value的值写到寄存器reg_name中
void write_reg(const koopa_raw_value_t &value, const std::string &reg_name);
// 将寄存器reg_name的值储存到栈帧中的value
void save_reg(const koopa_raw_value_t &value, const std::string &reg_name);
// 将value的地址写到reg_name中
void write_addr_reg(const koopa_raw_value_t &value, const std::string &reg_name);
// value是否是一个*...
int isPointer(const koopa_raw_value_t &value);
// 全局数组初始化
void global_array_init(const koopa_raw_value_t &value);
// base type的大小
int base_size(const range_t& se);

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // ...
  array_dims.clear();
  array_ranges.clear();

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
  if(func->bbs.len == 0) {
    return;
  }

  // 执行一些其他的必要操作
  std::cout << "  .text" << std::endl;
  std::cout << "  .globl " << func->name + 1 << std::endl;
  std::cout << func->name + 1 << ":" << std::endl;

  // 清空栈帧
  sf_size = sf_index = 0;
  stack_frame.clear();

  int S = 0, R = 0, A = 0;

  // 计算该函数的栈帧大小
  for(size_t i = 0; i < func->bbs.len; ++i) {
    auto block = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
    S += 4 * (block->insts.len);
    for(size_t j = 0; j < block->insts.len; ++j) {
      auto inst = reinterpret_cast<koopa_raw_value_t>(block->insts.buffer[j]);
      if(inst->ty->tag == KOOPA_RTT_UNIT) {
        S -= 4;
      } 
      if(inst->kind.tag == KOOPA_RVT_CALL) {
        R = 4;
        A = max(A, 4 * max(0, int(inst->kind.data.call.args.len) - 8));
      } else if(inst->kind.tag == KOOPA_RVT_ALLOC && inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY) {
        S -= 4;
        int cur_sz = 4;
        auto base = inst->ty->data.pointer.base;
        while(base->tag == KOOPA_RTT_ARRAY) {
          cur_sz *= base->data.array.len; 
          base = base->data.array.base;
        }
        S += cur_sz; 
      }
    }
  }

  sf_size = ALIGN_TO_16(S + R + A);
  // sf_index要从函数参数后开始
  sf_index = A;

  // 分配栈帧空间
  if(sf_size > 0 && sf_size <= 2048) {
    std::cout << "  addi sp, sp, -" << sf_size << std::endl;
  } else if (sf_size > 2048) {
    std::cout << "  li t0, " << -sf_size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;
  }

  save_ra = 0;
  if(R > 0) {
    save_ra = 1;
    if(sf_size - 4 <= 2047) {
      std::cout << "  sw ra, " << sf_size - 4 << "(sp)" << std::endl; 
    } else {
      std::cout << "  li t0, " << sf_size - 4 << std::endl;
      std::cout << "  add t0, t0, sp" << std::endl;
      std::cout << "  sw ra, 0(t0)" << std::endl;
    }
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
      VisitAlloc(value);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      Visit(kind.data.global_alloc, value);
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load, value);
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_GET_PTR:
      Visit(kind.data.get_ptr, value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      Visit(kind.data.get_elem_ptr, value);
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
    case KOOPA_RVT_CALL:
      Visit(kind.data.call, value);
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

// alloc
void VisitAlloc(const koopa_raw_value_t &alloc) {
  auto base = alloc->ty->data.pointer.base;
  if(base->tag == KOOPA_RTT_INT32) {
    stack_frame[alloc] = sf_index;
    sf_index += 4;
  } else if(base->tag == KOOPA_RTT_ARRAY ){
    int cur_sz = 4;
    for(int i: array_dims[alloc]) {std::cout << i << std::endl;}
    while(base->tag == KOOPA_RTT_ARRAY) {
      //std::cout << "len is " << base->data.array.len << std::endl;
      array_dims[alloc].push_back(base->data.array.len);
      cur_sz *= base->data.array.len;
      base = base->data.array.base;
    }
    array_ranges[alloc] = std::make_pair(array_dims[alloc].begin(), array_dims[alloc].end());
    stack_frame[alloc] = sf_index;
    sf_index += cur_sz;
    //for(int i: array_dims[alloc]) {std::cout << i << std::endl;}
  } else if(base->tag == KOOPA_RTT_POINTER) {
    while(base->tag == KOOPA_RTT_POINTER) {
      array_dims[alloc].push_back(0);
      base = base->data.array.base;
    }
    while(base->tag == KOOPA_RTT_ARRAY) {
      array_dims[alloc].push_back(base->data.array.len);
      base = base->data.array.base;
    }
    array_ranges[alloc] = std::make_pair(array_dims[alloc].begin(), array_dims[alloc].end());
    stack_frame[alloc] = sf_index;
    sf_index += 4;
  }
}

// global_alloc
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value) {
  std::cout << "  .data" << std::endl;
  std::cout << "  .globl " << value->name + 1 << std::endl;
  std::cout << value->name + 1 << ":" << std::endl;
  switch(global_alloc.init->kind.tag) {
    case KOOPA_RVT_ZERO_INIT: {
      auto base = value->ty->data.pointer.base;
      int cur_sz = 4;
      bool is_array = 0;
      while(base->tag == KOOPA_RTT_ARRAY) {
        is_array = 1;
        array_dims[value].push_back(base->data.array.len);
        cur_sz *= base->data.array.len;
        base = base->data.array.base;
      }
      if(is_array) {
        array_ranges[value] = std::make_pair(array_dims[value].begin(), array_dims[value].end());
      }
      std::cout << "  .zero " << cur_sz << std::endl;
      break;
    }
    case KOOPA_RVT_INTEGER:
      std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
      break;
    case KOOPA_RVT_AGGREGATE: {
      auto base = value->ty->data.pointer.base;
      while(base->tag == KOOPA_RTT_ARRAY) {
        array_dims[value].push_back(base->data.array.len);
        base = base->data.array.base;
      }
      array_ranges[value] = std::make_pair(array_dims[value].begin(), array_dims[value].end());
      global_array_init(global_alloc.init);
      break;
    }
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  std::cout << std::endl;
}


// 访问load
// 处理imm12, 若越界则
// li t3, imm12
// add t3, sp, t3
// sw dest, t3
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &dest) {
  write_reg(load.src, "t0");
  if(isPointer(load.src)) {
    std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  if(array_ranges.find(load.src) != array_ranges.end()) {
    array_ranges[dest] = array_ranges[load.src];
  }
  stack_frame[dest] = sf_index; 
  sf_index += 4;
  save_reg(dest, "t0");
}

// 访问store
void Visit(const koopa_raw_store_t &store) {
  write_reg(store.value, "t0");
  if(isPointer(store.value)) {
    std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  write_addr_reg(store.dest, "t3");
  if(isPointer(store.dest)) {
    std::cout << "  lw t3, 0(t3)" << std::endl;
  }
  std::cout << "  sw t0, 0(t3)" << std::endl;
}

// 访问 getptr 
void Visit(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t& dest) {
  write_addr_reg(getptr.src, "t0");
  if(isPointer(getptr.src)) {
    std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  write_reg(getptr.index, "t1");
  std::cout << "  li t2, " << base_size(array_ranges[getptr.src]) << std::endl;
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;
  auto se = array_ranges[getptr.src];
  auto s = se.first; ++s;
  array_ranges[dest] = std::make_pair(s, se.second);
  stack_frame[dest] = sf_index;
  sf_index += 4;
  save_reg(dest, "t0");
}

// 访问 getelemptr 
void Visit(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t& dest) {
  write_addr_reg(getelemptr.src, "t0");
  if(isPointer(getelemptr.src)) {
    std::cout << "  lw t0, 0(t0)" << std::endl;
  }
  write_reg(getelemptr.index, "t1");
  std::cout << "  li t2, " << base_size(array_ranges[getelemptr.src]) << std::endl;
  std::cout << "  mul t1, t1, t2" << std::endl;
  std::cout << "  add t0, t0, t1" << std::endl;
  auto se = array_ranges[getelemptr.src];
  auto s = se.first; ++s;
  array_ranges[dest] = std::make_pair(s, se.second);
  stack_frame[dest] = sf_index;
  sf_index += 4;
  save_reg(dest, "t0");
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

// call
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value) {
  // args
  for(size_t i = 0; i < call.args.len; ++i) {
    auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
    if(i < 8) {
      write_reg(arg, "a" + std::to_string(i));
    } else {
      write_reg(arg, "t0");
      int offset = (i - 8) * 4;
      if(IN_IMM12(offset)) {
        std::cout << "  sw t0, " << offset << "(sp)" << std::endl;
      } else {
        std::cout << "  li t3, " << offset << std::endl;
        std::cout << "  add t3, t3, sp" << std::endl;
        std::cout << "  sw t0, 0(t3)" << std::endl;
      }
    }
  }
  // call func_name
  std::cout << "  call " << call.callee->name+1 << std::endl; 

  // 如果有返回值, 将返回值入栈
  if(value->ty->tag != KOOPA_RTT_UNIT) {
    stack_frame[value] = sf_index;
    sf_index += 4;
    save_reg(value, "a0"); 
  }
}

// 访问return
void Visit(const koopa_raw_return_t &ret) {
  // void函数无返回值
  if(ret.value != nullptr) {
    write_reg(ret.value, "a0");
  }

  if(save_ra) {
    int offset = sf_size - 4;
    if(IN_IMM12(offset)) {
      std::cout << "  lw ra, " << offset << "(sp)" << std::endl;
    } else {
      std::cout << "  li t0, " << offset << std::endl;
      std::cout << "  add t0, t0, sp" << std::endl;
      std::cout << "  lw ra, 0(t0)" << std::endl;
    }
  }

  if(sf_size > 0 && sf_size <= 2048) {
    std::cout << "  addi sp, sp, " << sf_size << std::endl;
  } else if(sf_size > 2048) {
    std::cout << "  li t0, " << sf_size << std::endl;
    std::cout << "  add sp, sp, t0" << std::endl;  
  }
  std::cout << "  ret\n";
}

//将value的值写入寄存器
void write_reg(const koopa_raw_value_t &value, const std::string &reg_name) {
  const auto& kind = value->kind;
  int offset;
  size_t index;
  switch(kind.tag) {
    case KOOPA_RVT_INTEGER:
      std::cout << "  li " << reg_name << ", " << kind.data.integer.value << std::endl;
      break;
    case KOOPA_RVT_FUNC_ARG_REF:
      index = kind.data.func_arg_ref.index;
      if(index < 8) {
        std::cout << "  mv " << reg_name << ", a" << index << std::endl;
      } else {
        // 注意这里offset一定要加上 sf_size, 这样才能获取到存放在caller栈帧中的参数
        offset = sf_size + (index - 8) * 4;
        if(IN_IMM12(offset)) {
          std::cout << "  lw " << reg_name << ", " << offset << "(sp)" << std::endl;
        } else {
          std::cout << "  li t3, " << offset << std::endl;
          std::cout << "  add t3, t3, sp" << std::endl;
          std::cout << "  lw " << reg_name << ", 0(t3)" << std::endl;
        }
      }
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      std::cout << "  la t3, " << value->name + 1 << std::endl;
      std::cout << "  lw " << reg_name << ", 0(t3)" << std::endl;
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

// 将寄存器中的值写入栈帧或全局变量
void save_reg(const koopa_raw_value_t &value, const std::string &reg_name) {
  int offset = stack_frame[value];
  if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::cout << "  la t3, " << value->name + 1 << std::endl;
    std::cout << "  sw " << reg_name << ", 0(t3)" << std::endl;
  } else {
    if(IN_IMM12(offset)) {
      std::cout << "  sw " << reg_name << ", " << offset << "(sp)\n";
    } else {
      std::cout << "  li t3, " << offset << std::endl;
      std::cout << "  add t3, sp, t3" << std::endl;
      std::cout << "  sw " << reg_name << ", 0(t3)" << std::endl; 
    }
  }

}

// 将value的地址放入名字为reg_name的寄存器中
void write_addr_reg(const koopa_raw_value_t &value, const std::string &reg_name) {
  const auto& kind = value->kind;
  // 全局变量
  if(kind.tag == KOOPA_RVT_GLOBAL_ALLOC) {
    std::cout << "  la " << reg_name << ", " << value->name + 1 << std::endl;
  }
  // 函数参数 
  else if(kind.tag == KOOPA_RVT_FUNC_ARG_REF) {
    size_t index = kind.data.func_arg_ref.index;
    int offset = sf_size + (index - 8) * 4;
    std::cout << "  li " << reg_name << ", " << offset << std::endl;
    std::cout <<  "  add " << reg_name << ", " << reg_name << ", sp" << std::endl;
  }
  // 临时变量
  else {
    std::cout << "  li " << reg_name << ", " << stack_frame[value] << std::endl;
    std::cout <<  "  add " << reg_name << ", " << reg_name << ", sp" << std::endl;
  }
}

// 判断value是否是一个指针
int isPointer(const koopa_raw_value_t &value) {
  if(value->kind.tag == KOOPA_RVT_GET_PTR || value->kind.tag == KOOPA_RVT_GET_ELEM_PTR) {
    return 1;
  }
  if(value->kind.tag == KOOPA_RVT_LOAD) {
    const auto& src = value->kind.data.load.src;
    if(src->kind.tag == KOOPA_RVT_ALLOC && src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER) {
      return 1;
    }
  }
  return 0;
}

// 全局数组初始化
void global_array_init(const koopa_raw_value_t &value) {
  const auto& kind = value->kind;
  if(kind.tag == KOOPA_RVT_INTEGER) {
    std::cout << "  .word " << kind.data.integer.value << std::endl;
    return;
  }

  if(kind.tag == KOOPA_RVT_AGGREGATE) {
    const auto& ag = kind.data.aggregate;
    for(int i = 0; i < ag.elems.len; ++i) {
      global_array_init(reinterpret_cast<koopa_raw_value_t>(ag.elems.buffer[i]));
    }
  }

}

// base type的大小
int base_size(const range_t& se) {
  auto s = se.first, e = se.second;
  int sz = 4;
  ++s;
  for(; s != e; ++s) {
    sz *= *(s);
  }
  return sz;
}
