#pragma once
#include <ostream>
#include <string>
#include <vector>

#include "ast.h"

// Stage 1 backend: naive x86-64 code generation straight from the typed AST.
// Target: System V AMD64 ABI, GNU assembler, Intel syntax.
//
// Model: every expression leaves its value in rax. A binary operator
// evaluates the left operand, pushes it, evaluates the right operand, and
// pops the left one back. There is no register allocation.
class CodegenX86 {
 public:
  explicit CodegenX86(std::ostream& out) : out_(out) {}
  void emitProgram(const Program& program);

  // Implementation-defined default TotalWidth values (6.9.3.1).
  static constexpr int kDefaultIntegerWidth = 1;
  static constexpr int kDefaultBooleanWidth = 5;

 private:
  void ins(const std::string& text) { out_ << "    " << text << '\n'; }
  void label(const std::string& name) { out_ << name << ":\n"; }
  std::string newLabel() { return ".L" + std::to_string(nextLabel_++); }
  void push();
  void pop(const char* reg);
  void call(const char* symbol);
  void setLine(SourceLoc loc);

  void genStmt(const Stmt* s);
  void genWrite(const CallStmt* s);
  void genExpr(const Expr* e);
  void genBinary(const Binary* e);

  std::ostream& out_;
  int nextLabel_ = 0;
  int pushDepth_ = 0;  // 8-byte temporaries currently on the stack
  std::vector<std::string> strings_;
};
