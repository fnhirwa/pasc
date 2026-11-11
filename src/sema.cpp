#include "sema.h"

#include <limits>

namespace {

const char* opText(BinOp op) {
  switch (op) {
    case BinOp::Add: return "+";
    case BinOp::Sub: return "-";
    case BinOp::Mul: return "*";
    case BinOp::RealDiv: return "/";
    case BinOp::Div: return "div";
    case BinOp::Mod: return "mod";
    case BinOp::And: return "and";
    case BinOp::Or: return "or";
    case BinOp::Eq: return "=";
    case BinOp::Ne: return "<>";
    case BinOp::Lt: return "<";
    case BinOp::Le: return "<=";
    case BinOp::Gt: return ">";
    case BinOp::Ge: return ">=";
  }
  return "?";
}

}  // namespace

Sema::Sema(TypeContext& types, Arena& arena, Diag& diag)
    : types_(types), arena_(arena), diag_(diag) {
  auto add = [&](Decl* d) { required_.names[d->name] = d; };
  add(arena_.make<TypeDecl>("integer", types_.integerTy()));
  add(arena_.make<TypeDecl>("boolean", types_.booleanTy()));
  add(arena_.make<TypeDecl>("char", types_.charTy()));
  add(arena_.make<TypeDecl>("text", types_.textTy()));
  add(arena_.make<ConstDecl>("false", types_.booleanTy(), 0));
  add(arena_.make<ConstDecl>("true", types_.booleanTy(), 1));
  add(arena_.make<ConstDecl>("maxint", types_.integerTy(),
                             std::numeric_limits<int64_t>::max()));
  add(arena_.make<BuiltinProcDecl>("write", Builtin::Write));
  add(arena_.make<BuiltinProcDecl>("writeln", Builtin::Writeln));
  program_.parent = &required_;
}

Decl* Sema::lookup(const std::string& name) {
  for (Scope* s = scope_; s; s = s->parent) {
    auto it = s->names.find(name);
    if (it != s->names.end()) {
      for (Scope* inner = scope_; inner != s; inner = inner->parent)
        inner->usedOuter.insert(name);
      return it->second;
    }
  }
  return nullptr;
}

void Sema::declare(Decl* decl) {
  if (scope_->names.count(decl->name)) {
    diag_.error(decl->loc, "'" + decl->name + "' is already defined in this block");
    return;
  }
  // 6.2.2.9: a defining-point precedes every applied occurrence in its region.
  if (scope_->usedOuter.count(decl->name))
    diag_.error(decl->loc,
                "'" + decl->name +
                    "' is used earlier in this block with its outer meaning");
  scope_->names[decl->name] = decl;
}

void Sema::check(Program& program) {
  scope_ = &program_;

  // 6.10: input and output as program-parameters are defined as textfile
  // variables for the program-block.
  std::unordered_set<std::string> seen;
  for (const ProgramParam& p : program.params) {
    if (!seen.insert(p.name).second) {
      diag_.error(p.loc, "program parameter '" + p.name + "' is repeated");
      continue;
    }
    if (p.name == "output" || p.name == "input") {
      VarDecl* file = arena_.make<VarDecl>(p.name, p.loc);
      file->type = types_.textTy();
      declare(file);
      if (p.name == "output") hasOutput_ = true;
    }
  }

  checkVars(program);

  // 6.10: every other program-parameter is a variable of the program-block.
  for (const ProgramParam& p : program.params) {
    if (p.name == "output" || p.name == "input") continue;
    auto it = program_.names.find(p.name);
    if (it == program_.names.end() || it->second->kind != Decl::Kind::Var)
      diag_.error(p.loc, "program parameter '" + p.name +
                             "' is not declared as a variable");
  }

  checkStmt(program.body);
}

