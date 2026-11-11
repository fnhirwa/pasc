#include "parser.h"

namespace {

bool startsStatement(Tok k) {
  switch (k) {
    case Tok::Ident:
    case Tok::KwBegin:
    case Tok::KwIf:
    case Tok::KwWhile:
    case Tok::KwRepeat:
    case Tok::KwFor:
    case Tok::KwCase:
    case Tok::KwWith:
    case Tok::KwGoto:
      return true;
    default:
      return false;
  }
}

}  // namespace

Parser::Parser(Lexer& lexer, Arena& arena, Diag& diag)
    : lexer_(lexer), arena_(arena), diag_(diag) {
  advance();
}

bool Parser::accept(Tok k) {
  if (!at(k)) return false;
  advance();
  return true;
}

// Reports at most one error per token, which removes most cascades.
void Parser::error(const std::string& msg) {
  if (tok_.loc.line == lastErrorLoc_.line && tok_.loc.col == lastErrorLoc_.col)
    return;
  lastErrorLoc_ = tok_.loc;
  diag_.error(tok_.loc, msg);
}

bool Parser::expect(Tok k) {
  if (accept(k)) return true;
  error(std::string("expected ") + tokName(k) + " but found " +
        tokName(tok_.kind));
  return false;
}

void Parser::unsupported(const char* feature) {
  error(std::string(feature) + " is not implemented yet");
}

// Skips to a token that can follow a statement.
void Parser::syncStatement() {
  while (!at(Tok::Semi) && !at(Tok::KwEnd) && !at(Tok::KwUntil) &&
         !at(Tok::KwElse) && !at(Tok::Eof))
    advance();
}

// 6.10: program = program-heading ';' program-block '.' .
Program* Parser::parseProgram() {
  Program* p = arena_.make<Program>();
  p->loc = tok_.loc;
  expect(Tok::KwProgram);
  if (at(Tok::Ident)) {
    p->name = tok_.text;
    advance();
  } else {
    error("expected the program name");
  }
  if (accept(Tok::LParen)) {
    do {
      if (at(Tok::Ident)) {
        p->params.push_back({tok_.text, tok_.loc});
        advance();
      } else {
        error("expected a program parameter");
        break;
      }
    } while (accept(Tok::Comma));
    expect(Tok::RParen);
  }
  expect(Tok::Semi);
  parseBlock(*p);
  expect(Tok::Dot);
  if (!at(Tok::Eof)) error("text after the end of the program");
  return p;
}

// 6.2.1: the parts of a block appear in a fixed order.
void Parser::parseBlock(Program& p) {
  if (at(Tok::KwLabel)) unsupported("label-declaration-part");
  if (at(Tok::KwConst)) unsupported("constant-definition-part");
  if (at(Tok::KwType)) unsupported("type-definition-part");
  while (!at(Tok::KwVar) && !at(Tok::KwBegin) && !at(Tok::Eof) &&
         !at(Tok::KwProcedure) && !at(Tok::KwFunction))
    advance();  // only reached after an "unsupported" error above
  if (at(Tok::KwVar)) parseVarPart(p);
  if (at(Tok::KwProcedure) || at(Tok::KwFunction)) {
    unsupported("procedure-and-function-declaration-part");
    while (!at(Tok::Eof)) advance();
  }
  p.body = parseCompound();
}

// 6.2.1, 6.5.1: 'var' variable-declaration ';' { variable-declaration ';' } .
void Parser::parseVarPart(Program& p) {
  expect(Tok::KwVar);
  do {
    VarGroup group;
    do {
      if (!at(Tok::Ident)) {
        error("expected a variable identifier");
        break;
      }
      group.vars.push_back(arena_.make<VarDecl>(tok_.text, tok_.loc));
      advance();
    } while (accept(Tok::Comma));
    expect(Tok::Colon);
    group.typeLoc = tok_.loc;
    if (at(Tok::Ident)) {
      group.typeName = tok_.text;
      advance();
    } else {
      if (at(Tok::KwArray) || at(Tok::KwRecord) || at(Tok::KwSet) ||
          at(Tok::KwFile) || at(Tok::KwPacked) || at(Tok::Caret) ||
          at(Tok::LParen) || at(Tok::IntLit) || at(Tok::StrLit) ||
          at(Tok::Plus) || at(Tok::Minus))
        unsupported("a new-type in a variable-declaration");
      else
        error("expected a type identifier");
      while (!at(Tok::Semi) && !at(Tok::KwBegin) && !at(Tok::Eof)) advance();
    }
    p.varGroups.push_back(std::move(group));
    expect(Tok::Semi);
  } while (at(Tok::Ident));
}

