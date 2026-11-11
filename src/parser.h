#pragma once
#include "arena.h"
#include "ast.h"
#include "diag.h"
#include "lexer.h"

// Recursive descent parser for the implemented subset of ISO 7185 clause 6.
// It builds an unresolved AST and does no name lookup.
class Parser {
 public:
  Parser(Lexer& lexer, Arena& arena, Diag& diag);
  Program* parseProgram();

 private:
  bool at(Tok k) const { return tok_.kind == k; }
  void advance() { tok_ = lexer_.next(); }
  bool accept(Tok k);
  bool expect(Tok k);
  void error(const std::string& msg);
  void unsupported(const char* feature);
  void syncStatement();

  void parseBlock(Program& p);
  void parseVarPart(Program& p);
  CompoundStmt* parseCompound();
  void parseStatementSeq(std::vector<Stmt*>& out);
  Stmt* parseStatement();
  Stmt* parseIdentStatement();
  Expr* parseExpression();
  Expr* parseSimpleExpression();
  Expr* parseTerm();
  Expr* parseFactor();

  Lexer& lexer_;
  Arena& arena_;
  Diag& diag_;
  Token tok_;
  SourceLoc lastErrorLoc_;
};
