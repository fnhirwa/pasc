#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "source.h"
#include "types.h"

// The parser builds this tree with names unresolved. Sema fills in every
// field marked "set by sema". Backends read the tree and never do name
// lookup or type inference themselves.

// ---- Declarations ---------------------------------------------------------

struct Decl {
  enum class Kind { Var, Const, Type, BuiltinProc };
  Kind kind;
  std::string name;
  SourceLoc loc;

 protected:
  Decl(Kind k, std::string n, SourceLoc l)
      : kind(k), name(std::move(n)), loc(l) {}
};

struct VarDecl : Decl {
  const Type* type = nullptr;  // set by sema
  int offset = 0;              // set by sema: the variable is at [rbp - offset]
  VarDecl(std::string n, SourceLoc l) : Decl(Kind::Var, std::move(n), l) {}
};

struct ConstDecl : Decl {
  const Type* type;
  int64_t value;
  ConstDecl(std::string n, const Type* t, int64_t v)
      : Decl(Kind::Const, std::move(n), {}), type(t), value(v) {}
};

struct TypeDecl : Decl {
  const Type* type;
  TypeDecl(std::string n, const Type* t)
      : Decl(Kind::Type, std::move(n), {}), type(t) {}
};

enum class Builtin { Write, Writeln };

struct BuiltinProcDecl : Decl {
  Builtin which;
  BuiltinProcDecl(std::string n, Builtin b)
      : Decl(Kind::BuiltinProc, std::move(n), {}), which(b) {}
};

// ---- Expressions ----------------------------------------------------------

struct Expr {
  enum class Kind { IntLit, StrLit, NameRef, Unary, Binary };
  Kind kind;
  SourceLoc loc;
  const Type* type = nullptr;  // set by sema; null after a reported error

 protected:
  Expr(Kind k, SourceLoc l) : kind(k), loc(l) {}
};

struct IntLit : Expr {
  int64_t value;
  IntLit(int64_t v, SourceLoc l) : Expr(Kind::IntLit, l), value(v) {}
};

// A character-string. One element has char-type, more have a string-type.
struct StrLit : Expr {
  std::string value;
  StrLit(std::string v, SourceLoc l)
      : Expr(Kind::StrLit, l), value(std::move(v)) {}
};

struct NameRef : Expr {
  std::string name;
  Decl* decl = nullptr;  // set by sema: a VarDecl or a ConstDecl
  NameRef(std::string n, SourceLoc l)
      : Expr(Kind::NameRef, l), name(std::move(n)) {}
};

enum class UnOp { Plus, Neg, Not };

struct Unary : Expr {
  UnOp op;
  Expr* operand;
  Unary(UnOp o, Expr* e, SourceLoc l)
      : Expr(Kind::Unary, l), op(o), operand(e) {}
};

enum class BinOp {
  Add, Sub, Mul, RealDiv, Div, Mod, And, Or, Eq, Ne, Lt, Le, Gt, Ge
};

struct Binary : Expr {
  BinOp op;
  Expr* lhs;
  Expr* rhs;
  Binary(BinOp o, Expr* a, Expr* b, SourceLoc l)
      : Expr(Kind::Binary, l), op(o), lhs(a), rhs(b) {}
};

// ---- Statements -----------------------------------------------------------

struct Stmt {
  enum class Kind { Empty, Assign, Compound, If, While, Repeat, Call };
  Kind kind;
  SourceLoc loc;

 protected:
  Stmt(Kind k, SourceLoc l) : kind(k), loc(l) {}
};

struct EmptyStmt : Stmt {
  explicit EmptyStmt(SourceLoc l) : Stmt(Kind::Empty, l) {}
};

struct AssignStmt : Stmt {
  Expr* target;
  Expr* value;
  AssignStmt(Expr* t, Expr* v, SourceLoc l)
      : Stmt(Kind::Assign, l), target(t), value(v) {}
};

struct CompoundStmt : Stmt {
  std::vector<Stmt*> body;
  explicit CompoundStmt(SourceLoc l) : Stmt(Kind::Compound, l) {}
};

struct IfStmt : Stmt {
  Expr* cond;
  Stmt* thenStmt;
  Stmt* elseStmt;  // may be null
  IfStmt(Expr* c, Stmt* t, Stmt* e, SourceLoc l)
      : Stmt(Kind::If, l), cond(c), thenStmt(t), elseStmt(e) {}
};

struct WhileStmt : Stmt {
  Expr* cond;
  Stmt* body;
  WhileStmt(Expr* c, Stmt* b, SourceLoc l)
      : Stmt(Kind::While, l), cond(c), body(b) {}
};

struct RepeatStmt : Stmt {
  std::vector<Stmt*> body;
  Expr* cond = nullptr;
  explicit RepeatStmt(SourceLoc l) : Stmt(Kind::Repeat, l) {}
};

// An actual-parameter, or a write-parameter "e : TotalWidth : FracDigits".
struct Arg {
  Expr* value = nullptr;
  Expr* width = nullptr;  // may be null
  Expr* frac = nullptr;   // may be null
};

struct CallStmt : Stmt {
  std::string name;
  std::vector<Arg> args;
  Decl* decl = nullptr;  // set by sema
  CallStmt(std::string n, SourceLoc l)
      : Stmt(Kind::Call, l), name(std::move(n)) {}
};

// ---- Program --------------------------------------------------------------

// "a, b : T" in a variable-declaration-part. The identifiers of one group are
// all defined before T is resolved (6.2.2).
struct VarGroup {
  std::vector<VarDecl*> vars;
  std::string typeName;
  SourceLoc typeLoc;
};

struct ProgramParam {
  std::string name;
  SourceLoc loc;
};

struct Program {
  std::string name;
  SourceLoc loc;
  std::vector<ProgramParam> params;
  std::vector<VarGroup> varGroups;
  CompoundStmt* body = nullptr;
  int frameSize = 0;  // set by sema: bytes below rbp, a multiple of 16
};

void dumpAst(const Program& program);