CompoundStmt* Parser::parseCompound() {
  CompoundStmt* c = arena_.make<CompoundStmt>(tok_.loc);
  expect(Tok::KwBegin);
  parseStatementSeq(c->body);
  expect(Tok::KwEnd);
  return c;
}

// 6.8.3.1: statement-sequence = statement { ';' statement } .
void Parser::parseStatementSeq(std::vector<Stmt*>& out) {
  for (;;) {
    out.push_back(parseStatement());
    if (accept(Tok::Semi)) continue;
    if (at(Tok::KwEnd) || at(Tok::KwUntil) || at(Tok::Eof)) return;
    error(std::string("expected ';' but found ") + tokName(tok_.kind));
    if (!startsStatement(tok_.kind)) advance();  // guarantee progress
  }
}

Stmt* Parser::parseStatement() {
  SourceLoc loc = tok_.loc;
  switch (tok_.kind) {
    case Tok::KwBegin:
      return parseCompound();
    case Tok::KwIf: {
      advance();
      Expr* cond = parseExpression();
      expect(Tok::KwThen);
      Stmt* thenStmt = parseStatement();
      // The else binds to the nearest if (6.8.3.4).
      Stmt* elseStmt = accept(Tok::KwElse) ? parseStatement() : nullptr;
      return arena_.make<IfStmt>(cond, thenStmt, elseStmt, loc);
    }
    case Tok::KwWhile: {
      advance();
      Expr* cond = parseExpression();
      expect(Tok::KwDo);
      return arena_.make<WhileStmt>(cond, parseStatement(), loc);
    }
    case Tok::KwRepeat: {
      advance();
      RepeatStmt* r = arena_.make<RepeatStmt>(loc);
      parseStatementSeq(r->body);
      expect(Tok::KwUntil);
      r->cond = parseExpression();
      return r;
    }
    case Tok::Ident:
      return parseIdentStatement();
    case Tok::Semi:
    case Tok::KwEnd:
    case Tok::KwUntil:
    case Tok::KwElse:
    case Tok::Eof:
      return arena_.make<EmptyStmt>(loc);  // 6.8.2.1 empty-statement
    case Tok::KwFor: unsupported("for-statement"); break;
    case Tok::KwCase: unsupported("case-statement"); break;
    case Tok::KwWith: unsupported("with-statement"); break;
    case Tok::KwGoto: unsupported("goto-statement"); break;
    case Tok::IntLit: unsupported("a statement label"); break;
    default:
      error(std::string("expected a statement but found ") +
            tokName(tok_.kind));
      break;
  }
  syncStatement();
  return arena_.make<EmptyStmt>(loc);
}

// A statement that starts with an identifier is an assignment-statement when
// ':=' follows the variable-access, otherwise a procedure-statement.
Stmt* Parser::parseIdentStatement() {
  SourceLoc loc = tok_.loc;
  std::string name = tok_.text;
  advance();
  if (accept(Tok::Assign)) {
    Expr* target = arena_.make<NameRef>(name, loc);
    return arena_.make<AssignStmt>(target, parseExpression(), loc);
  }
  CallStmt* call = arena_.make<CallStmt>(name, loc);
  if (accept(Tok::LParen)) {
    do {
      Arg arg;
      arg.value = parseExpression();
      if (accept(Tok::Colon)) arg.width = parseExpression();
      if (accept(Tok::Colon)) arg.frac = parseExpression();
      call->args.push_back(arg);
    } while (accept(Tok::Comma));
    expect(Tok::RParen);
  } else if (!at(Tok::Semi) && !at(Tok::KwEnd) && !at(Tok::KwUntil) &&
             !at(Tok::KwElse) && !at(Tok::Eof)) {
    if (at(Tok::Eq))
      error("expected ':=' but found '='");
    else
      error(std::string("expected ':=' or '(' but found ") +
            tokName(tok_.kind));
    syncStatement();
  }
  return call;
}