// Frame layout: [rbp-8] is the static link slot, reserved in every frame so
// that nested procedures can be added without moving locals. Each variable
// then takes one 8-byte slot.
void Sema::checkVars(Program& program) {
  int offset = 8;
  for (VarGroup& group : program.varGroups) {
    for (VarDecl* v : group.vars) declare(v);
    if (group.typeName.empty()) continue;  // the parser already reported it
    Decl* d = lookup(group.typeName);
    const Type* type = nullptr;
    if (!d)
      diag_.error(group.typeLoc, "undeclared identifier '" + group.typeName + "'");
    else if (d->kind != Decl::Kind::Type)
      diag_.error(group.typeLoc, "'" + group.typeName + "' is not a type");
    else if (static_cast<TypeDecl*>(d)->type->kind == Type::Kind::Text)
      diag_.error(group.typeLoc, "file variables are not implemented yet");
    else
      type = static_cast<TypeDecl*>(d)->type;
    for (VarDecl* v : group.vars) {
      v->type = type;
      offset += 8;
      v->offset = offset;
    }
  }
  program.frameSize = (offset + 15) / 16 * 16;
}

void Sema::requireBoolean(Expr* e, const char* context) {
  const Type* t = checkExpr(e);
  if (t && t->kind != Type::Kind::Boolean)
    diag_.error(e->loc, std::string("the ") + context +
                            " must be Boolean, not " + typeName(t));
}

void Sema::checkStmt(Stmt* s) {
  switch (s->kind) {
    case Stmt::Kind::Empty:
      return;
    case Stmt::Kind::Compound:
      for (Stmt* c : static_cast<CompoundStmt*>(s)->body) checkStmt(c);
      return;
    case Stmt::Kind::If: {
      auto* i = static_cast<IfStmt*>(s);
      requireBoolean(i->cond, "condition of an if-statement");
      checkStmt(i->thenStmt);
      if (i->elseStmt) checkStmt(i->elseStmt);
      return;
    }
    case Stmt::Kind::While: {
      auto* w = static_cast<WhileStmt*>(s);
      requireBoolean(w->cond, "condition of a while-statement");
      checkStmt(w->body);
      return;
    }
    case Stmt::Kind::Repeat: {
      auto* r = static_cast<RepeatStmt*>(s);
      for (Stmt* c : r->body) checkStmt(c);
      requireBoolean(r->cond, "condition of a repeat-statement");
      return;
    }
    case Stmt::Kind::Assign: {
      // 6.8.2.2: the value shall be assignment-compatible with the variable.
      auto* a = static_cast<AssignStmt*>(s);
      const Type* dst = checkExpr(a->target);
      const Type* src = checkExpr(a->value);
      auto* ref = static_cast<NameRef*>(a->target);
      if (ref->decl && ref->decl->kind != Decl::Kind::Var) {
        diag_.error(a->target->loc, "'" + ref->name + "' is not a variable");
        return;
      }
      if (dst && src && !assignmentCompatible(dst, src))
        diag_.error(a->loc, "cannot assign " + typeName(src) + " to " +
                                typeName(dst));
      return;
    }
    case Stmt::Kind::Call:
      checkCall(static_cast<CallStmt*>(s));
      return;
  }
}

// 6.9.3 and 6.9.4: write and writeln on a textfile.
void Sema::checkCall(CallStmt* s) {
  s->decl = lookup(s->name);
  for (Arg& arg : s->args) {
    checkExpr(arg.value);
    if (arg.width) checkExpr(arg.width);
    if (arg.frac) checkExpr(arg.frac);
  }
  if (!s->decl) {
    diag_.error(s->loc, "undeclared identifier '" + s->name + "'");
    return;
  }
  if (s->decl->kind != Decl::Kind::BuiltinProc) {
    diag_.error(s->loc, "'" + s->name + "' is not a procedure");
    return;
  }
  Builtin which = static_cast<BuiltinProcDecl*>(s->decl)->which;

  // An explicit file parameter. Only the required textfile output exists.
  if (!s->args.empty() && s->args[0].value->type &&
      s->args[0].value->type->kind == Type::Kind::Text) {
    Arg file = s->args[0];
    auto* ref = static_cast<NameRef*>(file.value);
    if (ref->name != "output")
      diag_.error(file.value->loc, "only the textfile output can be written");
    if (file.width)
      diag_.error(file.width->loc, "a file parameter cannot have a field width");
    s->args.erase(s->args.begin());
  } else if (!hasOutput_) {
    // 6.9.3: omitting the file requires output in the program-parameter-list.
    diag_.error(s->loc, "'" + s->name +
                            "' needs output in the program parameter list");
  }

  if (which == Builtin::Write && s->args.empty())
    diag_.error(s->loc, "write needs at least one write-parameter");

  for (Arg& arg : s->args) {
    const Type* t = arg.value->type;
    if (t && t->kind == Type::Kind::Text)
      diag_.error(arg.value->loc, "a file cannot be written to a textfile");
    if (arg.width && arg.width->type &&
        arg.width->type->kind != Type::Kind::Integer)
      diag_.error(arg.width->loc, "the field width must be an integer");
    if (arg.frac)
      diag_.error(arg.frac->loc,
                  "a fraction-digits field is allowed only for real values");
  }
}

