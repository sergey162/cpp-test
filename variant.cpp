#include <memory>
#include <new>
#include <algorithm>

// Computation max_sizeof, which will use in align Variant buffer
template <typename...>
struct max_sizeof;

template <typename T, typename... Types>
struct max_sizeof<T, Types...> {
  static constexpr size_t value = std::max(sizeof(T), max_sizeof<Types..>::value);
};

template <typename T>
struct max_sizeof<T> {
  static constexpr size_t value = sizeof(T);
};

template <>
struct max_sizeof<> {
  static constexpr size_t value = 0;
};

// Computation Active Index in Variant args pack
template <typename...>
struct active_index;

template <typename... Types>
class Variant;

template <typename T, typename... Types>
class VariantAlternative {
public:
  VariantAlternative(const T& value) {
    auto variant = static_cast<Variant<Types...>*>(this);
      
  }
private:
  size_t active_index;
};


template <typename... Types>
class Variant :  {

private:
  alignas(max_sizeof<Types...>::value) char buffer[max_sizeof<Types...>::value];
  size_t active_index;
};