// 6.7.1: expression = simple-expression [ relational-operator
// simple-expression ] .
Expr* Parser::parseExpression() {
  Expr* lhs = parseSimpleExpression();
  BinOp op;
  switch (tok_.kind) {
    case Tok::Eq: op = BinOp::Eq; break;
    case Tok::Ne: op = BinOp::Ne; break;
    case Tok::Lt: op = BinOp::Lt; break;
    case Tok::Le: op = BinOp::Le; break;
    case Tok::Gt: op = BinOp::Gt; break;
    case Tok::Ge: op = BinOp::Ge; break;
    case Tok::KwIn:
      unsupported("the 'in' operator");
      advance();
      parseSimpleExpression();
      return lhs;
    default:
      return lhs;
  }
  SourceLoc loc = tok_.loc;
  advance();
  return arena_.make<Binary>(op, lhs, parseSimpleExpression(), loc);
}

// simple-expression = [ sign ] term { adding-operator term } .
// The sign applies to the first term only.
Expr* Parser::parseSimpleExpression() {
  SourceLoc signLoc = tok_.loc;
  bool neg = at(Tok::Minus);
  bool signedTerm = neg || at(Tok::Plus);
  if (signedTerm) advance();
  Expr* lhs = parseTerm();
  if (signedTerm)
    lhs = arena_.make<Unary>(neg ? UnOp::Neg : UnOp::Plus, lhs, signLoc);
  for (;;) {
    BinOp op;
    switch (tok_.kind) {
      case Tok::Plus: op = BinOp::Add; break;
      case Tok::Minus: op = BinOp::Sub; break;
      case Tok::KwOr: op = BinOp::Or; break;
      default: return lhs;
    }
    SourceLoc loc = tok_.loc;
    advance();
    lhs = arena_.make<Binary>(op, lhs, parseTerm(), loc);
  }
}

// term = factor { multiplying-operator factor } .
Expr* Parser::parseTerm() {
  Expr* lhs = parseFactor();
  for (;;) {
    BinOp op;
    switch (tok_.kind) {
      case Tok::Star: op = BinOp::Mul; break;
      case Tok::Slash: op = BinOp::RealDiv; break;
      case Tok::KwDiv: op = BinOp::Div; break;
      case Tok::KwMod: op = BinOp::Mod; break;
      case Tok::KwAnd: op = BinOp::And; break;
      default: return lhs;
    }
    SourceLoc loc = tok_.loc;
    advance();
    lhs = arena_.make<Binary>(op, lhs, parseFactor(), loc);
  }
}

Expr* Parser::parseFactor() {
  SourceLoc loc = tok_.loc;
  switch (tok_.kind) {
    case Tok::IntLit: {
      int64_t v = tok_.intVal;
      advance();
      return arena_.make<IntLit>(v, loc);
    }
    case Tok::StrLit: {
      std::string s = tok_.text;
      advance();
      return arena_.make<StrLit>(std::move(s), loc);
    }
    case Tok::Ident: {
      std::string name = tok_.text;
      advance();
      if (at(Tok::LParen)) unsupported("a function-designator");
      if (at(Tok::LBrack)) unsupported("an indexed-variable");
      return arena_.make<NameRef>(std::move(name), loc);
    }
    case Tok::LParen: {
      advance();
      Expr* e = parseExpression();
      expect(Tok::RParen);
      return e;
    }
    case Tok::KwNot:
      advance();
      return arena_.make<Unary>(UnOp::Not, parseFactor(), loc);
    case Tok::RealLit:
      unsupported("the real-type");
      advance();
      break;
    case Tok::KwNil:
      unsupported("pointer-types");
      advance();
      break;
    case Tok::LBrack:
      unsupported("a set-constructor");
      advance();
      break;
    default:
      error(std::string("expected an expression but found ") +
            tokName(tok_.kind));
      break;
  }
  return arena_.make<IntLit>(0, loc);  // placeholder after an error
}
