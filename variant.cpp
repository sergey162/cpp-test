#include <memory>
#include <new>
#include <cstdint>
#include <algorithm>
#include <string>
#include <iostream>
#include <exception>
#include <type_traits>

namespace customspace {
class BadVariantAccess : std::exception {
public:
  explicit BadVariantAccess(const std::string& text) : error_message_(text) {}

  explicit BadVariantAccess(std::string&& text) : error_message_(std::move(text)) {}
  const char* what() const noexcept override {
    return error_message_.data();
  }

private:
  std::string_view error_message_;
};
}
// Computation max_sizeof, which will use in align Variant buffer
template <typename...>
struct max_sizeof;

template <typename T, typename... Types>
struct max_sizeof<T, Types...> {
  static constexpr std::size_t value = std::max(sizeof(T), max_sizeof<Types...>::value);
};

template <typename T>
struct max_sizeof<T> {
  static constexpr std::size_t value = sizeof(T);
};

template <>
struct max_sizeof<> {
  static constexpr std::size_t value = 0;
};

// Computation Active Index in Variant args pack
template <typename...>
struct comp_active_index;

template <typename T, typename Head, typename... Tail>
struct comp_active_index<T, Head, Tail...> {
    static constexpr std::size_t value = 1 + comp_active_index<T, Tail...>::value;
};

template <typename T, typename... Tail>
struct comp_active_index<T, T, Tail...> {
    static constexpr std::size_t value = 1;
};

template <typename T>
struct comp_active_index<T, T> {
    static constexpr std::size_t value = 1;
};

template <typename T, typename Head>
struct comp_active_index<T, Head> {
    static constexpr std::size_t value = 0;
};

// declaration Variant
template <typename... Types>
class Variant;

// VariantEntity its struct with buffer and active_index Variant
template <typename... Types>
struct VariantEntity {
protected:
  alignas(max_sizeof<Types...>::value) char buffer[max_sizeof<Types...>::value];
  std::size_t active_index = 0;
};

// VariantAlternative use CRTP Idiom for correct call constructors/destructors and other Variant methods
template <typename T, typename... Types>
class VariantAlternative {
public:
  constexpr VariantAlternative() : pack_index(comp_active_index<T, Types...>::value) {}

  constexpr VariantAlternative(const T& value) : pack_index(comp_active_index<T, Types...>::value) {
    try {    
      auto variant = static_cast<Variant<Types...>*>(this);
      new (std::launder(variant->buffer)) T(value);
      variant->active_index = comp_active_index<T, Types...>::value;
    } catch(...) {
      throw;
    }
  }

  constexpr VariantAlternative(T&& value) : pack_index(comp_active_index<T, Types...>::value) {
    try {
      auto variant = static_cast<Variant<Types...>*>(this);
      new (std::launder(variant->buffer)) T(std::move(value));
      variant->active_index = comp_active_index<T, Types...>::value;
    } catch(...) {
      throw;
    }
  }
protected:

  constexpr void Destroy() noexcept {
    auto variant = static_cast<Variant<Types...>*>(this);
    if (pack_index == variant->active_index) {
      reinterpret_cast<T*>(variant->buffer)->~T();
      variant->active_index = 0;
    }
    variant->active_index = 0;
  }

  constexpr void Construct(Variant<Types...>&& other) {
    try {
      auto variant = static_cast<Variant<Types...>*>(this);
      if (other.active_index == pack_index) {
        new (std::launder(variant->buffer)) T(std::move(other).template Get<T>());
      }
    } catch(...) {
      throw;
    }
  }

  constexpr void Construct(const Variant<Types...>& other) {
    try {
      auto variant = static_cast<Variant<Types...>*>(this);
      if (other.active_index == pack_index) {
        new (std::launder(variant->buffer)) T(other.template Get<T>());
      }
    } catch(...) {
      throw;
    }
  }

  constexpr size_t PackIndex() const noexcept {
    return pack_index;
  }


