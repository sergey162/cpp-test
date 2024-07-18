#include <iterator>
#include <memory>


template <typename T, typename Alloc = std::allocator<T>>
class Vector {
private:
  template <bool IsConst>
  struct Iterator {
  private:
    std::conditional_t<IsConst, const T*, T*> ptr_;
    explicit Iterator(std::conditional_t<IsConst, const T*, T*> ptr) : ptr_(ptr){}   
     friend class Vector;
  public:
    using value_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = value_type&;
    using pointer_type = value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::contiguous_iterator_tag;

    Iterator() : ptr_(nullptr) {}
    Iterator(const Iterator&) = default;
    Iterator& operator=(const Iterator&) = default;
    reference_type operator*() const {
      return *ptr_;
    }
    pointer_type operator->() const {
      return ptr_;
    }
    Iterator& operator++() {
      ++ptr_;
      return *this;
    }


    ~Iterator() = delete;
  };
public:
  Vector() : arr_(nullptr), sz_(0), cap_(0), alloc_(Alloc()) {} 

private:
  T* arr_;
  size_t sz_;
  size_t cap_;
  [[no_unique_address]] Alloc alloc_;
};
