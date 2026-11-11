#include "codegen_x86.h"

namespace {

std::string slot(const VarDecl* v) {
  return "qword ptr [rbp-" + std::to_string(v->offset) + "]";
}

}  // namespace

void CodegenX86::push() {
  ins("push rax");
  ++pushDepth_;
}

void CodegenX86::pop(const char* reg) {
  ins(std::string("pop ") + reg);
  --pushDepth_;
}

// The ABI requires rsp to be 16-byte aligned at a call. The frame keeps it
// aligned, so only an odd number of pushed temporaries needs padding.
void CodegenX86::call(const char* symbol) {
  bool pad = pushDepth_ % 2 != 0;
  if (pad) ins("sub rsp, 8");
  ins(std::string("call ") + symbol);
  if (pad) ins("add rsp, 8");
}

// Loads the source line for the run-time error handlers. mov leaves the
// flags alone, so it can sit between an operation and its jo.
void CodegenX86::setLine(SourceLoc loc) {
  ins("mov edi, " + std::to_string(loc.line));
}

void CodegenX86::emitProgram(const Program& program) {
  out_ << "# program " << program.name << "\n";
  out_ << "    .intel_syntax noprefix\n";
  out_ << "    .text\n";
  out_ << "    .globl main\n";
  label("main");
  ins("push rbp");
  ins("mov rbp, rsp");
  ins("sub rsp, " + std::to_string(program.frameSize));
  ins("mov qword ptr [rbp-8], 0        # static link: none for the program");

  genStmt(program.body);

  ins("xor eax, eax");
  ins("leave");
  ins("ret");

  // Shared run-time error exits. edi already holds the source line. The
  // handlers do not return, so the stack is simply realigned.
  out_ << '\n';
  for (const char* name : {"overflow", "divzero", "mod"}) {
    label(std::string(".Lerr_") + name);
    ins("and rsp, -16");
    ins(std::string("call p_err_") + name);
  }

  if (!strings_.empty()) {
    out_ << "\n    .section .rodata\n";
    for (size_t i = 0; i < strings_.size(); ++i) {
      out_ << ".Lstr" << i << ":\n    .byte ";
      for (size_t j = 0; j < strings_[i].size(); ++j)
        out_ << (j ? ", " : "")
             << static_cast<int>(static_cast<unsigned char>(strings_[i][j]));
      out_ << '\n';
    }
  }
  out_ << "\n    .section .note.GNU-stack,\"\",@progbits\n";
}

void CodegenX86::genStmt(const Stmt* s) {
  switch (s->kind) {
    case Stmt::Kind::Empty:
      return;
    case Stmt::Kind::Compound:
      for (const Stmt* c : static_cast<const CompoundStmt*>(s)->body)
        genStmt(c);
      return;
    case Stmt::Kind::Assign: {
      auto* a = static_cast<const AssignStmt*>(s);
      auto* target = static_cast<const NameRef*>(a->target);
      out_ << "    # line " << s->loc.line << ": " << target->name << " := ...\n";
      genExpr(a->value);
      ins("mov " + slot(static_cast<const VarDecl*>(target->decl)) + ", rax");
      return;
    }
    case Stmt::Kind::If: {
      auto* i = static_cast<const IfStmt*>(s);
      std::string elseLabel = newLabel();
      out_ << "    # line " << s->loc.line << ": if\n";
      genExpr(i->cond);
      ins("test rax, rax");
      ins("jz " + elseLabel);
      genStmt(i->thenStmt);
      if (i->elseStmt) {
        std::string endLabel = newLabel();
        ins("jmp " + endLabel);
        label(elseLabel);
        genStmt(i->elseStmt);
        label(endLabel);
      } else {
        label(elseLabel);
      }
      return;
    }
    case Stmt::Kind::While: {
      auto* w = static_cast<const WhileStmt*>(s);
      std::string top = newLabel();
      std::string end = newLabel();
      out_ << "    # line " << s->loc.line << ": while\n";
      label(top);
      genExpr(w->cond);
      ins("test rax, rax");
      ins("jz " + end);
      genStmt(w->body);
      ins("jmp " + top);
      label(end);
      return;
    }
    case Stmt::Kind::Repeat: {
      auto* r = static_cast<const RepeatStmt*>(s);
      std::string top = newLabel();
      out_ << "    # line " << s->loc.line << ": repeat\n";
      label(top);
      for (const Stmt* c : r->body) genStmt(c);
      genExpr(r->cond);
      ins("test rax, rax");
      ins("jz " + top);
      return;
    }
    case Stmt::Kind::Call:
      genWrite(static_cast<const CallStmt*>(s));
      return;
  }
}