  ~VariantAlternative() = default;

private:
  std::size_t pack_index = 0;
};


template <typename... Types>
class Variant : public VariantEntity<Types...>, private VariantAlternative<Types, Types...>... {
private:
  template <typename T, typename... Ts>
  friend class VariantAlternative;

  using VariantAlternative<Types, Types...>::Destroy...;
  using VariantAlternative<Types, Types...>::Construct...;
  using VariantAlternative<Types, Types...>::PackIndex...;
public:

  using VariantAlternative<Types, Types...>::VariantAlternative...;


  constexpr Variant() {
    this->active_index = 0;
  }

  constexpr Variant(Variant&& other) {
    this->active_index = other.active_index;
    (VariantAlternative<Types, Types...>::Construct(std::move(other)), ...);
    other.active_index = 0;
  }

  constexpr Variant(const Variant& other) {
    this->active_index = other.active_index;
    (VariantAlternative<Types, Types...>::Construct(other), ...);
  }

  constexpr Variant& operator=(const Variant& other) {
    (VariantAlternative<Types, Types...>::Destroy(), ...);
    (VariantAlternative<Types, Types...>::Construct(other), ...);
    return *this;
  }

  constexpr Variant& operator=(Variant& other) {
    (VariantAlternative<Types, Types...>::Destroy(), ...);
    (VariantAlternative<Types, Types...>::Construct(other), ...);
    return *this;
  }

  constexpr Variant& operator=(Variant&& other) noexcept {
    (VariantAlternative<Types, Types...>::Destroy(), ...);
    (VariantAlternative<Types, Types...>::Construct(std::move(other)), ...);
    other.active_index = 0;
    return *this;
  }

  template <typename U>
  constexpr Variant& operator=(U&& value) {
    if (this->VariantAlternative<std::remove_cvref_t<U>, Types...>::PackIndex() == this->active_index) {
      Get<std::remove_cvref_t<U>>() = std::forward<U&&>(value);
    } else {
      (VariantAlternative<Types, Types...>::Destroy(), ...);
      this->active_index = VariantAlternative<std::remove_cvref_t<U>, Types...>::PackIndex();
      try {    
        new (std::launder(this->buffer)) std::remove_cvref_t<U>(std::forward<U&&>(value));
      } catch(...) {
        throw;
      }
    }
    return *this;
  }

  constexpr std::size_t Index() const noexcept {
    return this->active_index;
  };

  template <typename T>
  constexpr T& Get() & {
    if (this->VariantAlternative<std::remove_reference_t<T>, Types...>::PackIndex() != this->active_index) {
      throw customspace::BadVariantAccess("T is not now in Variant");
    }
    return *reinterpret_cast<T*>(this->buffer);
  }

  template <typename T>
  constexpr const T& Get() const & {
    if (this->VariantAlternative<std::remove_reference_t<T>, Types...>::PackIndex() != this->active_index) {
      throw customspace::BadVariantAccess("T is not now in Variant");
    }
    return *reinterpret_cast<T*>(this->buffer);
  }
  
  template <typename T>
  constexpr T&& Get() && {
    if (this->VariantAlternative<std::remove_reference_t<T>, Types...>::PackIndex() != this->active_index) {
      throw customspace::BadVariantAccess("T is not now in Variant");
    }
    return std::move(*reinterpret_cast<T*>(this->buffer));
  }

  template <typename T>
  constexpr const T&& Get() const && {
    if (this->VariantAlternative<std::remove_reference_t<T>, Types...>::PackIndex() != this->active_index) {
      throw customspace::BadVariantAccess("T is not now in Variant");
    }
    return std::move(*reinterpret_cast<T*>(this->buffer));
  }

  ~Variant() noexcept(noexcept((std::is_trivially_destructible_v<Types>, ...))){
    (VariantAlternative<Types, Types...>::Destroy(), ...);
  }

};


int main() {
  Variant<int, double> my = 2;
  int n;
  std::cin >> n;
  my = n;

}
