#include <iterator>
#include <memory>


template <typename T, typename Alloc = std::allocator<T>>
class Vector {
    T* arr_;
    size_t sz_;
    size_t cap_;
    // TODO
};
