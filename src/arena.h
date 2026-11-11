#pragma once
#include <memory>
#include <utility>
#include <vector>

// Owns every AST node and declaration for one compilation. Nodes refer to
// each other through raw non-owning pointers.
class Arena {
 public:
  template <class T, class... Args>
  T* make(Args&&... args) {
    T* p = new T(std::forward<Args>(args)...);
    items_.emplace_back(p, [](void* q) { delete static_cast<T*>(q); });
    return p;
  }

 private:
  std::vector<std::unique_ptr<void, void (*)(void*)>> items_;
};
