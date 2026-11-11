#include "lexer.h"

#include <cstdint>
#include <limits>
#include <unordered_map>

namespace {

bool isLetter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
bool isDigit(char c) { return c >= '0' && c <= '9'; }
char lower(char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; }

const std::unordered_map<std::string, Tok>& keywords() {
  static const std::unordered_map<std::string, Tok> table = {
#define X(name, text) {text, Tok::Kw##name},
      PASC_KEYWORDS(X)
#undef X
  };
  return table;
}

}  // namespace

const char* tokName(Tok kind) {
  switch (kind) {
    case Tok::Eof: return "end of file";
    case Tok::Ident: return "identifier";
    case Tok::IntLit: return "integer";
    case Tok::RealLit: return "real number";
    case Tok::StrLit: return "character-string";
#define X(name, text) case Tok::name: return "'" text "'";
    PASC_SYMBOLS(X)
#undef X
#define X(name, text) case Tok::Kw##name: return "'" text "'";
    PASC_KEYWORDS(X)
#undef X
  }
  return "?";
}

char Lexer::advance() {
  char c = src_[pos_++];
  if (c == '\n') {
    ++line_;
    col_ = 1;
  } else {
    ++col_;
  }
  return c;
}

// 6.1.8: spaces, line separations and comments. A comment opens with { or (*
// and closes with } or *), in any combination.
void Lexer::skipSeparators() {
  for (;;) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') {
      advance();
    } else if (c == '{' || (c == '(' && peek(1) == '*')) {
      SourceLoc start = loc();
      advance();
      if (c == '(') advance();
      bool closed = false;
      while (!atEnd()) {
        if (peek() == '}') {
          advance();
          closed = true;
          break;
        }
        if (peek() == '*' && peek(1) == ')') {
          advance();
          advance();
          closed = true;
          break;
        }
        advance();
      }
      if (!closed) diag_.error(start, "unterminated comment");
    } else {
      return;
    }
  }
}

// 6.1.3: case is insignificant, so the spelling is stored lowercased.
void Lexer::lexWord(Token& t) {
  while (isLetter(peek()) || isDigit(peek())) t.text += lower(advance());
  auto it = keywords().find(t.text);
  t.kind = it == keywords().end() ? Tok::Ident : it->second;
}

// 6.1.5. A '.' starts a fractional-part only when a digit follows, so that
// "1..10" lexes as 1, '..', 10 and "a[1.)" lexes as 1, '.)'.
void Lexer::lexNumber(Token& t) {
  t.kind = Tok::IntLit;
  while (isDigit(peek())) t.text += advance();
  if (peek() == '.' && isDigit(peek(1))) {
    t.kind = Tok::RealLit;
    t.text += advance();
    while (isDigit(peek())) t.text += advance();
  }
  if (peek() == 'e' || peek() == 'E') {
    size_t signLen = (peek(1) == '+' || peek(1) == '-') ? 1 : 0;
    if (isDigit(peek(1 + signLen))) {
      t.kind = Tok::RealLit;
      t.text += advance();
      if (signLen) t.text += advance();
      while (isDigit(peek())) t.text += advance();
    }
  }
  if (isLetter(peek()))
    diag_.error(loc(), "a separator is required between a number and a word");

  if (t.kind == Tok::IntLit) {
    const int64_t maxint = std::numeric_limits<int64_t>::max();
    int64_t v = 0;
    bool overflow = false;
    for (char c : t.text) {
      int d = c - '0';
      if (v > (maxint - d) / 10) {
        overflow = true;
        break;
      }
      v = v * 10 + d;
    }
    if (overflow) diag_.error(t.loc, "integer constant exceeds maxint");
    t.intVal = overflow ? 0 : v;
  }
}

// 6.1.7. Two adjacent apostrophes are the apostrophe-image.
void Lexer::lexString(Token& t) {
  t.kind = Tok::StrLit;
  advance();  // opening apostrophe
  for (;;) {
    if (atEnd() || peek() == '\n') {
      diag_.error(t.loc, "unterminated character-string");
      break;
    }
    char c = advance();
    if (c == '\'') {
      if (peek() != '\'') break;
      advance();
    }
    t.text += c;
  }
  if (t.text.empty()) {
    diag_.error(t.loc, "a character-string must have at least one element");
    t.text = " ";
  }
}

Token Lexer::next() {
  for (;;) {
    skipSeparators();
    Token t;
    t.loc = loc();
    if (atEnd()) return t;

    char c = peek();
    if (isLetter(c)) {
      lexWord(t);
      return t;
    }
    if (isDigit(c)) {
      lexNumber(t);
      return t;
    }
    if (c == '\'') {
      lexString(t);
      return t;
    }

    advance();
    auto two = [&](char second, Tok ifTwo, Tok ifOne) {
      if (peek() == second) {
        advance();
        return ifTwo;
      }
      return ifOne;
    };
    switch (c) {
      case '+': t.kind = Tok::Plus; return t;
      case '-': t.kind = Tok::Minus; return t;
      case '*': t.kind = Tok::Star; return t;
      case '/': t.kind = Tok::Slash; return t;
      case '=': t.kind = Tok::Eq; return t;
      case '[': t.kind = Tok::LBrack; return t;
      case ']': t.kind = Tok::RBrack; return t;
      case ',': t.kind = Tok::Comma; return t;
      case ';': t.kind = Tok::Semi; return t;
      case ')': t.kind = Tok::RParen; return t;
      case '^':
      case '@': t.kind = Tok::Caret; return t;       // 6.1.9 alternative
      case '(': t.kind = two('.', Tok::LBrack, Tok::LParen); return t;
      case ':': t.kind = two('=', Tok::Assign, Tok::Colon); return t;
      case '>': t.kind = two('=', Tok::Ge, Tok::Gt); return t;
      case '<':
        if (peek() == '>') { advance(); t.kind = Tok::Ne; }
        else t.kind = two('=', Tok::Le, Tok::Lt);
        return t;
      case '.':
        if (peek() == '.') { advance(); t.kind = Tok::DotDot; }
        else t.kind = two(')', Tok::RBrack, Tok::Dot);  // '.)' alternative
        return t;
      default:
        diag_.error(t.loc, std::string("illegal character '") + c + "'");
        // Skip it and try again.
    }
  }
}
