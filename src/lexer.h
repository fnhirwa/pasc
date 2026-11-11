#pragma once
#include <string_view>

#include "diag.h"
#include "token.h"

// ISO 7185 6.1: lexical tokens, token separators and lexical alternatives.
class Lexer {
 public:
  Lexer(std::string_view src, Diag& diag) : src_(src), diag_(diag) {}
  Token next();

 private:
  bool atEnd() const { return pos_ >= src_.size(); }
  char peek(size_t ahead = 0) const {
    return pos_ + ahead < src_.size() ? src_[pos_ + ahead] : '\0';
  }
  char advance();
  SourceLoc loc() const { return {line_, col_}; }
  void skipSeparators();
  void lexWord(Token& t);
  void lexNumber(Token& t);
  void lexString(Token& t);

  std::string_view src_;
  Diag& diag_;
  size_t pos_ = 0;
  int line_ = 1;
  int col_ = 1;
};
