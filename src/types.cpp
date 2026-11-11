#include "types.h"

const Type* TypeContext::stringTy(int64_t length) {
  auto& slot = strings_[length];
  if (!slot) slot.reset(new Type{Type::Kind::String, length});
  return slot.get();
}

bool isOrdinal(const Type* t) {
  switch (t->kind) {
    case Type::Kind::Integer:
    case Type::Kind::Boolean:
    case Type::Kind::Char:
      return true;
    case Type::Kind::String:
    case Type::Kind::Text:
      return false;
  }
  return false;
}

// 6.4.5. Subranges and sets will add cases b) and c). Case d), string-types
// with the same number of components, is pointer equality here because
// string-types are interned by length.
bool compatible(const Type* a, const Type* b) { return a == b; }

// 6.4.6. Case a) excludes file-types; case b), real := integer, arrives with
// the real-type.
bool assignmentCompatible(const Type* dst, const Type* src) {
  if (dst->kind == Type::Kind::Text) return false;
  return compatible(dst, src);
}

std::string typeName(const Type* t) {
  if (!t) return "<error>";
  switch (t->kind) {
    case Type::Kind::Integer: return "integer";
    case Type::Kind::Boolean: return "Boolean";
    case Type::Kind::Char: return "char";
    case Type::Kind::Text: return "text";
    case Type::Kind::String:
      return "packed array [1.." + std::to_string(t->length) + "] of char";
  }
  return "?";
}
