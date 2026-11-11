#include <iostream>

#include "ast.h"

namespace {

const char* unText(UnOp op) {
  switch (op) {
    case UnOp::Plus: return "+";
    case UnOp::Neg: return "-";
    case UnOp::Not: return "not";
  }
  return "?";
}

const char* binText(BinOp op) {
  static const char* const names[] = {"+", "-", "*", "/", "div", "mod", "and",
                                      "or", "=", "<>", "<", "<=", ">", ">="};
  return names[static_cast<int>(op)];
}

void indent(int depth) {
  for (int i = 0; i < depth; ++i) std::cout << "  ";
}

void dumpExpr(const Expr* e, int depth) {
  indent(depth);
  switch (e->kind) {
    case Expr::Kind::IntLit:
      std::cout << "IntLit " << static_cast<const IntLit*>(e)->value;
      break;
    case Expr::Kind::StrLit:
      std::cout << "StrLit '" << static_cast<const StrLit*>(e)->value << "'";
      break;
    case Expr::Kind::NameRef:
      std::cout << "NameRef " << static_cast<const NameRef*>(e)->name;
      break;
    case Expr::Kind::Unary:
      std::cout << "Unary " << unText(static_cast<const Unary*>(e)->op);
      break;
    case Expr::Kind::Binary:
      std::cout << "Binary " << binText(static_cast<const Binary*>(e)->op);
      break;
  }
  std::cout << " : " << typeName(e->type) << '\n';
  if (e->kind == Expr::Kind::Unary) {
    dumpExpr(static_cast<const Unary*>(e)->operand, depth + 1);
  } else if (e->kind == Expr::Kind::Binary) {
    dumpExpr(static_cast<const Binary*>(e)->lhs, depth + 1);
    dumpExpr(static_cast<const Binary*>(e)->rhs, depth + 1);
  }
}

void dumpStmt(const Stmt* s, int depth) {
  indent(depth);
  switch (s->kind) {
    case Stmt::Kind::Empty:
      std::cout << "Empty\n";
      break;
    case Stmt::Kind::Assign: {
      auto* a = static_cast<const AssignStmt*>(s);
      std::cout << "Assign\n";
      dumpExpr(a->target, depth + 1);
      dumpExpr(a->value, depth + 1);
      break;
    }
    case Stmt::Kind::Compound:
      std::cout << "Compound\n";
      for (const Stmt* c : static_cast<const CompoundStmt*>(s)->body)
        dumpStmt(c, depth + 1);
      break;
    case Stmt::Kind::If: {
      auto* i = static_cast<const IfStmt*>(s);
      std::cout << "If\n";
      dumpExpr(i->cond, depth + 1);
      dumpStmt(i->thenStmt, depth + 1);
      if (i->elseStmt) dumpStmt(i->elseStmt, depth + 1);
      break;
    }
    case Stmt::Kind::While: {
      auto* w = static_cast<const WhileStmt*>(s);
      std::cout << "While\n";
      dumpExpr(w->cond, depth + 1);
      dumpStmt(w->body, depth + 1);
      break;
    }
    case Stmt::Kind::Repeat: {
      auto* r = static_cast<const RepeatStmt*>(s);
      std::cout << "Repeat\n";
      for (const Stmt* c : r->body) dumpStmt(c, depth + 1);
      dumpExpr(r->cond, depth + 1);
      break;
    }
    case Stmt::Kind::Call: {
      auto* c = static_cast<const CallStmt*>(s);
      std::cout << "Call " << c->name << '\n';
      for (const Arg& a : c->args) {
        dumpExpr(a.value, depth + 1);
        if (a.width) {
          indent(depth + 1);
          std::cout << "width\n";
          dumpExpr(a.width, depth + 2);
        }
      }
      break;
    }
  }
}

}  // namespace

void dumpAst(const Program& program) {
  std::cout << "Program " << program.name << " (frame " << program.frameSize
            << " bytes)\n";
  for (const VarGroup& g : program.varGroups)
    for (const VarDecl* v : g.vars)
      std::cout << "  Var " << v->name << " : " << typeName(v->type)
                << " at [rbp-" << v->offset << "]\n";
  dumpStmt(program.body, 1);
}
