#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>

// ISO 7185 6.4. Types are created once and never copied, so pointer equality
// is "the same type" (every new-type is distinct from every other).
struct Type {
  enum class Kind { Integer, Boolean, Char, String, Text };
  Kind kind;
  int64_t length = 0;  // String: number of components
};

class TypeContext {
 public:
  const Type* integerTy() const { return &integer_; }
  const Type* booleanTy() const { return &boolean_; }
  const Type* charTy() const { return &char_; }
  const Type* textTy() const { return &text_; }
  // 6.1.7: all character-strings of one length possess the same string-type.
  const Type* stringTy(int64_t length);

 private:
  Type integer_{Type::Kind::Integer};
  Type boolean_{Type::Kind::Boolean};
  Type char_{Type::Kind::Char};
  Type text_{Type::Kind::Text};
  std::map<int64_t, std::unique_ptr<Type>> strings_;
};

bool isOrdinal(const Type* t);
bool compatible(const Type* a, const Type* b);                // 6.4.5
bool assignmentCompatible(const Type* dst, const Type* src);  // 6.4.6
std::string typeName(const Type* t);
