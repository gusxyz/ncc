#include "parser.h"
#include "type.h"
#include <codegen.h>
#include <error.h>
#include <emit.h>
#include <string.h>

static const char *argument_registers[] = {
//64bit registers
"rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

static const char *byte_argument_register[] = {
// byte sized registers
"dil", "sil", "dl", "cl", "r8b", "r9b"
};


char *get_type_size(Type *type) {
  char *size;

  if(type->kind == TYPE_U8)
    size = "BYTE";
  else
    size = "QWORD";

  return size;

}
char *get_type_reg(Type *type) {
  char *reg;

  if(type->kind == TYPE_U8)
    reg = "dil";
  else
    reg = "rdi";

  return reg;
}

static void load_local_value(Type *type, const char *reg) {
  if (type->kind == TYPE_U8)
    emit("movzx %s, BYTE PTR [rax]", reg);
  else
    emit("mov %s, QWORD PTR [rax]", reg);
}

void generate_lvalue_address(Node* node) {
  if(node->kind == NODE_LOCAL_VARIABLE) {
    emit("mov rax, rbp");
    emit("sub rax, %d", node->offset);
    emit("push rax");
    return;
  }
  if(node->kind == NODE_DEREFERENCE) {
    generate_expression(node->left_hand_side);
    return;
  }

  error("Expression is not assignable");
}

void generate_local_variable(Node* node) {
  if(node->kind != NODE_LOCAL_VARIABLE)
    error("Left-hand side of assignemnt is not a variable");
  
  generate_lvalue_address(node);
}

void generate_assign(Node *node) {
  generate_lvalue_address(node->left_hand_side);
  generate_expression(node->right_hand_side);

  Type *type = node->left_hand_side->type;

  emit("pop rdi");
  emit("pop rax");
  emit("mov %s PTR [rax], %s", get_type_size(type), get_type_reg(type));

  if (type->kind == TYPE_U8)
    emit("movzx edi, dil");

  emit("push rdi");
}

void generate_pre_increment(Node *node) {
  Node *var = node->left_hand_side;
  generate_local_variable(var);
  emit("pop rax");
  
  emit("add %s PTR [rax], 1", get_type_size(var->type));
  load_local_value(var->type, "rax");
  emit("push rax");
}

void generate_post_increment(Node *node) {
  Node *var = node->left_hand_side;
  generate_local_variable(var);

  emit("pop rax");
  load_local_value(var->type, "rdi");
  emit("push rdi");
  emit("add %s PTR [rax], 1", get_type_size(var->type));
}

void generate_pre_decrement(Node *node) {
  Node *var = node->left_hand_side;
  generate_local_variable(var);
  
  emit("pop rax");
  emit("sub %s PTR [rax], 1", get_type_size(var->type));
  load_local_value(var->type, "rax");
  emit("push rax");
}

void generate_post_decrement(Node *node) {
    Node *var = node->left_hand_side;
    generate_local_variable(var);
    emit("pop rax");

    load_local_value(var->type, "rdi");
    emit("push rdi");
    emit("sub %s PTR [rax], 1", get_type_size(var->type));
}

static int next_label_id;
static int break_label = -1;
static int continue_label = -1;

int new_label() { return next_label_id++; }

void generate_string_literal(Node *node) {
    int id = new_label();

    emit_directive(".section .rodata");
    emit_label(".L.string.%d", id);

    for(size_t i = 0; i < node->string_length; ++i)
      emit_directive(".byte %d", node->string_value[i]);

    emit_directive(".byte 0");
    emit_directive(".text"); // back to code

    emit("lea rax, [rip + .L.str.%d]", id);
    emit("push rax");
}

void print_header() {
  emit_directive(".intel_syntax noprefix");
  emit_directive(".text");
  emit_directive(".global main");
}

void allocate_prologue(int stack_size) {
  // Function prologue
  // Allocate space for 26 variables
  emit("push rbp");
  emit("mov rbp, rsp");
  emit("sub rsp, %d", stack_size);
}

void epilogue() {
  // Function epilogue
  // Leave's last functions result in rax
  emit("mov rsp, rbp");
  emit("pop rbp");
  emit("ret");
}

void generate_function(Function *function) {
    emit_label("%s", function->name);
    allocate_prologue(function->stack_size);

    for(size_t i = 0; i < function->paramaters.count; i++) {
      LocalVariable *paramater = function->paramaters.items[i];
      if(paramater->type->kind == TYPE_U8) {
        emit("mov BYTE PTR [rbp-%d], %s",
          paramater->offset,
          byte_argument_register[i]);
      } else {
        emit("mov QWORD PTR [rbp-%d], %s",
            paramater->offset,
            argument_registers[i]);
      }
    }

    generate_statement(function->body);
    emit("mov rax, 0");
    epilogue();
}

void generate_function_call(Node* node){
    int count = node->arguments.count;
    if (count > 6)
      error("At most 6 arguments supported");

    for (int i = 0; i < count; i++)
      generate_expression(node->arguments.items[i]);

    for(int i = count; i > 0; i--) {
      emit("pop %s", argument_registers[i -1]);
    }

    // stack alignment
    emit("mov rax, rsp");
    emit("and rsp, -16");
    emit("sub rsp, 16");
    emit("mov [rsp], rax");

    emit("call %s", node->function_name);
    
    emit("mov rsp, [rsp]");
    emit("push rax");
}

void generate_program(Program *program) {
  print_header();
  for(size_t i = 0; i <program->functions.count; i++)
    generate_function(program->functions.items[i]);
}

void generate_expression(Node *node) {
 switch(node->kind) {
  case NODE_NUMBER:
    emit("push %d", node->value);
    return;
  case NODE_STRING_LITERAL:
    generate_string_literal(node);
    return;
  case NODE_FUNCTION_CALL:
    generate_function_call(node);
    return;
  case NODE_LOCAL_VARIABLE:
    generate_local_variable(node);
    emit("pop rax");
    if (node->type->kind == TYPE_U8)
      emit("movzx rax, BYTE PTR [rax]");
    else
      emit("mov rax, QWORD PTR [rax]");
    emit("push rax");
    return;
  case NODE_DEREFERENCE:
    generate_expression(node->left_hand_side);
    emit("pop rax");
    if (node->type->kind == TYPE_U8)
      emit("movzx rax, BYTE PTR [rax]");
    else
      emit("mov rax, QWORD PTR [rax]");
    emit("push rax");
    return;
  case NODE_ADDRESS:
    generate_lvalue_address(node->left_hand_side);
    return;
  case NODE_ASSIGN:
    generate_assign(node);
    return;
  case NODE_POST_INCREMENT:
    generate_post_increment(node);
    return;
  case NODE_PRE_INCREMENT:
    generate_pre_increment(node);
    return;
  case NODE_POST_DECREMENT:
    generate_post_decrement(node);
    return;
  case NODE_PRE_DECREMENT:
    generate_pre_decrement(node);
    return;
  default:
    break;
  }

  generate_expression(node->left_hand_side);
  generate_expression(node->right_hand_side);

  // Pop last two Ints from the stack so we can do math.
  emit("pop rdi");
  emit("pop rax");

  switch (node->kind) {
  case NODE_ADD:
    emit("add rax, rdi");
    break;
  case NODE_SUBTRACT:
    emit("sub rax, rdi");
    break;
  case NODE_EQUALS:
    emit("cmp rax, rdi");
    emit("sete al");
    emit("movzx rax, al");
    break;
  case NODE_NOT_EQUALS:
    emit("cmp rax, rdi");
    emit("setne al");
    emit("movzx rax, al");
    break;
  case NODE_LESS_THAN:
    emit("cmp rax, rdi");
    emit("setl al");
    emit("movzx rax, al");
    break;
  case NODE_EQUAL_OR_LESS_THAN:
    emit("cmp rax, rdi");
    emit("setle al");
    emit("movzx rax, al");
    break;
  case NODE_MULTIPLY:
    emit("imul rax, rdi");
    break;
  case NODE_DIVIDE:
    emit("cqo");
    emit("idiv rdi");
    break;
  default:
    error("Invalid expression node kind %d", node->kind);
  }
  
  emit("push rax");
}

void generate_return(Node* node) {
    generate_expression(node->left_hand_side);
    emit("pop rax");
    emit("mov rsp, rbp");
    emit("pop rbp");
    emit("ret");
}

void generate_if(Node *node) {
  int id = new_label();
  
  generate_expression(node->condition);

  emit("pop rax");
  emit("cmp rax, 0");

  if(node->els) {
    emit("je .L.else.%d", id);
    generate_statement(node->then);
    emit("jmp .L.end.%d", id);

    emit_label(".L.else.%d", id);
    generate_statement(node->els);

    emit_label(".L.end.%d", id);

  } else {
    emit("je .L.end.%d", id);
    generate_statement(node->then);        
    emit_label(".L.end.%d", id);
  }
}

void generate_while(Node *node) {
  int id = new_label();
  int savedbreak = break_label;
  int savedcont = continue_label;
  break_label = id;
  continue_label = id;
  emit_label(".L.continue.%d", id);
  generate_expression(node->condition);
  emit("pop rax");
  emit("cmp rax, 0");
  emit("je .L.end.%d", id);
  generate_statement(node->then);
  emit("jmp .L.continue.%d", id);
  emit_label(".L.end.%d", id);
  break_label = savedbreak;
  continue_label = savedcont;
}

void generate_break() {
  if(break_label == -1)
    error("break outside of a loop");

  emit("jmp .L.end.%d", break_label);
}

void generate_continue() {
  if(continue_label == -1)
    error("continue outside of a loop");

  emit("jmp .L.continue.%d", continue_label);
}

void generate_for(Node *node) {
  int id = new_label();
  int savedbreak = break_label;
  int savedcont = continue_label;
  break_label = id;
  continue_label = id;
  if(node->init) {
    generate_expression(node->init);
    emit("pop rax");
  }
  emit_label(".L.loop.%d", id);
  if(node->condition) {
    generate_expression(node->condition);
    emit("pop rax");
    emit("cmp rax, 0");
    emit("je .L.end.%d", id);
  }
  generate_statement(node->then);

  emit_label(".L.continue.%d", id);

  if(node->inc) {
    generate_expression(node->inc);
    emit("pop rax");
  }
  emit("jmp .L.loop.%d", id);
  emit_label(".L.end.%d", id);
  break_label = savedbreak;
  continue_label = savedcont;
}

void generate_block(Node *node) {
  for (size_t i = 0; i < node->body.count; i++) {
    generate_statement(node->body.items[i]);
  }
}

void generate_statement(Node *node) {
  // NUMBERS, VARIABLES AND ASSIGNMENTS FIRST.
  if(node->kind == NODE_RETURN)
    return generate_return(node);
  if(node->kind == NODE_WHILE)
    return generate_while(node);
  if(node->kind == NODE_IF)
    return generate_if(node);
  if (node->kind == NODE_BLOCK)
    return generate_block(node);
  if (node->kind == NODE_FOR)
    return generate_for(node);
  if (node->kind == NODE_BREAK)
    return generate_break();
  if (node->kind == NODE_CONTINUE)
    return generate_continue();

  generate_expression(node);
  emit("pop rax");
}
