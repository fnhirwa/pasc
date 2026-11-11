#pragma once
#include <string>

#include "source.h"

// Collects compile-time diagnostics. User errors never throw; callers keep
// going and the driver checks errorCount() between phases.
class Diag {
 public:
  explicit Diag(std::string file) : file_(std::move(file)) {}
  void error(SourceLoc loc, const std::string& msg);
  int errorCount() const { return errors_; }

 private:
  std::string file_;
  int errors_ = 0;
};
