// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <type_traits>
#include <variant>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
template<typename T, typename Tuple>
struct variant_has_type;

//----------------------------------------------------------------------------------------
template<typename T, typename... Us>
struct variant_has_type<T, std::variant<Us...>> : std::disjunction<std::is_same<T, Us>...>
{};

} // namespace cobalt::graphics
