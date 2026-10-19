// clang-format Language: Cpp
#pragma once

#include <meta>
#include <string_view>
#include <type_traits>

struct EnumLabel {
  const char* text;
  consteval explicit EnumLabel(std::string_view label)
      : text(std::define_static_string(label)) {}
};

template <typename E>
  requires std::is_enum_v<E>
constexpr std::string_view enumName(E value) {
  template for (constexpr auto e :
                std::define_static_array(std::meta::enumerators_of(^^E))) {
    if (value == [:e:]) {
      constexpr auto labels = std::define_static_array(
          std::meta::annotations_of_with_type(e, ^^EnumLabel));
      if constexpr (labels.empty()) {
        return std::meta::identifier_of(e);
      } else {
        return std::meta::extract<EnumLabel>(labels[0]).text;
      }
    }
  }
  return "UNKNOWN";
}
