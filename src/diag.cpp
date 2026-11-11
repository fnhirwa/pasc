#include "diag.h"

#include <iostream>

void Diag::error(SourceLoc loc, const std::string& msg) {
  ++errors_;
  std::cerr << file_ << ':' << loc.line << ':' << loc.col << ": error: " << msg
            << '\n';
}