// write(e1, ..., en) is write(e1); ...; write(en), so the parameters are
// evaluated left to right (6.9.3). Each one becomes a run-time call:
//   p_write_int (value, width, line)      p_write_char(value, width, line)
//   p_write_bool(value, width, line)      p_write_str (ptr, len, width, line)
void CodegenX86::genWrite(const CallStmt* s) {
  out_ << "    # line " << s->loc.line << ": " << s->name << "\n";
  std::string line = std::to_string(s->loc.line);
  for (const Arg& arg : s->args) {
    const Type* t = arg.value->type;
    if (arg.width) {
      genExpr(arg.width);
      push();
    }
    if (t->kind == Type::Kind::String) {
      auto* lit = static_cast<const StrLit*>(arg.value);
      ins("lea rdi, [rip+.Lstr" + std::to_string(strings_.size()) + "]");
      ins("mov rsi, " + std::to_string(lit->value.size()));
      strings_.push_back(lit->value);
      if (arg.width) pop("rdx");
      else ins("mov rdx, rsi");
      ins("mov ecx, " + line);
      call("p_write_str");
      continue;
    }
    genExpr(arg.value);
    ins("mov rdi, rax");
    const char* fn = "p_write_int";
    int defaultWidth = kDefaultIntegerWidth;
    if (t->kind == Type::Kind::Char) {
      fn = "p_write_char";
      defaultWidth = 1;
    } else if (t->kind == Type::Kind::Boolean) {
      fn = "p_write_bool";
      defaultWidth = kDefaultBooleanWidth;
    }
    if (arg.width) pop("rsi");
    else ins("mov rsi, " + std::to_string(defaultWidth));
    ins("mov edx, " + line);
    call(fn);
  }
  if (static_cast<const BuiltinProcDecl*>(s->decl)->which == Builtin::Writeln)
    call("p_writeln");
}

void CodegenX86::genExpr(const Expr* e) {
  switch (e->kind) {
    case Expr::Kind::IntLit:
      ins("mov rax, " + std::to_string(static_cast<const IntLit*>(e)->value));
      return;
    case Expr::Kind::StrLit: {
      // Only a char reaches here; strings are handled by genWrite.
      unsigned char c = static_cast<const StrLit*>(e)->value[0];
      ins("mov rax, " + std::to_string(static_cast<int>(c)));
      return;
    }
    case Expr::Kind::NameRef: {
      auto* r = static_cast<const NameRef*>(e);
      if (r->decl->kind == Decl::Kind::Const)
        ins("mov rax, " +
            std::to_string(static_cast<const ConstDecl*>(r->decl)->value));
      else
        ins("mov rax, " + slot(static_cast<const VarDecl*>(r->decl)));
      return;
    }
    case Expr::Kind::Unary: {
      auto* u = static_cast<const Unary*>(e);
      genExpr(u->operand);
      if (u->op == UnOp::Neg) {
        ins("neg rax");
        setLine(e->loc);
        ins("jo .Lerr_overflow");
      } else if (u->op == UnOp::Not) {
        ins("xor rax, 1");
      }
      return;
    }
    case Expr::Kind::Binary:
      genBinary(static_cast<const Binary*>(e));
      return;
  }
}

// Left operand in rax, right operand in rcx, result in rax. Both operands of
// 'and' and 'or' are always evaluated; the standard leaves this open.
void CodegenX86::genBinary(const Binary* e) {
  genExpr(e->lhs);
  push();
  genExpr(e->rhs);
  ins("mov rcx, rax");
  pop("rax");

  auto compare = [&](const char* setcc) {
    ins("cmp rax, rcx");
    ins(std::string(setcc) + " al");
    ins("movzx eax, al");
  };

  switch (e->op) {
    case BinOp::Add:
      ins("add rax, rcx");
      setLine(e->loc);
      ins("jo .Lerr_overflow");
      return;
    case BinOp::Sub:
      ins("sub rax, rcx");
      setLine(e->loc);
      ins("jo .Lerr_overflow");
      return;
    case BinOp::Mul:
      ins("imul rax, rcx");
      setLine(e->loc);
      ins("jo .Lerr_overflow");
      return;
    case BinOp::Div: {
      // 6.7.2.2: truncation toward zero, which is what idiv does. A divisor
      // of -1 is negated instead, because idiv traps on the most negative
      // dividend.
      std::string general = newLabel();
      std::string done = newLabel();
      setLine(e->loc);
      ins("test rcx, rcx");
      ins("jz .Lerr_divzero");
      ins("cmp rcx, -1");
      ins("jne " + general);
      ins("neg rax");
      ins("jo .Lerr_overflow");
      ins("jmp " + done);
      label(general);
      ins("cqo");
      ins("idiv rcx");
      label(done);
      return;
    }
    case BinOp::Mod: {
      // 6.7.2.2: an error unless j > 0, and then 0 <= i mod j < j. idiv gives
      // a remainder with the sign of i, so a negative one is moved up by j.
      std::string done = newLabel();
      setLine(e->loc);
      ins("cmp rcx, 0");
      ins("jle .Lerr_mod");
      ins("cqo");
      ins("idiv rcx");
      ins("mov rax, rdx");
      ins("test rax, rax");
      ins("jns " + done);
      ins("add rax, rcx");
      label(done);
      return;
    }
    case BinOp::And: ins("and rax, rcx"); return;
    case BinOp::Or: ins("or rax, rcx"); return;
    case BinOp::Eq: compare("sete"); return;
    case BinOp::Ne: compare("setne"); return;
    case BinOp::Lt: compare("setl"); return;
    case BinOp::Le: compare("setle"); return;
    case BinOp::Gt: compare("setg"); return;
    case BinOp::Ge: compare("setge"); return;
    case BinOp::RealDiv: return;  // rejected by sema
  }
}