const Type* Sema::checkExpr(Expr* e) {
  switch (e->kind) {
    case Expr::Kind::IntLit:
      e->type = types_.integerTy();
      break;
    case Expr::Kind::StrLit: {
      // 6.1.7: one string-element denotes a char, more denote a string-type.
      auto* s = static_cast<StrLit*>(e);
      e->type = s->value.size() == 1 ? types_.charTy()
                                     : types_.stringTy(int64_t(s->value.size()));
      break;
    }
    case Expr::Kind::NameRef: {
      auto* r = static_cast<NameRef*>(e);
      r->decl = lookup(r->name);
      if (!r->decl) {
        diag_.error(e->loc, "undeclared identifier '" + r->name + "'");
      } else if (r->decl->kind == Decl::Kind::Var) {
        e->type = static_cast<VarDecl*>(r->decl)->type;
      } else if (r->decl->kind == Decl::Kind::Const) {
        e->type = static_cast<ConstDecl*>(r->decl)->type;
      } else {
        diag_.error(e->loc, "'" + r->name + "' does not denote a value");
        r->decl = nullptr;
      }
      break;
    }
    case Expr::Kind::Unary: {
      auto* u = static_cast<Unary*>(e);
      const Type* t = checkExpr(u->operand);
      Type::Kind want =
          u->op == UnOp::Not ? Type::Kind::Boolean : Type::Kind::Integer;
      if (t && t->kind != want) {
        diag_.error(e->loc, std::string("the operand of ") +
                                (u->op == UnOp::Not ? "'not'" : "a sign") +
                                " cannot be " + typeName(t));
        break;
      }
      e->type = t;
      break;
    }
    case Expr::Kind::Binary:
      e->type = checkBinary(static_cast<Binary*>(e));
      break;
  }
  return e->type;
}

// 6.7.2: operand and result types of the dyadic operators.
const Type* Sema::checkBinary(Binary* e) {
  const Type* l = checkExpr(e->lhs);
  const Type* r = checkExpr(e->rhs);
  if (e->op == BinOp::RealDiv) {
    diag_.error(e->loc, "the real-type is not implemented yet; use div");
    return nullptr;
  }
  if (!l || !r) return nullptr;

  auto mismatch = [&]() -> const Type* {
    diag_.error(e->loc, std::string("operator '") + opText(e->op) +
                            "' cannot be applied to " + typeName(l) + " and " +
                            typeName(r));
    return nullptr;
  };

  switch (e->op) {
    case BinOp::Add:
    case BinOp::Sub:
    case BinOp::Mul:
    case BinOp::Div:
    case BinOp::Mod:
      if (l->kind != Type::Kind::Integer || r->kind != Type::Kind::Integer)
        return mismatch();
      return types_.integerTy();
    case BinOp::And:
    case BinOp::Or:
      if (l->kind != Type::Kind::Boolean || r->kind != Type::Kind::Boolean)
        return mismatch();
      return types_.booleanTy();
    case BinOp::Eq:
    case BinOp::Ne:
    case BinOp::Lt:
    case BinOp::Le:
    case BinOp::Gt:
    case BinOp::Ge:
      if (!compatible(l, r) || l->kind == Type::Kind::Text) return mismatch();
      if (l->kind == Type::Kind::String) {
        diag_.error(e->loc, "string comparison is not implemented yet");
        return nullptr;
      }
      return types_.booleanTy();
    case BinOp::RealDiv:
      break;
  }
  return nullptr;
}
