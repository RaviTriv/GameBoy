// clang-format Language: Cpp
#pragma once

#include <atomic>
#include <cstdint>
#include <istream>
#include <memory>
#include <meta>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

class Serializable {
 public:
  virtual ~Serializable() = default;
  virtual void save(std::ostream& out) const = 0;
  virtual void load(std::istream& in) = 0;
};

namespace serial {

template <typename T>
void write(std::ostream& out, const T& value) {
  static_assert(std::is_trivially_copyable_v<T>);
  out.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

template <typename T>
void read(std::istream& in, T& value) {
  static_assert(std::is_trivially_copyable_v<T>);
  in.read(reinterpret_cast<char*>(&value), sizeof(T));
  if (!in) {
    throw std::runtime_error("Unexpected end of save state data");
  }
}

inline void writeBytes(std::ostream& out, const std::vector<uint8_t>& bytes) {
  write(out, static_cast<uint64_t>(bytes.size()));
  out.write(reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
}

inline void readBytes(std::istream& in, std::vector<uint8_t>& bytes) {
  uint64_t size = 0;
  read(in, size);
  if (size != bytes.size()) {
    throw std::runtime_error("Save state buffer size does not match");
  }
  in.read(reinterpret_cast<char*>(bytes.data()),
          static_cast<std::streamsize>(bytes.size()));
  if (!in) {
    throw std::runtime_error("Unexpected end of save state data");
  }
}

struct Skip {};
inline constexpr Skip skip{};

template <typename T>
concept SelfSerializing = requires(const T& constValue, T& value,
                                   std::ostream& out, std::istream& in) {
  constValue.save(out);
  value.load(in);
};

namespace detail {

template <typename T>
inline constexpr bool isAtomic = false;
template <typename T>
inline constexpr bool isAtomic<std::atomic<T>> = true;

template <typename T>
inline constexpr bool isUniquePtr = false;
template <typename T>
inline constexpr bool isUniquePtr<std::unique_ptr<T>> = true;

consteval bool isSkipped(std::meta::info member) {
  return !std::meta::annotations_of_with_type(member, ^^Skip).empty();
}

template <typename T>
consteval bool isRaw() {
  if constexpr (std::is_array_v<T>) {
    return isRaw<std::remove_all_extents_t<T>>();
  } else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    return true;
  } else if constexpr ((std::is_class_v<T> || std::is_union_v<T>) &&
                       std::is_trivially_copyable_v<T> && !SelfSerializing<T>) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto base :
                  std::define_static_array(std::meta::bases_of(^^T, ctx))) {
      if constexpr (!isRaw<typename[:std::meta::type_of(base):]>()) {
        return false;
      }
    }
    template for (constexpr auto member : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, ctx))) {
      if constexpr (isSkipped(member) ||
                    !isRaw<typename[:std::meta::type_of(member):]>()) {
        return false;
      }
    }
    return true;
  } else {
    return false;
  }
}

enum class Kind { Delegate, Atomic, Bytes, Owned, Raw, Fields, Unsupported };

template <typename T>
consteval Kind kindOf() {
  if constexpr (SelfSerializing<T>) {
    return Kind::Delegate;
  } else if constexpr (isAtomic<T>) {
    static_assert(isRaw<typename T::value_type>(),
                  "serial: atomic members must hold a raw value type");
    return Kind::Atomic;
  } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
    return Kind::Bytes;
  } else if constexpr (isUniquePtr<T>) {
    static_assert(SelfSerializing<typename T::element_type>,
                  "serial: unique_ptr members must point to a type with "
                  "save/load, or be annotated [[=serial::skip]]");
    return Kind::Owned;
  } else if constexpr (isRaw<T>()) {
    return Kind::Raw;
  } else if constexpr (std::is_class_v<T> && std::is_aggregate_v<T>) {
    return Kind::Fields;
  } else if constexpr (std::is_pointer_v<T>) {
    static_assert(false,
                  "serial: pointer members must be annotated "
                  "[[=serial::skip]] and restored after load");
    return Kind::Unsupported;
  } else {
    static_assert(false,
                  "serial: unsupported member type; annotate it "
                  "[[=serial::skip]] or give it save/load");
    return Kind::Unsupported;
  }
}

template <std::meta::info Member>
consteval bool isSaved() {
  if constexpr (isSkipped(Member)) {
    return false;
  } else {
    static_assert(!std::meta::is_reference_type(std::meta::type_of(Member)),
                  "serial: reference members must be annotated "
                  "[[=serial::skip]]");
    static_assert(!std::meta::is_bit_field(Member),
                  "serial: bit-field members must be inside a raw struct");
    return true;
  }
}

}  // namespace detail

template <typename T>
void saveFields(std::ostream& out, const T& obj);
template <typename T>
void loadFields(std::istream& in, T& obj);

template <typename T>
void saveValue(std::ostream& out, const T& value) {
  constexpr detail::Kind kind = detail::kindOf<T>();
  if constexpr (kind == detail::Kind::Delegate) {
    value.save(out);
  } else if constexpr (kind == detail::Kind::Atomic) {
    write(out, value.load(std::memory_order_relaxed));
  } else if constexpr (kind == detail::Kind::Bytes) {
    writeBytes(out, value);
  } else if constexpr (kind == detail::Kind::Owned) {
    value->save(out);
  } else if constexpr (kind == detail::Kind::Raw) {
    write(out, value);
  } else if constexpr (kind == detail::Kind::Fields) {
    saveFields(out, value);
  }
}

template <typename T>
void loadValue(std::istream& in, T& value) {
  constexpr detail::Kind kind = detail::kindOf<T>();
  if constexpr (kind == detail::Kind::Delegate) {
    value.load(in);
  } else if constexpr (kind == detail::Kind::Atomic) {
    typename T::value_type loaded{};
    read(in, loaded);
    value.store(loaded, std::memory_order_relaxed);
  } else if constexpr (kind == detail::Kind::Bytes) {
    readBytes(in, value);
  } else if constexpr (kind == detail::Kind::Owned) {
    value->load(in);
  } else if constexpr (kind == detail::Kind::Raw) {
    read(in, value);
  } else if constexpr (kind == detail::Kind::Fields) {
    loadFields(in, value);
  }
}

template <typename T>
void saveFields(std::ostream& out, const T& obj) {
  constexpr auto ctx = std::meta::access_context::unchecked();
  template for (constexpr auto base :
                std::define_static_array(std::meta::bases_of(^^T, ctx))) {
    saveFields(out,
               static_cast<const typename[:std::meta::type_of(base):]&>(obj));
  }
  template for (constexpr auto member : std::define_static_array(
                    std::meta::nonstatic_data_members_of(^^T, ctx))) {
    if constexpr (detail::isSaved<member>()) {
      saveValue(out, obj.[:member:]);
    }
  }
}

template <typename T>
void loadFields(std::istream& in, T& obj) {
  constexpr auto ctx = std::meta::access_context::unchecked();
  template for (constexpr auto base :
                std::define_static_array(std::meta::bases_of(^^T, ctx))) {
    loadFields(in, static_cast<typename[:std::meta::type_of(base):]&>(obj));
  }
  template for (constexpr auto member : std::define_static_array(
                    std::meta::nonstatic_data_members_of(^^T, ctx))) {
    if constexpr (detail::isSaved<member>()) {
      loadValue(in, obj.[:member:]);
    }
  }
}

}  // namespace serial
