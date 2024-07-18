#include <iterator>
#include <memory>


template <typename T, typename Alloc = std::allocator<T>>
class Vector {
public:
  Vector() : arr_(nullptr), sz_(0), cap_(0), alloc_(Alloc()) {} 

private:
  T* arr_;
  size_t sz_;
  size_t cap_;
  [[no_unique_address]] Alloc alloc_;
};
