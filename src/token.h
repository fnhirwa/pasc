#pragma once
#include <cstdint>
#include <string>

#include "source.h"

// ISO 7185 6.1.2 word-symbols.
#define PASC_KEYWORDS(X)                                                     \
  X(And, "and") X(Array, "array") X(Begin, "begin") X(Case, "case")          \
  X(Const, "const") X(Div, "div") X(Do, "do") X(Downto, "downto")            \
  X(Else, "else") X(End, "end") X(File, "file") X(For, "for")                \
  X(Function, "function") X(Goto, "goto") X(If, "if") X(In, "in")            \
  X(Label, "label") X(Mod, "mod") X(Nil, "nil") X(Not, "not") X(Of, "of")    \
  X(Or, "or") X(Packed, "packed") X(Procedure, "procedure")                  \
  X(Program, "program") X(Record, "record") X(Repeat, "repeat")              \
  X(Set, "set") X(Then, "then") X(To, "to") X(Type, "type")                  \
  X(Until, "until") X(Var, "var") X(While, "while") X(With, "with")

// ISO 7185 6.1.2 special-symbols other than word-symbols.
#define PASC_SYMBOLS(X)                                                      \
  X(Plus, "+") X(Minus, "-") X(Star, "*") X(Slash, "/") X(Eq, "=")           \
  X(Lt, "<") X(Gt, ">") X(LBrack, "[") X(RBrack, "]") X(Dot, ".")            \
  X(Comma, ",") X(Colon, ":") X(Semi, ";") X(Caret, "^") X(LParen, "(")      \
  X(RParen, ")") X(Ne, "<>") X(Le, "<=") X(Ge, ">=") X(Assign, ":=")         \
  X(DotDot, "..")

enum class Tok {
  Eof,
  Ident,
  IntLit,
  RealLit,
  StrLit,
#define X(name, text) name,
  PASC_SYMBOLS(X)
#undef X
#define X(name, text) Kw##name,
  PASC_KEYWORDS(X)
#undef X
};

struct Token {
  Tok kind = Tok::Eof;
  SourceLoc loc;
  // Ident: lowercased spelling (6.1.3). StrLit: decoded string-elements.
  // IntLit, RealLit: the digits as written.
  std::string text;
  int64_t intVal = 0;  // IntLit only
};

// Spelling for diagnostics, for example "';'" or "identifier".
const char* tokName(Tok kind);
