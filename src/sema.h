#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "arena.h"
#include "ast.h"
#include "diag.h"

// Name resolution (6.2.2), type checking (6.4, 6.7, 6.8) and frame layout.
// After check() succeeds the AST is fully resolved and typed.
class Sema {
 public:
  Sema(TypeContext& types, Arena& arena, Diag& diag);
  void check(Program& program);

 private:
  struct Scope {
    Scope* parent = nullptr;
    std::unordered_map<std::string, Decl*> names;
    // Identifiers of enclosing scopes that have an applied occurrence in
    // this scope. Defining one of them here afterwards violates 6.2.2.
    std::unordered_set<std::string> usedOuter;
  };

  Decl* lookup(const std::string& name);
  void declare(Decl* decl);

  void checkVars(Program& program);
  void checkStmt(Stmt* s);
  void checkCall(CallStmt* s);
  const Type* checkExpr(Expr* e);
  const Type* checkBinary(Binary* e);
  void requireBoolean(Expr* e, const char* context);

  TypeContext& types_;
  Arena& arena_;
  Diag& diag_;
  Scope required_;  // 6.2.2.10: required identifiers enclose the program
  Scope program_;
  Scope* scope_ = nullptr;
  bool hasOutput_ = false;
};
