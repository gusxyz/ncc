#include <codegen.h>
#include <error.h>
#include <emit.h>

static const char *argument_registers[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9",
};

void generate_local_variable(Node* node) {
  if(node->kind != NODE_LOCAL_VARIABLE)
    error("Left-hand side of assignemnt is not a variable");
  emit("mov rax, rbp");
  emit("sub rax, %d", node->offset);
  emit("push rax");
}

void generate_assign(Node *node) {
    generate_local_variable(node->left_hand_side);
    generate_expression(node->right_hand_side);

    emit("pop rdi");
    emit("pop rax");
    emit("mov [rax], rdi");
    emit("push rdi");
}

void generate_pre_increment(Node *node) {
  generate_local_variable(node->left_hand_side);
  
  emit("pop rax");
  emit("add QWORD PTR [rax], 1");
  emit("mov rax, [rax]");
  emit("push rax");
}

void generate_post_increment(Node *node) {
    generate_local_variable(node->left_hand_side);

    emit("pop rax");
    emit("mov rdi, [rax]");
    emit("push rdi");
    emit("add QWORD PTR [rax], 1");
}

void generate_pre_decrement(Node *node) {
  generate_local_variable(node->left_hand_side);
  
  emit("pop rax");
  emit("sub QWORD PTR [rax], 1");
  emit("mov rax, [rax]");
  emit("push rax");
}

void generate_post_decrement(Node *node) {
    generate_local_variable(node->left_hand_side);

    emit("pop rax");
    emit("mov rdi, [rax]");
    emit("push rdi");
    emit("sub QWORD PTR [rax], 1");
}

static int next_label_id;
static int break_label = -1;
static int continue_label = -1;

int new_label() { return next_label_id++; }

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

    for(size_t i = 0; i < function->arguments.count; i++) {
      LocalVariable *argument = function->arguments.items[i];
      emit("mov [rbp-%d], %s",
          argument->offset,
          argument_registers[i]);
    }

    generate_statement(function->body);
    emit("mov rax, 0");
    epilogue();
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
  case NODE_FUNCTION_CALL:
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
    return;
  case NODE_LOCAL_VARIABLE:
    generate_local_variable(node);
    emit("pop rax");
    emit("mov rax, [rax]");
    emit("push rax");
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
