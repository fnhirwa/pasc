#pragma once

// A position in the program text. Lines and columns are 1-based.
struct SourceLoc {
  int line = 0;
  int col = 0;
};
