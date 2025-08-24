/**
 * \file cppgres.hpp
 *
 * \mainpage Cppgres: Postgres extensions in C++
 *
 * \note __GitHub__ repository: [cppgres/cppgres](https://github.com/cppgres/cppgres)
 *
 * Cppgres allows you to build Postgres extensions using C++: a high-performance, feature-rich
 * language already supported by the same compiler toolchains used to develop for Postgres,
 * like GCC and Clang.
 *
 * \subsection Features
 *
 * - Header-only library
 * - Compile and runtime safety checks
 * - Automatic type mapping
 * - Ergonomic executor API
 * - Modern C+++20 interface & implementation
 * - Direct integration with C
 *
 * \subsection qstart Quick start example
 *
 * ```
 * #include <cppgres.hpp>
 *
 * extern "C" {
 *  PG_MODULE_MAGIC;
 * }
 *
 * postgres_function(demo_len, ([](std::string_view t) { return t.length(); }));
 * ```
 *
 */

#ifndef cppgres_hpp
#define cppgres_hpp

#if !defined(cppgres_prefer_fmt) && __has_include(<format>)
#include <format>
#if (defined(__clang__) && defined(_LIBCPP_HAS_NO_INCOMPLETE_FORMAT))
#if __has_include(<fmt/core.h>)
#define FMT_HEADER_ONLY
#include <fmt/core.h>
namespace cppgres::fmt {
using ::fmt::format;
}
#else
#error "Neither functional <format> nor <fmt/core.h> available"
#endif
#else
namespace cppgres::fmt {
using std::format;
}
#endif
#elif __has_include(<fmt/core.h>)
#define FMT_HEADER_ONLY
#include <fmt/core.h>
namespace cppgres::fmt {
using ::fmt::format;
}
#else
#error "Neither functional <format> nor <fmt/core.h> available"
#endif


#include <memory>

/**
 * \file
 */

/**
 * \file
 */

/**
* \file
 */

#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_HPP
#define BOOST_PFR_HPP

/// \file boost/pfr.hpp
/// Includes all the Boost.PFR headers

// Copyright (c) 2016-2023 Antony Polukhin
// Copyright (c) 2022 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_CONFIG_HPP
#define BOOST_PFR_CONFIG_HPP

#if __cplusplus >= 201402L || (defined(_MSC_VER) && defined(_MSVC_LANG) && _MSC_VER > 1900)
#include <type_traits> // to get non standard platform macro definitions (__GLIBCXX__ for example)
#endif

/// \file boost/pfr/config.hpp
/// Contains all the macros that describe Boost.PFR configuration, like BOOST_PFR_ENABLED
///
/// \note This header file doesn't require C++14 Standard and supports all C++ compilers, even pre C++14 compilers (C++11, C++03...).

// Reminder:
//  * MSVC++ 14.2 _MSC_VER == 1927 <- Loophole is known to work (Visual Studio ????)
//  * MSVC++ 14.1 _MSC_VER == 1916 <- Loophole is known to NOT work (Visual Studio 2017)
//  * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
//  * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)

#ifdef BOOST_PFR_NOT_SUPPORTED
#   error Please, do not set BOOST_PFR_NOT_SUPPORTED value manually, use '-DBOOST_PFR_ENABLED=0' instead of it
#endif

#if defined(_MSC_VER)
#   if !defined(_MSVC_LANG) || _MSC_VER <= 1900
#       define BOOST_PFR_NOT_SUPPORTED 1
#   endif
#elif __cplusplus < 201402L
#   define BOOST_PFR_NOT_SUPPORTED 1
#endif

#ifndef BOOST_PFR_USE_LOOPHOLE
#   if defined(_MSC_VER)
#       if _MSC_VER >= 1927
#           define BOOST_PFR_USE_LOOPHOLE 1
#       else
#           define BOOST_PFR_USE_LOOPHOLE 0
#       endif
#   elif defined(__clang_major__) && __clang_major__ >= 8
#       define BOOST_PFR_USE_LOOPHOLE 0
#   else
#       define BOOST_PFR_USE_LOOPHOLE 1
#   endif
#endif

#ifndef BOOST_PFR_USE_CPP17
#   ifdef __cpp_structured_bindings
#       define BOOST_PFR_USE_CPP17 1
#   elif defined(_MSVC_LANG)
#       if _MSVC_LANG >= 201703L
#           define BOOST_PFR_USE_CPP17 1
#       else
#           define BOOST_PFR_USE_CPP17 0
#       endif
#   else
#       define BOOST_PFR_USE_CPP17 0
#   endif
#endif

#if (!BOOST_PFR_USE_CPP17 && !BOOST_PFR_USE_LOOPHOLE)
#   if (defined(_MSC_VER) && _MSC_VER < 1916) ///< in Visual Studio 2017 v15.9 PFR library with classic engine normally works
#      define BOOST_PFR_NOT_SUPPORTED 1
#   endif
#endif

#ifndef BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE
// Assume that libstdc++ since GCC-7.3 does not have linear instantiation depth in std::make_integral_sequence
#   if defined( __GLIBCXX__) && __GLIBCXX__ >= 20180101
#       define BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE 1
#   elif defined(_MSC_VER)
#       define BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE 1
//# elif other known working lib
#   else
#       define BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE 0
#   endif
#endif

#ifndef BOOST_PFR_HAS_GUARANTEED_COPY_ELISION
#   if  defined(__cpp_guaranteed_copy_elision) && (!defined(_MSC_VER) || _MSC_VER > 1928)
#       define BOOST_PFR_HAS_GUARANTEED_COPY_ELISION 1
#   else
#       define BOOST_PFR_HAS_GUARANTEED_COPY_ELISION 0
#   endif
#endif

#ifndef BOOST_PFR_ENABLE_IMPLICIT_REFLECTION
#   if  defined(__cpp_lib_is_aggregate)
#       define BOOST_PFR_ENABLE_IMPLICIT_REFLECTION 1
#   else
// There is no way to detect potential ability to be reflectable without std::is_aggregare
#       define BOOST_PFR_ENABLE_IMPLICIT_REFLECTION 0
#   endif
#endif

#ifndef BOOST_PFR_CORE_NAME_ENABLED
#   if  (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#       if (defined(__cpp_nontype_template_args) && __cpp_nontype_template_args >= 201911) \
         || (defined(__clang_major__) && __clang_major__ >= 12)
#           define BOOST_PFR_CORE_NAME_ENABLED 1
#       else
#           define BOOST_PFR_CORE_NAME_ENABLED 0
#       endif
#   else
#       define BOOST_PFR_CORE_NAME_ENABLED 0
#   endif
#endif


#ifndef BOOST_PFR_CORE_NAME_PARSING
#   if defined(_MSC_VER)
#       define BOOST_PFR_CORE_NAME_PARSING (sizeof("auto __cdecl boost::pfr::detail::name_of_field_impl<") - 1, sizeof(">(void) noexcept") - 1, backward("->"))
#   elif defined(__clang__)
#       define BOOST_PFR_CORE_NAME_PARSING (sizeof("auto boost::pfr::detail::name_of_field_impl() [MsvcWorkaround = ") - 1, sizeof("}]") - 1, backward("."))
#   elif defined(__GNUC__)
#       define BOOST_PFR_CORE_NAME_PARSING (sizeof("consteval auto boost::pfr::detail::name_of_field_impl() [with MsvcWorkaround = ") - 1, sizeof(")]") - 1, backward("::"))
#   else
// Default parser for other platforms... Just skip nothing!
#       define BOOST_PFR_CORE_NAME_PARSING (0, 0, "")
#   endif
#endif

#if defined(__has_cpp_attribute)
#   if __has_cpp_attribute(maybe_unused)
#       define BOOST_PFR_MAYBE_UNUSED [[maybe_unused]]
#   endif
#endif

#ifndef BOOST_PFR_MAYBE_UNUSED
#   define BOOST_PFR_MAYBE_UNUSED
#endif

#ifndef BOOST_PFR_ENABLED
#   ifdef BOOST_PFR_NOT_SUPPORTED
#       define BOOST_PFR_ENABLED 0
#   else
#       define BOOST_PFR_ENABLED 1
#   endif
#endif

#undef BOOST_PFR_NOT_SUPPORTED

#endif // BOOST_PFR_CONFIG_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_CORE_HPP
#define BOOST_PFR_CORE_HPP

// Copyright (c) 2016-2023 Antony Polukhin
// Copyright (c) 2022 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CONFIG_HPP
#define BOOST_PFR_DETAIL_CONFIG_HPP


#if !BOOST_PFR_ENABLED

#error Boost.PFR library is not supported in your environment.             \
       Try one of the possible solutions:                                  \
       1. try to take away an '-DBOOST_PFR_ENABLED=0', if it exists        \
       2. enable C++14;                                                    \
       3. enable C++17;                                                    \
       4. update your compiler;                                            \
       or disable this error by '-DBOOST_PFR_ENABLED=1' if you really know what are you doing.

#endif // !BOOST_PFR_ENABLED

#endif // BOOST_PFR_DETAIL_CONFIG_HPP


// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CORE_HPP
#define BOOST_PFR_DETAIL_CORE_HPP


// Each core provides `boost::pfr::detail::tie_as_tuple` and
// `boost::pfr::detail::for_each_field_dispatcher` functions.
//
// The whole PFR library is build on top of those two functions.
#if BOOST_PFR_USE_CPP17
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PFR_DETAIL_CORE17_HPP
#define BOOST_PFR_DETAIL_CORE17_HPP

// Copyright (c) 2016-2023 Antony Polukhin
// Copyright (c) 2023 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////// THIS HEADER IS AUTO GENERATED BY misc/generate_cpp17.py                                    ////////////////
//////////////// MODIFY AND RUN THE misc/generate_cpp17.py INSTEAD OF DIRECTLY MODIFYING THE GENERATED FILE ////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_PFR_DETAIL_CORE17_GENERATED_HPP
#define BOOST_PFR_DETAIL_CORE17_GENERATED_HPP

#if !BOOST_PFR_USE_CPP17
#   error C++17 is required for this header.
#endif

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_SEQUENCE_TUPLE_HPP
#define BOOST_PFR_DETAIL_SEQUENCE_TUPLE_HPP

// Copyright (c) 2018 Sergei Fedorov
// Copyright (c) 2019-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_MAKE_INTEGER_SEQUENCE_HPP
#define BOOST_PFR_DETAIL_MAKE_INTEGER_SEQUENCE_HPP


#include <type_traits>
#include <utility>
#include <cstddef>

namespace boost { namespace pfr { namespace detail {

#if BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 0

#ifdef __has_builtin
#   if __has_builtin(__make_integer_seq)
#       define BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN
#   endif
#endif

#ifdef BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN

using std::integer_sequence;

// Clang unable to use namespace qualified std::integer_sequence in __make_integer_seq.
template <typename T, T N>
using make_integer_sequence = __make_integer_seq<integer_sequence, T, N>;

#undef BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN

#else

template <typename T, typename U>
struct join_sequences;

template <typename T, T... A, T... B>
struct join_sequences<std::integer_sequence<T, A...>, std::integer_sequence<T, B...>> {
    using type = std::integer_sequence<T, A..., B...>;
};

template <typename T, T Min, T Max>
struct build_sequence_impl {
    static_assert(Min < Max, "Start of range must be less than its end");
    static constexpr T size = Max - Min;
    using type = typename join_sequences<
            typename build_sequence_impl<T, Min, Min + size / 2>::type,
            typename build_sequence_impl<T, Min + size / 2 + 1, Max>::type
        >::type;
};

template <typename T, T V>
struct build_sequence_impl<T, V, V> {
    using type = std::integer_sequence<T, V>;
};

template <typename T, std::size_t N>
struct make_integer_sequence_impl : build_sequence_impl<T, 0, N - 1> {};

template <typename T>
struct make_integer_sequence_impl<T, 0> {
    using type = std::integer_sequence<T>;
};

template <typename T, T N>
using make_integer_sequence = typename make_integer_sequence_impl<T, N>::type;

#endif // !defined BOOST_PFR_USE_MAKE_INTEGER_SEQ_BUILTIN
#else // BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 1

template <typename T, T N>
using make_integer_sequence = std::make_integer_sequence<T, N>;

#endif // BOOST_PFR_USE_STD_MAKE_INTEGRAL_SEQUENCE == 1

template <std::size_t N>
using make_index_sequence = make_integer_sequence<std::size_t, N>;

template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

}}} // namespace boost::pfr::detail

#endif


#include <utility>      // metaprogramming stuff
#include <cstddef>      // std::size_t

///////////////////// Tuple that holds its values in the supplied order
namespace boost { namespace pfr { namespace detail { namespace sequence_tuple {

template <std::size_t N, class T>
struct base_from_member {
    T value;
};

template <class I, class ...Tail>
struct tuple_base;



template <std::size_t... I, class ...Tail>
struct tuple_base< std::index_sequence<I...>, Tail... >
    : base_from_member<I , Tail>...
{
    static constexpr std::size_t size_v = sizeof...(I);

    // We do not use `noexcept` in the following functions, because if user forget to put one then clang will issue an error:
    // "error: exception specification of explicitly defaulted default constructor does not match the calculated one".
    constexpr tuple_base() = default;
    constexpr tuple_base(tuple_base&&) = default;
    constexpr tuple_base(const tuple_base&) = default;

    constexpr tuple_base(Tail... v) noexcept
        : base_from_member<I, Tail>{ v }...
    {}
};

template <>
struct tuple_base<std::index_sequence<> > {
    static constexpr std::size_t size_v = 0;
};

template <std::size_t N, class T>
constexpr T& get_impl(base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <std::size_t N, class T>
constexpr const T& get_impl(const base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <std::size_t N, class T>
constexpr volatile T& get_impl(volatile base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <std::size_t N, class T>
constexpr const volatile T& get_impl(const volatile base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <std::size_t N, class T>
constexpr T&& get_impl(base_from_member<N, T>&& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return std::forward<T>(t.value);
}


template <class T, std::size_t N>
constexpr T& get_by_type_impl(base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <class T, std::size_t N>
constexpr const T& get_by_type_impl(const base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <class T, std::size_t N>
constexpr volatile T& get_by_type_impl(volatile base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <class T, std::size_t N>
constexpr const volatile T& get_by_type_impl(const volatile base_from_member<N, T>& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return t.value;
}

template <class T, std::size_t N>
constexpr T&& get_by_type_impl(base_from_member<N, T>&& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return std::forward<T>(t.value);
}

template <class T, std::size_t N>
constexpr const T&& get_by_type_impl(const base_from_member<N, T>&& t) noexcept {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    return std::forward<T>(t.value);
}




template <class ...Values>
struct tuple: tuple_base<
    detail::index_sequence_for<Values...>,
    Values...>
{
    using tuple_base<
        detail::index_sequence_for<Values...>,
        Values...
    >::tuple_base;

    constexpr static std::size_t size() noexcept { return sizeof...(Values); }
    constexpr static bool empty() noexcept { return size() == 0; }
};


template <std::size_t N, class ...T>
constexpr decltype(auto) get(tuple<T...>& t) noexcept {
    static_assert(N < tuple<T...>::size_v, "====================> Boost.PFR: Tuple index out of bounds");
    return sequence_tuple::get_impl<N>(t);
}

template <std::size_t N, class ...T>
constexpr decltype(auto) get(const tuple<T...>& t) noexcept {
    static_assert(N < tuple<T...>::size_v, "====================> Boost.PFR: Tuple index out of bounds");
    return sequence_tuple::get_impl<N>(t);
}

template <std::size_t N, class ...T>
constexpr decltype(auto) get(const volatile tuple<T...>& t) noexcept {
    static_assert(N < tuple<T...>::size_v, "====================> Boost.PFR: Tuple index out of bounds");
    return sequence_tuple::get_impl<N>(t);
}

template <std::size_t N, class ...T>
constexpr decltype(auto) get(volatile tuple<T...>& t) noexcept {
    static_assert(N < tuple<T...>::size_v, "====================> Boost.PFR: Tuple index out of bounds");
    return sequence_tuple::get_impl<N>(t);
}

template <std::size_t N, class ...T>
constexpr decltype(auto) get(tuple<T...>&& t) noexcept {
    static_assert(N < tuple<T...>::size_v, "====================> Boost.PFR: Tuple index out of bounds");
    return sequence_tuple::get_impl<N>(std::move(t));
}

template <std::size_t I, class T>
using tuple_element = std::remove_reference< decltype(
        ::boost::pfr::detail::sequence_tuple::get<I>( std::declval<T>() )
    ) >;

template <class... Args>
constexpr auto make_sequence_tuple(Args... args) noexcept {
    return ::boost::pfr::detail::sequence_tuple::tuple<Args...>{ args... };
}

}}}} // namespace boost::pfr::detail::sequence_tuple

#endif // BOOST_PFR_CORE_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_SIZE_T_HPP
#define BOOST_PFR_DETAIL_SIZE_T_HPP

namespace boost { namespace pfr { namespace detail {

///////////////////// General utility stuff
template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index >;

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_SIZE_T_HPP
#include <type_traits> // for std::conditional_t, std::is_reference

namespace boost { namespace pfr { namespace detail {

template <class... Args>
constexpr auto make_tuple_of_references(Args&&... args) noexcept {
  return sequence_tuple::tuple<Args&...>{ args... };
}

template<typename T, typename Arg>
constexpr decltype(auto) add_cv_like(Arg& arg) noexcept {
    if constexpr (std::is_const<T>::value && std::is_volatile<T>::value) {
        return const_cast<const volatile Arg&>(arg);
    }
    else if constexpr (std::is_const<T>::value) {
        return const_cast<const Arg&>(arg);
    }
    else if constexpr (std::is_volatile<T>::value) {
        return const_cast<volatile Arg&>(arg);
    }
    else {
        return const_cast<Arg&>(arg);
    }
}

// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78939
template<typename T, typename Sb, typename Arg>
constexpr decltype(auto) workaround_cast(Arg& arg) noexcept {
    using output_arg_t = std::conditional_t<!std::is_reference<Sb>(), decltype(detail::add_cv_like<T>(arg)), Sb>;
    return const_cast<output_arg_t>(arg);
}

template <class T>
constexpr auto tie_as_tuple(T& /*val*/, size_t_<0>) noexcept {
  return sequence_tuple::tuple<>{};
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<1>, std::enable_if_t<std::is_class< std::remove_cv_t<T> >::value>* = nullptr) noexcept {
  auto& [a] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.
  return ::boost::pfr::detail::make_tuple_of_references(detail::workaround_cast<T, decltype(a)>(a));
}


template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<1>, std::enable_if_t<!std::is_class< std::remove_cv_t<T> >::value>* = nullptr) noexcept {
  return ::boost::pfr::detail::make_tuple_of_references( val );
}


template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<2>) noexcept {
  auto& [a,b] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.
  return ::boost::pfr::detail::make_tuple_of_references(detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b));
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<3>) noexcept {
  auto& [a,b,c] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<4>) noexcept {
  auto& [a,b,c,d] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<5>) noexcept {
  auto& [a,b,c,d,e] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<6>) noexcept {
  auto& [a,b,c,d,e,f] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<7>) noexcept {
  auto& [a,b,c,d,e,f,g] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<8>) noexcept {
  auto& [a,b,c,d,e,f,g,h] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<9>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<10>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<11>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<12>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<13>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<14>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<15>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<16>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<17>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<18>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<19>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<20>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<21>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<22>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<23>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<24>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<25>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<26>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<27>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<28>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<29>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<30>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<31>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<32>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<33>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<34>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<35>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<36>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<37>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<38>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<39>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<40>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<41>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<42>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<43>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<44>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<45>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<46>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<47>) noexcept {
  auto& [a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<48>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<49>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<50>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<51>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<52>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<53>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<54>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<55>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<56>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<57>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<58>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<59>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<60>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<61>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<62>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<63>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<64>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<65>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<66>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<67>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<68>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<69>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<70>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<71>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<72>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<73>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<74>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<75>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<76>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<77>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<78>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<79>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<80>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<81>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<82>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<83>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<84>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<85>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<86>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<87>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<88>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<89>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<90>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<91>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<92>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<93>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<94>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<95>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<96>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba,bb
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba),detail::workaround_cast<T, decltype(bb)>(bb)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<97>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba,bb,bc
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba),detail::workaround_cast<T, decltype(bb)>(bb),
    detail::workaround_cast<T, decltype(bc)>(bc)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<98>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba,bb,bc,bd
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba),detail::workaround_cast<T, decltype(bb)>(bb),
    detail::workaround_cast<T, decltype(bc)>(bc),detail::workaround_cast<T, decltype(bd)>(bd)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<99>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba,bb,bc,bd,be
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba),detail::workaround_cast<T, decltype(bb)>(bb),
    detail::workaround_cast<T, decltype(bc)>(bc),detail::workaround_cast<T, decltype(bd)>(bd),detail::workaround_cast<T, decltype(be)>(be)
  );
}

template <class T>
constexpr auto tie_as_tuple(T& val, size_t_<100>) noexcept {
  auto& [
    a,b,c,d,e,f,g,h,j,k,l,m,n,p,q,r,s,t,u,v,w,x,y,z,A,B,C,D,E,F,G,H,J,K,L,M,N,P,Q,R,S,U,V,W,X,Y,Z,
    aa,ab,ac,ad,ae,af,ag,ah,aj,ak,al,am,an,ap,aq,ar,as,at,au,av,aw,ax,ay,az,aA,aB,aC,aD,aE,aF,aG,aH,aJ,aK,aL,aM,aN,aP,aQ,aR,aS,aU,aV,aW,aX,aY,aZ,
    ba,bb,bc,bd,be,bf
  ] = const_cast<std::remove_cv_t<T>&>(val); // ====================> Boost.PFR: User-provided type is not a SimpleAggregate.

  return ::boost::pfr::detail::make_tuple_of_references(
    detail::workaround_cast<T, decltype(a)>(a),detail::workaround_cast<T, decltype(b)>(b),detail::workaround_cast<T, decltype(c)>(c),
    detail::workaround_cast<T, decltype(d)>(d),detail::workaround_cast<T, decltype(e)>(e),detail::workaround_cast<T, decltype(f)>(f),
    detail::workaround_cast<T, decltype(g)>(g),detail::workaround_cast<T, decltype(h)>(h),detail::workaround_cast<T, decltype(j)>(j),
    detail::workaround_cast<T, decltype(k)>(k),detail::workaround_cast<T, decltype(l)>(l),detail::workaround_cast<T, decltype(m)>(m),
    detail::workaround_cast<T, decltype(n)>(n),detail::workaround_cast<T, decltype(p)>(p),detail::workaround_cast<T, decltype(q)>(q),
    detail::workaround_cast<T, decltype(r)>(r),detail::workaround_cast<T, decltype(s)>(s),detail::workaround_cast<T, decltype(t)>(t),
    detail::workaround_cast<T, decltype(u)>(u),detail::workaround_cast<T, decltype(v)>(v),detail::workaround_cast<T, decltype(w)>(w),
    detail::workaround_cast<T, decltype(x)>(x),detail::workaround_cast<T, decltype(y)>(y),detail::workaround_cast<T, decltype(z)>(z),
    detail::workaround_cast<T, decltype(A)>(A),detail::workaround_cast<T, decltype(B)>(B),detail::workaround_cast<T, decltype(C)>(C),
    detail::workaround_cast<T, decltype(D)>(D),detail::workaround_cast<T, decltype(E)>(E),detail::workaround_cast<T, decltype(F)>(F),
    detail::workaround_cast<T, decltype(G)>(G),detail::workaround_cast<T, decltype(H)>(H),detail::workaround_cast<T, decltype(J)>(J),
    detail::workaround_cast<T, decltype(K)>(K),detail::workaround_cast<T, decltype(L)>(L),detail::workaround_cast<T, decltype(M)>(M),
    detail::workaround_cast<T, decltype(N)>(N),detail::workaround_cast<T, decltype(P)>(P),detail::workaround_cast<T, decltype(Q)>(Q),
    detail::workaround_cast<T, decltype(R)>(R),detail::workaround_cast<T, decltype(S)>(S),detail::workaround_cast<T, decltype(U)>(U),
    detail::workaround_cast<T, decltype(V)>(V),detail::workaround_cast<T, decltype(W)>(W),detail::workaround_cast<T, decltype(X)>(X),
    detail::workaround_cast<T, decltype(Y)>(Y),detail::workaround_cast<T, decltype(Z)>(Z),detail::workaround_cast<T, decltype(aa)>(aa),
    detail::workaround_cast<T, decltype(ab)>(ab),detail::workaround_cast<T, decltype(ac)>(ac),detail::workaround_cast<T, decltype(ad)>(ad),
    detail::workaround_cast<T, decltype(ae)>(ae),detail::workaround_cast<T, decltype(af)>(af),detail::workaround_cast<T, decltype(ag)>(ag),
    detail::workaround_cast<T, decltype(ah)>(ah),detail::workaround_cast<T, decltype(aj)>(aj),detail::workaround_cast<T, decltype(ak)>(ak),
    detail::workaround_cast<T, decltype(al)>(al),detail::workaround_cast<T, decltype(am)>(am),detail::workaround_cast<T, decltype(an)>(an),
    detail::workaround_cast<T, decltype(ap)>(ap),detail::workaround_cast<T, decltype(aq)>(aq),detail::workaround_cast<T, decltype(ar)>(ar),
    detail::workaround_cast<T, decltype(as)>(as),detail::workaround_cast<T, decltype(at)>(at),detail::workaround_cast<T, decltype(au)>(au),
    detail::workaround_cast<T, decltype(av)>(av),detail::workaround_cast<T, decltype(aw)>(aw),detail::workaround_cast<T, decltype(ax)>(ax),
    detail::workaround_cast<T, decltype(ay)>(ay),detail::workaround_cast<T, decltype(az)>(az),detail::workaround_cast<T, decltype(aA)>(aA),
    detail::workaround_cast<T, decltype(aB)>(aB),detail::workaround_cast<T, decltype(aC)>(aC),detail::workaround_cast<T, decltype(aD)>(aD),
    detail::workaround_cast<T, decltype(aE)>(aE),detail::workaround_cast<T, decltype(aF)>(aF),detail::workaround_cast<T, decltype(aG)>(aG),
    detail::workaround_cast<T, decltype(aH)>(aH),detail::workaround_cast<T, decltype(aJ)>(aJ),detail::workaround_cast<T, decltype(aK)>(aK),
    detail::workaround_cast<T, decltype(aL)>(aL),detail::workaround_cast<T, decltype(aM)>(aM),detail::workaround_cast<T, decltype(aN)>(aN),
    detail::workaround_cast<T, decltype(aP)>(aP),detail::workaround_cast<T, decltype(aQ)>(aQ),detail::workaround_cast<T, decltype(aR)>(aR),
    detail::workaround_cast<T, decltype(aS)>(aS),detail::workaround_cast<T, decltype(aU)>(aU),detail::workaround_cast<T, decltype(aV)>(aV),
    detail::workaround_cast<T, decltype(aW)>(aW),detail::workaround_cast<T, decltype(aX)>(aX),detail::workaround_cast<T, decltype(aY)>(aY),
    detail::workaround_cast<T, decltype(aZ)>(aZ),detail::workaround_cast<T, decltype(ba)>(ba),detail::workaround_cast<T, decltype(bb)>(bb),
    detail::workaround_cast<T, decltype(bc)>(bc),detail::workaround_cast<T, decltype(bd)>(bd),detail::workaround_cast<T, decltype(be)>(be),
    detail::workaround_cast<T, decltype(bf)>(bf)
  );
}


template <class T, std::size_t I>
constexpr void tie_as_tuple(T& /*val*/, size_t_<I>) noexcept {
  static_assert(sizeof(T) && false,
                "====================> Boost.PFR: Too many fields in a structure T. Regenerate include/boost/pfr/detail/core17_generated.hpp file for appropriate count of fields. For example: `python ./misc/generate_cpp17.py 300 > include/boost/pfr/detail/core17_generated.hpp`");
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE17_GENERATED_HPP

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_FIELDS_COUNT_HPP
#define BOOST_PFR_DETAIL_FIELDS_COUNT_HPP

// Copyright (c) 2019-2023 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP
#define BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP


#include <type_traits>

namespace boost { namespace pfr { namespace detail {

// This function serves as a link-time assert. If linker requires it, then
// `unsafe_declval()` is used at runtime.
void report_if_you_see_link_error_with_this_function() noexcept;

// For returning non default constructible types. Do NOT use at runtime!
//
// GCCs std::declval may not be used in potentionally evaluated contexts,
// so we reinvent it.
template <class T>
constexpr T unsafe_declval() noexcept {
    report_if_you_see_link_error_with_this_function();

    typename std::remove_reference<T>::type* ptr = nullptr;
    ptr += 42; // suppresses 'null pointer dereference' warnings
    return static_cast<T>(*ptr);
}

}}} // namespace boost::pfr::detail


#endif // BOOST_PFR_DETAIL_UNSAFE_DECLVAL_HPP


#include <climits>      // CHAR_BIT
#include <type_traits>
#include <utility>      // metaprogramming stuff

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace boost { namespace pfr { namespace detail {

///////////////////// Structure that can be converted to reference to anything
struct ubiq_lref_constructor {
    std::size_t ignore;
    template <class Type> constexpr operator Type&() const && noexcept {  // tweak for template_unconstrained.cpp like cases
        return detail::unsafe_declval<Type&>();
    }

    template <class Type> constexpr operator Type&() const & noexcept {  // tweak for optional_chrono.cpp like cases
        return detail::unsafe_declval<Type&>();
    }
};

///////////////////// Structure that can be converted to rvalue reference to anything
struct ubiq_rref_constructor {
    std::size_t ignore;
    template <class Type> /*constexpr*/ operator Type() const && noexcept {  // Allows initialization of rvalue reference fields and move-only types
        return detail::unsafe_declval<Type>();
    }
};


#ifndef __cpp_lib_is_aggregate
///////////////////// Hand-made is_aggregate_initializable_n<T> trait

// Structure that can be converted to reference to anything except reference to T
template <class T, bool IsCopyConstructible>
struct ubiq_constructor_except {
    std::size_t ignore;
    template <class Type> constexpr operator std::enable_if_t<!std::is_same<T, Type>::value, Type&> () const noexcept; // Undefined
};

template <class T>
struct ubiq_constructor_except<T, false> {
    std::size_t ignore;
    template <class Type> constexpr operator std::enable_if_t<!std::is_same<T, Type>::value, Type&&> () const noexcept; // Undefined
};


// `std::is_constructible<T, ubiq_constructor_except<T>>` consumes a lot of time, so we made a separate lazy trait for it.
template <std::size_t N, class T> struct is_single_field_and_aggregate_initializable: std::false_type {};
template <class T> struct is_single_field_and_aggregate_initializable<1, T>: std::integral_constant<
    bool, !std::is_constructible<T, ubiq_constructor_except<T, std::is_copy_constructible<T>::value>>::value
> {};

// Hand-made is_aggregate<T> trait:
// Before C++20 aggregates could be constructed from `decltype(ubiq_?ref_constructor{I})...` but type traits report that
// there's no constructor from `decltype(ubiq_?ref_constructor{I})...`
// Special case for N == 1: `std::is_constructible<T, ubiq_?ref_constructor>` returns true if N == 1 and T is copy/move constructible.
template <class T, std::size_t N>
struct is_aggregate_initializable_n {
    template <std::size_t ...I>
    static constexpr bool is_not_constructible_n(std::index_sequence<I...>) noexcept {
        return (!std::is_constructible<T, decltype(ubiq_lref_constructor{I})...>::value && !std::is_constructible<T, decltype(ubiq_rref_constructor{I})...>::value)
            || is_single_field_and_aggregate_initializable<N, T>::value
        ;
    }

    static constexpr bool value =
           std::is_empty<T>::value
        || std::is_array<T>::value
        || std::is_fundamental<T>::value
        || is_not_constructible_n(detail::make_index_sequence<N>{})
    ;
};

#endif // #ifndef __cpp_lib_is_aggregate

///////////////////// Detect aggregates with inheritance
template <class Derived, class U>
constexpr bool static_assert_non_inherited() noexcept {
    static_assert(
            !std::is_base_of<U, Derived>::value,
            "====================> Boost.PFR: Boost.PFR: Inherited types are not supported."
    );
    return true;
}

template <class Derived>
struct ubiq_lref_base_asserting {
    template <class Type> constexpr operator Type&() const &&  // tweak for template_unconstrained.cpp like cases
        noexcept(detail::static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
    {
        return detail::unsafe_declval<Type&>();
    }

    template <class Type> constexpr operator Type&() const &  // tweak for optional_chrono.cpp like cases
        noexcept(detail::static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
    {
        return detail::unsafe_declval<Type&>();
    }
};

template <class Derived>
struct ubiq_rref_base_asserting {
    template <class Type> /*constexpr*/ operator Type() const &&  // Allows initialization of rvalue reference fields and move-only types
        noexcept(detail::static_assert_non_inherited<Derived, Type>())  // force the computation of assert function
    {
        return detail::unsafe_declval<Type>();
    }
};

template <class T, std::size_t I0, std::size_t... I, class /*Enable*/ = typename std::enable_if<std::is_copy_constructible<T>::value>::type>
constexpr auto assert_first_not_base(std::index_sequence<I0, I...>) noexcept
    -> typename std::add_pointer<decltype(T{ ubiq_lref_base_asserting<T>{}, ubiq_lref_constructor{I}... })>::type
{
    return nullptr;
}

template <class T, std::size_t I0, std::size_t... I, class /*Enable*/ = typename std::enable_if<!std::is_copy_constructible<T>::value>::type>
constexpr auto assert_first_not_base(std::index_sequence<I0, I...>) noexcept
    -> typename std::add_pointer<decltype(T{ ubiq_rref_base_asserting<T>{}, ubiq_rref_constructor{I}... })>::type
{
    return nullptr;
}

template <class T>
constexpr void* assert_first_not_base(std::index_sequence<>) noexcept
{
    return nullptr;
}

///////////////////// Helper for SFINAE on fields count
template <class T, std::size_t... I, class /*Enable*/ = typename std::enable_if<std::is_copy_constructible<T>::value>::type>
constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept
    -> typename std::add_pointer<decltype(T{ ubiq_lref_constructor{I}... })>::type;

template <class T, std::size_t... I, class /*Enable*/ = typename std::enable_if<!std::is_copy_constructible<T>::value>::type>
constexpr auto enable_if_constructible_helper(std::index_sequence<I...>) noexcept
    -> typename std::add_pointer<decltype(T{ ubiq_rref_constructor{I}... })>::type;

template <class T, std::size_t N, class /*Enable*/ = decltype( enable_if_constructible_helper<T>(detail::make_index_sequence<N>()) ) >
using enable_if_constructible_helper_t = std::size_t;

///////////////////// Helpers for range size detection
template <std::size_t Begin, std::size_t Last>
using is_one_element_range = std::integral_constant<bool, Begin == Last>;

using multi_element_range = std::false_type;
using one_element_range = std::true_type;

///////////////////// Non greedy fields count search. Templates instantiation depth is log(sizeof(T)), templates instantiation count is log(sizeof(T)).
template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(detail::one_element_range, long) noexcept {
    static_assert(
        Begin == Middle,
        "====================> Boost.PFR: Internal logic error."
    );
    return Begin;
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(detail::multi_element_range, int) noexcept;

template <class T, std::size_t Begin, std::size_t Middle>
constexpr auto detect_fields_count(detail::multi_element_range, long) noexcept
    -> detail::enable_if_constructible_helper_t<T, Middle>
{
    constexpr std::size_t next_v = Middle + (Middle - Begin + 1) / 2;
    return detail::detect_fields_count<T, Middle, next_v>(detail::is_one_element_range<Middle, next_v>{}, 1L);
}

template <class T, std::size_t Begin, std::size_t Middle>
constexpr std::size_t detect_fields_count(detail::multi_element_range, int) noexcept {
    constexpr std::size_t next_v = Begin + (Middle - Begin) / 2;
    return detail::detect_fields_count<T, Begin, next_v>(detail::is_one_element_range<Begin, next_v>{}, 1L);
}

///////////////////// Greedy search. Templates instantiation depth is log(sizeof(T)), templates instantiation count is log(sizeof(T))*T in worst case.
template <class T, std::size_t N>
constexpr auto detect_fields_count_greedy_remember(long) noexcept
    -> detail::enable_if_constructible_helper_t<T, N>
{
    return N;
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_greedy_remember(int) noexcept {
    return 0;
}

template <class T, std::size_t Begin, std::size_t Last>
constexpr std::size_t detect_fields_count_greedy(detail::one_element_range) noexcept {
    static_assert(
        Begin == Last,
        "====================> Boost.PFR: Internal logic error."
    );
    return detail::detect_fields_count_greedy_remember<T, Begin>(1L);
}

template <class T, std::size_t Begin, std::size_t Last>
constexpr std::size_t detect_fields_count_greedy(detail::multi_element_range) noexcept {
    constexpr std::size_t middle = Begin + (Last - Begin) / 2;
    constexpr std::size_t fields_count_big_range = detail::detect_fields_count_greedy<T, middle + 1, Last>(
        detail::is_one_element_range<middle + 1, Last>{}
    );

    constexpr std::size_t small_range_begin = (fields_count_big_range ? 0 : Begin);
    constexpr std::size_t small_range_last = (fields_count_big_range ? 0 : middle);
    constexpr std::size_t fields_count_small_range = detail::detect_fields_count_greedy<T, small_range_begin, small_range_last>(
        detail::is_one_element_range<small_range_begin, small_range_last>{}
    );
    return fields_count_big_range ? fields_count_big_range : fields_count_small_range;
}

///////////////////// Choosing between array size, greedy and non greedy search.
template <class T, std::size_t N>
constexpr auto detect_fields_count_dispatch(size_t_<N>, long, long) noexcept
    -> typename std::enable_if<std::is_array<T>::value, std::size_t>::type
{
    return sizeof(T) / sizeof(typename std::remove_all_extents<T>::type);
}

template <class T, std::size_t N>
constexpr auto detect_fields_count_dispatch(size_t_<N>, long, int) noexcept
    -> decltype(sizeof(T{}))
{
    constexpr std::size_t middle = N / 2 + 1;
    return detail::detect_fields_count<T, 0, middle>(detail::multi_element_range{}, 1L);
}

template <class T, std::size_t N>
constexpr std::size_t detect_fields_count_dispatch(size_t_<N>, int, int) noexcept {
    // T is not default aggregate initialzable. It means that at least one of the members is not default constructible,
    // so we have to check all the aggregate initializations for T up to N parameters and return the bigest succeeded
    // (we can not use binary search for detecting fields count).
    return detail::detect_fields_count_greedy<T, 0, N>(detail::multi_element_range{});
}

///////////////////// Returns fields count
template <class T>
constexpr std::size_t fields_count() noexcept {
    using type = std::remove_cv_t<T>;

    static_assert(
        !std::is_reference<type>::value,
        "====================> Boost.PFR: Attempt to get fields count on a reference. This is not allowed because that could hide an issue and different library users expect different behavior in that case."
    );

#if !BOOST_PFR_HAS_GUARANTEED_COPY_ELISION
    static_assert(
        std::is_copy_constructible<std::remove_all_extents_t<type>>::value || (
            std::is_move_constructible<std::remove_all_extents_t<type>>::value
            && std::is_move_assignable<std::remove_all_extents_t<type>>::value
        ),
        "====================> Boost.PFR: Type and each field in the type must be copy constructible (or move constructible and move assignable)."
    );
#endif  // #if !BOOST_PFR_HAS_GUARANTEED_COPY_ELISION

    static_assert(
        !std::is_polymorphic<type>::value,
        "====================> Boost.PFR: Type must have no virtual function, because otherwise it is not aggregate initializable."
    );

#ifdef __cpp_lib_is_aggregate
    static_assert(
        std::is_aggregate<type>::value             // Does not return `true` for built-in types.
        || std::is_scalar<type>::value,
        "====================> Boost.PFR: Type must be aggregate initializable."
    );
#endif

// Can't use the following. See the non_std_layout.cpp test.
//#if !BOOST_PFR_USE_CPP17
//    static_assert(
//        std::is_standard_layout<type>::value,   // Does not return `true` for structs that have non standard layout members.
//        "Type must be aggregate initializable."
//    );
//#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1920)
    // Workaround for msvc compilers. Versions <= 1920 have a limit of max 1024 elements in template parameter pack
    constexpr std::size_t max_fields_count = (sizeof(type) * CHAR_BIT >= 1024 ? 1024 : sizeof(type) * CHAR_BIT);
#else
    constexpr std::size_t max_fields_count = (sizeof(type) * CHAR_BIT); // We multiply by CHAR_BIT because the type may have bitfields in T
#endif

    constexpr std::size_t result = detail::detect_fields_count_dispatch<type>(size_t_<max_fields_count>{}, 1L, 1L);

    detail::assert_first_not_base<type>(detail::make_index_sequence<result>{});

#ifndef __cpp_lib_is_aggregate
    static_assert(
        is_aggregate_initializable_n<type, result>::value,
        "====================> Boost.PFR: Types with user specified constructors (non-aggregate initializable types) are not supported."
    );
#endif

    static_assert(
        result != 0 || std::is_empty<type>::value || std::is_fundamental<type>::value || std::is_reference<type>::value,
        "====================> Boost.PFR: If there's no other failed static asserts then something went wrong. Please report this issue to the github along with the structure you're reflecting."
    );

    return result;
}

}}} // namespace boost::pfr::detail

#ifdef __clang__
#   pragma clang diagnostic pop
#endif

#endif // BOOST_PFR_DETAIL_FIELDS_COUNT_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_FOR_EACH_FIELD_IMPL_HPP
#define BOOST_PFR_DETAIL_FOR_EACH_FIELD_IMPL_HPP


#include <utility>      // metaprogramming stuff

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_RVALUE_T_HPP
#define BOOST_PFR_DETAIL_RVALUE_T_HPP

#include <type_traits>
#include <utility>      // std::enable_if_t

// This header provides aliases rvalue_t and lvalue_t.
//
// Usage: template <class T> void foo(rvalue<T> rvalue);
//
// Those are useful for
//  * better type safety - you can validate at compile time that only rvalue reference is passed into the function
//  * documentation and readability - rvalue_t<T> is much better than T&&+SFINAE

namespace boost { namespace pfr { namespace detail {

/// Binds to rvalues only, no copying allowed.
template <class T
#ifdef BOOST_PFR_DETAIL_STRICT_RVALUE_TESTING
    , class = std::enable_if_t<std::is_rvalue_reference<T&&>::value>
#endif
>
using rvalue_t = T&&;

/// Binds to mutable lvalues only

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_RVALUE_T_HPP

namespace boost { namespace pfr { namespace detail {

template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index >;

template <class T, class F, class I, class = decltype(std::declval<F>()(std::declval<T>(), I{}))>
constexpr void for_each_field_impl_apply(T&& v, F&& f, I i, long) {
    std::forward<F>(f)(std::forward<T>(v), i);
}

template <class T, class F, class I>
constexpr void for_each_field_impl_apply(T&& v, F&& f, I /*i*/, int) {
    std::forward<F>(f)(std::forward<T>(v));
}

#if !defined(__cpp_fold_expressions) || __cpp_fold_expressions < 201603
template <class T, class F, std::size_t... I>
constexpr void for_each_field_impl(T& t, F&& f, std::index_sequence<I...>, std::false_type /*move_values*/) {
     const int v[] = {0, (
         detail::for_each_field_impl_apply(sequence_tuple::get<I>(t), std::forward<F>(f), size_t_<I>{}, 1L),
         0
     )...};
     (void)v;
}


template <class T, class F, std::size_t... I>
constexpr void for_each_field_impl(T& t, F&& f, std::index_sequence<I...>, std::true_type /*move_values*/) {
     const int v[] = {0, (
         detail::for_each_field_impl_apply(sequence_tuple::get<I>(std::move(t)), std::forward<F>(f), size_t_<I>{}, 1L),
         0
     )...};
     (void)v;
}
#else
template <class T, class F, std::size_t... I>
constexpr void for_each_field_impl(T& t, F&& f, std::index_sequence<I...>, std::false_type /*move_values*/) {
     (detail::for_each_field_impl_apply(sequence_tuple::get<I>(t), std::forward<F>(f), size_t_<I>{}, 1L), ...);
}

template <class T, class F, std::size_t... I>
constexpr void for_each_field_impl(T& t, F&& f, std::index_sequence<I...>, std::true_type /*move_values*/) {
     (detail::for_each_field_impl_apply(sequence_tuple::get<I>(std::move(t)), std::forward<F>(f), size_t_<I>{}, 1L), ...);
}
#endif

}}} // namespace boost::pfr::detail


#endif // BOOST_PFR_DETAIL_FOR_EACH_FIELD_IMPL_HPP

namespace boost { namespace pfr { namespace detail {

#ifndef _MSC_VER // MSVC fails to compile the following code, but compiles the structured bindings in core17_generated.hpp
struct do_not_define_std_tuple_size_for_me {
    bool test1 = true;
};

template <class T>
constexpr bool do_structured_bindings_work() noexcept { // ******************************************* IN CASE OF ERROR READ THE FOLLOWING LINES IN boost/pfr/detail/core17.hpp FILE:
    T val{};
    auto& [a] = val; // ******************************************* IN CASE OF ERROR READ THE FOLLOWING LINES IN boost/pfr/detail/core17.hpp FILE:

    /****************************************************************************
    *
    * It looks like your compiler or Standard Library can not handle C++17
    * structured bindings.
    *
    * Workaround: Define BOOST_PFR_USE_CPP17 to 0
    * It will disable the C++17 features for Boost.PFR library.
    *
    * Sorry for the inconvenience caused.
    *
    ****************************************************************************/

    return a;
}

static_assert(
    do_structured_bindings_work<do_not_define_std_tuple_size_for_me>(),
    "====================> Boost.PFR: Your compiler can not handle C++17 structured bindings. Read the above comments for workarounds."
);
#endif // #ifndef _MSC_VER

template <class T>
constexpr auto tie_as_tuple(T& val) noexcept {
  static_assert(
    !std::is_union<T>::value,
    "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
  );
  typedef size_t_<boost::pfr::detail::fields_count<T>()> fields_count_tag;
  return boost::pfr::detail::tie_as_tuple(val, fields_count_tag{});
}

template <class T, class F, std::size_t... I>
constexpr void for_each_field_dispatcher(T& t, F&& f, std::index_sequence<I...>) {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    std::forward<F>(f)(
        detail::tie_as_tuple(t)
    );
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE17_HPP
#elif BOOST_PFR_USE_LOOPHOLE
// Copyright (c) 2017-2018 Alexandr Poltavsky, Antony Polukhin.
// Copyright (c) 2019-2023 Antony Polukhin.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// The Great Type Loophole (C++14)
// Initial implementation by Alexandr Poltavsky, http://alexpolt.github.io
//
// Description:
//  The Great Type Loophole is a technique that allows to exchange type information with template
//  instantiations. Basically you can assign and read type information during compile time.
//  Here it is used to detect data members of a data type. I described it for the first time in
//  this blog post http://alexpolt.github.io/type-loophole.html .
//
// This technique exploits the http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2118
// CWG 2118. Stateful metaprogramming via friend injection
// Note: CWG agreed that such techniques should be ill-formed, although the mechanism for prohibiting them is as yet undetermined.

#ifndef BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP
#define BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP


#include <type_traits>
#include <utility>

#include <boost/pfr/detail/cast_to_layout_compatible.hpp> // still needed for enums
// Copyright (c) 2017-2018 Chris Beck
// Copyright (c) 2019-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_OFFSET_BASED_GETTER_HPP
#define BOOST_PFR_DETAIL_OFFSET_BASED_GETTER_HPP


#include <type_traits>
#include <utility>
#include <memory>  // std::addressof


namespace boost { namespace pfr { namespace detail {

// Our own implementation of std::aligned_storage. On godbolt with MSVC, I have compilation errors
// using the standard version, it seems the compiler cannot generate default ctor.

template<std::size_t s, std::size_t a>
struct internal_aligned_storage {
   alignas(a) char storage_[s];
};

// Metafunction that replaces tuple<T1, T2, T3, ...> with
// tuple<std::aligned_storage_t<sizeof(T1), alignof(T1)>, std::aligned_storage<sizeof(T2), alignof(T2)>, ...>
//
// The point is, the new tuple is "layout compatible" in the sense that members have the same offsets,
// but this tuple is constexpr constructible.

template <typename T>
struct tuple_of_aligned_storage;

template <typename... Ts>
struct tuple_of_aligned_storage<sequence_tuple::tuple<Ts...>> {
  using type = sequence_tuple::tuple<internal_aligned_storage<sizeof(Ts),
#if defined(__GNUC__) && __GNUC__ < 8 && !defined(__x86_64__) && !defined(__CYGWIN__)
      // Before GCC-8 the `alignof` was returning the optimal alignment rather than the minimal one.
      // We have to adjust the alignment because otherwise we get the wrong offset.
      (alignof(Ts) > 4 ? 4 : alignof(Ts))
#else
      alignof(Ts)
#endif
  >...>;
};

// Note: If pfr has a typelist also, could also have an overload for that here

template <typename T>
using tuple_of_aligned_storage_t = typename tuple_of_aligned_storage<T>::type;

/***
 * Given a structure type and its sequence of members, we want to build a function
 * object "getter" that implements a version of `std::get` using offset arithmetic
 * and reinterpret_cast.
 *
 * typename U should be a user-defined struct
 * typename S should be a sequence_tuple which is layout compatible with U
 */

template <typename U, typename S>
class offset_based_getter {
  using this_t = offset_based_getter<U, S>;

  static_assert(sizeof(U) == sizeof(S), "====================> Boost.PFR: Member sequence does not indicate correct size for struct type! Maybe the user-provided type is not a SimpleAggregate?");
  static_assert(alignof(U) == alignof(S), "====================> Boost.PFR: Member sequence does not indicate correct alignment for struct type!");

  static_assert(!std::is_const<U>::value, "====================> Boost.PFR: const should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later, this indicates an error within pfr");
  static_assert(!std::is_reference<U>::value, "====================> Boost.PFR: reference should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later, this indicates an error within pfr");
  static_assert(!std::is_volatile<U>::value, "====================> Boost.PFR: volatile should be stripped from user-defined type when using offset_based_getter or overload resolution will be ambiguous later. this indicates an error within pfr");

  // Get type of idx'th member
  template <std::size_t idx>
  using index_t = typename sequence_tuple::tuple_element<idx, S>::type;

  // Get offset of idx'th member
  // Idea: Layout object has the same offsets as instance of S, so if S and U are layout compatible, then these offset
  // calculations are correct.
  template <std::size_t idx>
  static constexpr std::ptrdiff_t offset() noexcept {
    constexpr tuple_of_aligned_storage_t<S> layout{};
    return &sequence_tuple::get<idx>(layout).storage_[0] - &sequence_tuple::get<0>(layout).storage_[0];
  }

  // Encapsulates offset arithmetic and reinterpret_cast
  template <std::size_t idx>
  static index_t<idx> * get_pointer(U * u) noexcept {
    return reinterpret_cast<index_t<idx> *>(reinterpret_cast<char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static const index_t<idx> * get_pointer(const U * u) noexcept {
    return reinterpret_cast<const index_t<idx> *>(reinterpret_cast<const char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static volatile index_t<idx> * get_pointer(volatile U * u) noexcept {
    return reinterpret_cast<volatile index_t<idx> *>(reinterpret_cast<volatile char *>(u) + this_t::offset<idx>());
  }

  template <std::size_t idx>
  static const volatile index_t<idx> * get_pointer(const volatile U * u) noexcept {
    return reinterpret_cast<const volatile index_t<idx> *>(reinterpret_cast<const volatile char *>(u) + this_t::offset<idx>());
  }

public:
  template <std::size_t idx>
  index_t<idx> & get(U & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> const & get(U const & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> volatile & get(U volatile & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  template <std::size_t idx>
  index_t<idx> const volatile & get(U const volatile & u, size_t_<idx>) const noexcept {
    return *this_t::get_pointer<idx>(std::addressof(u));
  }

  // rvalues must not be used here, to avoid template instantiation bloats.
  template <std::size_t idx>
  index_t<idx> && get(rvalue_t<U> u, size_t_<idx>) const = delete;
};


}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_OFFSET_LIST_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP
#define BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP


#include <utility>      // metaprogramming stuff


namespace boost { namespace pfr { namespace detail {

template <std::size_t Index>
using size_t_ = std::integral_constant<std::size_t, Index >;

// Helper: Make a "getter" object corresponding to built-in tuple::get
// For user-defined structures, the getter should be "offset_based_getter"
struct sequence_tuple_getter {
  template <std::size_t idx, typename TupleOfReferences>
  decltype(auto) get(TupleOfReferences&& t, size_t_<idx>) const noexcept {
    return sequence_tuple::get<idx>(std::forward<TupleOfReferences>(t));
  }
};


template <class TupleOrUserType, class Getter, std::size_t Begin, std::size_t Size>
constexpr auto make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<Size>) noexcept;

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr sequence_tuple::tuple<> make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<0>) noexcept;

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr auto make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<1>) noexcept;

template <class... T>
constexpr auto tie_as_tuple_with_references(T&... args) noexcept {
    return sequence_tuple::tuple<T&...>{ args... };
}

template <class... T>
constexpr decltype(auto) tie_as_tuple_with_references(detail::sequence_tuple::tuple<T...>& t) noexcept {
    return detail::make_flat_tuple_of_references(t, sequence_tuple_getter{}, size_t_<0>{}, size_t_<sequence_tuple::tuple<T...>::size_v>{});
}

template <class... T>
constexpr decltype(auto) tie_as_tuple_with_references(const detail::sequence_tuple::tuple<T...>& t) noexcept {
    return detail::make_flat_tuple_of_references(t, sequence_tuple_getter{}, size_t_<0>{}, size_t_<sequence_tuple::tuple<T...>::size_v>{});
}

template <class Tuple1, std::size_t... I1, class Tuple2, std::size_t... I2>
constexpr auto my_tuple_cat_impl(const Tuple1& t1, std::index_sequence<I1...>, const Tuple2& t2, std::index_sequence<I2...>) noexcept {
    return detail::tie_as_tuple_with_references(
        sequence_tuple::get<I1>(t1)...,
        sequence_tuple::get<I2>(t2)...
    );
}

template <class Tuple1, class Tuple2>
constexpr auto my_tuple_cat(const Tuple1& t1, const Tuple2& t2) noexcept {
    return detail::my_tuple_cat_impl(
        t1, detail::make_index_sequence< Tuple1::size_v >{},
        t2, detail::make_index_sequence< Tuple2::size_v >{}
    );
}

template <class TupleOrUserType, class Getter, std::size_t Begin, std::size_t Size>
constexpr auto make_flat_tuple_of_references(TupleOrUserType& t, const Getter& g, size_t_<Begin>, size_t_<Size>) noexcept {
    constexpr std::size_t next_size = Size / 2;
    return detail::my_tuple_cat(
        detail::make_flat_tuple_of_references(t, g, size_t_<Begin>{}, size_t_<next_size>{}),
        detail::make_flat_tuple_of_references(t, g, size_t_<Begin + Size / 2>{}, size_t_<Size - next_size>{})
    );
}

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr sequence_tuple::tuple<> make_flat_tuple_of_references(TupleOrUserType&, const Getter&, size_t_<Begin>, size_t_<0>) noexcept {
    return {};
}

template <class TupleOrUserType, class Getter, std::size_t Begin>
constexpr auto make_flat_tuple_of_references(TupleOrUserType& t, const Getter& g, size_t_<Begin>, size_t_<1>) noexcept {
    return detail::tie_as_tuple_with_references(
        g.get(t, size_t_<Begin>{})
    );
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_MAKE_FLAT_TUPLE_OF_REFERENCES_HPP


#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif


namespace boost { namespace pfr { namespace detail {

// tag<T,N> generates friend declarations and helps with overload resolution.
// There are two types: one with the auto return type, which is the way we read types later.
// The second one is used in the detection of instantiations without which we'd get multiple
// definitions.

template <class T, std::size_t N>
struct tag {
    friend auto loophole(tag<T,N>);
};

// The definitions of friend functions.
template <class T, class U, std::size_t N, bool B>
struct fn_def_lref {
    friend auto loophole(tag<T,N>) {
        // Standard Library containers do not SFINAE on invalid copy constructor. Because of that std::vector<std::unique_ptr<int>> reports that it is copyable,
        // which leads to an instantiation error at this place.
        //
        // To workaround the issue, we check that the type U is movable, and move it in that case.
        using no_extents_t = std::remove_all_extents_t<U>;
        return static_cast< std::conditional_t<std::is_move_constructible<no_extents_t>::value, no_extents_t&&, no_extents_t&> >(
            boost::pfr::detail::unsafe_declval<no_extents_t&>()
        );
    }
};
template <class T, class U, std::size_t N, bool B>
struct fn_def_rref {
    friend auto loophole(tag<T,N>) { return std::move(boost::pfr::detail::unsafe_declval< std::remove_all_extents_t<U>& >()); }
};


// Those specializations are to avoid multiple definition errors.
template <class T, class U, std::size_t N>
struct fn_def_lref<T, U, N, true> {};

template <class T, class U, std::size_t N>
struct fn_def_rref<T, U, N, true> {};


// This has a templated conversion operator which in turn triggers instantiations.
// Important point, using sizeof seems to be more reliable. Also default template
// arguments are "cached" (I think). To fix that I provide a U template parameter to
// the ins functions which do the detection using constexpr friend functions and SFINAE.
template <class T, std::size_t N>
struct loophole_ubiq_lref {
    template<class U, std::size_t M> static std::size_t ins(...);
    template<class U, std::size_t M, std::size_t = sizeof(loophole(tag<T,M>{})) > static char ins(int);

    template<class U, std::size_t = sizeof(fn_def_lref<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
    constexpr operator U&() const&& noexcept; // `const&&` here helps to avoid ambiguity in loophole instantiations. optional_like test validate that behavior.
};

template <class T, std::size_t N>
struct loophole_ubiq_rref {
    template<class U, std::size_t M> static std::size_t ins(...);
    template<class U, std::size_t M, std::size_t = sizeof(loophole(tag<T,M>{})) > static char ins(int);

    template<class U, std::size_t = sizeof(fn_def_rref<T, U, N, sizeof(ins<U, N>(0)) == sizeof(char)>)>
    constexpr operator U&&() const&& noexcept; // `const&&` here helps to avoid ambiguity in loophole instantiations. optional_like test validate that behavior.
};


// This is a helper to turn a data structure into a tuple.
template <class T, class U>
struct loophole_type_list_lref;

template <typename T, std::size_t... I>
struct loophole_type_list_lref< T, std::index_sequence<I...> >
     // Instantiating loopholes:
    : sequence_tuple::tuple< decltype(T{ loophole_ubiq_lref<T, I>{}... }, 0) >
{
    using type = sequence_tuple::tuple< decltype(loophole(tag<T, I>{}))... >;
};


template <class T, class U>
struct loophole_type_list_rref;

template <typename T, std::size_t... I>
struct loophole_type_list_rref< T, std::index_sequence<I...> >
     // Instantiating loopholes:
    : sequence_tuple::tuple< decltype(T{ loophole_ubiq_rref<T, I>{}... }, 0) >
{
    using type = sequence_tuple::tuple< decltype(loophole(tag<T, I>{}))... >;
};


// Lazily returns loophole_type_list_{lr}ref.
template <bool IsCopyConstructible /*= true*/, class T, class U>
struct loophole_type_list_selector {
    using type = loophole_type_list_lref<T, U>;
};

template <class T, class U>
struct loophole_type_list_selector<false /*IsCopyConstructible*/, T, U> {
    using type = loophole_type_list_rref<T, U>;
};

template <class T>
auto tie_as_tuple_loophole_impl(T& lvalue) noexcept {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    using indexes = detail::make_index_sequence<fields_count<type>()>;
    using loophole_type_list = typename detail::loophole_type_list_selector<
        std::is_copy_constructible<std::remove_all_extents_t<type>>::value, type, indexes
    >::type;
    using tuple_type = typename loophole_type_list::type;

    return boost::pfr::detail::make_flat_tuple_of_references(
        lvalue,
        offset_based_getter<type, tuple_type>{},
        size_t_<0>{},
        size_t_<tuple_type::size_v>{}
    );
}

template <class T>
auto tie_as_tuple(T& val) noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    return boost::pfr::detail::tie_as_tuple_loophole_impl(
        val
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher(T& t, F&& f, std::index_sequence<I...>) {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    std::forward<F>(f)(
        boost::pfr::detail::tie_as_tuple_loophole_impl(t)
    );
}

}}} // namespace boost::pfr::detail


#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif


#endif // BOOST_PFR_DETAIL_CORE14_LOOPHOLE_HPP

#else
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP
#define BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP


#include <type_traits>
#include <utility>      // metaprogramming stuff

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_SIZE_ARRAY_HPP
#define BOOST_PFR_DETAIL_SIZE_ARRAY_HPP


#include <cstddef>      // metaprogramming stuff

namespace boost { namespace pfr { namespace detail {

///////////////////// Array that has the constexpr
template <std::size_t N>
struct size_array {                         // libc++ misses constexpr on operator[]
    typedef std::size_t type;
    std::size_t data[N];

    static constexpr std::size_t size() noexcept { return N; }

    constexpr std::size_t count_nonzeros() const noexcept {
        std::size_t count = 0;
        for (std::size_t i = 0; i < size(); ++i) {
            if (data[i]) {
                ++ count;
            }
        }
        return count;
    }

    constexpr std::size_t count_from_opening_till_matching_parenthis_seq(std::size_t from, std::size_t opening_parenthis, std::size_t closing_parenthis) const noexcept {
        if (data[from] != opening_parenthis) {
            return 0;
        }
        std::size_t unclosed_parnthesis = 0;
        std::size_t count = 0;
        for (; ; ++from) {
            if (data[from] == opening_parenthis) {
                ++ unclosed_parnthesis;
            } else if (data[from] == closing_parenthis) {
                -- unclosed_parnthesis;
            }
            ++ count;

            if (unclosed_parnthesis == 0) {
                return count;
            }
        }

        return count;
    }
};

template <>
struct size_array<0> {                         // libc++ misses constexpr on operator[]
    typedef std::size_t type;
    std::size_t data[1];

    static constexpr std::size_t size() noexcept { return 0; }

    constexpr std::size_t count_nonzeros() const noexcept {
        return 0;
    }
};

template <std::size_t I, std::size_t N>
constexpr std::size_t get(const size_array<N>& a) noexcept {
    static_assert(I < N, "====================> Boost.PFR: Array index out of bounds");
    return a.data[I];
}



}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_SIZE_ARRAY_HPP

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces"
#   pragma clang diagnostic ignored "-Wundefined-inline"
#   pragma clang diagnostic ignored "-Wundefined-internal"
#   pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

namespace boost { namespace pfr { namespace detail {

///////////////////// General utility stuff

template <class T> struct identity {
    typedef T type;
};

template <class T>
constexpr T construct_helper() noexcept { // adding const here allows to deal with copyable only types
    return {};
}

template <class T> constexpr size_array<sizeof(T) * 3> fields_count_and_type_ids_with_zeros() noexcept;
template <class T> constexpr auto flat_array_of_type_ids() noexcept;

///////////////////// All the stuff for representing Type as integer and converting integer back to type
namespace typeid_conversions {

///////////////////// Helper constants and typedefs

#ifdef _MSC_VER
#   pragma warning( push )
    // '<<': check operator precedence for possible error; use parentheses to clarify precedence
#   pragma warning( disable : 4554 )
#endif

constexpr std::size_t native_types_mask = 31;
constexpr std::size_t bits_per_extension = 3;
constexpr std::size_t extension_mask = (
    static_cast<std::size_t>((1 << bits_per_extension) - 1)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);
constexpr std::size_t native_ptr_type = (
    static_cast<std::size_t>(1)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);
constexpr std::size_t native_const_ptr_type = (
    static_cast<std::size_t>(2)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_const_volatile_ptr_type = (
    static_cast<std::size_t>(3)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_volatile_ptr_type = (
    static_cast<std::size_t>(4)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

constexpr std::size_t native_ref_type = (
    static_cast<std::size_t>(5)
        << static_cast<std::size_t>(sizeof(std::size_t) * 8 - bits_per_extension)
);

template <std::size_t Index, std::size_t Extension>
using if_extension = std::enable_if_t< (Index & extension_mask) == Extension >*;

///////////////////// Helper functions
template <std::size_t Unptr>
constexpr std::size_t type_to_id_extension_apply(std::size_t ext) noexcept {
    constexpr std::size_t native_id = (Unptr & native_types_mask);
    constexpr std::size_t extensions = (Unptr & ~native_types_mask);
    static_assert(
        !((extensions >> bits_per_extension) & native_types_mask),
        "====================> Boost.PFR: Too many extensions for a single field (something close to `int************************** p;` is in the POD type)."
    );

    return (extensions >> bits_per_extension) | native_id | ext;
}

template <std::size_t Index>
using remove_1_ext = size_t_<
    ((Index & ~native_types_mask) << bits_per_extension) | (Index & native_types_mask)
>;

#ifdef _MSC_VER
#   pragma warning( pop )
#endif

///////////////////// Forward declarations

template <class Type> constexpr std::size_t type_to_id(identity<Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<const Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<const volatile Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<volatile Type*>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type&>) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_enum<Type>::value>* = nullptr) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_empty<Type>::value>* = nullptr) noexcept;
template <class Type> constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_union<Type>::value>* = nullptr) noexcept;
template <class Type> constexpr size_array<sizeof(Type) * 3> type_to_id(identity<Type>, std::enable_if_t<!std::is_enum<Type>::value && !std::is_empty<Type>::value && !std::is_union<Type>::value>* = 0) noexcept;

template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_ptr_type> = nullptr) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ptr_type> = nullptr) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_volatile_ptr_type> = nullptr) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_volatile_ptr_type> = nullptr) noexcept;
template <std::size_t Index> constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ref_type> = nullptr) noexcept;


///////////////////// Definitions of type_to_id and id_to_type for fundamental types
/// @cond
#define BOOST_MAGIC_GET_REGISTER_TYPE(Type, Index)              \
    constexpr std::size_t type_to_id(identity<Type>) noexcept { \
        return Index;                                           \
    }                                                           \
    constexpr Type id_to_type( size_t_<Index > ) noexcept {     \
        return detail::construct_helper<Type>();                \
    }                                                           \
    /**/
/// @endcond


// Register all base types here
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned char         , 1)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned short        , 2)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned int          , 3)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned long         , 4)
BOOST_MAGIC_GET_REGISTER_TYPE(unsigned long long    , 5)
BOOST_MAGIC_GET_REGISTER_TYPE(signed char           , 6)
BOOST_MAGIC_GET_REGISTER_TYPE(short                 , 7)
BOOST_MAGIC_GET_REGISTER_TYPE(int                   , 8)
BOOST_MAGIC_GET_REGISTER_TYPE(long                  , 9)
BOOST_MAGIC_GET_REGISTER_TYPE(long long             , 10)
BOOST_MAGIC_GET_REGISTER_TYPE(char                  , 11)
BOOST_MAGIC_GET_REGISTER_TYPE(wchar_t               , 12)
BOOST_MAGIC_GET_REGISTER_TYPE(char16_t              , 13)
BOOST_MAGIC_GET_REGISTER_TYPE(char32_t              , 14)
BOOST_MAGIC_GET_REGISTER_TYPE(float                 , 15)
BOOST_MAGIC_GET_REGISTER_TYPE(double                , 16)
BOOST_MAGIC_GET_REGISTER_TYPE(long double           , 17)
BOOST_MAGIC_GET_REGISTER_TYPE(bool                  , 18)
BOOST_MAGIC_GET_REGISTER_TYPE(void*                 , 19)
BOOST_MAGIC_GET_REGISTER_TYPE(const void*           , 20)
BOOST_MAGIC_GET_REGISTER_TYPE(volatile void*        , 21)
BOOST_MAGIC_GET_REGISTER_TYPE(const volatile void*  , 22)
BOOST_MAGIC_GET_REGISTER_TYPE(std::nullptr_t        , 23)
constexpr std::size_t tuple_begin_tag               = 24;
constexpr std::size_t tuple_end_tag                 = 25;

#undef BOOST_MAGIC_GET_REGISTER_TYPE

///////////////////// Definitions of type_to_id and id_to_type for types with extensions and nested types
template <class Type>
constexpr std::size_t type_to_id(identity<Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<const Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Const pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_const_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<const volatile Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Const volatile pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_const_volatile_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<volatile Type*>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: Volatile pointers to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_volatile_ptr_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type&>) noexcept {
    constexpr auto unptr = typeid_conversions::type_to_id(identity<Type>{});
    static_assert(
        std::is_same<const std::size_t, decltype(unptr)>::value,
        "====================> Boost.PFR: References to user defined types are not supported."
    );
    return typeid_conversions::type_to_id_extension_apply<unptr>(native_ref_type);
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_enum<Type>::value>*) noexcept {
    return typeid_conversions::type_to_id(identity<typename std::underlying_type<Type>::type >{});
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_empty<Type>::value>*) noexcept {
    static_assert(!std::is_empty<Type>::value, "====================> Boost.PFR: Empty classes/structures as members are not supported.");
    return 0;
}

template <class Type>
constexpr std::size_t type_to_id(identity<Type>, std::enable_if_t<std::is_union<Type>::value>*) noexcept {
    static_assert(
        !std::is_union<Type>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    return 0;
}

template <class Type>
constexpr size_array<sizeof(Type) * 3> type_to_id(identity<Type>, std::enable_if_t<!std::is_enum<Type>::value && !std::is_empty<Type>::value && !std::is_union<Type>::value>*) noexcept {
    constexpr auto t = detail::flat_array_of_type_ids<Type>();
    size_array<sizeof(Type) * 3> result {{tuple_begin_tag}};
    constexpr bool requires_tuplening = (
        (t.count_nonzeros() != 1)  || (t.count_nonzeros() == t.count_from_opening_till_matching_parenthis_seq(0, tuple_begin_tag, tuple_end_tag))
    );

    if (requires_tuplening) {
        for (std::size_t i = 0; i < t.size(); ++i)
            result.data[i + 1] = t.data[i];
        result.data[result.size() - 1] = tuple_end_tag;
    } else {
        for (std::size_t i = 0; i < t.size(); ++i)
            result.data[i] = t.data[i];
    }
    return result;
}



template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ptr_type>) noexcept {
    typedef decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}

template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_ptr_type>) noexcept {
    typedef const decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}

template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_const_volatile_ptr_type>) noexcept {
    typedef const volatile decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}


template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_volatile_ptr_type>) noexcept {
    typedef volatile decltype( typeid_conversions::id_to_type(remove_1_ext<Index>()) )* res_t;
    return detail::construct_helper<res_t>();
}


template <std::size_t Index>
constexpr auto id_to_type(size_t_<Index >, if_extension<Index, native_ref_type>) noexcept {
    static_assert(!Index, "====================> Boost.PFR: References are not supported");
    return nullptr;
}

} // namespace typeid_conversions

///////////////////// Structure that remembers types as integers on a `constexpr operator Type()` call
struct ubiq_val {
    std::size_t* ref_;

    template <class T>
    constexpr void assign(const T& typeids) const noexcept {
        for (std::size_t i = 0; i < T::size(); ++i)
            ref_[i] = typeids.data[i];
    }

    constexpr void assign(std::size_t val) const noexcept {
        ref_[0] = val;
    }

    template <class Type>
    constexpr operator Type() const noexcept {
        constexpr auto typeids = typeid_conversions::type_to_id(identity<Type>{});
        assign(typeids);
        return detail::construct_helper<Type>();
    }
};

///////////////////// Structure that remembers size of the type on a `constexpr operator Type()` call
struct ubiq_sizes {
    std::size_t& ref_;

    template <class Type>
    constexpr operator Type() const noexcept {
        ref_ = sizeof(Type);
        return detail::construct_helper<Type>();
    }
};

///////////////////// Returns array of (offsets without accounting alignments). Required for keeping places for nested type ids
template <class T, std::size_t N, std::size_t... I>
constexpr size_array<N> get_type_offsets() noexcept {
    typedef size_array<N> array_t;
    array_t sizes{};
    T tmp{ ubiq_sizes{sizes.data[I]}... };
    (void)tmp;

    array_t offsets{{0}};
    for (std::size_t i = 1; i < N; ++i)
        offsets.data[i] = offsets.data[i - 1] + sizes.data[i - 1];

    return offsets;
}

///////////////////// Returns array of typeids and zeros if constructor of a type accepts sizeof...(I) parameters
template <class T, std::size_t N, std::size_t... I>
constexpr void* flat_type_to_array_of_type_ids(std::size_t* types, std::index_sequence<I...>) noexcept
{
    static_assert(
        N <= sizeof(T),
        "====================> Boost.PFR: Bit fields are not supported."
    );

    constexpr auto offsets = detail::get_type_offsets<T, N, I...>();
    T tmp{ ubiq_val{types + get<I>(offsets) * 3}... };
    (void)types;
    (void)tmp;
    (void)offsets; // If type is empty offsets are not used
    return nullptr;
}

///////////////////// Returns array of typeids and zeros
template <class T>
constexpr size_array<sizeof(T) * 3> fields_count_and_type_ids_with_zeros() noexcept {
    size_array<sizeof(T) * 3> types{};
    constexpr std::size_t N = detail::fields_count<T>();
    detail::flat_type_to_array_of_type_ids<T, N>(types.data, detail::make_index_sequence<N>());
    return types;
}

///////////////////// Returns array of typeids without zeros
template <class T>
constexpr auto flat_array_of_type_ids() noexcept {
    constexpr auto types = detail::fields_count_and_type_ids_with_zeros<T>();
    constexpr std::size_t count = types.count_nonzeros();
    size_array<count> res{};
    std::size_t j = 0;
    for (std::size_t i = 0; i < decltype(types)::size(); ++i) {
        if (types.data[i]) {
            res.data[j] = types.data[i];
            ++ j;
        }
    }

    return res;
}

///////////////////// Convert array of typeids into sequence_tuple::tuple

template <class T, std::size_t First, std::size_t... I>
constexpr auto as_flat_tuple_impl(std::index_sequence<First, I...>) noexcept;

template <class T>
constexpr sequence_tuple::tuple<> as_flat_tuple_impl(std::index_sequence<>) noexcept {
    return sequence_tuple::tuple<>{};
}

template <std::size_t Increment, std::size_t... I>
constexpr auto increment_index_sequence(std::index_sequence<I...>) noexcept {
    return std::index_sequence<I + Increment...>{};
}

template <class T, std::size_t V, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<V>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(SubtupleLength == 0, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    return typeid_conversions::id_to_type(size_t_<V>{});
}

template <class T, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<typeid_conversions::tuple_end_tag>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(sizeof(T) == 0, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    return int{};
}

template <class T, std::size_t I, std::size_t SubtupleLength>
constexpr auto prepare_subtuples(size_t_<typeid_conversions::tuple_begin_tag>, size_t_<I>, size_t_<SubtupleLength>) noexcept {
    static_assert(SubtupleLength > 2, "====================> Boost.PFR: Internal error while representing nested field as tuple");
    constexpr auto seq = detail::make_index_sequence<SubtupleLength - 2>{};
    return detail::as_flat_tuple_impl<T>( detail::increment_index_sequence<I + 1>(seq) );
}


template <class Array>
constexpr Array remove_subtuples(Array indexes_plus_1, const Array& subtuple_lengths) noexcept {
    for (std::size_t i = 0; i < subtuple_lengths.size(); ++i) {
        if (subtuple_lengths.data[i]) {
            const std::size_t skips_count = subtuple_lengths.data[i];
            for (std::size_t j = i + 1; j < skips_count + i; ++j) {
                indexes_plus_1.data[j] = 0;
            }
            i += skips_count - 1;
        }
    }
    return indexes_plus_1;
}

template <std::size_t N, class Array>
constexpr size_array<N> resize_dropping_zeros_and_decrementing(size_t_<N>, const Array& a) noexcept {
    size_array<N> result{};
    std::size_t result_indx = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a.data[i]) {
            result.data[result_indx] = static_cast<std::size_t>(a.data[i] - 1);
            ++ result_indx;
        }
    }

    return result;
}

template <class T, std::size_t First, std::size_t... I, std::size_t... INew>
constexpr auto as_flat_tuple_impl_drop_helpers(std::index_sequence<First, I...>, std::index_sequence<INew...>) noexcept {
    constexpr auto a = detail::flat_array_of_type_ids<T>();

    constexpr size_array<sizeof...(I) + 1> subtuples_length {{
        a.count_from_opening_till_matching_parenthis_seq(First, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag),
        a.count_from_opening_till_matching_parenthis_seq(I, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag)...
    }};

    constexpr size_array<sizeof...(I) + 1> type_indexes_with_subtuple_internals {{ 1, 1 + I - First...}};
    constexpr auto type_indexes_plus_1_and_zeros_as_skips = detail::remove_subtuples(type_indexes_with_subtuple_internals, subtuples_length);
    constexpr auto new_size = size_t_<type_indexes_plus_1_and_zeros_as_skips.count_nonzeros()>{};
    constexpr auto type_indexes = detail::resize_dropping_zeros_and_decrementing(new_size, type_indexes_plus_1_and_zeros_as_skips);

    typedef sequence_tuple::tuple<
        decltype(detail::prepare_subtuples<T>(
            size_t_< a.data[ First + type_indexes.data[INew] ]          >{},    // id of type
            size_t_< First + type_indexes.data[INew]                    >{},    // index of current id in `a`
            size_t_< subtuples_length.data[ type_indexes.data[INew] ]   >{}     // if id of type is tuple, then length of that tuple
        ))...
    > subtuples_uncleanuped_t;

    return subtuples_uncleanuped_t{};
}

template <class Array>
constexpr std::size_t count_skips_in_array(std::size_t begin_index, std::size_t end_index, const Array& a) noexcept {
    std::size_t skips = 0;
    for (std::size_t i = begin_index; i < end_index; ++i) {
        if (a.data[i] == typeid_conversions::tuple_begin_tag) {
            const std::size_t this_tuple_size = a.count_from_opening_till_matching_parenthis_seq(i, typeid_conversions::tuple_begin_tag, typeid_conversions::tuple_end_tag) - 1;
            skips += this_tuple_size;
            i += this_tuple_size - 1;
        }
    }

    return skips;
}

template <class T, std::size_t First, std::size_t... I>
constexpr auto as_flat_tuple_impl(std::index_sequence<First, I...>) noexcept {
    constexpr auto a = detail::flat_array_of_type_ids<T>();
    constexpr std::size_t count_of_I = sizeof...(I);

    return detail::as_flat_tuple_impl_drop_helpers<T>(
        std::index_sequence<First, I...>{},
        detail::make_index_sequence< 1 + count_of_I - count_skips_in_array(First, First + count_of_I, a) >{}
    );
}

template <class T>
constexpr auto internal_tuple_with_same_alignment() noexcept {
    typedef typename std::remove_cv<T>::type type;

    static_assert(
        std::is_trivial<type>::value && std::is_standard_layout<type>::value,
        "====================> Boost.PFR: Type can not be reflected without Loophole or C++17, because it's not POD"
    );
    static_assert(!std::is_reference<type>::value, "====================> Boost.PFR: Not applyable");
    constexpr auto res = detail::as_flat_tuple_impl<type>(
        detail::make_index_sequence< decltype(detail::flat_array_of_type_ids<type>())::size() >()
    );

    return res;
}

template <class T>
using internal_tuple_with_same_alignment_t = decltype( detail::internal_tuple_with_same_alignment<T>() );


///////////////////// Flattening
struct ubiq_is_flat_refelectable {
    bool& is_flat_refelectable;

    template <class Type>
    constexpr operator Type() const noexcept {
        is_flat_refelectable = std::is_fundamental<std::remove_pointer_t<Type>>::value;
        return {};
    }
};

template <class T, std::size_t... I>
constexpr bool is_flat_refelectable(std::index_sequence<I...>) noexcept {
    constexpr std::size_t fields = sizeof...(I);
    bool result[fields] = {static_cast<bool>(I)...};
    const T v{ ubiq_is_flat_refelectable{result[I]}... };
    (void)v;

    for (std::size_t i = 0; i < fields; ++i) {
        if (!result[i]) {
            return false;
        }
    }

    return true;
}

template<class T>
constexpr bool is_flat_refelectable(std::index_sequence<>) noexcept {
    return true; ///< all empty structs always flat refelectable
}

template <class T>
auto tie_as_flat_tuple(T& lvalue) noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    using type = std::remove_cv_t<T>;
    using tuple_type = internal_tuple_with_same_alignment_t<type>;

    offset_based_getter<type, tuple_type> getter;
    return boost::pfr::detail::make_flat_tuple_of_references(lvalue, getter, size_t_<0>{}, size_t_<tuple_type::size_v>{});
}

template <class T>
auto tie_as_tuple(T& val) noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    static_assert(
        boost::pfr::detail::is_flat_refelectable<T>( detail::make_index_sequence<boost::pfr::detail::fields_count<T>()>{} ),
        "====================> Boost.PFR: Not possible in C++14 to represent that type without loosing information. Change type definition or enable C++17"
    );
    return boost::pfr::detail::tie_as_flat_tuple(val);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////// Structure that can be converted to copy of anything
struct ubiq_constructor_constexpr_copy {
    std::size_t ignore;

    template <class Type>
    constexpr operator Type() const noexcept {
        static_assert(
            std::is_trivially_destructible<Type>::value,
            "====================> Boost.PFR: One of the fields in the type passed to `for_each_field` has non trivial destructor."
        );
        return {};
    }
};

/////////////////////

template <class T, std::size_t... I>
struct is_constexpr_aggregate_initializable {
    template<class T2, std::size_t... I2>
    static constexpr void* constexpr_aggregate_initializer() noexcept {
        T2 tmp{ ubiq_constructor_constexpr_copy{I2}... };
        (void)tmp;
        return nullptr;
    }

    template <void* = constexpr_aggregate_initializer<T, I...>() >
    static std::true_type test(long) noexcept;

    static std::false_type test(...) noexcept;

    static constexpr bool value = decltype(test(0)){};
};


template <class T, class F, std::size_t I0, std::size_t... I, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<I0, I...>, identity<Fields>...);

template <class T, class F, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<>, identity<Fields>...);

template <class T, class F, class IndexSeq, class... Fields>
struct next_step {
    T& t;
    F& f;

    template <class Field>
    operator Field() const {
         boost::pfr::detail::for_each_field_in_depth(
             t,
             std::forward<F>(f),
             IndexSeq{},
             identity<Fields>{}...,
             identity<Field>{}
         );

         return {};
    }
};

template <class T, class F, std::size_t I0, std::size_t... I, class... Fields>
void for_each_field_in_depth(T& t, F&& f, std::index_sequence<I0, I...>, identity<Fields>...) {
    (void)std::add_const_t<std::remove_reference_t<T>>{
        Fields{}...,
        next_step<T, F, std::index_sequence<I...>, Fields...>{t, f},
        ubiq_constructor_constexpr_copy{I}...
    };
}

template <class T, class F, class... Fields>
void for_each_field_in_depth(T& lvalue, F&& f, std::index_sequence<>, identity<Fields>...) {
    using tuple_type = sequence_tuple::tuple<Fields...>;

    offset_based_getter<std::remove_cv_t<std::remove_reference_t<T>>, tuple_type> getter;
    std::forward<F>(f)(
        boost::pfr::detail::make_flat_tuple_of_references(lvalue, getter, size_t_<0>{}, size_t_<sizeof...(Fields)>{})
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher_1(T& t, F&& f, std::index_sequence<I...>, std::true_type /*is_flat_refelectable*/) {
    std::forward<F>(f)(
        boost::pfr::detail::tie_as_flat_tuple(t)
    );
}


template <class T, class F, std::size_t... I>
void for_each_field_dispatcher_1(T& t, F&& f, std::index_sequence<I...>, std::false_type /*is_flat_refelectable*/) {
    boost::pfr::detail::for_each_field_in_depth(
        t,
        std::forward<F>(f),
        std::index_sequence<I...>{}
    );
}

template <class T, class F, std::size_t... I>
void for_each_field_dispatcher(T& t, F&& f, std::index_sequence<I...>) {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    static_assert(is_constexpr_aggregate_initializable<T, I...>::value, "====================> Boost.PFR: T must be a constexpr initializable type");

    constexpr bool is_flat_refelectable_val = detail::is_flat_refelectable<T>( std::index_sequence<I...>{} );
    detail::for_each_field_dispatcher_1(
        t,
        std::forward<F>(f),
        std::index_sequence<I...>{},
        std::integral_constant<bool, is_flat_refelectable_val>{}
    );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __clang__
#   pragma clang diagnostic pop
#endif

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE14_CLASSIC_HPP
#endif

#endif // BOOST_PFR_DETAIL_CORE_HPP

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_STDTUPLE_HPP
#define BOOST_PFR_DETAIL_STDTUPLE_HPP


#include <utility>      // metaprogramming stuff
#include <tuple>


namespace boost { namespace pfr { namespace detail {

template <class T, std::size_t... I>
constexpr auto make_stdtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::make_tuple(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

template <class T, std::size_t... I>
constexpr auto make_stdtiedtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::tie(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

template <class T, std::size_t... I>
constexpr auto make_conststdtiedtuple_from_tietuple(const T& t, std::index_sequence<I...>) noexcept {
    return std::tuple<
        std::add_lvalue_reference_t<std::add_const_t<
            std::remove_reference_t<decltype(boost::pfr::detail::sequence_tuple::get<I>(t))>
        >>...
    >(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_STDTUPLE_HPP
// Copyright (c) 2018 Adam Butcher, Antony Polukhin
// Copyright (c) 2019-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_TIE_FROM_STRUCTURE_TUPLE_HPP
#define BOOST_PFR_DETAIL_TIE_FROM_STRUCTURE_TUPLE_HPP



// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PFR_TUPLE_SIZE_HPP
#define BOOST_PFR_TUPLE_SIZE_HPP


#include <type_traits>
#include <utility>      // metaprogramming stuff


/// \file boost/pfr/tuple_size.hpp
/// Contains tuple-like interfaces to get fields count \forcedlink{tuple_size}, \forcedlink{tuple_size_v}.
///
/// \b Synopsis:
namespace boost { namespace pfr {

/// Has a static const member variable `value` that contains fields count in a T.
/// Works for any T that satisfies \aggregate.
///
/// \b Example:
/// \code
///     std::array<int, boost::pfr::tuple_size<my_structure>::value > a;
/// \endcode
template <class T>
using tuple_size = detail::size_t_< boost::pfr::detail::fields_count<T>() >;


/// `tuple_size_v` is a template variable that contains fields count in a T and
/// works for any T that satisfies \aggregate.
///
/// \b Example:
/// \code
///     std::array<int, boost::pfr::tuple_size_v<my_structure> > a;
/// \endcode
template <class T>
constexpr std::size_t tuple_size_v = tuple_size<T>::value;

}} // namespace boost::pfr

#endif // BOOST_PFR_TUPLE_SIZE_HPP

#include <tuple>

namespace boost { namespace pfr { namespace detail {

/// \brief A `std::tuple` capable of de-structuring assignment used to support
/// a tie of multiple lvalue references to fields of an aggregate T.
///
/// \sa boost::pfr::tie_from_structure
template <typename... Elements>
struct tie_from_structure_tuple : std::tuple<Elements&...> {
    using base = std::tuple<Elements&...>;
    using base::base;

    template <typename T>
    constexpr tie_from_structure_tuple& operator= (T const& t) {
        base::operator=(
            detail::make_stdtiedtuple_from_tietuple(
                detail::tie_as_tuple(t),
                detail::make_index_sequence<tuple_size_v<T>>()));
        return *this;
    }
};

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_TIE_FROM_STRUCTURE_TUPLE_HPP

#include <type_traits>
#include <utility>      // metaprogramming stuff


/// \file boost/pfr/core.hpp
/// Contains all the basic tuple-like interfaces \forcedlink{get}, \forcedlink{tuple_size}, \forcedlink{tuple_element_t}, and others.
///
/// \b Synopsis:

namespace boost { namespace pfr {

/// \brief Returns reference or const reference to a field with index `I` in \aggregate `val`.
/// Overload taking the type `U` returns reference or const reference to a field
/// with provided type `U` in \aggregate `val` if there's only one field of such type in `val`.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///
///     assert(boost::pfr::get<0>(s) == 10);
///     boost::pfr::get<1>(s) = 0;
///
///     assert(boost::pfr::get<int>(s) == 10);
///     boost::pfr::get<short>(s) = 11;
/// \endcode
template <std::size_t I, class T>
constexpr decltype(auto) get(const T& val) noexcept {
    return detail::sequence_tuple::get<I>( detail::tie_as_tuple(val) );
}

/// \overload get
template <std::size_t I, class T>
constexpr decltype(auto) get(T& val
#if !BOOST_PFR_USE_CPP17
    , std::enable_if_t<std::is_assignable<T, T>::value>* = nullptr
#endif
) noexcept {
    return detail::sequence_tuple::get<I>( detail::tie_as_tuple(val) );
}

#if !BOOST_PFR_USE_CPP17
/// \overload get
template <std::size_t I, class T>
constexpr auto get(T&, std::enable_if_t<!std::is_assignable<T, T>::value>* = nullptr) noexcept {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Calling boost::pfr::get on non const non assignable type is allowed only in C++17");
    return 0;
}
#endif


/// \overload get
template <std::size_t I, class T>
constexpr auto get(T&& val, std::enable_if_t< std::is_rvalue_reference<T&&>::value>* = nullptr) noexcept {
    return std::move(detail::sequence_tuple::get<I>( detail::tie_as_tuple(val) ));
}


/// \overload get
template <class U, class T>
constexpr const U& get(const T& val) noexcept {
    return detail::sequence_tuple::get_by_type_impl<const U&>( detail::tie_as_tuple(val) );
}


/// \overload get
template <class U, class T>
constexpr U& get(T& val
#if !BOOST_PFR_USE_CPP17
    , std::enable_if_t<std::is_assignable<T, T>::value>* = nullptr
#endif
) noexcept {
    return detail::sequence_tuple::get_by_type_impl<U&>( detail::tie_as_tuple(val) );
}

#if !BOOST_PFR_USE_CPP17
/// \overload get
template <class U, class T>
constexpr U& get(T&, std::enable_if_t<!std::is_assignable<T, T>::value>* = nullptr) noexcept {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Calling boost::pfr::get on non const non assignable type is allowed only in C++17");
    return 0;
}
#endif


/// \overload get
template <class U, class T>
constexpr U&& get(T&& val, std::enable_if_t< std::is_rvalue_reference<T&&>::value>* = nullptr) noexcept {
    return std::move(detail::sequence_tuple::get_by_type_impl<U&>( detail::tie_as_tuple(val) ));
}


/// \brief `tuple_element` has a member typedef `type` that returns the type of a field with index I in \aggregate T.
///
/// \b Example:
/// \code
///     std::vector< boost::pfr::tuple_element<0, my_structure>::type > v;
/// \endcode
template <std::size_t I, class T>
using tuple_element = detail::sequence_tuple::tuple_element<I, decltype( ::boost::pfr::detail::tie_as_tuple(std::declval<T&>()) ) >;


/// \brief Type of a field with index `I` in \aggregate `T`.
///
/// \b Example:
/// \code
///     std::vector< boost::pfr::tuple_element_t<0, my_structure> > v;
/// \endcode
template <std::size_t I, class T>
using tuple_element_t = typename tuple_element<I, T>::type;


/// \brief Creates a `std::tuple` from fields of an \aggregate `val`.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///     std::tuple<int, short> t = boost::pfr::structure_to_tuple(s);
///     assert(get<0>(t) == 10);
/// \endcode
template <class T>
constexpr auto structure_to_tuple(const T& val) noexcept {
    return detail::make_stdtuple_from_tietuple(
        detail::tie_as_tuple(val),
        detail::make_index_sequence< tuple_size_v<T> >()
    );
}


/// \brief std::tie` like function that ties fields of a structure.
///
/// \returns a `std::tuple` with lvalue and const lvalue references to fields of an \aggregate `val`.
///
/// \b Example:
/// \code
///     void foo(const int&, const short&);
///     struct my_struct { int i, short s; };
///
///     const my_struct const_s{1, 2};
///     std::apply(foo, boost::pfr::structure_tie(const_s));
///
///     my_struct s;
///     boost::pfr::structure_tie(s) = std::tuple<int, short>{10, 11};
///     assert(s.s == 11);
/// \endcode
template <class T>
constexpr auto structure_tie(const T& val) noexcept {
    return detail::make_conststdtiedtuple_from_tietuple(
        detail::tie_as_tuple(const_cast<T&>(val)),
        detail::make_index_sequence< tuple_size_v<T> >()
    );
}


/// \overload structure_tie
template <class T>
constexpr auto structure_tie(T& val
#if !BOOST_PFR_USE_CPP17
    , std::enable_if_t<std::is_assignable<T, T>::value>* = nullptr
#endif
) noexcept {
    return detail::make_stdtiedtuple_from_tietuple(
        detail::tie_as_tuple(val),
        detail::make_index_sequence< tuple_size_v<T> >()
    );
}

#if !BOOST_PFR_USE_CPP17
/// \overload structure_tie
template <class T>
constexpr auto structure_tie(T&, std::enable_if_t<!std::is_assignable<T, T>::value>* = nullptr) noexcept {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Calling boost::pfr::structure_tie on non const non assignable type is allowed only in C++17");
    return 0;
}
#endif


/// \overload structure_tie
template <class T>
constexpr auto structure_tie(T&&, std::enable_if_t< std::is_rvalue_reference<T&&>::value>* = nullptr) noexcept {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Calling boost::pfr::structure_tie on rvalue references is forbidden");
    return 0;
}

/// Calls `func` for each field of a `value`.
///
/// \param func must have one of the following signatures:
///     * any_return_type func(U&& field)                // field of value is perfect forwarded to function
///     * any_return_type func(U&& field, std::size_t i)
///     * any_return_type func(U&& value, I i)           // Here I is an `std::integral_constant<size_t, field_index>`
///
/// \param value To each field of this variable will be the `func` applied.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     int sum = 0;
///     boost::pfr::for_each_field(my_struct{20, 22}, [&sum](const auto& field) { sum += field; });
///     assert(sum == 42);
/// \endcode
template <class T, class F>
constexpr void for_each_field(T&& value, F&& func) {
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [f = std::forward<F>(func)](auto&& t) mutable {
            // MSVC related workaround. Its lambdas do not capture constexprs.
            constexpr std::size_t fields_count_val_in_lambda
                = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

            ::boost::pfr::detail::for_each_field_impl(
                t,
                std::forward<F>(f),
                detail::make_index_sequence<fields_count_val_in_lambda>{},
                std::is_rvalue_reference<T&&>{}
            );
        },
        detail::make_index_sequence<fields_count_val>{}
    );
}

/// \brief std::tie-like function that allows assigning to tied values from aggregates.
///
/// \returns an object with lvalue references to `args...`; on assignment of an \aggregate value to that
/// object each field of an aggregate is assigned to the corresponding `args...` reference.
///
/// \b Example:
/// \code
///     auto f() {
///       struct { struct { int x, y } p; short s; } res { { 4, 5 }, 6 };
///       return res;
///     }
///     auto [p, s] = f();
///     boost::pfr::tie_from_structure(p, s) = f();
/// \endcode
template <typename... Elements>
constexpr detail::tie_from_structure_tuple<Elements...> tie_from_structure(Elements&... args) noexcept {
    return detail::tie_from_structure_tuple<Elements...>(args...);
}

}} // namespace boost::pfr

#endif // BOOST_PFR_CORE_HPP
// Copyright (c) 2023 Bela Schaum, X-Ryl669, Denis Mikhailov.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// Initial implementation by Bela Schaum, https://github.com/schaumb
// The way to make it union and UB free by X-Ryl669, https://github.com/X-Ryl669
//

#ifndef BOOST_PFR_CORE_NAME_HPP
#define BOOST_PFR_CORE_NAME_HPP


// Copyright (c) 2023 Bela Schaum, X-Ryl669, Denis Mikhailov.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// Initial implementation by Bela Schaum, https://github.com/schaumb
// The way to make it union and UB free by X-Ryl669, https://github.com/X-Ryl669
//

#ifndef BOOST_PFR_DETAIL_CORE_NAME_HPP
#define BOOST_PFR_DETAIL_CORE_NAME_HPP


// Each core_name provides `boost::pfr::detail::get_name` and
// `boost::pfr::detail::tie_as_names_tuple` functions.
//
// The whole functional of extracting field's names is build on top of those
// two functions.
#if BOOST_PFR_CORE_NAME_ENABLED
// Copyright (c) 2023 Bela Schaum, X-Ryl669, Denis Mikhailov.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// Initial implementation by Bela Schaum, https://github.com/schaumb
// The way to make it union and UB free by X-Ryl669, https://github.com/X-Ryl669
//

#ifndef BOOST_PFR_DETAIL_CORE_NAME20_STATIC_HPP
#define BOOST_PFR_DETAIL_CORE_NAME20_STATIC_HPP

// Copyright (c) 2023 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_STDARRAY_HPP
#define BOOST_PFR_DETAIL_STDARRAY_HPP


#include <utility> // metaprogramming stuff
#include <array>
#include <type_traits> // for std::common_type_t
#include <cstddef>


namespace boost { namespace pfr { namespace detail {

template <class... Types>
constexpr auto make_stdarray(const Types&... t) noexcept {
    return std::array<std::common_type_t<Types...>, sizeof...(Types)>{t...};
}

template <class T, std::size_t... I>
constexpr auto make_stdarray_from_tietuple(const T& t, std::index_sequence<I...>, int) noexcept {
    return detail::make_stdarray(
        boost::pfr::detail::sequence_tuple::get<I>(t)...
    );
}

template <class T>
constexpr auto make_stdarray_from_tietuple(const T&, std::index_sequence<>, long) noexcept {
    return std::array<std::nullptr_t, 0>{};
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_STDARRAY_HPP

// Copyright (c) 2023 Bela Schaum, X-Ryl669, Denis Mikhailov.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// Initial implementation by Bela Schaum, https://github.com/schaumb
// The way to make it union and UB free by X-Ryl669, https://github.com/X-Ryl669
//

#ifndef BOOST_PFR_DETAIL_FAKE_OBJECT_HPP
#define BOOST_PFR_DETAIL_FAKE_OBJECT_HPP


namespace boost { namespace pfr { namespace detail {

template <class T>
extern const T fake_object;

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_FAKE_OBJECT_HPP

#include <type_traits>
#include <string_view>
#include <array>
#include <memory> // for std::addressof

namespace boost { namespace pfr { namespace detail {

struct core_name_skip {
    std::size_t size_at_begin;
    std::size_t size_at_end;
    bool is_backward;
    std::string_view until_runtime;

    consteval std::string_view apply(std::string_view sv) const noexcept {
        // We use std::min here to make the compiler diagnostic shorter and
        // cleaner in case of misconfigured BOOST_PFR_CORE_NAME_PARSING
        sv.remove_prefix((std::min)(size_at_begin, sv.size()));
        sv.remove_suffix((std::min)(size_at_end, sv.size()));
        if (until_runtime.empty()) {
            return sv;
        }

        const auto found = is_backward ? sv.rfind(until_runtime)
                                       : sv.find(until_runtime);

        const auto cut_until = found + until_runtime.size();
        const auto safe_cut_until = (std::min)(cut_until, sv.size());
        return sv.substr(safe_cut_until);
    }
};

struct backward {
    explicit consteval backward(std::string_view value) noexcept
        : value(value)
    {}

    std::string_view value;
};

consteval core_name_skip make_core_name_skip(std::size_t size_at_begin,
                                             std::size_t size_at_end,
                                             std::string_view until_runtime) noexcept
{
    return core_name_skip{size_at_begin, size_at_end, false, until_runtime};
}

consteval core_name_skip make_core_name_skip(std::size_t size_at_begin,
                                             std::size_t size_at_end,
                                             backward until_runtime) noexcept
{
    return core_name_skip{size_at_begin, size_at_end, true, until_runtime.value};
}

// it might be compilation failed without this workaround sometimes
// See https://github.com/llvm/llvm-project/issues/41751 for details
template <class>
consteval std::string_view clang_workaround(std::string_view value) noexcept
{
    return value;
}

template <class MsvcWorkaround, auto ptr>
consteval auto name_of_field_impl() noexcept {
    // Some of the following compiler specific macro may be defined only
    // inside the function body:

#ifndef BOOST_PFR_FUNCTION_SIGNATURE
#   if defined(__FUNCSIG__)
#       define BOOST_PFR_FUNCTION_SIGNATURE __FUNCSIG__
#   elif defined(__PRETTY_FUNCTION__) || defined(__GNUC__) || defined(__clang__)
#       define BOOST_PFR_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#   else
#       define BOOST_PFR_FUNCTION_SIGNATURE ""
#   endif
#endif

    constexpr std::string_view sv = detail::clang_workaround<MsvcWorkaround>(BOOST_PFR_FUNCTION_SIGNATURE);
    static_assert(!sv.empty(),
        "====================> Boost.PFR: Field reflection parser configured in a wrong way. "
        "Please define the BOOST_PFR_FUNCTION_SIGNATURE to a compiler specific macro, "
        "that outputs the whole function signature including non-type template parameters."  
    );

    constexpr auto skip = detail::make_core_name_skip BOOST_PFR_CORE_NAME_PARSING;
    static_assert(skip.size_at_begin + skip.size_at_end + skip.until_runtime.size() < sv.size(),
        "====================> Boost.PFR: Field reflection parser configured in a wrong way. "
        "It attempts to skip more chars than available. "
        "Please define BOOST_PFR_CORE_NAME_PARSING to correct values. See documentation section "
        "'Limitations and Configuration' for more information."
    );
    constexpr auto fn = skip.apply(sv);
    static_assert(
        !fn.empty(),
        "====================> Boost.PFR: Extraction of field name is misconfigured for your compiler. "
        "It skipped all the input, leaving the field name empty. "
        "Please define BOOST_PFR_CORE_NAME_PARSING to correct values. See documentation section "
        "'Limitations and Configuration' for more information."
    );
    auto res = std::array<char, fn.size()+1>{};

    auto* out = res.data();
    for (auto x: fn) {
        *out = x;
        ++out;
    }

    return res;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"

// clang 16 and earlier don't support address of non-static member as template parameter
// but fortunately it's possible to use C++20 non-type template parameters in another way
// even in clang 16 and more older clangs
// all we need is to wrap pointer into 'clang_wrapper_t' and then pass it into template
template <class T>
struct clang_wrapper_t {
    T v;
};
template <class T>
clang_wrapper_t(T) -> clang_wrapper_t<T>;

template <class T>
constexpr auto make_clang_wrapper(const T& arg) noexcept {
    return clang_wrapper_t{arg};
}

#else

template <class T>
constexpr const T& make_clang_wrapper(const T& arg) noexcept {
    // It's everything OK with address of non-static member as template parameter support on this compiler
    // so we don't need a wrapper here, just pass the pointer into template
    return arg;
}

#endif

template <class MsvcWorkaround, auto ptr>
consteval auto name_of_field() noexcept {
    // Sanity check: known field name must match the deduced one
    static_assert(
        sizeof(MsvcWorkaround)  // do not trigger if `name_of_field()` is not used
        && std::string_view{
            detail::name_of_field_impl<
                core_name_skip, detail::make_clang_wrapper(std::addressof(
                    fake_object<core_name_skip>.size_at_begin
                ))
            >().data()
        } == "size_at_begin",
        "====================> Boost.PFR: Extraction of field name is misconfigured for your compiler. "
        "It does not return the proper field name. "
        "Please define BOOST_PFR_CORE_NAME_PARSING to correct values. See documentation section "
        "'Limitations and Configuration' for more information."
    );

    return detail::name_of_field_impl<MsvcWorkaround, ptr>();
}

// Storing part of a string literal into an array minimizes the binary size.
//
// Without passing 'T' into 'name_of_field' different fields from different structures might have the same name!
// See https://developercommunity.visualstudio.com/t/__FUNCSIG__-outputs-wrong-value-with-C/10458554 for details
template <class T, std::size_t I>
inline constexpr auto stored_name_of_field = detail::name_of_field<T,
    detail::make_clang_wrapper(std::addressof(detail::sequence_tuple::get<I>(
        detail::tie_as_tuple(detail::fake_object<T>)
    )))
>();

#ifdef __clang__
#pragma clang diagnostic pop
#endif

template <class T, std::size_t... I>
constexpr auto tie_as_names_tuple_impl(std::index_sequence<I...>) noexcept {
    return detail::sequence_tuple::make_sequence_tuple(std::string_view{stored_name_of_field<T, I>.data()}...);
}

template <class T, std::size_t I>
constexpr std::string_view get_name() noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    static_assert(
        !std::is_array<T>::value,
        "====================> Boost.PFR: It is impossible to extract name from old C array since it doesn't have named members"
    );
    static_assert(
        sizeof(T) && BOOST_PFR_USE_CPP17,
        "====================> Boost.PFR: Extraction of field's names is allowed only when the BOOST_PFR_USE_CPP17 macro enabled."
   );

   return stored_name_of_field<T, I>.data();
}

template <class T>
constexpr auto tie_as_names_tuple() noexcept {
    static_assert(
        !std::is_union<T>::value,
        "====================> Boost.PFR: For safety reasons it is forbidden to reflect unions. See `Reflection of unions` section in the docs for more info."
    );
    static_assert(
        !std::is_array<T>::value,
        "====================> Boost.PFR: It is impossible to extract name from old C array since it doesn't have named members"
    );
    static_assert(
        sizeof(T) && BOOST_PFR_USE_CPP17,
        "====================> Boost.PFR: Extraction of field's names is allowed only when the BOOST_PFR_USE_CPP17 macro enabled."
    );

    return detail::tie_as_names_tuple_impl<T>(detail::make_index_sequence<detail::fields_count<T>()>{});
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE_NAME20_STATIC_HPP

#else
// Copyright (c) 2023 Bela Schaum, X-Ryl669, Denis Mikhailov.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


// Initial implementation by Bela Schaum, https://github.com/schaumb
// The way to make it union and UB free by X-Ryl669, https://github.com/X-Ryl669
//

#ifndef BOOST_PFR_DETAIL_CORE_NAME14_DISABLED_HPP
#define BOOST_PFR_DETAIL_CORE_NAME14_DISABLED_HPP


namespace boost { namespace pfr { namespace detail {

template <class T, std::size_t I>
constexpr auto get_name() noexcept {
    static_assert(
        sizeof(T) && false,
        "====================> Boost.PFR: Field's names extracting functionality requires C++20."
    );

    return nullptr;
}

template <class T>
constexpr auto tie_as_names_tuple() noexcept {
    static_assert(
        sizeof(T) && false,
        "====================> Boost.PFR: Field's names extracting functionality requires C++20."
    );

    return detail::sequence_tuple::make_sequence_tuple();
}

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_CORE_NAME14_DISABLED_HPP

#endif

#endif // BOOST_PFR_DETAIL_CORE_NAME_HPP



#include <cstddef> // for std::size_t


/// \file boost/pfr/core_name.hpp
/// Contains functions \forcedlink{get_name} and \forcedlink{names_as_array} to know which names each field of any \aggregate has.
///
/// \fnrefl for details.
///
/// \b Synopsis:

namespace boost { namespace pfr {

/// \brief Returns name of a field with index `I` in \aggregate `T`.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///
///     assert(boost::pfr::get_name<0, my_struct>() == "i");
///     assert(boost::pfr::get_name<1, my_struct>() == "s");
/// \endcode
template <std::size_t I, class T>
constexpr
#ifdef BOOST_PFR_DOXYGEN_INVOKED
std::string_view
#else
auto
#endif
get_name() noexcept {
    return detail::get_name<T, I>();
}

// FIXME: implement this
// template<class U, class T>
// constexpr auto get_name() noexcept {
//     return detail::sequence_tuple::get_by_type_impl<U>( detail::tie_as_names_tuple<T>() );
// }

/// \brief Creates a `std::array` from names of fields of an \aggregate `T`.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     std::array<std::string_view, 2> a = boost::pfr::names_as_array<my_struct>();
///     assert(a[0] == "i");
/// \endcode
template <class T>
constexpr
#ifdef BOOST_PFR_DOXYGEN_INVOKED
std::array<std::string_view, boost::pfr::tuple_size_v<T>>
#else
auto
#endif
names_as_array() noexcept {
    return detail::make_stdarray_from_tietuple(
        detail::tie_as_names_tuple<T>(),
        detail::make_index_sequence< tuple_size_v<T> >(),
        1L
    );
}

}} // namespace boost::pfr

#endif // BOOST_PFR_CORE_NAME_HPP

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FUNCTIONS_FOR_HPP
#define BOOST_PFR_FUNCTIONS_FOR_HPP


// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_OPS_FIELDS_HPP
#define BOOST_PFR_OPS_FIELDS_HPP


// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_FUNCTIONAL_HPP
#define BOOST_PFR_DETAIL_FUNCTIONAL_HPP


#include <functional>
#include <cstdint>


namespace boost { namespace pfr { namespace detail {
    template <std::size_t I, std::size_t N>
    struct equal_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return ::boost::pfr::detail::sequence_tuple::get<I>(v1) == ::boost::pfr::detail::sequence_tuple::get<I>(v2)
                && equal_impl<I + 1, N>::cmp(v1, v2);
        }
    };

    template <std::size_t N>
    struct equal_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v == U::size_v;
        }
    };

    template <std::size_t I, std::size_t N>
    struct not_equal_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return ::boost::pfr::detail::sequence_tuple::get<I>(v1) != ::boost::pfr::detail::sequence_tuple::get<I>(v2)
                || not_equal_impl<I + 1, N>::cmp(v1, v2);
        }
    };

    template <std::size_t N>
    struct not_equal_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v != U::size_v;
        }
    };

    template <std::size_t I, std::size_t N>
    struct less_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return sequence_tuple::get<I>(v1) < sequence_tuple::get<I>(v2)
                || (sequence_tuple::get<I>(v1) == sequence_tuple::get<I>(v2) && less_impl<I + 1, N>::cmp(v1, v2));
        }
    };

    template <std::size_t N>
    struct less_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v < U::size_v;
        }
    };

    template <std::size_t I, std::size_t N>
    struct less_equal_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return sequence_tuple::get<I>(v1) < sequence_tuple::get<I>(v2)
                || (sequence_tuple::get<I>(v1) == sequence_tuple::get<I>(v2) && less_equal_impl<I + 1, N>::cmp(v1, v2));
        }
    };

    template <std::size_t N>
    struct less_equal_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v <= U::size_v;
        }
    };

    template <std::size_t I, std::size_t N>
    struct greater_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return sequence_tuple::get<I>(v1) > sequence_tuple::get<I>(v2)
                || (sequence_tuple::get<I>(v1) == sequence_tuple::get<I>(v2) && greater_impl<I + 1, N>::cmp(v1, v2));
        }
    };

    template <std::size_t N>
    struct greater_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v > U::size_v;
        }
    };

    template <std::size_t I, std::size_t N>
    struct greater_equal_impl {
        template <class T, class U>
        constexpr static bool cmp(const T& v1, const U& v2) noexcept {
            return sequence_tuple::get<I>(v1) > sequence_tuple::get<I>(v2)
                || (sequence_tuple::get<I>(v1) == sequence_tuple::get<I>(v2) && greater_equal_impl<I + 1, N>::cmp(v1, v2));
        }
    };

    template <std::size_t N>
    struct greater_equal_impl<N, N> {
        template <class T, class U>
        constexpr static bool cmp(const T&, const U&) noexcept {
            return T::size_v >= U::size_v;
        }
    };

    // Hash combine functions copied from Boost.ContainerHash
    // https://github.com/boostorg/container_hash/blob/171c012d4723c5e93cc7cffe42919afdf8b27dfa/include/boost/container_hash/hash.hpp#L311
    // that is based on Peter Dimov's proposal
    // http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf
    // issue 6.18.
    //
    // This also contains public domain code from MurmurHash. From the
    // MurmurHash header:
    //
    // MurmurHash3 was written by Austin Appleby, and is placed in the public
    // domain. The author hereby disclaims copyright to this source code.
    template <typename SizeT>
    constexpr void hash_combine(SizeT& seed, SizeT value) noexcept {
        seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    constexpr auto rotl(std::uint32_t x, std::uint32_t r) noexcept {
        return (x << r) | (x >> (32 - r));
    }

    constexpr void hash_combine(std::uint32_t& h1, std::uint32_t k1) noexcept {
          const std::uint32_t c1 = 0xcc9e2d51;
          const std::uint32_t c2 = 0x1b873593;

          k1 *= c1;
          k1 = detail::rotl(k1,15);
          k1 *= c2;

          h1 ^= k1;
          h1 = detail::rotl(h1,13);
          h1 = h1*5+0xe6546b64;
    }

#if defined(INT64_MIN) && defined(UINT64_MAX)
    constexpr void hash_combine(std::uint64_t& h, std::uint64_t k) noexcept {
        const std::uint64_t m = 0xc6a4a7935bd1e995ULL;
        const int r = 47;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;

        // Completely arbitrary number, to prevent 0's
        // from hashing to 0.
        h += 0xe6546b64;
    }
#endif

    template <typename T>
    auto compute_hash(const T& value, long /*priority*/)
        -> decltype(std::hash<T>()(value))
    {
        return std::hash<T>()(value);
    }

    template <typename T>
    std::size_t compute_hash(const T& /*value*/, int /*priority*/) {
        static_assert(sizeof(T) && false, "====================> Boost.PFR: std::hash not specialized for type T");
        return 0;
    }

    template <std::size_t I, std::size_t N>
    struct hash_impl {
        template <class T>
        constexpr static std::size_t compute(const T& val) noexcept {
            std::size_t h = detail::compute_hash( ::boost::pfr::detail::sequence_tuple::get<I>(val), 1L );
            detail::hash_combine(h, hash_impl<I + 1, N>::compute(val) );
            return h;
        }
    };

    template <std::size_t N>
    struct hash_impl<N, N> {
        template <class T>
        constexpr static std::size_t compute(const T&) noexcept {
            return 0;
        }
    };

///////////////////// Define min_element and to avoid inclusion of <algorithm>
    constexpr std::size_t min_size(std::size_t x, std::size_t y) noexcept {
        return x < y ? x : y;
    }

    template <template <std::size_t, std::size_t> class Visitor, class T, class U>
    constexpr bool binary_visit(const T& x, const U& y) {
        constexpr std::size_t fields_count_lhs = detail::fields_count<std::remove_reference_t<T>>();
        constexpr std::size_t fields_count_rhs = detail::fields_count<std::remove_reference_t<U>>();
        constexpr std::size_t fields_count_min = detail::min_size(fields_count_lhs, fields_count_rhs);
        typedef Visitor<0, fields_count_min> visitor_t;

#if BOOST_PFR_USE_CPP17 || BOOST_PFR_USE_LOOPHOLE
        return visitor_t::cmp(detail::tie_as_tuple(x), detail::tie_as_tuple(y));
#else
        bool result = true;
        ::boost::pfr::detail::for_each_field_dispatcher(
            x,
            [&result, &y](const auto& lhs) {
                constexpr std::size_t fields_count_rhs_ = detail::fields_count<std::remove_reference_t<U>>();
                ::boost::pfr::detail::for_each_field_dispatcher(
                    y,
                    [&result, &lhs](const auto& rhs) {
                        result = visitor_t::cmp(lhs, rhs);
                    },
                    detail::make_index_sequence<fields_count_rhs_>{}
                );
            },
            detail::make_index_sequence<fields_count_lhs>{}
        );

        return result;
#endif
    }

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_FUNCTIONAL_HPP

/// \file boost/pfr/ops_fields.hpp
/// Contains field-by-fields comparison and hash functions.
///
/// \b Example:
/// \code
///     #include <boost/pfr/ops_fields.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s;
///     };
///     // ...
///
///     comparable_struct s1 {0, 1};
///     comparable_struct s2 {0, 2};
///     assert(boost::pfr::lt_fields(s1, s2));
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:
namespace boost { namespace pfr {

    /// Does a field-by-field equality comparison.
    ///
    /// \returns `L == R && tuple_size_v<T> == tuple_size_v<U>`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool eq_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::equal_impl>(lhs, rhs);
    }


    /// Does a field-by-field inequality comparison.
    ///
    /// \returns `L != R || tuple_size_v<T> != tuple_size_v<U>`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool ne_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::not_equal_impl>(lhs, rhs);
    }

    /// Does a field-by-field greter comparison.
    ///
    /// \returns `L > R || (L == R && tuple_size_v<T> > tuple_size_v<U>)`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool gt_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::greater_impl>(lhs, rhs);
    }


    /// Does a field-by-field less comparison.
    ///
    /// \returns `L < R || (L == R && tuple_size_v<T> < tuple_size_v<U>)`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool lt_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::less_impl>(lhs, rhs);
    }


    /// Does a field-by-field greater equal comparison.
    ///
    /// \returns `L > R || (L == R && tuple_size_v<T> >= tuple_size_v<U>)`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool ge_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::greater_equal_impl>(lhs, rhs);
    }


    /// Does a field-by-field less equal comparison.
    ///
    /// \returns `L < R || (L == R && tuple_size_v<T> <= tuple_size_v<U>)`, where `L` and
    /// `R` are the results of calling `std::tie` on first `N` fields of `lhs` and
    // `rhs` respectively; `N` is `std::min(tuple_size_v<T>, tuple_size_v<U>)`.
    template <class T, class U>
    constexpr bool le_fields(const T& lhs, const U& rhs) noexcept {
        return detail::binary_visit<detail::less_equal_impl>(lhs, rhs);
    }


    /// Does a field-by-field hashing.
    ///
    /// \returns combined hash of all the fields
    template <class T>
    std::size_t hash_fields(const T& x) {
        constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();
#if BOOST_PFR_USE_CPP17 || BOOST_PFR_USE_LOOPHOLE
        return detail::hash_impl<0, fields_count_val>::compute(detail::tie_as_tuple(x));
#else
        std::size_t result = 0;
        ::boost::pfr::detail::for_each_field_dispatcher(
            x,
            [&result](const auto& lhs) {
                // We can not reuse `fields_count_val` in lambda because compilers had issues with
                // passing constexpr variables into lambdas. Computing is again is the most portable solution.
                constexpr std::size_t fields_count_val_lambda = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();
                result = detail::hash_impl<0, fields_count_val_lambda>::compute(lhs);
            },
            detail::make_index_sequence<fields_count_val>{}
        );

        return result;
#endif
    }
}} // namespace boost::pfr

#endif // BOOST_PFR_OPS_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PFR_IO_FIELDS_HPP
#define BOOST_PFR_IO_FIELDS_HPP



#include <type_traits>
#include <utility>      // metaprogramming stuff

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_IO_HPP
#define BOOST_PFR_DETAIL_IO_HPP


#include <iosfwd>       // stream operators
#include <iomanip>

#if defined(__has_include)
#   if __has_include(<string_view>) && BOOST_PFR_USE_CPP17
#       include <string_view>
#   endif
#endif

namespace boost { namespace pfr { namespace detail {

inline auto quoted_helper(const std::string& s) noexcept {
    return std::quoted(s);
}

#if defined(__has_include)
#   if __has_include(<string_view>) && BOOST_PFR_USE_CPP17
template <class CharT, class Traits>
inline auto quoted_helper(std::basic_string_view<CharT, Traits> s) noexcept {
    return std::quoted(s);
}
#   endif
#endif

inline auto quoted_helper(std::string& s) noexcept {
    return std::quoted(s);
}

template <class T>
inline decltype(auto) quoted_helper(T&& v) noexcept {
    return std::forward<T>(v);
}

template <std::size_t I, std::size_t N>
struct print_impl {
    template <class Stream, class T>
    static void print (Stream& out, const T& value) {
        if (!!I) out << ", ";
        out << detail::quoted_helper(boost::pfr::detail::sequence_tuple::get<I>(value));
        print_impl<I + 1, N>::print(out, value);
    }
};

template <std::size_t I>
struct print_impl<I, I> {
    template <class Stream, class T> static void print (Stream&, const T&) noexcept {}
};


template <std::size_t I, std::size_t N>
struct read_impl {
    template <class Stream, class T>
    static void read (Stream& in, const T& value) {
        char ignore = {};
        if (!!I) {
            in >> ignore;
            if (ignore != ',') in.setstate(Stream::failbit);
            in >> ignore;
            if (ignore != ' ')  in.setstate(Stream::failbit);
        }
        in >> detail::quoted_helper( boost::pfr::detail::sequence_tuple::get<I>(value) );
        read_impl<I + 1, N>::read(in, value);
    }
};

template <std::size_t I>
struct read_impl<I, I> {
    template <class Stream, class T> static void read (Stream&, const T&) {}
};

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_IO_HPP

/// \file boost/pfr/io_fields.hpp
/// Contains IO manipulator \forcedlink{io_fields} to read/write any \aggregate field-by-field.
///
/// \b Example:
/// \code
///     struct my_struct {
///         int i;
///         short s;
///     };
///
///     std::ostream& operator<<(std::ostream& os, const my_struct& x) {
///         return os << boost::pfr::io_fields(x);  // Equivalent to: os << "{ " << x.i << " ," <<  x.s << " }"
///     }
///
///     std::istream& operator>>(std::istream& is, my_struct& x) {
///         return is >> boost::pfr::io_fields(x);  // Equivalent to: is >> "{ " >> x.i >> " ," >>  x.s >> " }"
///     }
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:

namespace boost { namespace pfr {

namespace detail {

template <class T>
struct io_fields_impl {
    T value;
};


template <class Char, class Traits, class T>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, io_fields_impl<const T&>&& x) {
    const T& value = x.value;
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<T>();
    out << '{';
#if BOOST_PFR_USE_CPP17 || BOOST_PFR_USE_LOOPHOLE
    detail::print_impl<0, fields_count_val>::print(out, detail::tie_as_tuple(value));
#else
    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [&out](const auto& val) {
            // We can not reuse `fields_count_val` in lambda because compilers had issues with
            // passing constexpr variables into lambdas. Computing is again is the most portable solution.
            constexpr std::size_t fields_count_val_lambda = boost::pfr::detail::fields_count<T>();
            detail::print_impl<0, fields_count_val_lambda>::print(out, val);
        },
        detail::make_index_sequence<fields_count_val>{}
    );
#endif
    return out << '}';
}


template <class Char, class Traits, class T>
std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, io_fields_impl<T>&& x) {
    return out << io_fields_impl<const std::remove_reference_t<T>&>{x.value};
}

template <class Char, class Traits, class T>
std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, io_fields_impl<T&>&& x) {
    T& value = x.value;
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<T>();

    const auto prev_exceptions = in.exceptions();
    in.exceptions( typename std::basic_istream<Char, Traits>::iostate(0) );
    const auto prev_flags = in.flags( typename std::basic_istream<Char, Traits>::fmtflags(0) );

    char parenthis = {};
    in >> parenthis;
    if (parenthis != '{') in.setstate(std::basic_istream<Char, Traits>::failbit);

#if BOOST_PFR_USE_CPP17 || BOOST_PFR_USE_LOOPHOLE
    detail::read_impl<0, fields_count_val>::read(in, detail::tie_as_tuple(value));
#else
    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [&in](const auto& val) {
            // We can not reuse `fields_count_val` in lambda because compilers had issues with
            // passing constexpr variables into lambdas. Computing is again is the most portable solution.
            constexpr std::size_t fields_count_val_lambda = boost::pfr::detail::fields_count<T>();
            detail::read_impl<0, fields_count_val_lambda>::read(in, val);
        },
        detail::make_index_sequence<fields_count_val>{}
    );
#endif

    in >> parenthis;
    if (parenthis != '}') in.setstate(std::basic_istream<Char, Traits>::failbit);

    in.flags(prev_flags);
    in.exceptions(prev_exceptions);

    return in;
}

template <class Char, class Traits, class T>
std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, io_fields_impl<const T&>&& ) {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Attempt to use istream operator on a boost::pfr::io_fields wrapped type T with const qualifier.");
    return in;
}

template <class Char, class Traits, class T>
std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, io_fields_impl<T>&& ) {
    static_assert(sizeof(T) && false, "====================> Boost.PFR: Attempt to use istream operator on a boost::pfr::io_fields wrapped temporary of type T.");
    return in;
}

} // namespace detail

/// IO manipulator to read/write \aggregate `value` field-by-field.
///
/// \b Example:
/// \code
///     struct my_struct {
///         int i;
///         short s;
///     };
///
///     std::ostream& operator<<(std::ostream& os, const my_struct& x) {
///         return os << boost::pfr::io_fields(x);  // Equivalent to: os << "{ " << x.i << " ," <<  x.s << " }"
///     }
///
///     std::istream& operator>>(std::istream& is, my_struct& x) {
///         return is >> boost::pfr::io_fields(x);  // Equivalent to: is >> "{ " >> x.i >> " ," >>  x.s >> " }"
///     }
/// \endcode
///
/// Input and output streaming operators for `boost::pfr::io_fields` are symmetric, meaning that you get the original value by streaming it and
/// reading back if each fields streaming operator is symmetric.
///
/// \customio
template <class T>
auto io_fields(T&& value) noexcept {
    return detail::io_fields_impl<T>{std::forward<T>(value)};
}

}} // namespace boost::pfr

#endif // BOOST_PFR_IO_FIELDS_HPP

/// \file boost/pfr/functions_for.hpp
/// Contains BOOST_PFR_FUNCTIONS_FOR macro that defined comparison and stream operators for T along with hash_value function.
/// \b Example:
/// \code
///     #include <boost/pfr/functions_for.hpp>
///
///     namespace my_namespace {
///         struct my_struct {      // No operators defined for that structure
///             int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///         };
///         BOOST_PFR_FUNCTIONS_FOR(my_struct)
///     }
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:

/// \def BOOST_PFR_FUNCTIONS_FOR(T)
/// Defines comparison and stream operators for T along with hash_value function.
///
/// \b Example:
/// \code
///     #include <boost/pfr/functions_for.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     BOOST_PFR_FUNCTIONS_FOR(comparable_struct)
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(s1 < s2);
///     std::cout << s1 << std::endl; // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Defines \b following \b for \b T:
/// \code
/// bool operator==(const T& lhs, const T& rhs);
/// bool operator!=(const T& lhs, const T& rhs);
/// bool operator< (const T& lhs, const T& rhs);
/// bool operator> (const T& lhs, const T& rhs);
/// bool operator<=(const T& lhs, const T& rhs);
/// bool operator>=(const T& lhs, const T& rhs);
///
/// template <class Char, class Traits>
/// std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const T& value);
///
/// template <class Char, class Traits>
/// std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, T& value);
///
/// // helper function for Boost unordered containers and boost::hash<>.
/// std::size_t hash_value(const T& value);
/// \endcode

#define BOOST_PFR_FUNCTIONS_FOR(T)                                                                                                          \
    BOOST_PFR_MAYBE_UNUSED inline bool operator==(const T& lhs, const T& rhs) { return ::boost::pfr::eq_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator!=(const T& lhs, const T& rhs) { return ::boost::pfr::ne_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator< (const T& lhs, const T& rhs) { return ::boost::pfr::lt_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator> (const T& lhs, const T& rhs) { return ::boost::pfr::gt_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator<=(const T& lhs, const T& rhs) { return ::boost::pfr::le_fields(lhs, rhs); }                 \
    BOOST_PFR_MAYBE_UNUSED inline bool operator>=(const T& lhs, const T& rhs) { return ::boost::pfr::ge_fields(lhs, rhs); }                 \
    template <class Char, class Traits>                                                                                                     \
    BOOST_PFR_MAYBE_UNUSED inline ::std::basic_ostream<Char, Traits>& operator<<(::std::basic_ostream<Char, Traits>& out, const T& value) { \
        return out << ::boost::pfr::io_fields(value);                                                                                       \
    }                                                                                                                                       \
    template <class Char, class Traits>                                                                                                     \
    BOOST_PFR_MAYBE_UNUSED inline ::std::basic_istream<Char, Traits>& operator>>(::std::basic_istream<Char, Traits>& in, T& value) {        \
        return in >> ::boost::pfr::io_fields(value);                                                                                        \
    }                                                                                                                                       \
    BOOST_PFR_MAYBE_UNUSED inline std::size_t hash_value(const T& v) {                                                                      \
        return ::boost::pfr::hash_fields(v);                                                                                                \
    }                                                                                                                                       \
/**/

#endif // BOOST_PFR_FUNCTIONS_FOR_HPP

// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FUNCTORS_HPP
#define BOOST_PFR_FUNCTORS_HPP


// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_OPS_HPP
#define BOOST_PFR_OPS_HPP


// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_DETECTORS_HPP
#define BOOST_PFR_DETAIL_DETECTORS_HPP


#include <functional>
#include <type_traits>

namespace boost { namespace pfr { namespace detail {
///////////////////// `value` is true if Detector<Tleft, Tright> does not compile (SFINAE)
    struct can_not_apply{};

    template <template <class, class> class Detector, class Tleft, class Tright>
    struct not_appliable {
        static constexpr bool value = std::is_same<
            Detector<Tleft, Tright>,
            can_not_apply
        >::value;
    };

///////////////////// Detectors for different operators
    template <class S, class T> auto comp_eq_detector_msvc_helper(long) -> decltype(std::declval<S>() == std::declval<T>());
    template <class S, class T> can_not_apply comp_eq_detector_msvc_helper(int);
    template <class T1, class T2> using comp_eq_detector = decltype(comp_eq_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_ne_detector_msvc_helper(long) -> decltype(std::declval<S>() != std::declval<T>());
    template <class S, class T> can_not_apply comp_ne_detector_msvc_helper(int);
    template <class T1, class T2> using comp_ne_detector = decltype(comp_ne_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_lt_detector_msvc_helper(long) -> decltype(std::declval<S>() < std::declval<T>());
    template <class S, class T> can_not_apply comp_lt_detector_msvc_helper(int);
    template <class T1, class T2> using comp_lt_detector = decltype(comp_lt_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_le_detector_msvc_helper(long) -> decltype(std::declval<S>() <= std::declval<T>());
    template <class S, class T> can_not_apply comp_le_detector_msvc_helper(int);
    template <class T1, class T2> using comp_le_detector = decltype(comp_le_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_gt_detector_msvc_helper(long) -> decltype(std::declval<S>() > std::declval<T>());
    template <class S, class T> can_not_apply comp_gt_detector_msvc_helper(int);
    template <class T1, class T2> using comp_gt_detector = decltype(comp_gt_detector_msvc_helper<T1,T2>(1L));

    template <class S, class T> auto comp_ge_detector_msvc_helper(long) -> decltype(std::declval<S>() >= std::declval<T>());
    template <class S, class T> can_not_apply comp_ge_detector_msvc_helper(int);
    template <class T1, class T2> using comp_ge_detector = decltype(comp_ge_detector_msvc_helper<T1,T2>(1L));


    template <class S> auto hash_detector_msvc_helper(long) -> decltype(std::hash<S>{}(std::declval<S>()));
    template <class S> can_not_apply hash_detector_msvc_helper(int);
    template <class T1, class T2> using hash_detector = decltype(hash_detector_msvc_helper<T1,T2>(1L));


    template <class S, class T> auto ostreamable_detector_msvc_helper(long) -> decltype(std::declval<S>() << std::declval<T>());
    template <class S, class T> can_not_apply ostreamable_detector_msvc_helper(int);
    template <class S, class T> using ostreamable_detector = decltype(ostreamable_detector_msvc_helper<S,T>(1L));

    template <class S, class T> auto istreamable_detector_msvc_helper(long) -> decltype(std::declval<S>() >> std::declval<T>());
    template <class S, class T> can_not_apply istreamable_detector_msvc_helper(int);
    template <class S, class T> using istreamable_detector = decltype(istreamable_detector_msvc_helper<S,T>(1L));

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_DETECTORS_HPP



/// \file boost/pfr/ops.hpp
/// Contains comparison and hashing functions.
/// If type is comparable using its own operator or its conversion operator, then the types operator is used. Otherwise
/// the operation is done via corresponding function from boost/pfr/ops.hpp header.
///
/// \b Example:
/// \code
///     #include <boost/pfr/ops.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(boost::pfr::lt(s1, s2));
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:
namespace boost { namespace pfr {

namespace detail {

///////////////////// Helper typedefs that are used by all the ops
    template <template <class, class> class Detector, class T, class U>
    using enable_not_comp_base_t = std::enable_if_t<
        not_appliable<Detector, T const&, U const&>::value,
        bool
    >;

    template <template <class, class> class Detector, class T, class U>
    using enable_comp_base_t = std::enable_if_t<
        !not_appliable<Detector, T const&, U const&>::value,
        bool
    >;
///////////////////// std::enable_if_t like functions that enable only if types do not support operation

    template <class T, class U> using enable_not_eq_comp_t = enable_not_comp_base_t<comp_eq_detector, T, U>;
    template <class T, class U> using enable_not_ne_comp_t = enable_not_comp_base_t<comp_ne_detector, T, U>;
    template <class T, class U> using enable_not_lt_comp_t = enable_not_comp_base_t<comp_lt_detector, T, U>;
    template <class T, class U> using enable_not_le_comp_t = enable_not_comp_base_t<comp_le_detector, T, U>;
    template <class T, class U> using enable_not_gt_comp_t = enable_not_comp_base_t<comp_gt_detector, T, U>;
    template <class T, class U> using enable_not_ge_comp_t = enable_not_comp_base_t<comp_ge_detector, T, U>;

    template <class T> using enable_not_hashable_t = std::enable_if_t<
        not_appliable<hash_detector, const T&, const T&>::value,
        std::size_t
    >;
///////////////////// std::enable_if_t like functions that enable only if types do support operation

    template <class T, class U> using enable_eq_comp_t = enable_comp_base_t<comp_eq_detector, T, U>;
    template <class T, class U> using enable_ne_comp_t = enable_comp_base_t<comp_ne_detector, T, U>;
    template <class T, class U> using enable_lt_comp_t = enable_comp_base_t<comp_lt_detector, T, U>;
    template <class T, class U> using enable_le_comp_t = enable_comp_base_t<comp_le_detector, T, U>;
    template <class T, class U> using enable_gt_comp_t = enable_comp_base_t<comp_gt_detector, T, U>;
    template <class T, class U> using enable_ge_comp_t = enable_comp_base_t<comp_ge_detector, T, U>;

    template <class T> using enable_hashable_t = std::enable_if_t<
        !not_appliable<hash_detector, const T&, const T&>::value,
        std::size_t
    >;
} // namespace detail


/// \brief Compares lhs and rhs for equality using their own comparison and conversion operators; if no operators available returns \forcedlink{eq_fields}(lhs, rhs).
///
/// \returns true if lhs is equal to rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_eq_comp_t<T, U> eq(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::eq_fields(lhs, rhs);
}

/// \overload eq
template <class T, class U>
constexpr detail::enable_eq_comp_t<T, U> eq(const T& lhs, const U& rhs) {
    return lhs == rhs;
}


/// \brief Compares lhs and rhs for inequality using their own comparison and conversion operators; if no operators available returns \forcedlink{ne_fields}(lhs, rhs).
///
/// \returns true if lhs is not equal to rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_ne_comp_t<T, U> ne(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::ne_fields(lhs, rhs);
}

/// \overload ne
template <class T, class U>
constexpr detail::enable_ne_comp_t<T, U> ne(const T& lhs, const U& rhs) {
    return lhs != rhs;
}


/// \brief Compares lhs and rhs for less-than using their own comparison and conversion operators; if no operators available returns \forcedlink{lt_fields}(lhs, rhs).
///
/// \returns true if lhs is less than rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_lt_comp_t<T, U> lt(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::lt_fields(lhs, rhs);
}

/// \overload lt
template <class T, class U>
constexpr detail::enable_lt_comp_t<T, U> lt(const T& lhs, const U& rhs) {
    return lhs < rhs;
}


/// \brief Compares lhs and rhs for greater-than using their own comparison and conversion operators; if no operators available returns \forcedlink{lt_fields}(lhs, rhs).
///
/// \returns true if lhs is greater than rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_gt_comp_t<T, U> gt(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::gt_fields(lhs, rhs);
}

/// \overload gt
template <class T, class U>
constexpr detail::enable_gt_comp_t<T, U> gt(const T& lhs, const U& rhs) {
    return lhs > rhs;
}


/// \brief Compares lhs and rhs for less-equal using their own comparison and conversion operators; if no operators available returns \forcedlink{le_fields}(lhs, rhs).
///
/// \returns true if lhs is less or equal to rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_le_comp_t<T, U> le(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::le_fields(lhs, rhs);
}

/// \overload le
template <class T, class U>
constexpr detail::enable_le_comp_t<T, U> le(const T& lhs, const U& rhs) {
    return lhs <= rhs;
}


/// \brief Compares lhs and rhs for greater-equal using their own comparison and conversion operators; if no operators available returns \forcedlink{ge_fields}(lhs, rhs).
///
/// \returns true if lhs is greater or equal to rhs; false otherwise
template <class T, class U>
constexpr detail::enable_not_ge_comp_t<T, U> ge(const T& lhs, const U& rhs) noexcept {
    return boost::pfr::ge_fields(lhs, rhs);
}

/// \overload ge
template <class T, class U>
constexpr detail::enable_ge_comp_t<T, U> ge(const T& lhs, const U& rhs) {
    return lhs >= rhs;
}


/// \brief Hashes value using its own std::hash specialization; if no std::hash specialization available returns \forcedlink{hash_fields}(value).
///
/// \returns std::size_t with hash of the value
template <class T>
constexpr detail::enable_not_hashable_t<T> hash_value(const T& value) noexcept {
    return boost::pfr::hash_fields(value);
}

/// \overload hash_value
template <class T>
constexpr detail::enable_hashable_t<T> hash_value(const T& value) {
    return std::hash<T>{}(value);
}

}} // namespace boost::pfr

#endif // BOOST_PFR_OPS_HPP


/// \file boost/pfr/functors.hpp
/// Contains functors that are close to the Standard Library ones.
/// Each functor calls corresponding Boost.PFR function from boost/pfr/ops.hpp
///
/// \b Example:
/// \code
///     #include <boost/pfr/functors.hpp>
///     struct my_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     // ...
///
///     std::unordered_set<
///         my_struct,
///         boost::pfr::hash<>,
///         boost::pfr::equal_to<>
///     > my_set;
/// \endcode
///
/// \b Synopsis:
namespace boost { namespace pfr {

///////////////////// Comparisons

/// \brief std::equal_to like comparator that returns \forcedlink{eq}(x, y)
template <class T = void> struct equal_to {
    /// \return \b true if each field of \b x equals the field with same index of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::eq(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct equal_to<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::eq(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::not_equal like comparator that returns \forcedlink{ne}(x, y)
template <class T = void> struct not_equal {
    /// \return \b true if at least one field \b x not equals the field with same index of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::ne(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct not_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::ne(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::greater like comparator that returns \forcedlink{gt}(x, y)
template <class T = void> struct greater {
    /// \return \b true if field of \b x greater than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::gt(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct greater<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::gt(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::less like comparator that returns \forcedlink{lt}(x, y)
template <class T = void> struct less {
    /// \return \b true if field of \b x less than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::lt(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct less<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::lt(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::greater_equal like comparator that returns \forcedlink{ge}(x, y)
template <class T = void> struct greater_equal {
    /// \return \b true if field of \b x greater than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y;
    /// or if each field of \b x equals the field with same index of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::ge(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct greater_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::ge(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::less_equal like comparator that returns \forcedlink{le}(x, y)
template <class T = void> struct less_equal {
    /// \return \b true if field of \b x less than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y;
    /// or if each field of \b x equals the field with same index of \b y.
    bool operator()(const T& x, const T& y) const {
        return boost::pfr::le(x, y);
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const;
#endif
};

/// @cond
template <> struct less_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const {
        return boost::pfr::le(x, y);
    }

    typedef std::true_type is_transparent;
};
/// @endcond


/// \brief std::hash like functor that returns \forcedlink{hash_value}(x)
template <class T> struct hash {
    /// \return hash value of \b x.
    std::size_t operator()(const T& x) const {
        return boost::pfr::hash_value(x);
    }
};

}} // namespace boost::pfr

#endif // BOOST_PFR_FUNCTORS_HPP
// Copyright (c) 2016-2023 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_IO_HPP
#define BOOST_PFR_IO_HPP



/// \file boost/pfr/io.hpp
/// Contains IO stream manipulator \forcedlink{io} for types.
/// If type is streamable using its own operator or its conversion operator, then the types operator is used.
///
/// \b Example:
/// \code
///     #include <boost/pfr/io.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     std::cout << boost::pfr::io(s1);  // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \podops for other ways to define operators and more details.
///
/// \b Synopsis:
namespace boost { namespace pfr {

namespace detail {

///////////////////// Helper typedefs
    template <class Stream, class Type>
    using enable_not_ostreamable_t = std::enable_if_t<
        not_appliable<ostreamable_detector, Stream&, const std::remove_reference_t<Type>&>::value,
        Stream&
    >;

    template <class Stream, class Type>
    using enable_not_istreamable_t = std::enable_if_t<
        not_appliable<istreamable_detector, Stream&, Type&>::value,
        Stream&
    >;

    template <class Stream, class Type>
    using enable_ostreamable_t = std::enable_if_t<
        !not_appliable<ostreamable_detector, Stream&, const std::remove_reference_t<Type>&>::value,
        Stream&
    >;

    template <class Stream, class Type>
    using enable_istreamable_t = std::enable_if_t<
        !not_appliable<istreamable_detector, Stream&, Type&>::value,
        Stream&
    >;

///////////////////// IO impl

template <class T>
struct io_impl {
    T value;
};

template <class Char, class Traits, class T>
enable_not_ostreamable_t<std::basic_ostream<Char, Traits>, T> operator<<(std::basic_ostream<Char, Traits>& out, io_impl<T>&& x) {
    return out << boost::pfr::io_fields(std::forward<T>(x.value));
}

template <class Char, class Traits, class T>
enable_ostreamable_t<std::basic_ostream<Char, Traits>, T> operator<<(std::basic_ostream<Char, Traits>& out, io_impl<T>&& x) {
    return out << x.value;
}

template <class Char, class Traits, class T>
enable_not_istreamable_t<std::basic_istream<Char, Traits>, T> operator>>(std::basic_istream<Char, Traits>& in, io_impl<T>&& x) {
    return in >> boost::pfr::io_fields(std::forward<T>(x.value));
}

template <class Char, class Traits, class T>
enable_istreamable_t<std::basic_istream<Char, Traits>, T> operator>>(std::basic_istream<Char, Traits>& in, io_impl<T>&& x) {
    return in >> x.value;
}

} // namespace detail

/// IO manipulator to read/write \aggregate `value` using its IO stream operators or using \forcedlink{io_fields} if operators are not available.
///
/// \b Example:
/// \code
///     struct my_struct { int i; short s; };
///     my_struct x;
///     std::stringstream ss;
///     ss << "{ 12, 13 }";
///     ss >> boost::pfr::io(x);
///     assert(x.i == 12);
///     assert(x.s == 13);
/// \endcode
///
/// \customio
template <class T>
auto io(T&& value) noexcept {
    return detail::io_impl<T>{std::forward<T>(value)};
}

}} // namespace boost::pfr

#endif // BOOST_PFR_IO_HPP
// Copyright (c) 2022 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_TRAITS_FWD_HPP
#define BOOST_PFR_DETAIL_TRAITS_FWD_HPP


namespace boost { namespace pfr {

template<class T, class WhatFor>
struct is_reflectable;

}} // namespace boost::pfr

#endif // BOOST_PFR_DETAIL_TRAITS_FWD_HPP


// Copyright (c) 2022 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_TRAITS_HPP
#define BOOST_PFR_TRAITS_HPP


// Copyright (c) 2022 Denis Mikhailov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_POSSIBLE_REFLECTABLE_HPP
#define BOOST_PFR_DETAIL_POSSIBLE_REFLECTABLE_HPP


#include <type_traits> // for std::is_aggregate

namespace boost { namespace pfr { namespace detail {

///////////////////// Returns false when the type exactly wasn't be reflectable
template <class T, class WhatFor>
constexpr decltype(is_reflectable<T, WhatFor>::value) possible_reflectable(long) noexcept {
    return is_reflectable<T, WhatFor>::value;
}

#if BOOST_PFR_ENABLE_IMPLICIT_REFLECTION

template <class T, class WhatFor>
constexpr bool possible_reflectable(int) noexcept {
#   if  defined(__cpp_lib_is_aggregate)
    using type = std::remove_cv_t<T>;
    return std::is_aggregate<type>();
#   else
    return true;
#   endif
}

#else

template <class T, class WhatFor>
constexpr bool possible_reflectable(int) noexcept {
    // negative answer here won't change behaviour in PFR-dependent libraries(like Fusion)
    return false;
}

#endif

}}} // namespace boost::pfr::detail

#endif // BOOST_PFR_DETAIL_POSSIBLE_REFLECTABLE_HPP


#include <type_traits>

/// \file boost/pfr/traits.hpp
/// Contains traits \forcedlink{is_reflectable} and \forcedlink{is_implicitly_reflectable} for detecting an ability to reflect type.
///
/// \b Synopsis:

namespace boost { namespace pfr {

/// Has a static const member variable `value` when it is known that type T can or can't be reflected using Boost.PFR; otherwise, there is no member variable.
/// Every user may (and in some difficult cases - should) specialize is_reflectable on his own.
///
/// \b Example:
/// \code
///     namespace boost { namespace pfr {
///         template<class All> struct is_reflectable<A, All> : std::false_type {};       // 'A' won't be interpreted as reflectable everywhere
///         template<> struct is_reflectable<B, boost_fusion_tag> : std::false_type {};   // 'B' won't be interpreted as reflectable in only Boost Fusion
///     }}
/// \endcode
/// \note is_reflectable affects is_implicitly_reflectable, the decision made by is_reflectable is used by is_implicitly_reflectable.
template<class T, class WhatFor>
struct is_reflectable { /*  does not have 'value' because value is unknown */ };

// these specs can't be inherited from 'std::integral_constant< bool, boost::pfr::is_reflectable<T, WhatFor>::value >',
// because it will break the sfinae-friendliness
template<class T, class WhatFor>
struct is_reflectable<const T, WhatFor> : boost::pfr::is_reflectable<T, WhatFor> {};

template<class T, class WhatFor>
struct is_reflectable<volatile T, WhatFor> : boost::pfr::is_reflectable<T, WhatFor> {};

template<class T, class WhatFor>
struct is_reflectable<const volatile T, WhatFor> : boost::pfr::is_reflectable<T, WhatFor> {};

/// Checks the input type for the potential to be reflected.
/// Specialize is_reflectable if you disagree with is_implicitly_reflectable's default decision.
template<class T, class WhatFor>
using is_implicitly_reflectable = std::integral_constant< bool, boost::pfr::detail::possible_reflectable<T, WhatFor>(1L) >;

/// Checks the input type for the potential to be reflected.
/// Specialize is_reflectable if you disagree with is_implicitly_reflectable_v's default decision.
template<class T, class WhatFor>
constexpr bool is_implicitly_reflectable_v = is_implicitly_reflectable<T, WhatFor>::value;

}} // namespace boost::pfr

#endif // BOOST_PFR_TRAITS_HPP


#endif // BOOST_PFR_HPP
#define CPPGRES_USE_BOOST_PFR 1
/*
// This has been commented out while Boost.PFR is embedded with Cppgres

#if __has_include(<boost/pfr.hpp>)
#include <boost/pfr.hpp>
#define CPPGRES_USE_BOOST_PFR 1
#else
#define CPPGRES_USE_BOOST_PFR 0
#endif
*/
namespace cppgres::utils {

// Primary template: if T is not an optional, just yield T.
template <typename T> struct remove_optional {
  using type = T;
};

// Partial specialization for std::optional.
template <typename T> struct remove_optional<std::optional<T>> {
  using type = T;
};

// Convenience alias template.
template <typename T> using remove_optional_t = typename utils::remove_optional<T>::type;

template <typename T>
concept is_optional =
    requires { typename T::value_type; } && std::same_as<T, std::optional<typename T::value_type>>;

template <typename T> constexpr std::string_view type_name() {
#ifdef __clang__
  constexpr std::string_view p = __PRETTY_FUNCTION__;
  constexpr std::string_view key = "T = ";
  const auto start = p.find(key) + key.size();
  const auto end = p.find(']', start);
  return p.substr(start, end - start);
#elif defined(__GNUC__)
  constexpr std::string_view p = __PRETTY_FUNCTION__;
  constexpr std::string_view key = "T = ";
  const auto start = p.find(key) + key.size();
  const auto end = p.find(';', start);
  return p.substr(start, end - start);
#elif defined(_MSC_VER)
  constexpr std::string_view p = __FUNCSIG__;
  constexpr std::string_view key = "type_name<";
  const auto start = p.find(key) + key.size();
  const auto end = p.find(">(void)");
  return p.substr(start, end - start);
#else
  return "Unsupported compiler";
#endif
}

template <typename T, typename = void> struct tuple_traits_impl {
  using tuple_size_type = std::integral_constant<std::size_t, 1>;

  template <std::size_t I, typename U = T> static constexpr decltype(auto) get(U &&t) noexcept {
    return std::forward<U>(t);
  }

  struct tuple_element_t {
    using type = T;
  };

  template <std::size_t I> using tuple_element = tuple_element_t;
};

// Primary implementation: for types that already have a tuple-like interface
template <typename T>
struct tuple_traits_impl<T, std::void_t<decltype(std::tuple_size<T>::value)>> {
  using tuple_size_type = std::tuple_size<T>;

  template <std::size_t I, typename U = T> static constexpr decltype(auto) get(U &&t) noexcept {
    return std::get<I>(std::forward<U>(t));
  }

  template <std::size_t I> using tuple_element = std::tuple_element<I, T>;
};

// Specialization: for aggregates that Boost.PFR can handle.
// This specialization is enabled if the type is an aggregate
#if CPPGRES_USE_BOOST_PFR
template <typename T> struct tuple_traits_impl<T, std::enable_if_t<std::is_aggregate_v<T>>> {
  using tuple_size_type = boost::pfr::tuple_size<T>;

  template <std::size_t I, typename U = T> static constexpr decltype(auto) get(U &&t) noexcept {
    return boost::pfr::get<I>(std::forward<U>(t));
  }

  template <std::size_t I> using tuple_element = boost::pfr::tuple_element<I, T>;
};

#endif

template <typename T> using tuple_size = typename tuple_traits_impl<T>::tuple_size_type;

template <typename T> constexpr std::size_t tuple_size_v = tuple_size<T>::value;

template <std::size_t I, typename T>
using tuple_element = typename tuple_traits_impl<T>::template tuple_element<I>;

template <std::size_t I, typename T> using tuple_element_t = typename tuple_element<I, T>::type;

template <std::size_t I, typename T> constexpr decltype(auto) get(T &&t) noexcept {
  return tuple_traits_impl<std::remove_cv_t<std::remove_reference_t<T>>>::template get<I>(
      std::forward<T>(t));
}

template <typename T> struct is_std_tuple : std::false_type {};

template <typename... Ts> struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
concept std_tuple = is_std_tuple<T>::value;

template <typename T> decltype(auto) tie(T &val) {
#if CPPGRES_USE_BOOST_PFR == 1
  if constexpr (std::is_aggregate_v<T>) {
    return boost::pfr::structure_tie(val);
  } else
#endif
      if constexpr (std_tuple<T>) {
    return val;
  } else {
    return std::tuple(val);
  }
}

} // namespace cppgres::utils

/**
* \file
 */
#ifdef __cplusplus
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"
#endif
#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/amapi.h>
#include <access/tableam.h>
#include <access/tsmapi.h>
#include <access/tupdesc.h>
#include <catalog/namespace.h>
#include <catalog/pg_class.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <commands/event_trigger.h>
#include <executor/spi.h>
#include <foreign/fdwapi.h>
#include <funcapi.h>
#include <miscadmin.h>
#include <nodes/execnodes.h>
#include <nodes/extensible.h>
#include <nodes/memnodes.h>
#if __has_include(<nodes/miscnodes.h>)
#include <nodes/miscnodes.h>
#endif
#include <nodes/nodeFuncs.h>
#include <nodes/nodes.h>
#include <nodes/parsenodes.h>
#include <nodes/pathnodes.h>
#include <nodes/replnodes.h>
#include <nodes/supportnodes.h>
#include <nodes/tidbitmap.h>
#include <parser/analyze.h>
#include <parser/parser.h>
#include <storage/ipc.h>
#include <utils/builtins.h>
#include <utils/expandeddatum.h>
#include <utils/lsyscache.h>
#include <utils/memutils.h>
#include <utils/snapmgr.h>
#include <utils/syscache.h>
#include <utils/tuplestore.h>
#include <utils/typcache.h>
#ifdef __cplusplus
}
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
/**
 * \file
 */

#include <concepts>
#include <memory>

/**
* \file
 */

extern "C" {
#include <setjmp.h>
}

#include <iostream>
#include <utility>

/**
 * \file
 */

#include <string>
#include <tuple>

#include <iostream>

/**
 * \file
 */

namespace cppgres {
class pg_exception : public std::exception {
  ::MemoryContext mcxt;
  ::MemoryContext error_cxt;
  ::ErrorData *error;

  pg_exception(::MemoryContext mcxt);

  const char *what() const noexcept override { return error->message; }

  template <typename Func> friend struct ffi_guard;

public:
  const char *message() const noexcept { return error->message; }
  ~pg_exception();
};
} // namespace cppgres

namespace cppgres {

inline void error(pg_exception e);
inline void error(pg_exception e) {
  ::errstart(ERROR, TEXTDOMAIN);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
  ::errmsg("%s", e.message());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  ::errfinish(__FILE__, __LINE__, __func__);
  __builtin_unreachable();
}

template <typename T>
concept error_formattable =
    std::integral<std::decay_t<T>> ||
    (std::is_pointer_v<std::decay_t<T>> &&
     std::same_as<std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>, char>) ||
    std::is_pointer_v<std::decay_t<T>>;

template <std::size_t N, error_formattable... Args>
inline void report(int elevel, const char (&fmt)[N], Args... args) {
  ::errstart(elevel, TEXTDOMAIN);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
  ::errmsg(fmt, args...);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  ::errfinish(__FILE__, __LINE__, __func__);
  if (elevel >= ERROR) {
    __builtin_unreachable();
  }
}
} // namespace cppgres
/**
* \file
 */

namespace cppgres::utils::function_traits {
// Primary template (will be specialized below)
template <typename T> struct function_traits;

// Specialization for function pointers.
template <typename R, typename... Args> struct function_traits<R (*)(Args...)> {
  using argument_types = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

// Specialization for function references.
template <typename R, typename... Args> struct function_traits<R (&)(Args...)> {
  using argument_types = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

// Specialization for function types themselves.
template <typename R, typename... Args> struct function_traits<R(Args...)> {
  using argument_types = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

// Specialization for member function pointers (e.g. for lambdas' operator())
template <typename C, typename R, typename... Args>
struct function_traits<R (C:: *)(Args...) const> {
  using argument_types = std::tuple<Args...>;
  static constexpr std::size_t arity = sizeof...(Args);
};

// Fallback for functors/lambdas that are not plain function pointers.
// This will delegate to the member function pointer version.
template <typename T> struct function_traits : function_traits<decltype(&T::operator())> {};

// Primary template (left undefined)
template <typename Func, typename Tuple> struct invoke_result_from_tuple;

// Partial specialization for when Tuple is a std::tuple<Args...>
template <typename Func, typename... Args>
struct invoke_result_from_tuple<Func, std::tuple<Args...>> {
  using type = std::invoke_result_t<Func, Args...>;
};

// Convenience alias template.
template <typename Func, typename Tuple>
using invoke_result_from_tuple_t = typename invoke_result_from_tuple<Func, Tuple>::type;

} // namespace cppgres::utils::function_traits

namespace cppgres {

template <typename Func> struct ffi_guard {
  Func func;

  explicit ffi_guard(Func f) : func(std::move(f)) {}

  template <typename... Args>
  auto operator()(Args &&...args) -> decltype(func(std::forward<Args>(args)...)) {
    int state;
    sigjmp_buf *pbuf;
    ::ErrorContextCallback *cb;
    sigjmp_buf buf;
    ::MemoryContext mcxt = ::CurrentMemoryContext;

    pbuf = ::PG_exception_stack;
    cb = ::error_context_stack;
    ::PG_exception_stack = &buf;

    // restore state upon exit
    std::shared_ptr<void> defer(nullptr, [&](...) {
      ::error_context_stack = cb;
      ::PG_exception_stack = pbuf;
    });

    state = sigsetjmp(buf, 1);

    if (state == 0) {
      return func(std::forward<Args>(args)...);
    } else if (state == 1) {
      throw pg_exception(mcxt);
    }
    __builtin_unreachable();
  }
};

/**
 * @brief Wraps a C++ function to catch exceptions and report them as Postgres errors
 *
 * It ensures that if the C++ exception throws an error, it'll be caught and transformed into
 * a Postgres error report.
 *
 * @note It will also handle Postgres errors caught during the call that were automatically transformed
 *       into @ref cppgres::pg_exception by @ref cppgres::ffi_guard and report them as errors.
 *
 * @tparam Func C++ function to call
 */
template <typename Func> struct exception_guard {
  Func func;

  explicit exception_guard(Func f) : func(std::move(f)) {}

  template <typename... Args>
  auto operator()(Args &&...args) -> decltype(func(std::forward<Args>(args)...)) {
    try {
      return func(std::forward<Args>(args)...);
    } catch (const pg_exception &e) {
      error(e);
    } catch (const std::exception &e) {
      report(ERROR, "exception: %s", e.what());
    } catch (...) {
      report(ERROR, "some exception occurred");
    }
    __builtin_unreachable();
  }
};

} // namespace cppgres

namespace cppgres {

struct abstract_memory_context {

  template <typename T = std::byte> T *alloc(size_t n = 1) {
    return static_cast<T *>(ffi_guard{::MemoryContextAlloc}(_memory_context(), sizeof(T) * n));
  }
  template <typename T = void> void free(T *ptr) { ffi_guard{::pfree}(ptr); }

  void reset() { ffi_guard{::MemoryContextReset}(_memory_context()); }

  bool operator==(abstract_memory_context &c) noexcept {
    return _memory_context() == c._memory_context();
  }
  bool operator!=(abstract_memory_context &c) noexcept {
    return _memory_context() != c._memory_context();
  }

  operator ::MemoryContext() { return _memory_context(); }

  ::MemoryContextCallback *register_reset_callback(::MemoryContextCallbackFunction func,
                                                   void *arg) {
    auto cb = alloc<::MemoryContextCallback>(sizeof(::MemoryContextCallback));
    cb->func = func;
    cb->arg = arg;
    ffi_guard{::MemoryContextRegisterResetCallback}(_memory_context(), cb);
    return cb;
  }

  void delete_context() { ffi_guard{::MemoryContextDelete}(_memory_context()); }

protected:
  virtual ::MemoryContext _memory_context() = 0;
};

struct owned_memory_context : public abstract_memory_context {
  friend struct memory_context;

protected:
  owned_memory_context(::MemoryContext context) : context(context), moved(false) {}

  ~owned_memory_context() {
    if (!moved) {
      delete_context();
    }
  }

  ::MemoryContext context;
  bool moved;

  ::MemoryContext _memory_context() override { return context; }
};

struct memory_context : public abstract_memory_context {

  friend struct owned_memory_context;

  explicit memory_context() : context(::CurrentMemoryContext) {}
  explicit memory_context(::MemoryContext context) : context(context) {}
  explicit memory_context(abstract_memory_context &&context) : context(context) {}

  explicit memory_context(owned_memory_context &&ctx) : context(ctx) { ctx.moved = true; }

  static memory_context for_pointer(void *ptr) {
    if (ptr == nullptr || ptr != (void *)MAXALIGN(ptr)) {
      throw std::runtime_error("invalid pointer");
    }
    return memory_context(ffi_guard{::GetMemoryChunkContext}(ptr));
  }

  template <typename C> requires std::derived_from<C, abstract_memory_context>
  friend struct tracking_memory_context;

protected:
  ::MemoryContext context;

  ::MemoryContext _memory_context() noexcept override { return context; }
};

struct always_current_memory_context : public abstract_memory_context {
  always_current_memory_context() = default;

protected:
  ::MemoryContext _memory_context() override { return ::CurrentMemoryContext; }
};

struct alloc_set_memory_context : public owned_memory_context {
  using owned_memory_context::owned_memory_context;
  alloc_set_memory_context()
      : owned_memory_context(ffi_guard{::AllocSetContextCreateInternal}(
            ::CurrentMemoryContext, nullptr, ALLOCSET_DEFAULT_SIZES)) {}
  alloc_set_memory_context(memory_context &ctx)
      : owned_memory_context(
            ffi_guard{::AllocSetContextCreateInternal}(ctx, nullptr, ALLOCSET_DEFAULT_SIZES)) {}

  alloc_set_memory_context(memory_context &&ctx)
      : owned_memory_context(
            ffi_guard{::AllocSetContextCreateInternal}(ctx, nullptr, ALLOCSET_DEFAULT_SIZES)) {}
};

inline memory_context top_memory_context() { return memory_context(TopMemoryContext); };

template <typename C> requires std::derived_from<C, abstract_memory_context>
struct tracking_memory_context : public abstract_memory_context {
  explicit tracking_memory_context(tracking_memory_context<C> const &context)
      : ctx(context.ctx), counter(context.counter), cb(context.cb) {
    cb->arg = this;
  }

  explicit tracking_memory_context(C ctx)
      : ctx(ctx), counter(0),
        cb(std::shared_ptr<::MemoryContextCallback>(
            this->register_reset_callback(
                [](void *i) { static_cast<struct tracking_memory_context<C> *>(i)->counter++; },
                this),
            /* custom deleter */
            [](auto) {})) {}

  tracking_memory_context(tracking_memory_context &&other) noexcept
      : ctx(std::move(other.ctx)), counter(std::move(other.counter)), cb(std::move(other.cb)) {
    other.cb = nullptr;
    cb->arg = this;
  }

  tracking_memory_context(tracking_memory_context &other) noexcept
      : ctx(std::move(other.ctx)), counter(std::move(other.counter)), cb(std::move(other.cb)) {
    cb->arg = this;
    other.cb = nullptr;
  }

  tracking_memory_context &operator=(tracking_memory_context &&other) noexcept {
    ctx = other.ctx;
    cb = other.cb;
    counter = other.counter;
    cb->arg = this;
    return *this;
  }

  ~tracking_memory_context() {
    if (cb != nullptr) {
      if (cb.use_count() == 1) {
        cb->func = [](void *) {};
      }
    }
  }

  uint64_t resets() const { return counter; }
  C &get_memory_context() { return ctx; }

private:
  template <typename T> requires std::integral<T>
  struct shared_counter {
    T value;
    constexpr explicit shared_counter(T init = 0) noexcept : value(init) {}

    shared_counter &operator=(T v) noexcept {
      value = v;
      return *this;
    }

    shared_counter &operator++() noexcept {
      ++value;
      return *this;
    }

    T operator++(int) noexcept {
      T old = value;
      ++value;
      return old;
    }

    constexpr operator T() const noexcept { return value; }
  };
  C ctx;
  shared_counter<uint64_t> counter;
  std::shared_ptr<::MemoryContextCallback> cb;

protected:
  ::MemoryContext _memory_context() override { return ctx._memory_context(); }
};

template <typename T>
concept a_memory_context =
    std::derived_from<T, abstract_memory_context> && std::default_initializable<T>;

template <a_memory_context Context> struct memory_context_scope {
  explicit memory_context_scope(Context &ctx)
      : previous(::CurrentMemoryContext), ctx(ctx.operator ::MemoryContext()) {
    ::CurrentMemoryContext = ctx;
  }
  explicit memory_context_scope(Context &&ctx)
      : previous(::CurrentMemoryContext), ctx(ctx.operator ::MemoryContext()) {
    ::CurrentMemoryContext = ctx;
  }

  ~memory_context_scope() { ::CurrentMemoryContext = previous; }

private:
  ::MemoryContext previous;
  ::MemoryContext ctx;
};

template <class T, a_memory_context Context = memory_context> struct memory_context_allocator {
  using value_type = T;
  memory_context_allocator() noexcept : context(Context()), explicit_deallocation(false) {}
  memory_context_allocator(Context &&ctx, bool explicit_deallocation) noexcept
      : context(std::move(ctx)), explicit_deallocation(explicit_deallocation) {}

  constexpr memory_context_allocator(const memory_context_allocator<T> &c) noexcept
      : context(c.context) {}

  [[nodiscard]] T *allocate(std::size_t n) {
    try {
      return context.template alloc<T>(n);
    } catch (pg_exception &e) {
      throw std::bad_alloc();
    }
  }

  void deallocate(T *p, std::size_t n) noexcept {
    if (explicit_deallocation || context == top_memory_context()) {
      context.free(p);
    }
  }

  bool operator==(const memory_context_allocator &c) { return context == c.context; }
  bool operator!=(const memory_context_allocator &c) { return context != c.context; }

  Context &memory_context() { return context; }

private:
  Context context;
  bool explicit_deallocation;
};

struct pointer_gone_exception : public std::exception {
  const char *what() const noexcept override {
    return "pointer belongs to a MemoryContext that has been reset or deleted";
  }
};

} // namespace cppgres

#include <cstdint>
#include <optional>
#include <string>

namespace cppgres {

struct oid {
  oid(::Oid oid) : oid_(oid) {}
  oid(oid &oid) : oid_(oid.oid_) {}

  bool operator==(const oid &rhs) const { return oid_ == rhs.oid_; }
  bool operator!=(const oid &rhs) const { return !(rhs == *this); }

  bool operator==(const ::Oid &rhs) const { return oid_ == rhs; }
  bool operator!=(const ::Oid &rhs) const { return oid_ != rhs; }

  operator ::Oid() const { return oid_; }
  operator ::Oid &() { return oid_; }

private:
  ::Oid oid_;
};
static_assert(sizeof(::Oid) == sizeof(oid));

struct datum {
  template <typename T, typename> friend struct datum_conversion;

  operator const ::Datum &() const { return _datum; }

  operator Pointer() const { return reinterpret_cast<Pointer>(_datum); }

  datum() : _datum(0) {}
  explicit datum(::Datum datum) : _datum(datum) {}

  bool operator==(const datum &other) const { return _datum == other._datum; }

private:
  ::Datum _datum;
  friend struct nullable_datum;
};

class null_datum_exception : public std::exception {
  const char *what() const noexcept override { return "passed datum is null"; }
};

struct nullable_datum {

  bool is_null() const noexcept { return _ndatum.isnull; }

  operator struct datum &() {
    if (_ndatum.isnull) {
      throw null_datum_exception();
    }
    return _datum;
  }

  operator const ::Datum &() {
    if (_ndatum.isnull) {
      throw null_datum_exception();
    }
    return _datum.operator const ::Datum &();
  }

  operator const struct datum &() const {
    if (_ndatum.isnull) {
      throw null_datum_exception();
    }
    return _datum;
  }

  explicit nullable_datum(::NullableDatum d) : _ndatum(d) {}

  explicit nullable_datum() : _ndatum({.isnull = true}) {}

  explicit nullable_datum(::Datum d) : _ndatum({.value = d, .isnull = false}) {}
  explicit nullable_datum(datum d) : _ndatum({.value = d._datum, .isnull = false}) {}

  bool operator==(const nullable_datum &other) const {
    if (is_null()) {
      return other.is_null();
    }
    if (other.is_null()) {
      return is_null();
    }
    return _datum.operator==(other._datum);
  }

private:
  union {
    ::NullableDatum _ndatum;
    datum _datum;
  };
};

/**
 * @brief A trait to convert from and into a @ref cppgres::datum
 *
 * @tparam T C++ type to convert into and from
 */
template <typename T, typename = void> struct datum_conversion {

  /**
   * @brief Convert from a nullable datum
   *
   * Gets an optional @ref cppgres::memory_context when available to be able to determine the source
   * of the (pointer) datum.
   */
  static T from_nullable_datum(const nullable_datum &d, const oid oid,
                               std::optional<memory_context> context = std::nullopt) = delete;

  /**
   * @brief Convert from a datum
   *
   * Gets an optional @ref cppgres::memory_context when available to be able to determine the source
   * of the (pointer) datum.
   */
  static T from_datum(const datum &, const oid,
                      std::optional<memory_context> context = std::nullopt) = delete;
  /**
   * @brief Convert datum into a type
   *
   * Unlike @ref from_datum, gets no memory context.
   */
  static datum into_datum(const T &d) = delete;

  /**
   * @brief Convert into a nullable datum
   */
  static nullable_datum into_nullable_datum(const T &d) = delete;
};

template <typename T, typename R = T> struct default_datum_conversion {
  /**
   * @brief Convert from a nullable datum
   *
   * Gets an optional @ref cppgres::memory_context when available to be able to determine the source
   * of the (pointer) datum.
   */
  static R from_nullable_datum(const nullable_datum &d, const oid oid,
                               std::optional<memory_context> context = std::nullopt) {
    if (d.is_null()) {
      throw std::runtime_error(cppgres::fmt::format("datum is null and can't be coerced into {}",
                                                    utils::type_name<T>()));
    }
    return datum_conversion<T>::from_datum(d, oid, context);
  }

  /**
   * @brief Convert into a nullable datum
   */
  static nullable_datum into_nullable_datum(const T &d) {
    return nullable_datum(datum_conversion<T>::into_datum(d));
  }
};

template <typename T>
concept convertible_into_datum = requires(T t) {
  { datum_conversion<T, void>::into_datum(std::declval<T>()) } -> std::same_as<datum>;
};

template <typename T>
concept convertible_from_datum = requires(datum d, oid oid, std::optional<memory_context> context) {
  { datum_conversion<T, void>::from_datum(d, oid, context) } -> std::same_as<T>;
};

template <typename T> struct unsupported_type {};

template <typename T>
requires convertible_from_datum<std::remove_cv_t<T>>
T from_nullable_datum(const nullable_datum &d, const oid oid,
                      std::optional<memory_context> context = std::nullopt) {
  return datum_conversion<std::remove_cv_t<T>>::from_nullable_datum(d, oid, context);
}

template <typename T> nullable_datum into_nullable_datum(const std::optional<T> &v) {
  if (v.has_value()) {
    return nullable_datum(datum_conversion<T>::into_datum(v.value()));
  } else {
    return nullable_datum();
  }
}

template <typename T> nullable_datum into_nullable_datum(const T &v) {
  if constexpr (std::same_as<nullable_datum, T>) {
    return v;
  }
  return datum_conversion<T>::into_nullable_datum(v);
}

template <typename T>
concept convertible_from_nullable_datum = requires {
  {
    cppgres::from_nullable_datum<T>(std::declval<nullable_datum>(), std::declval<oid>(),
                                    std::declval<std::optional<memory_context>>())
  } -> std::same_as<T>;
};

template <typename T>
concept convertible_into_nullable_datum = requires {
  { cppgres::into_nullable_datum(std::declval<T>()) } -> std::same_as<nullable_datum>;
};

template <typename T> struct all_from_nullable_datum {
private:
  static constexpr std::size_t N = utils::tuple_size_v<T>;

  template <std::size_t... I> static constexpr bool impl(std::index_sequence<I...>) {
    return (
        ... &&
        convertible_from_nullable_datum<utils::remove_optional_t<utils::tuple_element_t<I, T>>>);
  }

public:
  static constexpr bool value = impl(std::make_index_sequence<N>{});
};

} // namespace cppgres



namespace cppgres {

struct name {

  template <int N> requires(N < NAMEDATALEN)
  name(const char name[N]) : _name({.data = name}) {}

  name(const char *name) { strncpy(NameStr(_name), name, NAMEDATALEN - 1); }

  operator NameData &() const { return _name; }

private:
  mutable NameData _name;
};

} // namespace cppgres


/**
 * \file
 */

#include <stack>

extern "C" {
#include <access/xact.h>
}

namespace cppgres {

using command_id = ::CommandId;

struct transaction_id {

  transaction_id() : id_(InvalidTransactionId) {}
  transaction_id(::TransactionId id) : id_(id) {}
  transaction_id(const transaction_id &id) : id_(id.id_) {}

  static transaction_id current(bool acquire = true) {
    return transaction_id(
        ffi_guard{acquire ? ::GetCurrentTransactionId : ::GetCurrentTransactionIdIfAny}());
  }

  bool is_valid() const { return TransactionIdIsValid(id_); }

  bool operator==(const transaction_id &other) const { return TransactionIdEquals(id_, other.id_); }
  bool operator>(const transaction_id &other) const { return TransactionIdFollows(id_, other.id_); }
  bool operator>=(const transaction_id &other) const {
    return TransactionIdFollowsOrEquals(id_, other.id_);
  }
  bool operator<(const transaction_id &other) const {
    return TransactionIdPrecedes(id_, other.id_);
  }
  bool operator<=(const transaction_id &other) const {
    return TransactionIdPrecedesOrEquals(id_, other.id_);
  }

  bool did_abort() const { return is_valid() && TransactionIdDidAbort(id_); }
  bool did_commit() const { return is_valid() && TransactionIdDidCommit(id_); }

private:
  ::TransactionId id_;
};

static_assert(sizeof(transaction_id) == sizeof(::TransactionId));

struct internal_subtransaction {
  internal_subtransaction(bool commit = true)
      : owner(::CurrentResourceOwner), commit(commit), name("") {
    if (txns.empty()) {
      ffi_guard{::BeginInternalSubTransaction}(nullptr);
      txns.push(this);
    } else {
      throw std::runtime_error("internal subtransaction already started");
    }
  }

  internal_subtransaction(std::string_view name, bool commit = true)
      : owner(::CurrentResourceOwner), commit(commit), name(name) {
    if (txns.empty()) {
      ffi_guard{::BeginInternalSubTransaction}(this->name.c_str());
      txns.push(this);
    } else {
      throw std::runtime_error("internal subtransaction already started");
    }
  }

  ~internal_subtransaction() {
    txns.pop();
    if (commit) {
      ffi_guard{::ReleaseCurrentSubTransaction}();
    } else {
      ffi_guard{::RollbackAndReleaseCurrentSubTransaction}();
    }
    ::CurrentResourceOwner = owner;
  }

private:
  ::ResourceOwner owner;
  bool commit;
  std::string name;
  static inline std::stack<internal_subtransaction *> txns;
};

struct transaction {
  transaction(bool commit = true) : should_commit(commit), released(false) {
    ffi_guard([]() {
      if (!::IsTransactionState()) {
        ::SetCurrentStatementStartTimestamp();
        ::StartTransactionCommand();
        ::PushActiveSnapshot(::GetTransactionSnapshot());
      }
    })();
  }

  ~transaction() {
    if (!released) {
      ffi_guard([this]() {
        ::PopActiveSnapshot();
        if (should_commit) {
          ::CommitTransactionCommand();
        } else {
          ::AbortCurrentTransaction();
        }
      })();
    }
  }

  void commit() {
    ffi_guard([]() {
      ::PopActiveSnapshot();
      ::CommitTransactionCommand();
    })();
    released = true;
  }

  void rollback() {
    ffi_guard([]() {
      ::PopActiveSnapshot();
      ::AbortCurrentTransaction();
    })();
    released = true;
  }

private:
  bool should_commit;
  bool released;
};

} // namespace cppgres

namespace cppgres {

/**
 * @brief Heap tuple convenience wrapper
 */
struct heap_tuple {

  heap_tuple(::HeapTuple tuple) : tuple_(tuple) {}

  operator ::HeapTuple() const { return tuple_; }

  transaction_id xmin(bool raw = false) const {
    return raw ? HeapTupleHeaderGetRawXmin(tuple_->t_data) : HeapTupleHeaderGetXmin(tuple_->t_data);
  }

  transaction_id xmax() const { return HeapTupleHeaderGetRawXmax(tuple_->t_data); }

  transaction_id update_xid() const { return HeapTupleHeaderGetUpdateXid(tuple_->t_data); }

  command_id cmin() const { return HeapTupleHeaderGetCmin(tuple_->t_data); }
  command_id cmax() const { return HeapTupleHeaderGetCmax(tuple_->t_data); }

private:
  HeapTuple tuple_;
};

static_assert(sizeof(heap_tuple) == sizeof(::HeapTuple));

} // namespace cppgres

namespace cppgres {

template <typename T> struct syscache_traits {};

template <> struct syscache_traits<Form_pg_type> {
  static constexpr ::SysCacheIdentifier cache_id = TYPEOID;
};

template <> struct syscache_traits<Form_pg_proc> {
  static constexpr ::SysCacheIdentifier cache_id = PROCOID;
};

template <typename T>
concept syscached = requires(T t) {
  { *t };
  { syscache_traits<T>::cache_id } -> std::same_as<const ::SysCacheIdentifier &>;
};

template <syscached T, convertible_into_datum... D> struct syscache {
  syscache(const D &...key) : syscache(syscache_traits<T>::cache_id, key...) {}
  syscache(::SysCacheIdentifier cache_id, const D &...key)
      requires(sizeof...(key) > 0 && sizeof...(key) < 5)
      : cache_id(cache_id), tuple([&]() {
          datum keys[4] = {datum_conversion<D>::into_datum(key)...};
          return ffi_guard{::SearchSysCache}(cache_id, keys[0], keys[1], keys[2], keys[3]);
        }()) {
    if (!HeapTupleIsValid(tuple)) {
      throw std::runtime_error("invalid tuple");
    }
  }

  ~syscache() { ReleaseSysCache(tuple); }

  decltype(*std::declval<T>()) &operator*() {
    return *reinterpret_cast<T>(GETSTRUCT(tuple.operator HeapTuple()));
  }
  const decltype(*std::declval<T>()) &operator*() const {
    return *reinterpret_cast<T>(GETSTRUCT(tuple.operator HeapTuple()));
  }

  /**
   * @brief Get an attribute by index
   *
   * @tparam V type to convert to
   * @param attr attribute index
   * @return
   */
  template <convertible_from_datum V> std::optional<V> get_attribute(int attr) {
    bool isnull;
    Datum ret = ffi_guard{::SysCacheGetAttr}(cache_id, tuple, attr, &isnull);
    if (isnull) {
      return std::nullopt;
    }
    return from_nullable_datum<V>(nullable_datum(ret), oid(/*FIXME*/ InvalidOid));
  }

  operator const heap_tuple &() const { return tuple; }

private:
  ::SysCacheIdentifier cache_id;
  heap_tuple tuple;
};

} // namespace cppgres
/**
* \file
 */

#include <cstddef>
#include <span>
#include <string>


namespace cppgres {

/**
 * @brief Postgres type
 */
struct type {
  ::Oid oid;

  /**
   * @brief Type name as defined in Postgres
   *
   * @param qualified if set to true (false by default), it will always include the schema name.
   *                  Otherwise, if the schema is in the `search_path`, the schema will not be
   *                  included.
   */
  std::string_view name(bool qualified = false) {
    if (!OidIsValid(oid)) {
      throw std::runtime_error("invalid type");
    }
    return (qualified ? ffi_guard{::format_type_be_qualified}
                      : ffi_guard{::format_type_be})(oid);
  }

  bool operator==(const type &other) const { return oid == other.oid; }
};

template <typename T, typename = void> struct type_traits {
  type_traits() {}
  type_traits(const T &) {}
  bool is(const type &t) { return false; }
  type type_for() = delete;
};

template <typename T> requires std::is_reference_v<T>
struct type_traits<T> {
  type_traits() {}
  type_traits(const T &) {}
  constexpr type type_for() { return type_traits<std::remove_reference_t<T>>().type_for(); }
};

template <typename T> struct type_traits<std::optional<T>> {
  type_traits() {}
  type_traits(const std::optional<T> &) {}
  bool is(const type &t) { return type_traits<T>().is(t); }
  constexpr type type_for() { return type_traits<T>().type_for(); }
};

struct non_by_value_type : public type {
  friend struct datum;

  non_by_value_type(std::pair<const struct datum &, std::optional<memory_context>> init)
      : non_by_value_type(init.first, init.second) {}
  non_by_value_type(const struct datum &datum, std::optional<memory_context> ctx)
      : value_datum(datum),
        ctx(tracking_memory_context(ctx.has_value() ? *ctx : top_memory_context())) {}

  non_by_value_type(const non_by_value_type &other)
      : value_datum(other.value_datum), ctx(other.ctx) {}
  non_by_value_type(non_by_value_type &&other) noexcept
      : value_datum(std::move(other.value_datum)), ctx(std::move(other.ctx)) {}
  non_by_value_type &operator=(non_by_value_type &&other) noexcept {
    value_datum = std::move(other.value_datum);
    ctx = std::move(other.ctx);
    return *this;
  }

  memory_context &get_memory_context() { return ctx.get_memory_context(); }

  datum get_datum() const { return value_datum; }

protected:
  datum value_datum;
  tracking_memory_context<memory_context> ctx;
  void *ptr(bool tracked = true) const {
    if (tracked && ctx.resets() > 0) {
      throw pointer_gone_exception();
    }
    return reinterpret_cast<void *>(value_datum.operator const ::Datum &());
  }
};

static_assert(std::copy_constructible<non_by_value_type>);

struct varlena : public non_by_value_type {
  using non_by_value_type::non_by_value_type;

  operator void *() { return VARDATA_ANY(detoasted_ptr()); }

  datum get_datum() const { return value_datum; }

  bool is_detoasted() const { return detoasted != nullptr; }

protected:
  void *detoasted = nullptr;
  void *detoasted_ptr() {
    if (detoasted != nullptr) {
      return detoasted;
    }
    detoasted = ffi_guard{::pg_detoast_datum}(reinterpret_cast<::varlena *>(ptr()));
    return detoasted;
  }
};

struct text : public varlena {
  using varlena::varlena;

  operator std::string_view() {
    return {static_cast<char *>(this->operator void *()), VARSIZE_ANY_EXHDR(this->detoasted_ptr())};
  }
};

using byte_array = std::span<const std::byte>;

struct bytea : public varlena {
  using varlena::varlena;

  bytea(const byte_array &ba, memory_context ctx)
      : varlena(([&]() {
                  auto alloc = ctx.alloc<std::byte>(VARHDRSZ + ba.size_bytes());
                  SET_VARSIZE(alloc, VARHDRSZ + ba.size_bytes());
                  auto ptr = VARDATA_ANY(alloc);
                  std::copy(ba.begin(), ba.end(), reinterpret_cast<std::byte *>(ptr));
                  return datum(PointerGetDatum(alloc));
                })(),
                ctx) {}

  operator byte_array() {
    return {reinterpret_cast<std::byte *>(this->operator void *()),
            VARSIZE_ANY_EXHDR(this->detoasted_ptr())};
  }
};

template <typename T>
concept flattenable = requires(T t, std::span<std::byte> span) {
  { T::type() } -> std::same_as<type>;
  { t.flat_size() } -> std::same_as<std::size_t>;
  { t.flatten_into(span) };
  { T::restore_from(span) } -> std::same_as<T>;
};

template <flattenable T> struct expanded_varlena : public varlena {
  using flattenable_type = T;
  using varlena::varlena;

  expanded_varlena()
      : varlena(([]() {
          auto ctx = memory_context(std::move(alloc_set_memory_context()));
          auto *e = new (ctx.alloc<expanded>()) expanded(T());
          ctx.register_reset_callback(
              [](void *arg) {
                auto v = reinterpret_cast<expanded *>(arg);
                v->inner.~T();
              },
              e);
          init(&e->hdr, ctx);
          return std::make_pair(datum(PointerGetDatum(e)), ctx);
        })()),
        detoasted_value(reinterpret_cast<expanded *>(DatumGetPointer(value_datum))) {}

  operator T &() {
    if (detoasted_value.has_value()) {
      return detoasted_value.value()->inner;
    } else {
      auto *ptr1 = reinterpret_cast<std::byte *>(varlena::operator void *());
      auto ctx = memory_context(std::move(alloc_set_memory_context()));
      auto *value = new (ctx.alloc<expanded>())
          expanded(T::restore_from(std::span(ptr1, VARSIZE_ANY_EXHDR(detoasted_ptr()))));
      ctx.register_reset_callback(
          [](void *arg) {
            auto v = reinterpret_cast<expanded *>(arg);
            v->inner.~T();
          },
          value);
      init(&value->hdr, ctx);
      detoasted_value = value;
      return value->inner;
    }
  }

  datum get_expanded_datum() const {
    if (!detoasted_value.has_value()) {
      throw std::runtime_error("hasn't been expanded yet");
    }
    return datum(EOHPGetRWDatum(&detoasted_value.value()->hdr));
  }

private:
  struct expanded {
    expanded(T &&t) : inner(std::move(t)) {}
    ::ExpandedObjectHeader hdr;
    T inner;
  };
  std::optional<expanded *> detoasted_value = std::nullopt;

  static void init(ExpandedObjectHeader *hdr, memory_context &ctx) {
    using header = int32_t;

    static const ::ExpandedObjectMethods eom = {
        .get_flat_size =
            [](ExpandedObjectHeader *eohptr) {
              auto *e = reinterpret_cast<expanded *>(eohptr);
              T *inner = &e->inner;
              return inner->flat_size() + sizeof(header);
            },
        .flatten_into =
            [](ExpandedObjectHeader *eohptr, void *result, size_t allocated_size) {
              auto *e = reinterpret_cast<expanded *>(eohptr);
              T *inner = &e->inner;
              SET_VARSIZE(reinterpret_cast<header *>(result), allocated_size);
              auto bytes = reinterpret_cast<std::byte *>(result) + sizeof(header);
              std::span buffer(bytes, allocated_size - sizeof(header));
              inner->flatten_into(buffer);
            }};

    ffi_guard{::EOH_init_header}(hdr, &eom, ctx);
  }
};

template <typename T>
concept expanded_varlena_type = requires { typename T::flattenable_type; };

template <typename T>
concept has_a_type = requires(type_traits<T> t) {
  { t.type_for() } -> std::same_as<type>;
};

} // namespace cppgres
/**
 * \file
 */

#include <optional>
#include <span>
#include <string>
#include <utility>


namespace cppgres {

template <> struct type_traits<void *> {
  bool is(const type &t) { return t.oid == INTERNALOID; }
  constexpr type type_for() { return type{.oid = INTERNALOID}; }
};

template <> struct type_traits<void> {
  bool is(const type &t) { return t.oid == VOIDOID; }
  constexpr type type_for() { return type{.oid = VOIDOID}; }
};

template <> struct type_traits<oid> {
  static bool is(const type &t) { return t.oid == OIDOID; }
  static constexpr type type_for() { return type{.oid = OIDOID}; }
};

template <> struct type_traits<nullable_datum> {
  static bool is(const type &t) { return true; }
  static constexpr type type_for() { return type{.oid = ANYOID}; }
};

template <> struct type_traits<datum> {
  static bool is(const type &t) { return true; }
  static constexpr type type_for() { return type{.oid = ANYOID}; }
};

template <typename S> struct type_traits<S, std::enable_if_t<utils::is_std_tuple<S>::value>> {
  bool is(const type &t) {
    if (t.oid == RECORDOID) {
      return true;
    } else if constexpr (std::tuple_size_v<S> == 1) {
      // special case when we have a tuple of 1 matching the type
      return type_traits<std::tuple_element_t<0, S>>().is(t);
    }
    return false;
  }
  constexpr type type_for() { return type{.oid = RECORDOID}; }
};

template <> struct type_traits<bool> {
  type_traits() {}
  type_traits(const bool &) {}
  bool is(const type &t) { return t.oid == BOOLOID; }
  constexpr type type_for() { return type{.oid = BOOLOID}; }
};

template <> struct type_traits<int64_t> {
  type_traits() {}
  type_traits(const int64_t &) {}
  bool is(const type &t) { return t.oid == INT8OID || t.oid == INT4OID || t.oid == INT2OID; }
  constexpr type type_for() { return type{.oid = INT8OID}; }
};

template <> struct type_traits<int32_t> {
  type_traits() {}
  type_traits(const int32_t &) {}
  bool is(const type &t) { return t.oid == INT4OID || t.oid == INT2OID; }
  constexpr type type_for() { return type{.oid = INT4OID}; }
};

template <> struct type_traits<int16_t> {
  type_traits() {}
  type_traits(const int16_t &) {}
  bool is(const type &t) { return t.oid == INT2OID; }
  constexpr type type_for() { return type{.oid = INT2OID}; }
};

template <> struct type_traits<int8_t> {
  type_traits() {}
  type_traits(const int8_t &) {}
  bool is(const type &t) { return t.oid == INT2OID; }
  constexpr type type_for() { return type{.oid = INT2OID}; }
};

template <> struct type_traits<double> {
  type_traits() {}
  type_traits(const double &) {}
  bool is(const type &t) { return t.oid == FLOAT8OID || t.oid == FLOAT4OID; }
  constexpr type type_for() { return type{.oid = FLOAT8OID}; }
};

template <> struct type_traits<float> {
  type_traits() {}
  type_traits(const float &) {}
  bool is(const type &t) { return t.oid == FLOAT4OID; }
  constexpr type type_for() { return type{.oid = FLOAT4OID}; }
};

template <> struct type_traits<text> {
  type_traits() {}
  type_traits(const text &) {}
  bool is(const type &t) { return t.oid == TEXTOID; }
  constexpr type type_for() { return type{.oid = TEXTOID}; }
};

template <> struct type_traits<std::string_view> {
  type_traits() {}
  type_traits(const std::string_view &) {}
  bool is(const type &t) { return t.oid == TEXTOID; }
  constexpr type type_for() { return type{.oid = TEXTOID}; }
};

template <> struct type_traits<std::string> {
  type_traits() {}
  type_traits(const std::string &) {}
  bool is(const type &t) { return t.oid == TEXTOID; }
  constexpr type type_for() { return type{.oid = TEXTOID}; }
};

template <> struct type_traits<byte_array> {
  type_traits() {}
  type_traits(const byte_array &) {}
  bool is(const type &t) { return t.oid == BYTEAOID; }
  constexpr type type_for() { return type{.oid = BYTEAOID}; }
};

template <> struct type_traits<bytea> {
  type_traits() {}
  type_traits(const bytea &) {}
  bool is(const type &t) { return t.oid == BYTEAOID; }
  constexpr type type_for() { return type{.oid = BYTEAOID}; }
};

template <> struct type_traits<char *> {
  type_traits() {}
  type_traits(const char *&) {}
  bool is(const type &t) { return t.oid == CSTRINGOID; }
  constexpr type type_for() { return type{.oid = CSTRINGOID}; }
};

template <> struct type_traits<const char *> {
  type_traits() {}
  type_traits(const char *&) {}
  bool is(const type &t) { return t.oid == CSTRINGOID; }
  constexpr type type_for() { return type{.oid = CSTRINGOID}; }
};

template <std::size_t N> struct type_traits<const char[N]> {
  type_traits() {}
  type_traits(const char (&)[N]) {}
  bool is(const type &t) { return t.oid == CSTRINGOID; }
  constexpr type type_for() { return type{.oid = CSTRINGOID}; }
};

template <flattenable F> struct type_traits<expanded_varlena<F>> {
  type_traits() {}
  type_traits(const expanded_varlena<F> &) {}
  bool is(const type &t) { return t.oid == F::type().oid; }
  constexpr type type_for() { return F::type(); }
};

template <> struct datum_conversion<datum> : default_datum_conversion<datum> {
  static datum from_datum(const datum &d, oid, std::optional<memory_context>) { return d; }

  static datum into_datum(const datum &t) { return t; }
};

template <> struct datum_conversion<nullable_datum> : default_datum_conversion<nullable_datum> {
  static nullable_datum from_datum(const datum &d, oid, std::optional<memory_context>) {
    return nullable_datum(d);
  }

  static datum into_datum(const nullable_datum &t) { return t.is_null() ? datum(0) : t; }
};

template <> struct datum_conversion<void *> : default_datum_conversion<void *> {
  static void *from_datum(const datum &d, oid, std::optional<memory_context>) {
    return reinterpret_cast<void *>(d.operator const ::Datum &());
  }

  static datum into_datum(const void *const &t) { return datum(reinterpret_cast<::Datum>(t)); }
};

template <> struct datum_conversion<oid> : default_datum_conversion<oid> {
  static oid from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<oid>(d.operator const ::Datum &());
  }

  static datum into_datum(const oid &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<size_t> : default_datum_conversion<size_t> {
  static size_t from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<size_t>(d.operator const ::Datum &());
  }

  static datum into_datum(const size_t &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<int64_t> : default_datum_conversion<int64_t> {
  static int64_t from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<int64_t>(d.operator const ::Datum &());
  }

  static datum into_datum(const int64_t &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<int32_t> : default_datum_conversion<int32_t> {
  static int32_t from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<int32_t>(d.operator const ::Datum &());
  }
  static datum into_datum(const int32_t &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<int16_t> : default_datum_conversion<int16_t> {
  static int16_t from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<int16_t>(d.operator const ::Datum &());
  }
  static datum into_datum(const int16_t &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<bool> : default_datum_conversion<bool> {
  static bool from_datum(const datum &d, oid, std::optional<memory_context>) {
    return static_cast<bool>(d.operator const ::Datum &());
  }
  static datum into_datum(const bool &t) { return datum(static_cast<::Datum>(t)); }
};

template <> struct datum_conversion<double> : default_datum_conversion<double> {
  static double from_datum(const datum &d, oid, std::optional<memory_context>) {
    return DatumGetFloat8(d.operator const ::Datum &());
  }

  static datum into_datum(const double &t) { return datum(Float8GetDatum(t)); }
};

template <> struct datum_conversion<float> : default_datum_conversion<float> {
  static float from_datum(const datum &d, oid, std::optional<memory_context>) {
    return DatumGetFloat4(d.operator const ::Datum &());
  }

  static datum into_datum(const float &t) { return datum(Float4GetDatum(t)); }
};

// Specializations for text and bytea:
template <> struct datum_conversion<text> : default_datum_conversion<text> {
  static text from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return text{d, ctx};
  }

  static datum into_datum(const text &t) { return t.get_datum(); }
};

template <> struct datum_conversion<bytea> : default_datum_conversion<bytea> {
  static bytea from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return bytea{d, ctx};
  }

  static datum into_datum(const bytea &t) { return t.get_datum(); }
};

template <> struct datum_conversion<byte_array> : default_datum_conversion<byte_array> {
  static byte_array from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return bytea{d, ctx};
  }

  static datum into_datum(const byte_array &t) {
    // This is not perfect if the data was already allocated with Postgres
    // But once we're with the byte_array (std::span) we've lost this information
    // TODO: can we do any better here?
    return bytea(t, memory_context()).get_datum();
  }
};

// Specializations for std::string_view and std::string.
// Here we re-use the conversion for text.
template <> struct datum_conversion<std::string_view> : default_datum_conversion<std::string_view> {
  static std::string_view from_datum(const datum &d, oid oid, std::optional<memory_context> ctx) {
    return datum_conversion<text>::from_datum(d, oid, ctx);
  }

  static datum into_datum(const std::string_view &t) {
    size_t sz = t.size();
    void *result = ffi_guard{::palloc}(sz + VARHDRSZ);
    SET_VARSIZE(result, t.size() + VARHDRSZ);
    memcpy(VARDATA(result), t.data(), sz);
    return datum(reinterpret_cast<::Datum>(result));
  }
};

template <> struct datum_conversion<std::string> : default_datum_conversion<std::string> {
  static std::string from_datum(const datum &d, oid oid, std::optional<memory_context> ctx) {
    // Convert the text to a std::string_view then construct a std::string.
    return std::string(datum_conversion<text>::from_datum(d, oid, ctx).operator std::string_view());
  }

  static datum into_datum(const std::string &t) {
    return datum_conversion<std::string_view>::into_datum(t);
  }
};

template <> struct datum_conversion<const char *> : default_datum_conversion<const char *> {
  static const char *from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return DatumGetPointer(d);
  }

  static datum into_datum(const char *const &t) { return datum(PointerGetDatum(t)); }
};

template <std::size_t N>
struct datum_conversion<char[N]> : default_datum_conversion<char[N], const char *> {
  static const char *from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return DatumGetPointer(d);
  }

  static datum into_datum(const char (&t)[N]) { return datum(PointerGetDatum(t)); }
};

template <typename T>
struct datum_conversion<T, std::enable_if_t<expanded_varlena_type<T>>>
    : default_datum_conversion<T> {
  static T from_datum(const datum &d, oid, std::optional<memory_context> ctx) { return {d, ctx}; }

  static datum into_datum(const T &t) { return t.get_expanded_datum(); }
};

template <typename T> struct datum_conversion<T, std::enable_if_t<utils::is_optional<T>>> {

  static T from_nullable_datum(const nullable_datum &d, const oid oid,
                               std::optional<memory_context> context = std::nullopt) {
    if (d.is_null()) {
      return std::nullopt;
    }
    return from_datum(d, oid, context);
  }

  static T from_datum(const datum &d, oid oid, std::optional<memory_context> ctx) {
    return datum_conversion<utils::remove_optional_t<T>>::from_datum(d, oid, ctx);
  }

  static datum into_datum(const T &t) { return t.get_expanded_datum(); }
};

/**
 * @brief Type identified by its name
 *
 * @note Once constructed, the resolved type stays the same and doesn't change during
 *       the lifetime of the value.
 */
struct named_type : public type {
  /**
   * @brief Type identified by an unqualified name
   *
   * @param name unqualified type name
   */
  named_type(const std::string_view name) : type(type{.oid = ::TypenameGetTypid(name.data())}) {}
  /**
   * @brief Type identified by a qualified name
   *
   * @param schema schema name
   * @param name type name
   */
  named_type(const std::string_view schema, const std::string_view name)
      : type({.oid = ([&]() {
                cppgres::oid nsoid = ffi_guard{::LookupExplicitNamespace}(schema.data(), false);
                cppgres::oid oid = InvalidOid;
                if (OidIsValid(nsoid)) {
                  oid = (*syscache<Form_pg_type, const char *, cppgres::oid>(
                             TYPENAMENSP, std::string(name).c_str(), nsoid))
                            .oid;
                }
                return oid;
              })()}) {}
};

} // namespace cppgres

#include <ranges>

namespace cppgres {

/**
 * @brief Tuple descriptor operator
 *
 * Allows to create new or manipulate existing tuple descriptors
 */
struct tuple_descriptor {

  /**
   * @brief Create a tuple descriptor for a given number of attributes
   *
   * @param nattrs number of attributes
   * @param ctx memory context, current transaction context by default
   */
  tuple_descriptor(int nattrs, memory_context ctx = memory_context())
      : tupdesc(([&]() {
          memory_context_scope<memory_context> scope(ctx);
          auto res = ffi_guard{::CreateTemplateTupleDesc}(nattrs);
          return res;
        }())),
        blessed(false), owned(true) {
    for (int i = 0; i < nattrs; i++) {
      operator[](i).attcollation = InvalidOid;
      operator[](i).attisdropped = false;
    }
  }
  /**
   * @brief Create a tuple descriptor for a given `TupleDesc`
   *
   * @param tupdesc existing attribute
   * @param blessed true if already blessed (default)
   */
  tuple_descriptor(TupleDesc tupdesc, bool blessed = true)
      : tupdesc(tupdesc), blessed(blessed), owned(false) {}

  /**
   * @brief Copy constructor
   *
   * Creates a copy instance of the tuple descriptor in the current memory contet
   */
  tuple_descriptor(tuple_descriptor &other)
      : tupdesc(ffi_guard{::CreateTupleDescCopyConstr}(other.populate_compact_attribute())),
        blessed(other.blessed), owned(other.owned) {}

  /**
   * @brief Move constructor
   */
  tuple_descriptor(tuple_descriptor &&other) noexcept
      : tupdesc(other.tupdesc), blessed(other.blessed), owned(other.owned) {}

  /**
   * @brief Copy assignment
   *
   * Creates a copy instance of the tuple descriptor in the current memory contet
   */
  tuple_descriptor &operator=(const tuple_descriptor &other) {
    tupdesc = ffi_guard{::CreateTupleDescCopyConstr}(other.populate_compact_attribute());
    blessed = other.blessed;
    return *this;
  }

  /**
   * @brief Move assignment
   */
  tuple_descriptor &operator=(tuple_descriptor &&other) noexcept {
    if (this != &other) {
      tupdesc = other.tupdesc;
      blessed = other.blessed;
      owned = other.owned;
    }
    return *this;
  }

  /**
   * @brief Number of attributes
   */
  int attributes() const noexcept { return tupdesc->natts; }

  /**
   * @brief Get a reference to `Form_pg_attribute`
   *
   * @throws std::out_of_range when attribute index is out of range
   */
  ::FormData_pg_attribute &operator[](int n) const {
    check_bounds(n);
    return *TupleDescAttr(tupdesc, n);
  }

  type get_type(int n) const {
    auto &att = operator[](n);
    return {att.atttypid};
  }

  /**
   * @brief Set attribute type
   *
   * @param n Zero-based attribute index
   * @param type new attribute type
   *
   * @throws std::out_of_range when attribute index is out of range
   */
  void set_type(int n, const type &type) {
    check_not_blessed();
    syscache<Form_pg_type, oid> typ(type.oid);
    auto &att = operator[](n);
    att.atttypid = (*typ).oid;
    att.attcollation = (*typ).typcollation;
    att.attlen = (*typ).typlen;
    att.attstorage = (*typ).typstorage;
    att.attalign = (*typ).typalign;
    att.atttypmod = (*typ).typtypmod;
    att.attbyval = (*typ).typbyval;
  }

  std::string_view get_name(int n) {
    auto &att = operator[](n);
    return NameStr(att.attname);
  }

  /**
   * @brief Set attribute name
   *
   * @param n Zero-based attribute index
   * @param name new attribute name
   *
   * @throws std::out_of_range when attribute index is out of range
   */
  void set_name(int n, const name &name) {
    check_not_blessed();
    auto &att = operator[](n);
    att.attname = name;
  }

  /**
   * @brief returns a pointer to `TupleDesc`
   *
   * At this point, it'll be prepared and blessed.
   */
  operator TupleDesc() {
    populate_compact_attribute();
    if (!blessed) {
      tupdesc = ffi_guard{::BlessTupleDesc}(tupdesc);
      blessed = true;
    }
    return tupdesc;
  }

#if PG_MAJORVERSION_NUM > 16
  /**
   * @brief Determines whether two tuple descriptors have equal row types.
   *
   * This is used to check whether two record types are compatible, whether
   * function return row types are the same, and other similar situations.
   */
  bool equal_row_types(const tuple_descriptor &other) {
    return ffi_guard{::equalRowTypes}(tupdesc, other.tupdesc);
  }
#endif

  /**
   * @brief Determines whether two tuple descriptors have equal row types.
   *
   * This is used to check whether two record types are compatible, whether
   * function return row types are the same, and other similar situations.
   */
  bool equal_types(const tuple_descriptor &other) {
    if (tupdesc->natts != other.tupdesc->natts)
      return false;
    if (tupdesc->tdtypeid != other.tupdesc->tdtypeid)
      return false;

    for (int i = 0; i < other.attributes(); i++) {
      FormData_pg_attribute &att1 = operator[](i);
      FormData_pg_attribute &att2 = other[i];

      if (att1.atttypid != att2.atttypid || att1.atttypmod != att2.atttypmod ||
          att1.attcollation != att2.attcollation || att1.attisdropped != att2.attisdropped ||
          att1.attlen != att2.attlen || att1.attalign != att2.attalign) {
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Compare two TupleDesc structures for logical equality
   *
   * @note This includes checking attribute names.
   */
  bool operator==(const tuple_descriptor &other) {
    return ffi_guard{::equalTupleDescs}(tupdesc, other.tupdesc);
  }

  /**
   * @brief Returns true if the tuple descriptor is blessed
   */
  bool is_blessed() const noexcept { return blessed; }

  operator TupleDesc() const { return tupdesc; }

private:
  inline void check_bounds(int n) const {
    if (n + 1 > tupdesc->natts || n < 0) {
      throw std::out_of_range(cppgres::fmt::format(
          "attribute index {} is out of bounds for the tuple descriptor with the size of {}", n,
          tupdesc->natts));
    }
  }
  inline void check_not_blessed() const {
    if (blessed) {
      throw std::runtime_error("tuple_descriptor already blessed");
    }
  }

  TupleDesc populate_compact_attribute() const {
#if PG_MAJORVERSION_NUM >= 18
    for (int i = 0; i < tupdesc->natts; i++) {
      ffi_guard{::populate_compact_attribute}(tupdesc, i);
    }
#endif
    return tupdesc;
  }

  TupleDesc tupdesc;
  bool blessed;
  bool owned;
};

static_assert(std::copy_constructible<tuple_descriptor>);
static_assert(std::move_constructible<tuple_descriptor>);
static_assert(std::is_copy_assignable_v<tuple_descriptor>);

/**
 * @brief Runtime-typed value of `record` type
 *
 * These records don't have a structure known upfront, for example, when a value
 * of this type (`record`) are passed into a function.
 */
struct record {

  friend struct datum_conversion<record>;

  record(HeapTupleHeader heap_tuple, abstract_memory_context &ctx)
      : tupdesc(/* FIXME: can we use the non-copy version with refcounting? */ ffi_guard{
            ::lookup_rowtype_tupdesc_copy}(HeapTupleHeaderGetTypeId(heap_tuple),
                                           HeapTupleHeaderGetTypMod(heap_tuple))),
        tuple(ctx.template alloc<HeapTupleData>()) {
#if PG_MAJORVERSION_NUM < 18
    tuple->t_len = HeapTupleHeaderGetDatumLength(tupdesc.operator TupleDesc());
#else
    tuple->t_len = HeapTupleHeaderGetDatumLength(heap_tuple);
#endif
    tuple->t_data = heap_tuple;
  }
  record(HeapTupleHeader heap_tuple, abstract_memory_context &&ctx) : record(heap_tuple, ctx) {}

  template <std::input_iterator Iter>
  requires convertible_into_nullable_datum<typename std::iterator_traits<Iter>::value_type>
  record(tuple_descriptor &tupdesc, Iter begin, Iter end)
      : tupdesc(tupdesc), tuple([&]() {
          std::vector<::Datum> values;
          std::vector<uint8_t> nulls;
          for (auto it = begin; it != end; ++it) {
            auto nd = into_nullable_datum(*it);
            values.push_back(nd.is_null() ? ::Datum(0) : nd);
            nulls.push_back(nd.is_null() ? 1 : 0);
          }
          return ffi_guard{::heap_form_tuple}(this->tupdesc, values.data(),
                                              reinterpret_cast<bool *>(nulls.data()));
        }()) {}

  template <convertible_into_nullable_datum... D>
  record(tuple_descriptor &tupdesc, D &&...args)
      : tupdesc(tupdesc), tuple([&]() {
          std::array<nullable_datum, sizeof...(D)> datums = {
              cppgres::into_nullable_datum(std::move(args))...};
          std::array<bool, sizeof...(D)> nulls;
          std::ranges::copy(datums | std::views::transform([](auto &v) { return v.is_null(); }),
                            nulls.begin());
          std::array<::Datum, sizeof...(D)> values;
          std::ranges::copy(
              datums | std::views::transform([](auto &v) { return v.is_null() ? ::Datum(0) : v; }),
              values.begin());
          return ffi_guard{::heap_form_tuple}(this->tupdesc, values.data(), nulls.data());
        }()) {}

  /**
   * @brief Number of attributes in the record
   */
  int attributes() const { return tupdesc.operator TupleDesc()->natts; }

  /**
   * @brief Type of attribute using a 0-based index
   *
   * @throws std::out_of_range
   */
  type attribute_type(int n) const {
    check_bounds(n);
    return {.oid = TupleDescAttr(tupdesc.operator TupleDesc(), n)->atttypid};
  }

  /**
   * @brief Name of attribute using a 0-based index
   *
   * @throws std::out_of_range
   */
  std::string_view attribute_name(int n) const {
    check_bounds(n);
    return {NameStr(TupleDescAttr(tupdesc.operator TupleDesc(), n)->attname)};
  }

  /**
   * @brief Get attribute value (datum) using a 0-based index
   *
   * @throws std::out_of_range
   */
  nullable_datum get_attribute(int n) {
    bool isnull;
    check_bounds(n);
    auto _heap_getattr = ffi_guard{
#if PG_MAJORVERSION_NUM < 15
        // Handle the fact that it is a macro
        [](::HeapTuple tup, int attnum, ::TupleDesc tupleDesc, bool *isnull) {
          return heap_getattr(tup, attnum, tupleDesc, isnull);
        }
#else
        ::heap_getattr
#endif
    };
    datum d(_heap_getattr(tuple, n + 1, tupdesc.operator TupleDesc(), &isnull));
    return isnull ? nullable_datum() : nullable_datum(d);
  }

  /**
   * @brief Get attribute by name
   *
   * @throws std::out_of_range
   */
  nullable_datum operator[](std::string_view name) {
    for (int i = 0; i < attributes(); i++) {
      if (attribute_name(i) == name) {
        return get_attribute(i);
      }
    }
    throw std::out_of_range(cppgres::fmt::format("no attribute by the name of {}", name));
  }

  /**
   * @brief Get attribute by 0-based index
   *
   * @throws std::out_of_range
   */
  nullable_datum operator[](int n) { return get_attribute(n); }

  operator HeapTuple() const noexcept { return tuple; }

  /**
   * @brief Returns tuple descriptor
   */
  tuple_descriptor get_tuple_descriptor() const noexcept { return tupdesc; }

  record(const record &other) : tupdesc(other.tupdesc), tuple(other.tuple) {}
  record(record &&other) noexcept : tupdesc(std::move(other.tupdesc)), tuple(other.tuple) {}
  record &operator=(const record &other) {
    tupdesc = other.tupdesc;
    tuple = other.tuple;
    return *this;
  }

  record &operator=(record &&other) noexcept {
    if (this != &other) {
      tupdesc = std::move(other.tupdesc);
      tuple = other.tuple;
    }
    return *this;
  }

private:
  inline void check_bounds(int n) const {
    if (n + 1 > attributes() || n < 0) {
      throw std::out_of_range(cppgres::fmt::format(
          "attribute index {} is out of bounds for record with the size of {}", n, attributes()));
    }
  }

  tuple_descriptor tupdesc;
  HeapTuple tuple;
};
static_assert(std::copy_constructible<record>);
static_assert(std::move_constructible<record>);
static_assert(std::is_copy_assignable_v<record>);

template <> struct datum_conversion<record> : default_datum_conversion<record> {
  static record from_datum(const datum &d, oid, std::optional<memory_context> ctx) {
    return {reinterpret_cast<HeapTupleHeader>(ffi_guard{::pg_detoast_datum}(
                reinterpret_cast<struct ::varlena *>(d.operator const ::Datum &()))),
            ctx.has_value() ? ctx.value() : memory_context()};
  }

  static datum into_datum(const record &t) { return datum(PointerGetDatum(t.tuple)); }
};

template <> struct type_traits<record> {
  bool is(const type &t) {
    if (t.oid == RECORDOID)
      return true;
    // Check if it is a composite type and therefore can be coerced to a record
    syscache<Form_pg_type, oid> cache(t.oid);
    return (*cache).typtype == 'c';
  }
  constexpr type type_for() { return {.oid = RECORDOID}; }
};

} // namespace cppgres
/**
* \file
 */

#include <iterator>


namespace cppgres {

template <typename I>
concept datumable_iterator =
    requires(I i) {
      { std::begin(i) } -> std::input_iterator;
      { std::end(i) } -> std::sentinel_for<decltype(std::begin(i))>;
    } &&
    all_from_nullable_datum<typename std::iterator_traits<decltype(std::begin(
        std::declval<I &>()))>::value_type>::value;

template <datumable_iterator I> struct set_iterator_traits {
  using value_type = std::iterator_traits<decltype(std::begin(std::declval<I &>()))>::value_type;
};

template <typename I> requires datumable_iterator<I>
struct type_traits<I> {
  bool is(type &t) { return t.oid == RECORDOID; }
};

} // namespace cppgres


namespace cppgres {

struct value {

  value(nullable_datum &&datum, type &&type) : datum_(datum), type_(type) {}

  const type &get_type() const { return type_; }

  const nullable_datum &get_nullable_datum() const { return datum_; };

private:
  nullable_datum datum_;
  type type_;
};

template <> struct datum_conversion<value> {

  static value from_nullable_datum(const nullable_datum &d, oid oid,
                                   std::optional<memory_context>) {
    return {nullable_datum(d), type{.oid = oid}};
  }

  static value from_datum(const datum &d, oid oid, std::optional<memory_context>) {
    return {nullable_datum(d), type{.oid = oid}};
  }

  static datum into_datum(const value &t) { return t.get_nullable_datum(); }

  static nullable_datum into_nullable_datum(const value &t) { return t.get_nullable_datum(); }
};

template <> struct type_traits<value> {
  type_traits() : value_(std::nullopt) {}
  type_traits(value &value) : value_(std::optional(std::ref(value))) {}
  bool is(const type &t) { return !value_.has_value() || (*value_).get().get_type() == t; }
  constexpr type type_for() {
    if (value_.has_value()) {
      return (*value_).get().get_type();
    }
    throw std::runtime_error("can't determine type for an uninitialized value");
  }

private:
  std::optional<std::reference_wrapper<value>> value_;
};

} // namespace cppgres

#include <array>
#include <complex>
#include <iostream>
#include <stack>
#include <tuple>
#include <typeinfo>

namespace cppgres {

template <typename T>
concept convertible_into_nullable_datum_or_set_iterator_or_void =
    convertible_into_nullable_datum<T> || datumable_iterator<T> || std::same_as<T, void>;

/**
 * @brief Function that operates on values of Postgres types
 *
 * These functions take @ref cppgres::convertible_from_nullable_datum arguments and returns
 * @ref cppgres::convertible_into_nullable_datum, or @ref cppgres::datumable_iterator
 * (for [Set-Returning
 * Functions](https://www.postgresql.org/docs/current/xfunc-c.html#XFUNC-C-RETURN-SET)).
 */
template <typename Func>
concept datumable_function =
    requires { typename utils::function_traits::function_traits<Func>::argument_types; } &&
    all_from_nullable_datum<
        typename utils::function_traits::function_traits<Func>::argument_types>::value &&
    requires(Func f) {
      {
        std::apply(
            f,
            std::declval<typename utils::function_traits::function_traits<Func>::argument_types>())
      } -> convertible_into_nullable_datum_or_set_iterator_or_void;
    };

struct function_call_info {
  function_call_info(::FunctionCallInfo info) : info_(info) {}

  operator FunctionCallInfo() const { return info_; }

  /**
   * @brief number of arguments actually passed
   */
  short nargs() const { return info_->nargs; }

  /**
   * @brief passed arguments
   */

  auto args() const {
    return std::views::iota(0, nargs()) | std::views::transform([this](int i) -> nullable_datum {
             return nullable_datum(info_->args[i]);
           });
  }

  /**
   * @brief argument types
   */
  auto arg_types() const {
    return std::views::iota(0, nargs()) | std::views::transform([this](int i) -> type {
             return {.oid = ffi_guard{::get_fn_expr_argtype}(info_->flinfo, i)};
           });
  }

  /**
   * @brief typed passed argument
   */

  auto arg_values() const {
    return std::views::iota(0, nargs()) | std::views::transform([this](int i) -> value {
             return value(nullable_datum(info_->args[i]),
                          {.oid = ffi_guard{::get_fn_expr_argtype}(info_->flinfo, i)});
           });
  }

  /**
   * @brief called function OID
   */
  oid called_function_oid() const { return info_->flinfo->fn_oid; }

  /**
   * @brief return type
   */
  type return_type() const { return {.oid = ffi_guard{::get_fn_expr_rettype}(info_->flinfo)}; }

  oid collation() const { return info_->fncollation; }

private:
  ::FunctionCallInfo info_;
};

struct current_postgres_function {

  static std::optional<bool> atomic() {
    if (!calls.empty()) {
      auto ctx = calls.top()->context;
      if (ctx != nullptr && IsA(ctx, CallContext)) {
        return reinterpret_cast<::CallContext *>(ctx)->atomic;
      }
    }

    return std::nullopt;
  }

  static std::optional<function_call_info> call_info() {
    if (!calls.empty()) {
      return calls.top();
    }

    return std::nullopt;
  }

  template <datumable_function Func> friend struct postgres_function;

private:
  struct handle {
    ~handle() { calls.pop(); }

    friend struct current_postgres_function;

  private:
    handle() {}
  };

  static handle push(::FunctionCallInfo fcinfo) {
    calls.push(fcinfo);
    return handle{};
  }
  static void pop() { calls.pop(); }

  static inline std::stack<::FunctionCallInfo> calls;
};

/**
 * @brief Postgres function implemented in C++
 *
 * It wraps a function to handle conversion of arguments and return, enforce arity, convert C++
 * exceptions to errors. Additionally, it handles [Set-Returning
 * Functions](https://www.postgresql.org/docs/current/xfunc-c.html#XFUNC-C-RETURN-SET) (SRFs),
 * implemented by functions returning @ref cppgres::datumable_iterator.
 *
 * @tparam Func function (or lambda) that conforms to the @ref cppgres::datumable_function concept.
 */
template <datumable_function Func> struct postgres_function {
  Func func;

  explicit postgres_function(Func f) : func(f) {}

  // Use the function_traits to extract argument types.
  using traits = utils::function_traits::function_traits<Func>;
  using argument_types = typename traits::argument_types;
  using return_type = utils::function_traits::invoke_result_from_tuple_t<Func, argument_types>;
  static constexpr std::size_t arity = traits::arity;

  /**
   * Invoke the function as per Postgres convention
   */
  auto operator()(FunctionCallInfo fc) -> ::Datum {

    return exception_guard([&] {
      // return type
      auto rettype = type{.oid = ffi_guard{::get_fn_expr_rettype}(fc->flinfo)};
      auto retset = fc->flinfo->fn_retset;

      if constexpr (datumable_iterator<return_type>) {
        // Check this before checking the type of `retset`
        auto rsinfo = reinterpret_cast<::ReturnSetInfo *>(fc->resultinfo);
        if (rsinfo == nullptr) {
          report(ERROR, "caller is not expecting a set");
        }
      }

      if (!OidIsValid(rettype.oid)) {
        // TODO: not very efficient to look it up every time
        syscache<Form_pg_proc, oid> cache(fc->flinfo->fn_oid);
        rettype = type{.oid = (*cache).prorettype};
        retset = (*cache).proretset;
      }

      if (retset) {
        if constexpr (datumable_iterator<return_type>) {
          using set_value_type = set_iterator_traits<return_type>::value_type;
          if (!type_traits<set_value_type>().is(rettype)) {
            report(ERROR, "unexpected set's return type, can't convert `%s` into `%.*s`",
                   rettype.name().data(), utils::type_name<set_value_type>().length(),
                   utils::type_name<set_value_type>().data());
          }
        } else {
          report(ERROR,
                 "unexpected return type, set is expected, but `%.*s` does not conform to "
                 "`cppgres::datumable_iterator`",
                 utils::type_name<return_type>().length(), utils::type_name<return_type>().data());
        }
      } else if (!type_traits<return_type>().is(rettype)) {
        report(ERROR, "unexpected return type, can't convert `%s` into `%.*s`",
               rettype.name().data(), utils::type_name<return_type>().length(),
               utils::type_name<return_type>().data());
      }

      // arguments
      short accounted_for_args = 0;
      auto t = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return argument_types{([&]() -> utils::tuple_element_t<Is, argument_types> {
          using ptyp = utils::tuple_element_t<Is, argument_types>;
          auto typ = type{.oid = ffi_guard{::get_fn_expr_argtype}(fc->flinfo, Is)};
          if (!OidIsValid(typ.oid)) {
            // TODO: not very efficient to look it up every time
            syscache<Form_pg_proc, oid> cache(fc->flinfo->fn_oid);
            if ((*cache).proargtypes.dim1 > Is) {
              typ = type{.oid = (*cache).proargtypes.values[Is]};
            }
          }
          if (!type_traits<ptyp>().is(typ)) {
            report(ERROR, "unexpected type in position %d, can't convert `%s` into `%.*s`", Is,
                   typ.name().data(), utils::type_name<ptyp>().length(),
                   utils::type_name<ptyp>().data());
          }
          accounted_for_args++;
          return from_nullable_datum<ptyp>(nullable_datum(fc->args[Is]), typ.oid);
        }())...};
      }(std::make_index_sequence<utils::tuple_size_v<argument_types>>{});

      if (arity != accounted_for_args) {
        report(ERROR, "expected %d arguments, got %d instead", arity, accounted_for_args);
      }

      auto call_handle = current_postgres_function::push(fc);

      if constexpr (datumable_iterator<return_type>) {
        auto rsinfo = reinterpret_cast<::ReturnSetInfo *>(fc->resultinfo);
        // TODO: For now, let's assume materialized model
        using set_value_type = set_iterator_traits<return_type>::value_type;
        if constexpr (std::same_as<set_value_type, record>) {

          auto natts = rsinfo->expectedDesc == nullptr ? -1 : rsinfo->expectedDesc->natts;

          rsinfo->returnMode = SFRM_Materialize;

          memory_context_scope scope(memory_context(rsinfo->econtext->ecxt_per_query_memory));

          ::Tuplestorestate *tupstore = ffi_guard{::tuplestore_begin_heap}(
              (rsinfo->allowedModes & SFRM_Materialize_Random) == SFRM_Materialize_Random, false,
              work_mem);
          rsinfo->setResult = tupstore;

          auto res = std::apply(func, t);

          bool checked = false;
          for (auto r : res) {
            auto nargs = r.attributes();
            if (!checked) {
              if (rsinfo->expectedDesc != nullptr && nargs != natts) {
                throw std::runtime_error(
                    cppgres::fmt::format("expected record with {} value{}, got {} instead", nargs,
                                nargs == 1 ? "" : "s", natts));
              }
              if (rsinfo->expectedDesc != nullptr &&
                  !r.get_tuple_descriptor().equal_types(
                      cppgres::tuple_descriptor(rsinfo->expectedDesc))) {
                throw std::runtime_error("expected and returned records do not match");
              }
              checked = true;
            }

            ffi_guard{::tuplestore_puttuple}(tupstore, r);
          }
          fc->isnull = true;
          return ::Datum(0);
        } else {
          constexpr auto nargs = utils::tuple_size_v<set_value_type>;

          auto natts = rsinfo->expectedDesc->natts;

          if (nargs != natts) {
            throw std::runtime_error(cppgres::fmt::format("expected set with {} value{}, got {} instead",
                                                 nargs, nargs == 1 ? "" : "s", natts));
          }

          [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (([&] {
               auto oid = ffi_guard{::SPI_gettypeid}(rsinfo->expectedDesc, Is + 1);
               auto t = type{.oid = oid};
               using typ = utils::tuple_element_t<Is, set_value_type>;
               if (!type_traits<typ>().is(t)) {
                 throw std::invalid_argument(
                     cppgres::fmt::format("invalid type in record's position {} ({}), got OID {}", Is,
                                 utils::type_name<typ>(), oid));
               }
             }()),
             ...);
          }(std::make_index_sequence<nargs>{});

          rsinfo->returnMode = SFRM_Materialize;

          memory_context_scope scope(memory_context(rsinfo->econtext->ecxt_per_query_memory));

          ::Tuplestorestate *tupstore = ffi_guard{::tuplestore_begin_heap}(
              (rsinfo->allowedModes & SFRM_Materialize_Random) == SFRM_Materialize_Random, false,
              work_mem);
          rsinfo->setResult = tupstore;

          auto result = std::apply(func, t);

          for (auto it : result) {
            CHECK_FOR_INTERRUPTS();
            std::array<::Datum, nargs> values = std::apply(
                [](auto &&...elems) -> std::array<::Datum, sizeof...(elems)> {
                  return {into_nullable_datum(elems)...};
                },
                utils::tie(it));
            std::array<bool, nargs> isnull = std::apply(
                [](auto &&...elems) -> std::array<bool, sizeof...(elems)> {
                  return {into_nullable_datum(elems).is_null()...};
                },
                utils::tie(it));
            ffi_guard{::tuplestore_putvalues}(tupstore, rsinfo->expectedDesc, values.data(),
                                              isnull.data());
          }

          fc->isnull = true;
          return ::Datum(0);
        }
      } else {
        if constexpr (std::same_as<return_type, void>) {
          std::apply(func, t);
          return ::Datum(0);
        } else {
          auto result = std::apply(func, t);
          nullable_datum nd = into_nullable_datum(result);
          if (nd.is_null()) {
            fc->isnull = true;
            return ::Datum(0);
          }
          return nd.operator const ::Datum &();
        }
      }
    })();
    __builtin_unreachable();
  }
};

} // namespace cppgres

namespace cppgres {

template <class T, class... Args>
concept aggregate = requires(T t, Args &&...args) {
  { t.update(args...) };
};

template <class T, class... Args>
concept finalizable_aggregate = aggregate<T, Args...> && requires(T t) {
  { t.finalize() } -> convertible_into_datum;
};

template <class T, class... Args>
concept serializable_aggregate = aggregate<T, Args...> && requires(T t, bytea &ba) {
  { t.serialize() } -> std::same_as<bytea>;
  { T(ba) } -> std::same_as<T>;
};

template <class T, class... Args>
concept combinable_aggregate = aggregate<T, Args...> && requires(T &&t, T &&t1) {
  { T(t, t1) } -> std::same_as<T>;
};

template <class Agg, typename... InTs> datum aggregate_sfunc(value state, InTs... args) {

  MemoryContext aggctx;
  if (!ffi_guard{::AggCheckCallContext}(current_postgres_function::call_info().operator*(),
                                        &aggctx)) {
    report(ERROR, "not aggregate context");
  }

  if constexpr (!convertible_into_datum<Agg> && finalizable_aggregate<Agg, InTs...>) {
    Agg *state0;
    if (state.get_nullable_datum().is_null()) {
      state0 = memory_context(aggctx).alloc<Agg>();
      std::construct_at(state0);
    } else {
      state0 = reinterpret_cast<Agg *>(
          from_nullable_datum<void *>(state.get_nullable_datum(), state.get_type().oid));
    }

    state0->update(args...);

    return datum_conversion<void *>::into_datum(reinterpret_cast<void *>(state0));
  } else if constexpr (convertible_into_datum<Agg>) {
    Agg state0 = datum_conversion<Agg>::from_nullable_datum(state.get_nullable_datum(), ANYOID);
    state0.update(args...);
    return datum_conversion<Agg>::into_datum(state0);
  }
  report(ERROR, "not supported");
  __builtin_unreachable();
}

template <class Agg, typename... InTs> nullable_datum aggregate_ffunc(value state) {
  if constexpr (finalizable_aggregate<Agg, InTs...>) {
    Agg *state0;
    if (state.get_nullable_datum().is_null()) {
      state0 = memory_context(memory_context()).alloc<Agg>();
      std::construct_at(state0);
    } else {
      state0 = reinterpret_cast<Agg *>(
          from_nullable_datum<void *>(state.get_nullable_datum(), state.get_type().oid));
    }
    return into_nullable_datum(state0->finalize());
  } else {
    report(ERROR, "this aggregate does not support final function");
    __builtin_unreachable();
  }
}

template <class Agg, typename... InTs> bytea aggregate_serial(value state) {
  if constexpr (serializable_aggregate<Agg, InTs...>) {
    if (state.get_type().oid == INTERNALOID) {
      Agg *state0;
      state0 = reinterpret_cast<Agg *>(
          from_nullable_datum<void *>(state.get_nullable_datum(), state.get_type().oid));
      bytea ba = state0->serialize();
      return ba;
    }
  }
  report(ERROR, "this aggregate does not support serialize");
  __builtin_unreachable();
}

template <class Agg, typename... InTs> datum aggregate_deserial(bytea ba, value) {
  if constexpr (serializable_aggregate<Agg, InTs...>) {
    MemoryContext aggctx;
    if (!ffi_guard{::AggCheckCallContext}(current_postgres_function::call_info().operator*(),
                                          &aggctx)) {
      report(ERROR, "not aggregate context");
    }
    Agg *state0 = memory_context(aggctx).alloc<Agg>();
    std::construct_at(state0, ba);
    return datum_conversion<void *>::into_datum(reinterpret_cast<void *>(state0));
  }
  report(ERROR, "this aggregate does not support serialize");
  __builtin_unreachable();
}

template <class Agg, typename... InTs> datum aggregate_combine(value state, value other) {
  MemoryContext aggctx;
  if (!ffi_guard{::AggCheckCallContext}(current_postgres_function::call_info().operator*(),
                                        &aggctx)) {
    report(ERROR, "not aggregate context");
  }
  if constexpr (combinable_aggregate<Agg, InTs...>) {
    if constexpr (!convertible_into_datum<Agg> && finalizable_aggregate<Agg, InTs...>) {
      Agg *state0;
      if (state.get_nullable_datum().is_null()) {
        state0 = memory_context(aggctx).alloc<Agg>();
        std::construct_at(state0);
      } else {
        state0 = reinterpret_cast<Agg *>(
            from_nullable_datum<void *>(state.get_nullable_datum(), state.get_type().oid));
      }

      Agg *state1;
      if (other.get_nullable_datum().is_null()) {
        state1 = memory_context(aggctx).alloc<Agg>();
        std::construct_at(state1);
      } else {
        state1 = reinterpret_cast<Agg *>(
            from_nullable_datum<void *>(other.get_nullable_datum(), other.get_type().oid));
      }

      Agg *newstate = memory_context(aggctx).alloc<Agg>();
      std::construct_at(newstate, *state0, *state1);

      return datum_conversion<void *>::into_datum(reinterpret_cast<void *>(newstate));
    } else if constexpr (convertible_into_datum<Agg>) {
      Agg state0 = datum_conversion<Agg>::from_nullable_datum(state.get_nullable_datum(), ANYOID);
      Agg state1 = datum_conversion<Agg>::from_nullable_datum(state.get_nullable_datum(), ANYOID);
      return datum_conversion<Agg>::into_datum(Agg(state0, state1));
    }
  }
  report(ERROR, "not supported");
  __builtin_unreachable();
}

} // namespace cppgres

#define declare_aggregate(name, typname, ...)                                                      \
  static_assert(::cppgres::aggregate<typname, ##__VA_ARGS__>);                                     \
  static_assert(::cppgres::convertible_into_datum<typname> ||                                      \
                    ::cppgres::finalizable_aggregate<typname, ##__VA_ARGS__>,                   \
                "must be convertible to datum or have finalize()");                                \
  postgres_function(name##_sfunc, (cppgres::aggregate_sfunc<typname, ##__VA_ARGS__>));             \
  postgres_function(name##_ffunc, (cppgres::aggregate_ffunc<typname, ##__VA_ARGS__>));             \
  postgres_function(name##_serial, (cppgres::aggregate_serial<typname, ##__VA_ARGS__>));           \
  postgres_function(name##_deserial, (cppgres::aggregate_deserial<typname, ##__VA_ARGS__>));       \
  postgres_function(name##_combine, (cppgres::aggregate_combine<typname, ##__VA_ARGS__>));
/**
 * \file
 */



namespace cppgres {

enum q { backend };

namespace backend_type {

/**
 * @brief Backend type
 *
 * A copy of Postgres' BackendType with minor renaming
 */
enum type {
  invalid = B_INVALID,
  backend = B_BACKEND,
  autovac_launcher = B_AUTOVAC_LAUNCHER,
  autovac_worker = B_AUTOVAC_WORKER,
  bg_worker = B_BG_WORKER,
  wal_sender = B_WAL_SENDER,
#if PG_MAJORVERSION_NUM >= 17
  slotsync_worker = B_SLOTSYNC_WORKER,
  standalone_backend = B_STANDALONE_BACKEND,
#endif
  archiver = B_ARCHIVER,
  bg_writer = B_BG_WRITER,
  checkpointer = B_CHECKPOINTER,
  startup = B_STARTUP,
  wal_receiver = B_WAL_RECEIVER,
#if PG_MAJORVERSION_NUM >= 17
  wal_summarizer = B_WAL_SUMMARIZER,
#endif
  wal_writer = B_WAL_WRITER,
  logger = B_LOGGER
};
}

/**
 * @brief Backend management
 */
struct backend {
  /**
   * @brief get current backend type
   * @return backend type
   */
  static backend_type::type type() { return static_cast<backend_type::type>(::MyBackendType); };

  /**
   * @brief Register a callback for when Postgres will be exiting
   *
   * This allows passing a lambda with a closure (not just a plain C function / lambda without a
   * closure), it'll initialize the closure in the top memory context.
   */
  template <typename T> requires requires(T t, int code) {
    { t(code) };
  }
  static void atexit(T &&func) {
    T *raw_mem = top_memory_context().alloc<T>();
    T *allocation = new (raw_mem) T(std::forward<T>(func));

    ffi_guard{::on_proc_exit}(
        [](int code, ::Datum datum) {
          T *func = reinterpret_cast<T *>(DatumGetPointer(datum));
          (*func)(code);
        },
        PointerGetDatum(allocation));
  }
};

} // namespace cppgres
#include <functional>
#include <variant>

namespace cppgres::utils {

template <typename T> struct maybe_ref {
  // In-place constructor: constructs T inside the variant using forwarded arguments.
  template <typename... Args> maybe_ref(Args &&...args) : data(std::forward<Args>(args)...) {}
  maybe_ref(const T &value) : data(value) {}
  maybe_ref(T &ref) : data(std::ref(ref)) {}

  operator T &() {
    if (std::holds_alternative<T>(data))
      return std::get<T>(data);
    else
      return std::get<std::reference_wrapper<T>>(data).get();
  }

  operator const T &() const {
    if (std::holds_alternative<T>(data))
      return std::get<T>(data);
    else
      return std::get<std::reference_wrapper<T>>(data).get();
  }

  T *operator->() { return &operator T &(); }
  const T *operator->() const { return &operator const T &(); }

  bool is_ref() const { return !std::holds_alternative<T>(data); }

private:
  std::variant<T, std::reference_wrapper<T>> data;
};
} // namespace cppgres::utils

namespace cppgres {
/**
 * @brief Background worker construction and operations
 */
struct background_worker {

  /**
   * @brief Initialize background worker specification
   *
   * @note Initializes pid notification to be directed to the current process by default,
   *       can be overriden by notify_pid(pid_t)
   */
  background_worker() { worker->bgw_notify_pid = ::MyProcPid; }

  /**
   * Initializes from a background worker specification reference
   * @param worker
   */
  background_worker(::BackgroundWorker &worker) : worker(worker) {}

  background_worker &name(std::string_view name) {
    size_t n = std::min(name.size(), static_cast<size_t>(sizeof(worker->bgw_name) - 1));
    std::copy_n(name.data(), n, worker->bgw_name);
    worker->bgw_name[n] = '\0';
    return *this;
  }
  std::string_view name() { return worker->bgw_name; }

  background_worker &type(std::string_view name) {
    size_t n = std::min(name.size(), static_cast<size_t>(sizeof(worker->bgw_type) - 1));
    std::copy_n(name.data(), n, worker->bgw_type);
    worker->bgw_type[n] = '\0';
    return *this;
  }
  std::string_view type() { return worker->bgw_type; }

  background_worker &library_name(std::string_view name) {
    size_t n = std::min(name.size(), static_cast<size_t>(sizeof(worker->bgw_library_name) - 1));
    std::copy_n(name.data(), n, worker->bgw_library_name);
    worker->bgw_library_name[n] = '\0';
    return *this;
  }
  std::string_view library_name() { return worker->bgw_library_name; }

  background_worker &function_name(std::string_view name) {
    size_t n = std::min(name.size(), static_cast<size_t>(sizeof(worker->bgw_function_name) - 1));
    std::copy_n(name.data(), n, worker->bgw_function_name);
    worker->bgw_function_name[n] = '\0';
    return *this;
  }
  std::string_view function_name() { return worker->bgw_function_name; }

  background_worker &start_time(BgWorkerStartTime time) {
    worker->bgw_start_time = time;
    return *this;
  }

  ::BgWorkerStartTime start_time() const { return worker->bgw_start_time; }

  background_worker &restart_time(int time) {
    worker->bgw_restart_time = time;
    return *this;
  }

  int restart_time() const { return worker->bgw_restart_time; }

  background_worker &flags(int flags) {
    worker->bgw_flags = flags;
    return *this;
  }

  int flags() const { return worker->bgw_flags; }

  background_worker &main_arg(datum datum) {
    worker->bgw_main_arg = datum;
    return *this;
  }

  datum main_arg() const { return datum(worker->bgw_main_arg); }

  background_worker &extra(std::string_view name) {
    size_t n = std::min(name.size(), static_cast<size_t>(sizeof(worker->bgw_extra) - 1));
    std::copy_n(name.data(), n, worker->bgw_extra);
    worker->bgw_extra[n] = '\0';
    return *this;
  }
  std::string_view extra() { return worker->bgw_extra; }

  background_worker &notify_pid(pid_t pid) {
    worker->bgw_notify_pid = pid;
    return *this;
  }

  pid_t notify_pid() const { return worker->bgw_notify_pid; }

  operator BackgroundWorker &() { return worker; }
  operator BackgroundWorker *() { return worker.operator->(); }

  struct worker_stopped : public std::exception {
    const char *what() const noexcept override { return "Background worker stopped"; }
  };

  struct worker_not_yet_started : public std::exception {
    const char *what() const noexcept override {
      return "Background worker hasn't been started yet";
    }
  };

  struct postmaster_died : public std::exception {
    const char *what() const noexcept override { return "postmaster died"; }
  };

  struct handle {
    handle() : handle_(nullptr) {}
    handle(::BackgroundWorkerHandle *handle) : handle_(handle) {}

    ::BackgroundWorkerHandle *operator->() { return handle_; }

    bool has_value() const { return handle_ != nullptr; }
    ::BackgroundWorkerHandle *value() { return handle_; }

    pid_t wait_for_startup() {
      if (has_value()) {
        pid_t pid;
        ffi_guard{::WaitForBackgroundWorkerStartup}(value(), &pid);
        return pid;
      }
      throw std::logic_error("Attempting to wait for a background worker with no handle");
    }

    void wait_for_shutdown() {
      if (has_value()) {
        ffi_guard{::WaitForBackgroundWorkerShutdown}(value());
        return;
      }
      throw std::logic_error("Attempting to wait for a background worker with no handle");
    }

    void terminate() {
      if (has_value()) {
        ffi_guard{::TerminateBackgroundWorker}(value());
        return;
      }
      throw std::logic_error("Attempting to terminate a background worker with no handle");
    }

    pid_t get_pid() {
      if (has_value()) {
        pid_t pid;
        auto rc = ffi_guard{::GetBackgroundWorkerPid}(value(), &pid);
        switch (rc) {
        case BGWH_STARTED:
          return pid;
        case BGWH_STOPPED:
          throw worker_stopped();
        case BGWH_NOT_YET_STARTED:
          throw worker_not_yet_started();
        case BGWH_POSTMASTER_DIED:
          throw postmaster_died();
        }
      }
      throw std::logic_error("Attempting to get a PID of a background worker with no handle");
    }

    const char *worker_type() { return ffi_guard{::GetBackgroundWorkerTypeByPid}(get_pid()); }

  private:
    ::BackgroundWorkerHandle *handle_;
  };

  handle start(bool dynamic = true) {
    if (!dynamic) {
      if (::IsUnderPostmaster || !::IsPostmasterEnvironment) {
        throw std::runtime_error(
            "static background worker can only be start in the postmaster process");
      }
      ffi_guard{::RegisterBackgroundWorker}(operator BackgroundWorker *());
      return {};
    }
    ::BackgroundWorkerHandle *handle;
    ffi_guard{::RegisterDynamicBackgroundWorker}(operator BackgroundWorker *(), &handle);
    return {handle};
  }

private:
  utils::maybe_ref<::BackgroundWorker> worker = {};
};

struct background_worker_database_conection_flag {
  virtual int flag() const { return 0; }
};

struct background_worker_bypass_allow_connection
    : public background_worker_database_conection_flag {
  int flag() const override { return BGWORKER_BYPASS_ALLOWCONN; }
};

#if PG_MAJORVERSION_NUM >= 17
struct background_worker_bypass_role_login_check
    : public background_worker_database_conection_flag {
  int flag() const override { return BGWORKER_BYPASS_ROLELOGINCHECK; }
};
#endif

struct current_background_worker : public background_worker {
  friend std::optional<current_background_worker> get_current_background_worker();

  /**
   * @brief gets current background worker's entry
   *
   * @throws std::logic_error if not in a background worker; to check the backend type
   *         use cppgres::backend::type()
   */
  current_background_worker() : background_worker(*::MyBgworkerEntry) {
    if (backend::type() != backend_type::bg_worker) {
      throw std::logic_error("can't access current background worker in a different backend type");
    }
  }

  void unblock_signals() { ffi_guard{::BackgroundWorkerUnblockSignals}(); }

  void block_signals() { ffi_guard{::BackgroundWorkerBlockSignals}(); }

  /**
   * @brief Connect to the database using db name and, optionally, username
   *
   * @tparam Flags connection flags of @ref cppgres::background_worker_database_conection_flag
   *               derived flags
   * @param dbname database name
   * @param user user name
   * @param flags connection flags of @ref cppgres::background_worker_database_conection_flag
   * derived flags
   */
  template <typename... Flags>
  void connect(std::string dbname, std::optional<std::string> user = std::nullopt, Flags... flags)
      requires(
          std::conjunction_v<std::is_base_of<background_worker_database_conection_flag, Flags>...>)
  {
    ffi_guard{::BackgroundWorkerInitializeConnection}(
        dbname.c_str(), user.has_value() ? user.value().c_str() : nullptr,
        (flags.flag() | ... | 0));
  }

  /**
   * @brief Connect to the database using db oid and, optionally, user oid
   *
   * @tparam Flags connection flags of @ref cppgres::background_worker_database_conection_flag
   *               derived flags
   * @param db database oid
   * @param user user oid
   * @param flags connection flags of @ref cppgres::background_worker_database_conection_flag
   * derived flags
   */
  template <typename... Flags>
  void connect(oid db, std::optional<oid> user = std::nullopt, Flags... flags) requires(
      std::conjunction_v<std::is_base_of<background_worker_database_conection_flag, Flags>...>)
  {
    ffi_guard{::BackgroundWorkerInitializeConnectionByOid}(
        db, user.has_value() ? user.value() : oid(InvalidOid), (flags.flag() | ... | 0));
  }
};

} // namespace cppgres


namespace cppgres {

struct collation {

  collation(oid oid) : oid_(oid) {}

  [[nodiscard]]
  std::string name() const {
    return ffi_guard{::get_collation_name}(oid_);
  }

private:
  oid oid_;
};

} // namespace cppgres
/**
* \file
 */


namespace cppgres {

inline pg_exception::pg_exception(::MemoryContext mcxt) : mcxt(mcxt) {
  ::CurrentMemoryContext = error_cxt =
      memory_context(std::move(alloc_set_memory_context(top_memory_context())));
  error = ffi_guard{::CopyErrorData}();
  ::CurrentMemoryContext = mcxt;
  ffi_guard{::FlushErrorState}();
}

inline pg_exception::~pg_exception() { memory_context(error_cxt).delete_context(); }

} // namespace cppgres
/**
 * \file
 */

#include <string>
#include <string_view>

namespace cppgres::utils {

template <typename T>
concept is_cstring = std::same_as<T, char *> || std::same_as<T, const char *>;

template <typename T>
concept c_str_available = requires(T t) {
  { t.c_str() } -> std::same_as<const char *>;
};

template <typename T>
concept data_length_available = requires(T t) {
  { t.data() } -> std::same_as<const char *>;
  { t.length() } -> std::same_as<std::size_t>;
} && !c_str_available<T>;

template <typename T>
concept convertible_to_cstring = is_cstring<T> || c_str_available<T> || data_length_available<T>;

template <is_cstring S> const char *to_cstring(S string) {
  return const_cast<const char *>(string);
}

template <c_str_available S> const char *to_cstring(S &&string) { return string.c_str(); }

struct owned_cstring {
  owned_cstring(std::string s) : str_(s) {}

  operator const char *() const { return str_.c_str(); }

private:
  std::string str_;
};

template <data_length_available S> owned_cstring to_cstring(S &&string) {
  return std::string(string.data(), string.length());
}

} // namespace cppgres::utils

#include <iterator>
#include <optional>
#include <stack>
#include <vector>

namespace cppgres {

template <typename T>
concept convertible_into_nullable_datum_and_has_a_type =
    convertible_into_nullable_datum<T> && has_a_type<T>;

template <convertible_from_nullable_datum... Args> struct spi_plan {
  friend struct spi_executor;

  spi_plan(spi_plan &&p) : kept(p.kept), plan(p.plan), ctx(std::move(p.ctx)) { p.kept = false; }

  operator ::SPIPlanPtr() {
    if (ctx.resets() > 0) {
      throw pointer_gone_exception();
    }
    return plan;
  }

  void keep() {
    ffi_guard{::SPI_keepplan}(*this);
    kept = true;
  }

  ~spi_plan() {
    if (kept) {
      ffi_guard{::SPI_freeplan}(*this);
    }
  }

private:
  spi_plan(::SPIPlanPtr plan)
      : kept(false), plan(plan), ctx(tracking_memory_context(memory_context::for_pointer(plan))) {}

  bool kept;
  ::SPIPlanPtr plan;
  tracking_memory_context<memory_context> ctx;
};

struct executor {};

template <typename T>
concept a_vector = requires {
  typename T::value_type;
  typename T::allocator_type;
} && std::same_as<T, std::vector<typename T::value_type, typename T::allocator_type>>;

/**
 * @brief [SPI](https://www.postgresql.org/docs/current/spi.html) executor API
 */
struct spi_executor : public executor {
  /**
   * @brief Creates an SPI executor
   */
  spi_executor() : spi_executor(0) {}
  ~spi_executor() {
    ffi_guard{::SPI_finish}();
    executors.pop();
  }

  template <typename T> struct result_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    ::SPITupleTable *tuptable;
    size_t index;
    mutable std::vector<std::optional<T>> tuples;

    constexpr result_iterator() noexcept {}

    constexpr result_iterator(::SPITupleTable *tuptable) noexcept
        : tuptable(tuptable), index(0),
          tuples(std::vector<std::optional<T>>(tuptable->numvals, std::nullopt)) {
      tuples.reserve(tuptable->numvals);
    }
    constexpr result_iterator(::SPITupleTable *tuptable, size_t n) noexcept
        : tuptable(tuptable), index(n),
          tuples(std::vector<std::optional<T>>(tuptable->numvals, std::nullopt)) {
      tuples.reserve(tuptable->numvals);
    }

    bool operator==(size_t end_index) const { return index == end_index; }
    bool operator!=(size_t end_index) const { return index != end_index; }

    constexpr T &operator*() const { return this->operator[](static_cast<difference_type>(index)); }

    constexpr result_iterator &operator++() noexcept {
      index++;
      return *this;
    }
    constexpr result_iterator operator++(int) noexcept {
      index++;
      return this;
    }

    constexpr result_iterator &operator--() noexcept {
      index--;
      return this;
    }
    constexpr result_iterator operator--(int) noexcept {
      index--;
      return this;
    }

    constexpr result_iterator operator+(const difference_type n) const noexcept {
      return result_iterator(tuptable, index + n);
    }

    result_iterator &operator+=(difference_type n) noexcept {
      index += n;
      return this;
    }

    constexpr result_iterator operator-(difference_type n) const noexcept {
      return result_iterator(tuptable, index - n);
    }

    result_iterator &operator-=(difference_type n) noexcept {
      index -= n;
      return this;
    }

    constexpr difference_type operator-(const result_iterator &other) const noexcept {
      return index - other.index;
    }

    T &operator[](difference_type n) const {
      if (tuples.at(n).has_value()) {
        return tuples.at(n).value();
      }
      if constexpr (convertible_from_datum<T>) {
        if (tuptable->tupdesc->natts == 1) {
          // if a special case of a directly convertible type
          bool isnull;
          ::Datum value =
              ffi_guard{::SPI_getbinval}(tuptable->vals[n], tuptable->tupdesc, 1, &isnull);
          ::NullableDatum datum = {.value = value, .isnull = isnull};
          auto ret = from_nullable_datum<T>(nullable_datum(datum),
                                            ffi_guard{::SPI_gettypeid}(tuptable->tupdesc, 1),
                                            memory_context(tuptable->tuptabcxt));
          tuples.emplace(std::next(tuples.begin(), n), std::in_place, ret);
          return tuples.at(n).value();
        }
      }
      if constexpr (a_vector<T>) {
        T ret;
        for (int i = 0; i < tuptable->tupdesc->natts; i++) {
          bool isnull;
          ::Datum value =
              ffi_guard{::SPI_getbinval}(tuptable->vals[n], tuptable->tupdesc, i + 1, &isnull);
          ::NullableDatum datum = {.value = value, .isnull = isnull};
          auto nd = nullable_datum(datum);
          ret.emplace_back(from_nullable_datum<typename T::value_type>(
              nd, ffi_guard{::SPI_gettypeid}(tuptable->tupdesc, i + 1),
              memory_context(tuptable->tuptabcxt)));
        }
        tuples.emplace(std::next(tuples.begin(), n), std::in_place, ret);
      } else {
        auto ret = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
          return T{([&] {
            bool isnull;
            ::Datum value =
                ffi_guard{::SPI_getbinval}(tuptable->vals[n], tuptable->tupdesc, Is + 1, &isnull);
            ::NullableDatum datum = {.value = value, .isnull = isnull};
            auto nd = nullable_datum(datum);
            return from_nullable_datum<utils::tuple_element_t<Is, T>>(
                nd, ffi_guard{::SPI_gettypeid}(tuptable->tupdesc, Is + 1),
                memory_context(tuptable->tuptabcxt));
          }())...};
        }(std::make_index_sequence<utils::tuple_size_v<T>>{});
        tuples.emplace(std::next(tuples.begin(), n), std::in_place, ret);
      }
      return tuples.at(n).value();
    }

    constexpr bool operator==(const result_iterator &other) const noexcept {
      return tuptable == other.tuptable && index == other.index;
    }
    constexpr bool operator!=(const result_iterator &other) const noexcept {
      return !(tuptable == other.tuptable && index == other.index);
    }
    constexpr bool operator<(const result_iterator &other) const noexcept {
      return index < other.index;
    }
    constexpr bool operator>(const result_iterator &other) const noexcept {
      return index > other.index;
    }
    constexpr bool operator<=(const result_iterator &other) const noexcept {
      return index <= other.index;
    }
    constexpr bool operator>=(const result_iterator &other) const noexcept {
      return index >= other.index;
    }

    operator const heap_tuple() const { return tuptable->vals[index]; }

  private:
  };

  template <typename Ret> struct results {
    ::SPITupleTable *table;

    results(::SPITupleTable *table) : table(table) {
      auto natts = table->tupdesc->natts;
      if constexpr (a_vector<Ret>) {
        for (int i = 0; i < natts; i++) {
          auto oid = ffi_guard{::SPI_gettypeid}(table->tupdesc, i + 1);
          auto t = type{.oid = oid};
          if (!type_traits<typename Ret::value_type>().is(t)) {
            throw std::invalid_argument(
                cppgres::fmt::format("invalid return type in position {} ({}), got OID {}", i,
                                     utils::type_name<typename Ret::value_type>(), oid));
          }
        }
      } else {
        if (natts != utils::tuple_size_v<Ret>) {
          if (natts == 1 && convertible_from_datum<Ret>) {
            // okay, this is just a type we can convert
          } else {
            throw std::runtime_error(cppgres::fmt::format("expected {} return values, got {}",
                                                          utils::tuple_size_v<Ret>, natts));
          }
        } else {
          [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (([&] {
               auto oid = ffi_guard{::SPI_gettypeid}(table->tupdesc, Is + 1);
               auto t = type{.oid = oid};
               if (!type_traits<utils::tuple_element_t<Is, Ret>>().is(t)) {
                 throw std::invalid_argument(cppgres::fmt::format(
                     "invalid return type in position {} ({}), got OID {}", Is,
                     utils::type_name<utils::tuple_element_t<Is, Ret>>(), oid));
               }
             }()),
             ...);
          }(std::make_index_sequence<utils::tuple_size_v<Ret>>{});
        }
      }
    }

    result_iterator<Ret> begin() const { return result_iterator<Ret>(table); }
    size_t end() const { return count(); }

    size_t count() const { return table->numvals; }

    tuple_descriptor get_tuple_descriptor() const { return table->tupdesc; }
  };

  struct options {
    explicit options() : read_only_(false), count_(0) {}
    options(bool read_only) : read_only_(read_only), count_(0) {}
    options(int count) : read_only_(false), count_(count) {}
    options(bool read_only, int count) : read_only_(read_only), count_(count) {}

    bool read_only() const { return read_only_; }
    int count() const { return count_; }

  private:
    bool read_only_;
    int count_;
  };

  /**
   * @brief Queries using a string view
   *
   * @param query Query string
   * @param args Query arguments
   *
   * @note if you need to be able to configure the execution, use another version of
   *       the function with @ref cppgres::spi_executor::options argument
   *
   * @return Iterable @ref cppgres::spi_executor::results, can be a single value
   *
   * @throws std::runtime_error if there's another SPI executor in scope
   * @throws std::runtime_error if there's an SPI error
   */
  template <typename Ret, convertible_into_nullable_datum_and_has_a_type... Args>
  results<Ret> query(utils::convertible_to_cstring auto query, Args &&...args) {
    return this->query<Ret>(query, options(), std::forward<Args>(args)...);
  }

  /**
   * @brief Queries using a string view
   *
   * @param query Query string
   * @param opts Execution options
   * @param args Query arguments
   *
   * @return Iterable @ref cppgres::spi_executor::results, can be a single value
   *
   * @throws std::runtime_error if there's another SPI executor in scope
   * @throws std::runtime_error if there's an SPI error
   */
  template <typename Ret, convertible_into_nullable_datum_and_has_a_type... Args>
  results<Ret> query(utils::convertible_to_cstring auto query, options &&opts, Args &&...args) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    constexpr size_t nargs = sizeof...(Args);
    std::array<::Oid, nargs> types = {type_traits<Args>(args...).type_for().oid...};
    std::array<::Datum, nargs> datums = {into_nullable_datum(args)...};
    std::array<const char, nargs> nulls = {into_nullable_datum(args).is_null() ? 'n' : ' ' ...};
    auto rc = ffi_guard{::SPI_execute_with_args}(utils::to_cstring(query), nargs, types.data(),
                                                 datums.data(), nulls.data(), opts.read_only(),
                                                 opts.count());
    if (rc > 0) {
      //      static_assert(std::random_access_iterator<result_iterator<Ret>>);
      return results<Ret>(SPI_tuptable);
    } else {
      throw std::runtime_error("spi error");
    }
  }

  template <convertible_into_nullable_datum_and_has_a_type... Args>
  spi_plan<Args...> plan(utils::convertible_to_cstring auto query) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    constexpr size_t nargs = sizeof...(Args);
    std::array<::Oid, nargs> types = {type_traits<Args>().type_for().oid...};
    return spi_plan<Args...>(
        ffi_guard{::SPI_prepare}(utils::to_cstring(query), nargs, types.data()));
  }

  template <typename Ret, convertible_into_nullable_datum... Args>
  results<Ret> query(spi_plan<Args...> &query, Args &&...args) {
    return this->query<Ret, Args...>(query, options(), std::forward<Args>(args)...);
  }

  template <typename Ret, convertible_into_nullable_datum... Args>
  results<Ret> query(spi_plan<Args...> &query, options &&opts, Args &&...args) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    constexpr size_t nargs = sizeof...(Args);
    std::array<::Datum, nargs> datums = {into_nullable_datum(args)...};
    std::array<const char, nargs> nulls = {into_nullable_datum(args).is_null() ? 'n' : ' ' ...};
    auto rc = ffi_guard{::SPI_execute_plan}(query, datums.data(), nulls.data(), opts.read_only(),
                                            opts.count());
    if (rc > 0) {
      //      static_assert(std::random_access_iterator<result_iterator<Ret>>);
      return results<Ret>(SPI_tuptable);
    } else {
      throw std::runtime_error("spi error");
    }
  }

  template <convertible_into_nullable_datum_and_has_a_type... Args>
  uint64_t execute(std::string_view query, Args &&...args) {
    return execute(query, options(), std::forward<Args>(args)...);
  }

  template <convertible_into_nullable_datum_and_has_a_type... Args>
  uint64_t execute(std::string_view query, options &&opts, Args &&...args) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    constexpr size_t nargs = sizeof...(Args);
    std::array<::Oid, nargs> types = {type_traits<Args>(args...).type_for().oid...};
    std::array<::Datum, nargs> datums = {into_nullable_datum(args)...};
    std::array<const char, nargs> nulls = {into_nullable_datum(args).is_null() ? 'n' : ' ' ...};
    auto rc = ffi_guard{::SPI_execute_with_args}(query.data(), nargs, types.data(), datums.data(),
                                                 nulls.data(), opts.read_only(), opts.count());
    if (rc >= 0) {
      return SPI_processed;
    } else {
      throw std::runtime_error(cppgres::fmt::format("spi error"));
    }
  }

private:
  ::MemoryContext before_spi;
  ::MemoryContext spi;

protected:
  static inline std::stack<spi_executor *> executors;
  spi_executor(int flags) : before_spi(::CurrentMemoryContext) {
    ffi_guard{::SPI_connect_ext}(flags);
    spi = ::CurrentMemoryContext;
    ::CurrentMemoryContext = before_spi;
    executors.push(this);
  }
};

struct spi_nonatomic_executor : public spi_executor {
  using spi_executor::spi_executor;

  spi_nonatomic_executor() : spi_executor(SPI_OPT_NONATOMIC) {
    auto atomic = cppgres::current_postgres_function::atomic();
    if (atomic.has_value() && atomic.value()) {
      throw std::runtime_error("must be called in a non-atomic context");
    }
  }

  void commit(bool chain = false) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    ffi_guard(chain ? ::SPI_commit_and_chain : ::SPI_commit)();
  }

  void rollback(bool chain = false) {
    if (executors.top() != this) {
      throw std::runtime_error("not a current SPI executor");
    }
    ffi_guard(chain ? ::SPI_rollback_and_chain : ::SPI_rollback)();
  }
};

} // namespace cppgres
#include <concepts>
#include <type_traits>


#if PG_MAJORVERSION_NUM < 16
typedef bool (*tree_walker_callback)(Node *node, void *context);
#endif

namespace cppgres {

using node_tag = ::NodeTag;

template <typename T>
concept node_tagged = std::is_standard_layout_v<T> && requires(T t) {
  { t.type } -> std::convertible_to<node_tag>;
} && (offsetof(T, type) == 0);

template <typename T>
concept node_xpr_tagged = std::is_standard_layout_v<T> && requires(T t) {
  { t.xpr } -> std::convertible_to<::Expr>;
} && (offsetof(T, xpr) == 0);

template <typename T>
concept node_inherited_base = std::is_standard_layout_v<std::remove_cvref_t<T>> && requires(T t) {
  requires node_tagged<std::remove_cvref_t<decltype(boost::pfr::get<0>(t))>> ||
               node_xpr_tagged<std::remove_cvref_t<decltype(boost::pfr::get<0>(t))>>;
};

template <typename T>
concept node_inherited = std::is_standard_layout_v<std::remove_cvref_t<T>> && requires(T t) {
  requires node_tagged<std::remove_cvref_t<decltype(boost::pfr::get<0>(t))>> ||
               node_xpr_tagged<std::remove_cvref_t<decltype(boost::pfr::get<0>(t))>> ||
               node_inherited_base<std::remove_cvref_t<decltype(boost::pfr::get<0>(t))>>;
};

template <typename T>
concept node = node_tagged<T> || node_xpr_tagged<T> || node_inherited<T>;

template <typename T> struct node_traits {
  static inline bool is(auto &node) { return false; }
};
template <node_tag T> struct node_tag_traits;
template <typename T> struct node_coverage;

#define node_mapping(name)                                                                         \
  namespace nodes {                                                                                \
  struct name {                                                                                    \
    using underlying_type = ::name;                                                                \
    static constexpr inline node_tag tag = T_##name;                                               \
    name(underlying_type v) : val(v) {}                                                            \
    name() { reinterpret_cast<node_tag &>(val) = T_##name; }                                       \
    underlying_type &as_ref() { return val; }                                                      \
    underlying_type *as_ptr() { return &val; }                                                     \
                                                                                                   \
  private:                                                                                         \
    [[maybe_unused]] underlying_type val{};                                                        \
  };                                                                                               \
  }                                                                                                \
  static_assert((sizeof(name) == sizeof(::name)) && (alignof(name) == alignof(::name)));           \
  static_assert(std::is_standard_layout_v<name>);                                                  \
  static_assert(std::is_aggregate_v<name>);                                                        \
  template <> struct node_coverage<::name> {                                                       \
    using type = nodes::name;                                                                      \
  };                                                                                               \
  template <> struct node_tag_traits<node_tag::T_##name> {                                         \
    using type = nodes::name;                                                                      \
  };                                                                                               \
  template <> struct node_traits<nodes::name> {                                                    \
    static inline constexpr node_tag tag = node_tag::T_##name;                                     \
    static inline bool is(::Node *node) {                                                          \
      return *reinterpret_cast<node_tag *>(node) == node_tag::T_##name;                            \
    }                                                                                              \
    static inline bool is(::name *node) {                                                          \
      return *reinterpret_cast<node_tag *>(node) == node_tag::T_##name;                            \
    }                                                                                              \
    static inline bool is(void *node) {                                                            \
      return *reinterpret_cast<node_tag *>(node) == node_tag::T_##name;                            \
    }                                                                                              \
    static inline bool is(nodes::name *node) { return true; }                                      \
    static inline bool is(nodes::name &node) { return true; }                                      \
    static inline bool is(auto &node) { return false; }                                            \
    static inline nodes::name *allocate(abstract_memory_context &&ctx = memory_context()) {        \
      auto ptr = ctx.alloc<nodes::name>();                                                         \
      reinterpret_cast<node_tag &>(ptr) = tag;                                                     \
      return ptr;                                                                                  \
    }                                                                                              \
  }

#define node_mapping_no_node_traits(name)                                                          \
  namespace nodes {                                                                                \
  using name = node_coverage<::name>::type;                                                        \
  }                                                                                                \
  template <> struct node_tag_traits<node_tag::T_##name> {                                         \
    using type = nodes::name;                                                                      \
    static_assert(::cppgres::node<name>);                                                          \
  }

node_mapping(List);
node_mapping(Alias);
node_mapping(RangeVar);
node_mapping(TableFunc);
node_mapping(IntoClause);
node_mapping(Var);
node_mapping(Const);
node_mapping(Param);
node_mapping(Aggref);
node_mapping(GroupingFunc);
node_mapping(WindowFunc);
#if PG_MAJORVERSION_NUM >= 17
node_mapping(WindowFuncRunCondition);
node_mapping(MergeSupportFunc);
#endif
node_mapping(SubscriptingRef);
node_mapping(FuncExpr);
node_mapping(NamedArgExpr);
node_mapping(OpExpr);
node_mapping_no_node_traits(DistinctExpr);
node_mapping_no_node_traits(NullIfExpr);
node_mapping(ScalarArrayOpExpr);
node_mapping(BoolExpr);
node_mapping(SubLink);
node_mapping(SubPlan);
node_mapping(AlternativeSubPlan);
node_mapping(FieldSelect);
node_mapping(FieldStore);
node_mapping(RelabelType);
node_mapping(CoerceViaIO);
node_mapping(ArrayCoerceExpr);
node_mapping(ConvertRowtypeExpr);
node_mapping(CollateExpr);
node_mapping(CaseExpr);
node_mapping(CaseWhen);
node_mapping(CaseTestExpr);
node_mapping(ArrayExpr);
node_mapping(RowExpr);
node_mapping(RowCompareExpr);
node_mapping(CoalesceExpr);
node_mapping(MinMaxExpr);
node_mapping(SQLValueFunction);
node_mapping(XmlExpr);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(JsonFormat);
node_mapping(JsonReturning);
node_mapping(JsonValueExpr);
node_mapping(JsonConstructorExpr);
node_mapping(JsonIsPredicate);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_mapping(JsonBehavior);
node_mapping(JsonExpr);
node_mapping(JsonTablePath);
node_mapping(JsonTablePathScan);
node_mapping(JsonTableSiblingJoin);
#endif
node_mapping(NullTest);
node_mapping(BooleanTest);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(MergeAction);
#endif
node_mapping(CoerceToDomain);
node_mapping(CoerceToDomainValue);
node_mapping(SetToDefault);
node_mapping(CurrentOfExpr);
node_mapping(NextValueExpr);
node_mapping(InferenceElem);
#if PG_MAJORVERSION_NUM >= 18
node_mapping(ReturningExpr);
#endif
node_mapping(TargetEntry);
node_mapping(RangeTblRef);
node_mapping(JoinExpr);
node_mapping(FromExpr);
node_mapping(OnConflictExpr);
node_mapping(Query);
node_mapping(TypeName);
node_mapping(ColumnRef);
node_mapping(ParamRef);
node_mapping(A_Expr);
node_mapping(A_Const);
node_mapping(TypeCast);
node_mapping(CollateClause);
node_mapping(RoleSpec);
node_mapping(FuncCall);
node_mapping(A_Star);
node_mapping(A_Indices);
node_mapping(A_Indirection);
node_mapping(A_ArrayExpr);
node_mapping(ResTarget);
node_mapping(MultiAssignRef);
node_mapping(SortBy);
node_mapping(WindowDef);
node_mapping(RangeSubselect);
node_mapping(RangeFunction);
node_mapping(RangeTableFunc);
node_mapping(RangeTableFuncCol);
node_mapping(RangeTableSample);
node_mapping(ColumnDef);
node_mapping(TableLikeClause);
node_mapping(IndexElem);
node_mapping(DefElem);
node_mapping(LockingClause);
node_mapping(XmlSerialize);
node_mapping(PartitionElem);
#if PG_MAJORVERSION_NUM == 17
node_mapping(SinglePartitionSpec);
#endif
node_mapping(PartitionSpec);
node_mapping(PartitionBoundSpec);
node_mapping(PartitionRangeDatum);
node_mapping(PartitionCmd);
node_mapping(RangeTblEntry);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(RTEPermissionInfo);
#endif
node_mapping(RangeTblFunction);
node_mapping(TableSampleClause);
node_mapping(WithCheckOption);
node_mapping(SortGroupClause);
node_mapping(GroupingSet);
node_mapping(WindowClause);
node_mapping(RowMarkClause);
node_mapping(WithClause);
node_mapping(InferClause);
node_mapping(OnConflictClause);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(CTESearchClause);
node_mapping(CTECycleClause);
#endif
node_mapping(CommonTableExpr);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(MergeWhenClause);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_mapping(ReturningOption);
node_mapping(ReturningClause);
#endif
node_mapping(TriggerTransition);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(JsonOutput);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_mapping(JsonArgument);
node_mapping(JsonFuncExpr);
node_mapping(JsonTablePathSpec);
node_mapping(JsonTable);
node_mapping(JsonTableColumn);
node_mapping(JsonKeyValue);
node_mapping(JsonParseExpr);
node_mapping(JsonScalarExpr);
node_mapping(JsonSerializeExpr);
#endif
#if PG_MAJORVERSION_NUM >= 16
node_mapping(JsonObjectConstructor);
node_mapping(JsonArrayConstructor);
node_mapping(JsonArrayQueryConstructor);
node_mapping(JsonAggConstructor);
node_mapping(JsonObjectAgg);
node_mapping(JsonArrayAgg);
#endif
node_mapping(RawStmt);
node_mapping(InsertStmt);
node_mapping(DeleteStmt);
node_mapping(UpdateStmt);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(MergeStmt);
#endif
node_mapping(SelectStmt);
node_mapping(SetOperationStmt);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(ReturnStmt);
#endif
#if PG_MAJORVERSION_NUM >= 14
node_mapping(PLAssignStmt);
#endif
node_mapping(CreateSchemaStmt);
node_mapping(AlterTableStmt);
node_mapping(AlterTableCmd);
#if PG_MAJORVERSION_NUM >= 18
node_mapping(ATAlterConstraint);
#endif
node_mapping(ReplicaIdentityStmt);
node_mapping(AlterCollationStmt);
node_mapping(AlterDomainStmt);
node_mapping(GrantStmt);
node_mapping(ObjectWithArgs);
node_mapping(AccessPriv);
node_mapping(GrantRoleStmt);
node_mapping(AlterDefaultPrivilegesStmt);
node_mapping(CopyStmt);
node_mapping(VariableSetStmt);
node_mapping(VariableShowStmt);
node_mapping(CreateStmt);
node_mapping(Constraint);
node_mapping(CreateTableSpaceStmt);
node_mapping(DropTableSpaceStmt);
node_mapping(AlterTableSpaceOptionsStmt);
node_mapping(AlterTableMoveAllStmt);
node_mapping(CreateExtensionStmt);
node_mapping(AlterExtensionStmt);
node_mapping(AlterExtensionContentsStmt);
node_mapping(CreateFdwStmt);
node_mapping(AlterFdwStmt);
node_mapping(CreateForeignServerStmt);
node_mapping(AlterForeignServerStmt);
node_mapping(CreateForeignTableStmt);
node_mapping(CreateUserMappingStmt);
node_mapping(AlterUserMappingStmt);
node_mapping(DropUserMappingStmt);
node_mapping(ImportForeignSchemaStmt);
node_mapping(CreatePolicyStmt);
node_mapping(AlterPolicyStmt);
node_mapping(CreateAmStmt);
node_mapping(CreateTrigStmt);
node_mapping(CreateEventTrigStmt);
node_mapping(AlterEventTrigStmt);
node_mapping(CreatePLangStmt);
node_mapping(CreateRoleStmt);
node_mapping(AlterRoleStmt);
node_mapping(AlterRoleSetStmt);
node_mapping(DropRoleStmt);
node_mapping(CreateSeqStmt);
node_mapping(AlterSeqStmt);
node_mapping(DefineStmt);
node_mapping(CreateDomainStmt);
node_mapping(CreateOpClassStmt);
node_mapping(CreateOpClassItem);
node_mapping(CreateOpFamilyStmt);
node_mapping(AlterOpFamilyStmt);
node_mapping(DropStmt);
node_mapping(TruncateStmt);
node_mapping(CommentStmt);
node_mapping(SecLabelStmt);
node_mapping(DeclareCursorStmt);
node_mapping(ClosePortalStmt);
node_mapping(FetchStmt);
node_mapping(IndexStmt);
node_mapping(CreateStatsStmt);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(StatsElem);
#endif
node_mapping(AlterStatsStmt);
node_mapping(CreateFunctionStmt);
node_mapping(FunctionParameter);
node_mapping(AlterFunctionStmt);
node_mapping(DoStmt);
node_mapping(InlineCodeBlock);
node_mapping(CallStmt);
node_mapping(CallContext);
node_mapping(RenameStmt);
node_mapping(AlterObjectDependsStmt);
node_mapping(AlterObjectSchemaStmt);
node_mapping(AlterOwnerStmt);
node_mapping(AlterOperatorStmt);
node_mapping(AlterTypeStmt);
node_mapping(RuleStmt);
node_mapping(NotifyStmt);
node_mapping(ListenStmt);
node_mapping(UnlistenStmt);
node_mapping(TransactionStmt);
node_mapping(CompositeTypeStmt);
node_mapping(CreateEnumStmt);
node_mapping(CreateRangeStmt);
node_mapping(AlterEnumStmt);
node_mapping(ViewStmt);
node_mapping(LoadStmt);
node_mapping(CreatedbStmt);
node_mapping(AlterDatabaseStmt);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(AlterDatabaseRefreshCollStmt);
#endif
node_mapping(AlterDatabaseSetStmt);
node_mapping(DropdbStmt);
node_mapping(AlterSystemStmt);
node_mapping(ClusterStmt);
node_mapping(VacuumStmt);
node_mapping(VacuumRelation);
node_mapping(ExplainStmt);
node_mapping(CreateTableAsStmt);
node_mapping(RefreshMatViewStmt);
node_mapping(CheckPointStmt);
node_mapping(DiscardStmt);
node_mapping(LockStmt);
node_mapping(ConstraintsSetStmt);
node_mapping(ReindexStmt);
node_mapping(CreateConversionStmt);
node_mapping(CreateCastStmt);
node_mapping(CreateTransformStmt);
node_mapping(PrepareStmt);
node_mapping(ExecuteStmt);
node_mapping(DeallocateStmt);
node_mapping(DropOwnedStmt);
node_mapping(ReassignOwnedStmt);
node_mapping(AlterTSDictionaryStmt);
node_mapping(AlterTSConfigurationStmt);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(PublicationTable);
node_mapping(PublicationObjSpec);
#endif
node_mapping(CreatePublicationStmt);
node_mapping(AlterPublicationStmt);
node_mapping(CreateSubscriptionStmt);
node_mapping(AlterSubscriptionStmt);
node_mapping(DropSubscriptionStmt);
node_mapping(PlannerGlobal);
node_mapping(PlannerInfo);
node_mapping(RelOptInfo);
node_mapping(IndexOptInfo);
node_mapping(ForeignKeyOptInfo);
node_mapping(StatisticExtInfo);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(JoinDomain);
#endif
node_mapping(EquivalenceClass);
node_mapping(EquivalenceMember);
node_mapping(PathKey);
#if PG_MAJORVERSION_NUM >= 17
node_mapping(GroupByOrdering);
#endif
node_mapping(PathTarget);
node_mapping(ParamPathInfo);
node_mapping(Path);
node_mapping(IndexPath);
node_mapping(IndexClause);
node_mapping(BitmapHeapPath);
node_mapping(BitmapAndPath);
node_mapping(BitmapOrPath);
node_mapping(TidPath);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(TidRangePath);
#endif
node_mapping(SubqueryScanPath);
node_mapping(ForeignPath);
node_mapping(CustomPath);
node_mapping(AppendPath);
node_mapping(MergeAppendPath);
node_mapping(GroupResultPath);
node_mapping(MaterialPath);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(MemoizePath);
#endif
node_mapping(UniquePath);
node_mapping(GatherPath);
node_mapping(GatherMergePath);
node_mapping(NestPath);
node_mapping(MergePath);
node_mapping(HashPath);
node_mapping(ProjectionPath);
node_mapping(ProjectSetPath);
node_mapping(SortPath);
node_mapping(IncrementalSortPath);
node_mapping(GroupPath);
#if PG_MAJORVERSION_NUM < 19
node_mapping(UpperUniquePath);
#endif
node_mapping(AggPath);
node_mapping(GroupingSetData);
node_mapping(RollupData);
node_mapping(GroupingSetsPath);
node_mapping(MinMaxAggPath);
node_mapping(WindowAggPath);
node_mapping(SetOpPath);
node_mapping(RecursiveUnionPath);
node_mapping(LockRowsPath);
node_mapping(ModifyTablePath);
node_mapping(LimitPath);
node_mapping(RestrictInfo);
node_mapping(PlaceHolderVar);
node_mapping(SpecialJoinInfo);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(OuterJoinClauseInfo);
#endif
node_mapping(AppendRelInfo);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(RowIdentityVarInfo);
#endif
node_mapping(PlaceHolderInfo);
node_mapping(MinMaxAggInfo);
node_mapping(PlannerParamItem);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(AggInfo);
node_mapping(AggTransInfo);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_mapping(UniqueRelInfo);
#endif
node_mapping(PlannedStmt);
node_mapping(Result);
node_mapping(ProjectSet);
node_mapping(ModifyTable);
node_mapping(Append);
node_mapping(MergeAppend);
node_mapping(RecursiveUnion);
node_mapping(BitmapAnd);
node_mapping(BitmapOr);
node_mapping(SeqScan);
node_mapping(SampleScan);
node_mapping(IndexScan);
node_mapping(IndexOnlyScan);
node_mapping(BitmapIndexScan);
node_mapping(BitmapHeapScan);
node_mapping(TidScan);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(TidRangeScan);
#endif
node_mapping(SubqueryScan);
node_mapping(FunctionScan);
node_mapping(ValuesScan);
node_mapping(TableFuncScan);
node_mapping(CteScan);
node_mapping(NamedTuplestoreScan);
node_mapping(WorkTableScan);
node_mapping(ForeignScan);
node_mapping(CustomScan);
node_mapping(NestLoop);
node_mapping(NestLoopParam);
node_mapping(MergeJoin);
node_mapping(HashJoin);
node_mapping(Material);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(Memoize);
#endif
node_mapping(Sort);
node_mapping(IncrementalSort);
node_mapping(Group);
node_mapping(Agg);
node_mapping(WindowAgg);
node_mapping(Unique);
node_mapping(Gather);
node_mapping(GatherMerge);
node_mapping(Hash);
node_mapping(SetOp);
node_mapping(LockRows);
node_mapping(Limit);
node_mapping(PlanRowMark);
node_mapping(PartitionPruneInfo);
node_mapping(PartitionedRelPruneInfo);
node_mapping(PartitionPruneStepOp);
node_mapping(PartitionPruneStepCombine);
node_mapping(PlanInvalItem);
node_mapping(ExprState);
node_mapping(IndexInfo);
node_mapping(ExprContext);
node_mapping(ReturnSetInfo);
node_mapping(ProjectionInfo);
node_mapping(JunkFilter);
node_mapping(OnConflictSetState);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(MergeActionState);
#endif
node_mapping(ResultRelInfo);
node_mapping(EState);
node_mapping(WindowFuncExprState);
node_mapping(SetExprState);
node_mapping(SubPlanState);
node_mapping(DomainConstraintState);
node_mapping(ResultState);
node_mapping(ProjectSetState);
node_mapping(ModifyTableState);
node_mapping(AppendState);
node_mapping(MergeAppendState);
node_mapping(RecursiveUnionState);
node_mapping(BitmapAndState);
node_mapping(BitmapOrState);
node_mapping(ScanState);
node_mapping(SeqScanState);
node_mapping(SampleScanState);
node_mapping(IndexScanState);
node_mapping(IndexOnlyScanState);
node_mapping(BitmapIndexScanState);
node_mapping(BitmapHeapScanState);
node_mapping(TidScanState);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(TidRangeScanState);
#endif
node_mapping(SubqueryScanState);
node_mapping(FunctionScanState);
node_mapping(ValuesScanState);
node_mapping(TableFuncScanState);
node_mapping(CteScanState);
node_mapping(NamedTuplestoreScanState);
node_mapping(WorkTableScanState);
node_mapping(ForeignScanState);
node_mapping(CustomScanState);
node_mapping(JoinState);
node_mapping(NestLoopState);
node_mapping(MergeJoinState);
node_mapping(HashJoinState);
node_mapping(MaterialState);
#if PG_MAJORVERSION_NUM >= 14
node_mapping(MemoizeState);
#endif
node_mapping(SortState);
node_mapping(IncrementalSortState);
node_mapping(GroupState);
node_mapping(AggState);
node_mapping(WindowAggState);
node_mapping(UniqueState);
node_mapping(GatherState);
node_mapping(GatherMergeState);
node_mapping(HashState);
node_mapping(SetOpState);
node_mapping(LockRowsState);
node_mapping(LimitState);
node_mapping(IndexAmRoutine);
node_mapping(TableAmRoutine);
node_mapping(TsmRoutine);
node_mapping(EventTriggerData);
node_mapping(TriggerData);
node_mapping(TupleTableSlot);
node_mapping(FdwRoutine);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(Bitmapset);
#endif
node_mapping(ExtensibleNode);
#if PG_MAJORVERSION_NUM >= 16
node_mapping(ErrorSaveContext);
#endif
node_mapping(IdentifySystemCmd);
node_mapping(BaseBackupCmd);
node_mapping(CreateReplicationSlotCmd);
node_mapping(DropReplicationSlotCmd);
#if PG_MAJORVERSION_NUM >= 17
node_mapping(AlterReplicationSlotCmd);
#endif
node_mapping(StartReplicationCmd);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(ReadReplicationSlotCmd);
#endif
node_mapping(TimeLineHistoryCmd);
#if PG_MAJORVERSION_NUM >= 17
node_mapping(UploadManifestCmd);
#endif
node_mapping(SupportRequestSimplify);
node_mapping(SupportRequestSelectivity);
node_mapping(SupportRequestCost);
node_mapping(SupportRequestRows);
node_mapping(SupportRequestIndexCondition);
#if PG_MAJORVERSION_NUM >= 15
node_mapping(SupportRequestWFuncMonotonic);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_mapping(SupportRequestOptimizeWindowClause);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_mapping(SupportRequestModifyInPlace);
#endif
#if PG_MAJORVERSION_NUM >= 15
node_mapping(Integer);
node_mapping(Float);
node_mapping(Boolean);
node_mapping(String);
node_mapping(BitString);
#endif
node_mapping(ForeignKeyCacheInfo);
#undef node_mapping

namespace nodes {
template <typename T> struct unknown_node {
  T node;
};
}; // namespace nodes

#define node_dispatch(name)                                                                        \
  case T_##name:                                                                                   \
    static_assert(                                                                                 \
        covering_node<std::remove_pointer_t<decltype(reinterpret_cast<nodes::name *>(node))>>);    \
    visitor(*reinterpret_cast<nodes::name *>(node));                                               \
    break

template <typename T>
concept covering_node =
    std::is_class_v<node_traits<T>> && requires { typename T::underlying_type; };

template <typename Visitor> void visit_node(covering_node auto node, Visitor &&visitor) {
  visitor(node);
}

template <typename T>
concept covered_node = std::is_class_v<node_coverage<T>> &&
                       requires { typename node_coverage<T>::type::underlying_type; };

template <typename Visitor> void visit_node(covered_node auto *node, Visitor &&visitor) {
  visitor(*reinterpret_cast<node_coverage<std::remove_pointer_t<decltype(node)>>::type *>(node));
}

template <typename Visitor> void visit_node(void *node, Visitor &&visitor) {
  switch (nodeTag(node)) {
node_dispatch(List);
node_dispatch(Alias);
node_dispatch(RangeVar);
node_dispatch(TableFunc);
node_dispatch(IntoClause);
node_dispatch(Var);
node_dispatch(Const);
node_dispatch(Param);
node_dispatch(Aggref);
node_dispatch(GroupingFunc);
node_dispatch(WindowFunc);
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(WindowFuncRunCondition);
node_dispatch(MergeSupportFunc);
#endif
node_dispatch(SubscriptingRef);
node_dispatch(FuncExpr);
node_dispatch(NamedArgExpr);
node_dispatch(OpExpr);
node_dispatch(DistinctExpr);
node_dispatch(NullIfExpr);
node_dispatch(ScalarArrayOpExpr);
node_dispatch(BoolExpr);
node_dispatch(SubLink);
node_dispatch(SubPlan);
node_dispatch(AlternativeSubPlan);
node_dispatch(FieldSelect);
node_dispatch(FieldStore);
node_dispatch(RelabelType);
node_dispatch(CoerceViaIO);
node_dispatch(ArrayCoerceExpr);
node_dispatch(ConvertRowtypeExpr);
node_dispatch(CollateExpr);
node_dispatch(CaseExpr);
node_dispatch(CaseWhen);
node_dispatch(CaseTestExpr);
node_dispatch(ArrayExpr);
node_dispatch(RowExpr);
node_dispatch(RowCompareExpr);
node_dispatch(CoalesceExpr);
node_dispatch(MinMaxExpr);
node_dispatch(SQLValueFunction);
node_dispatch(XmlExpr);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(JsonFormat);
node_dispatch(JsonReturning);
node_dispatch(JsonValueExpr);
node_dispatch(JsonConstructorExpr);
node_dispatch(JsonIsPredicate);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(JsonBehavior);
node_dispatch(JsonExpr);
node_dispatch(JsonTablePath);
node_dispatch(JsonTablePathScan);
node_dispatch(JsonTableSiblingJoin);
#endif
node_dispatch(NullTest);
node_dispatch(BooleanTest);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(MergeAction);
#endif
node_dispatch(CoerceToDomain);
node_dispatch(CoerceToDomainValue);
node_dispatch(SetToDefault);
node_dispatch(CurrentOfExpr);
node_dispatch(NextValueExpr);
node_dispatch(InferenceElem);
#if PG_MAJORVERSION_NUM >= 18
node_dispatch(ReturningExpr);
#endif
node_dispatch(TargetEntry);
node_dispatch(RangeTblRef);
node_dispatch(JoinExpr);
node_dispatch(FromExpr);
node_dispatch(OnConflictExpr);
node_dispatch(Query);
node_dispatch(TypeName);
node_dispatch(ColumnRef);
node_dispatch(ParamRef);
node_dispatch(A_Expr);
node_dispatch(A_Const);
node_dispatch(TypeCast);
node_dispatch(CollateClause);
node_dispatch(RoleSpec);
node_dispatch(FuncCall);
node_dispatch(A_Star);
node_dispatch(A_Indices);
node_dispatch(A_Indirection);
node_dispatch(A_ArrayExpr);
node_dispatch(ResTarget);
node_dispatch(MultiAssignRef);
node_dispatch(SortBy);
node_dispatch(WindowDef);
node_dispatch(RangeSubselect);
node_dispatch(RangeFunction);
node_dispatch(RangeTableFunc);
node_dispatch(RangeTableFuncCol);
node_dispatch(RangeTableSample);
node_dispatch(ColumnDef);
node_dispatch(TableLikeClause);
node_dispatch(IndexElem);
node_dispatch(DefElem);
node_dispatch(LockingClause);
node_dispatch(XmlSerialize);
node_dispatch(PartitionElem);
#if PG_MAJORVERSION_NUM == 17
node_dispatch(SinglePartitionSpec);
#endif
node_dispatch(PartitionSpec);
node_dispatch(PartitionBoundSpec);
node_dispatch(PartitionRangeDatum);
node_dispatch(PartitionCmd);
node_dispatch(RangeTblEntry);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(RTEPermissionInfo);
#endif
node_dispatch(RangeTblFunction);
node_dispatch(TableSampleClause);
node_dispatch(WithCheckOption);
node_dispatch(SortGroupClause);
node_dispatch(GroupingSet);
node_dispatch(WindowClause);
node_dispatch(RowMarkClause);
node_dispatch(WithClause);
node_dispatch(InferClause);
node_dispatch(OnConflictClause);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(CTESearchClause);
node_dispatch(CTECycleClause);
#endif
node_dispatch(CommonTableExpr);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(MergeWhenClause);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_dispatch(ReturningOption);
node_dispatch(ReturningClause);
#endif
node_dispatch(TriggerTransition);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(JsonOutput);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(JsonArgument);
node_dispatch(JsonFuncExpr);
node_dispatch(JsonTablePathSpec);
node_dispatch(JsonTable);
node_dispatch(JsonTableColumn);
node_dispatch(JsonKeyValue);
node_dispatch(JsonParseExpr);
node_dispatch(JsonScalarExpr);
node_dispatch(JsonSerializeExpr);
#endif
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(JsonObjectConstructor);
node_dispatch(JsonArrayConstructor);
node_dispatch(JsonArrayQueryConstructor);
node_dispatch(JsonAggConstructor);
node_dispatch(JsonObjectAgg);
node_dispatch(JsonArrayAgg);
#endif
node_dispatch(RawStmt);
node_dispatch(InsertStmt);
node_dispatch(DeleteStmt);
node_dispatch(UpdateStmt);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(MergeStmt);
#endif
node_dispatch(SelectStmt);
node_dispatch(SetOperationStmt);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(ReturnStmt);
#endif
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(PLAssignStmt);
#endif
node_dispatch(CreateSchemaStmt);
node_dispatch(AlterTableStmt);
node_dispatch(AlterTableCmd);
#if PG_MAJORVERSION_NUM >= 18
node_dispatch(ATAlterConstraint);
#endif
node_dispatch(ReplicaIdentityStmt);
node_dispatch(AlterCollationStmt);
node_dispatch(AlterDomainStmt);
node_dispatch(GrantStmt);
node_dispatch(ObjectWithArgs);
node_dispatch(AccessPriv);
node_dispatch(GrantRoleStmt);
node_dispatch(AlterDefaultPrivilegesStmt);
node_dispatch(CopyStmt);
node_dispatch(VariableSetStmt);
node_dispatch(VariableShowStmt);
node_dispatch(CreateStmt);
node_dispatch(Constraint);
node_dispatch(CreateTableSpaceStmt);
node_dispatch(DropTableSpaceStmt);
node_dispatch(AlterTableSpaceOptionsStmt);
node_dispatch(AlterTableMoveAllStmt);
node_dispatch(CreateExtensionStmt);
node_dispatch(AlterExtensionStmt);
node_dispatch(AlterExtensionContentsStmt);
node_dispatch(CreateFdwStmt);
node_dispatch(AlterFdwStmt);
node_dispatch(CreateForeignServerStmt);
node_dispatch(AlterForeignServerStmt);
node_dispatch(CreateForeignTableStmt);
node_dispatch(CreateUserMappingStmt);
node_dispatch(AlterUserMappingStmt);
node_dispatch(DropUserMappingStmt);
node_dispatch(ImportForeignSchemaStmt);
node_dispatch(CreatePolicyStmt);
node_dispatch(AlterPolicyStmt);
node_dispatch(CreateAmStmt);
node_dispatch(CreateTrigStmt);
node_dispatch(CreateEventTrigStmt);
node_dispatch(AlterEventTrigStmt);
node_dispatch(CreatePLangStmt);
node_dispatch(CreateRoleStmt);
node_dispatch(AlterRoleStmt);
node_dispatch(AlterRoleSetStmt);
node_dispatch(DropRoleStmt);
node_dispatch(CreateSeqStmt);
node_dispatch(AlterSeqStmt);
node_dispatch(DefineStmt);
node_dispatch(CreateDomainStmt);
node_dispatch(CreateOpClassStmt);
node_dispatch(CreateOpClassItem);
node_dispatch(CreateOpFamilyStmt);
node_dispatch(AlterOpFamilyStmt);
node_dispatch(DropStmt);
node_dispatch(TruncateStmt);
node_dispatch(CommentStmt);
node_dispatch(SecLabelStmt);
node_dispatch(DeclareCursorStmt);
node_dispatch(ClosePortalStmt);
node_dispatch(FetchStmt);
node_dispatch(IndexStmt);
node_dispatch(CreateStatsStmt);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(StatsElem);
#endif
node_dispatch(AlterStatsStmt);
node_dispatch(CreateFunctionStmt);
node_dispatch(FunctionParameter);
node_dispatch(AlterFunctionStmt);
node_dispatch(DoStmt);
node_dispatch(InlineCodeBlock);
node_dispatch(CallStmt);
node_dispatch(CallContext);
node_dispatch(RenameStmt);
node_dispatch(AlterObjectDependsStmt);
node_dispatch(AlterObjectSchemaStmt);
node_dispatch(AlterOwnerStmt);
node_dispatch(AlterOperatorStmt);
node_dispatch(AlterTypeStmt);
node_dispatch(RuleStmt);
node_dispatch(NotifyStmt);
node_dispatch(ListenStmt);
node_dispatch(UnlistenStmt);
node_dispatch(TransactionStmt);
node_dispatch(CompositeTypeStmt);
node_dispatch(CreateEnumStmt);
node_dispatch(CreateRangeStmt);
node_dispatch(AlterEnumStmt);
node_dispatch(ViewStmt);
node_dispatch(LoadStmt);
node_dispatch(CreatedbStmt);
node_dispatch(AlterDatabaseStmt);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(AlterDatabaseRefreshCollStmt);
#endif
node_dispatch(AlterDatabaseSetStmt);
node_dispatch(DropdbStmt);
node_dispatch(AlterSystemStmt);
node_dispatch(ClusterStmt);
node_dispatch(VacuumStmt);
node_dispatch(VacuumRelation);
node_dispatch(ExplainStmt);
node_dispatch(CreateTableAsStmt);
node_dispatch(RefreshMatViewStmt);
node_dispatch(CheckPointStmt);
node_dispatch(DiscardStmt);
node_dispatch(LockStmt);
node_dispatch(ConstraintsSetStmt);
node_dispatch(ReindexStmt);
node_dispatch(CreateConversionStmt);
node_dispatch(CreateCastStmt);
node_dispatch(CreateTransformStmt);
node_dispatch(PrepareStmt);
node_dispatch(ExecuteStmt);
node_dispatch(DeallocateStmt);
node_dispatch(DropOwnedStmt);
node_dispatch(ReassignOwnedStmt);
node_dispatch(AlterTSDictionaryStmt);
node_dispatch(AlterTSConfigurationStmt);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(PublicationTable);
node_dispatch(PublicationObjSpec);
#endif
node_dispatch(CreatePublicationStmt);
node_dispatch(AlterPublicationStmt);
node_dispatch(CreateSubscriptionStmt);
node_dispatch(AlterSubscriptionStmt);
node_dispatch(DropSubscriptionStmt);
node_dispatch(PlannerGlobal);
node_dispatch(PlannerInfo);
node_dispatch(RelOptInfo);
node_dispatch(IndexOptInfo);
node_dispatch(ForeignKeyOptInfo);
node_dispatch(StatisticExtInfo);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(JoinDomain);
#endif
node_dispatch(EquivalenceClass);
node_dispatch(EquivalenceMember);
node_dispatch(PathKey);
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(GroupByOrdering);
#endif
node_dispatch(PathTarget);
node_dispatch(ParamPathInfo);
node_dispatch(Path);
node_dispatch(IndexPath);
node_dispatch(IndexClause);
node_dispatch(BitmapHeapPath);
node_dispatch(BitmapAndPath);
node_dispatch(BitmapOrPath);
node_dispatch(TidPath);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(TidRangePath);
#endif
node_dispatch(SubqueryScanPath);
node_dispatch(ForeignPath);
node_dispatch(CustomPath);
node_dispatch(AppendPath);
node_dispatch(MergeAppendPath);
node_dispatch(GroupResultPath);
node_dispatch(MaterialPath);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(MemoizePath);
#endif
node_dispatch(UniquePath);
node_dispatch(GatherPath);
node_dispatch(GatherMergePath);
node_dispatch(NestPath);
node_dispatch(MergePath);
node_dispatch(HashPath);
node_dispatch(ProjectionPath);
node_dispatch(ProjectSetPath);
node_dispatch(SortPath);
node_dispatch(IncrementalSortPath);
node_dispatch(GroupPath);
#if PG_MAJORVERSION_NUM < 19
node_dispatch(UpperUniquePath);
#endif
node_dispatch(AggPath);
node_dispatch(GroupingSetData);
node_dispatch(RollupData);
node_dispatch(GroupingSetsPath);
node_dispatch(MinMaxAggPath);
node_dispatch(WindowAggPath);
node_dispatch(SetOpPath);
node_dispatch(RecursiveUnionPath);
node_dispatch(LockRowsPath);
node_dispatch(ModifyTablePath);
node_dispatch(LimitPath);
node_dispatch(RestrictInfo);
node_dispatch(PlaceHolderVar);
node_dispatch(SpecialJoinInfo);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(OuterJoinClauseInfo);
#endif
node_dispatch(AppendRelInfo);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(RowIdentityVarInfo);
#endif
node_dispatch(PlaceHolderInfo);
node_dispatch(MinMaxAggInfo);
node_dispatch(PlannerParamItem);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(AggInfo);
node_dispatch(AggTransInfo);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_dispatch(UniqueRelInfo);
#endif
node_dispatch(PlannedStmt);
node_dispatch(Result);
node_dispatch(ProjectSet);
node_dispatch(ModifyTable);
node_dispatch(Append);
node_dispatch(MergeAppend);
node_dispatch(RecursiveUnion);
node_dispatch(BitmapAnd);
node_dispatch(BitmapOr);
node_dispatch(SeqScan);
node_dispatch(SampleScan);
node_dispatch(IndexScan);
node_dispatch(IndexOnlyScan);
node_dispatch(BitmapIndexScan);
node_dispatch(BitmapHeapScan);
node_dispatch(TidScan);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(TidRangeScan);
#endif
node_dispatch(SubqueryScan);
node_dispatch(FunctionScan);
node_dispatch(ValuesScan);
node_dispatch(TableFuncScan);
node_dispatch(CteScan);
node_dispatch(NamedTuplestoreScan);
node_dispatch(WorkTableScan);
node_dispatch(ForeignScan);
node_dispatch(CustomScan);
node_dispatch(NestLoop);
node_dispatch(NestLoopParam);
node_dispatch(MergeJoin);
node_dispatch(HashJoin);
node_dispatch(Material);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(Memoize);
#endif
node_dispatch(Sort);
node_dispatch(IncrementalSort);
node_dispatch(Group);
node_dispatch(Agg);
node_dispatch(WindowAgg);
node_dispatch(Unique);
node_dispatch(Gather);
node_dispatch(GatherMerge);
node_dispatch(Hash);
node_dispatch(SetOp);
node_dispatch(LockRows);
node_dispatch(Limit);
node_dispatch(PlanRowMark);
node_dispatch(PartitionPruneInfo);
node_dispatch(PartitionedRelPruneInfo);
node_dispatch(PartitionPruneStepOp);
node_dispatch(PartitionPruneStepCombine);
node_dispatch(PlanInvalItem);
node_dispatch(ExprState);
node_dispatch(IndexInfo);
node_dispatch(ExprContext);
node_dispatch(ReturnSetInfo);
node_dispatch(ProjectionInfo);
node_dispatch(JunkFilter);
node_dispatch(OnConflictSetState);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(MergeActionState);
#endif
node_dispatch(ResultRelInfo);
node_dispatch(EState);
node_dispatch(WindowFuncExprState);
node_dispatch(SetExprState);
node_dispatch(SubPlanState);
node_dispatch(DomainConstraintState);
node_dispatch(ResultState);
node_dispatch(ProjectSetState);
node_dispatch(ModifyTableState);
node_dispatch(AppendState);
node_dispatch(MergeAppendState);
node_dispatch(RecursiveUnionState);
node_dispatch(BitmapAndState);
node_dispatch(BitmapOrState);
node_dispatch(ScanState);
node_dispatch(SeqScanState);
node_dispatch(SampleScanState);
node_dispatch(IndexScanState);
node_dispatch(IndexOnlyScanState);
node_dispatch(BitmapIndexScanState);
node_dispatch(BitmapHeapScanState);
node_dispatch(TidScanState);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(TidRangeScanState);
#endif
node_dispatch(SubqueryScanState);
node_dispatch(FunctionScanState);
node_dispatch(ValuesScanState);
node_dispatch(TableFuncScanState);
node_dispatch(CteScanState);
node_dispatch(NamedTuplestoreScanState);
node_dispatch(WorkTableScanState);
node_dispatch(ForeignScanState);
node_dispatch(CustomScanState);
node_dispatch(JoinState);
node_dispatch(NestLoopState);
node_dispatch(MergeJoinState);
node_dispatch(HashJoinState);
node_dispatch(MaterialState);
#if PG_MAJORVERSION_NUM >= 14
node_dispatch(MemoizeState);
#endif
node_dispatch(SortState);
node_dispatch(IncrementalSortState);
node_dispatch(GroupState);
node_dispatch(AggState);
node_dispatch(WindowAggState);
node_dispatch(UniqueState);
node_dispatch(GatherState);
node_dispatch(GatherMergeState);
node_dispatch(HashState);
node_dispatch(SetOpState);
node_dispatch(LockRowsState);
node_dispatch(LimitState);
node_dispatch(IndexAmRoutine);
node_dispatch(TableAmRoutine);
node_dispatch(TsmRoutine);
node_dispatch(EventTriggerData);
node_dispatch(TriggerData);
node_dispatch(TupleTableSlot);
node_dispatch(FdwRoutine);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(Bitmapset);
#endif
node_dispatch(ExtensibleNode);
#if PG_MAJORVERSION_NUM >= 16
node_dispatch(ErrorSaveContext);
#endif
node_dispatch(IdentifySystemCmd);
node_dispatch(BaseBackupCmd);
node_dispatch(CreateReplicationSlotCmd);
node_dispatch(DropReplicationSlotCmd);
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(AlterReplicationSlotCmd);
#endif
node_dispatch(StartReplicationCmd);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(ReadReplicationSlotCmd);
#endif
node_dispatch(TimeLineHistoryCmd);
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(UploadManifestCmd);
#endif
node_dispatch(SupportRequestSimplify);
node_dispatch(SupportRequestSelectivity);
node_dispatch(SupportRequestCost);
node_dispatch(SupportRequestRows);
node_dispatch(SupportRequestIndexCondition);
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(SupportRequestWFuncMonotonic);
#endif
#if PG_MAJORVERSION_NUM >= 17
node_dispatch(SupportRequestOptimizeWindowClause);
#endif
#if PG_MAJORVERSION_NUM >= 18
node_dispatch(SupportRequestModifyInPlace);
#endif
#if PG_MAJORVERSION_NUM >= 15
node_dispatch(Integer);
node_dispatch(Float);
node_dispatch(Boolean);
node_dispatch(String);
node_dispatch(BitString);
#endif
node_dispatch(ForeignKeyCacheInfo);
  default:
    break;
  }
  if constexpr (requires {
                  std::declval<Visitor>()(std::declval<nodes::unknown_node<decltype(node)>>());
                }) {
    visitor(nodes::unknown_node<decltype(node)>{node});
  } else {
    throw std::runtime_error("unknown node tag");
  }
}

template <typename T>
concept walker_implementation = requires(T t, ::Node *node, ::tree_walker_callback cb, void *ctx) {
  { t(node, cb, ctx) } -> std::same_as<bool>;
};

template <typename T> struct node_walker {
  void operator()(T &node, auto &&visitor, const walker_implementation auto &walker)
      requires covering_node<T>
  {
    auto *recasted_node = reinterpret_cast<::Node *>(node.as_ptr());
    (*this)(recasted_node, visitor, walker);
  }

  void operator()(::Node *recasted_node, auto &&visitor, const walker_implementation auto &walker) {
    /// In theory, this Boost PFR could have worked, but in practice some structures are
    /// way too large to introspect. Also, makes the binary really large.
    /// TODO: wait for C++26/P3435 and see if that would be any better than hand-crafted walkers
    /// from PG

    struct _ctx {
      decltype(visitor) _visitor;
    };
    _ctx c{std::forward<decltype(visitor)>(visitor)};
    ffi_guard{walker}(
        recasted_node,
        [](::Node *node, void *ctx) {
          if (node == nullptr) {
            return false;
          }
          _ctx *c = reinterpret_cast<_ctx *>(ctx);
          if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(visitor)>>) {
            cppgres::visit_node(node, *c->_visitor);
          } else {
            cppgres::visit_node(node, c->_visitor);
          }
          return false;
        },
        &c);
  }
};

template <> struct node_walker<nodes::RawStmt> {
  void operator()(nodes::RawStmt &node, auto &&visitor, const walker_implementation auto &walker) {
    if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(visitor)>>) {
      cppgres::visit_node(node.as_ref().stmt, *visitor);
    } else {
      cppgres::visit_node(node.as_ref().stmt, visitor);
    }
  }
};

template <> struct node_walker<nodes::List> {
  void operator()(nodes::List &node, auto &&visitor, const walker_implementation auto &walker) {
    ::ListCell *lc;
    ::List *node_p = node.as_ptr();
    if (nodeTag(node_p) == T_List) {
      foreach (lc, node_p) {
        void *node = lfirst(lc);
        if constexpr (std::is_pointer_v<std::remove_cvref_t<decltype(visitor)>>) {
          cppgres::visit_node(node, *visitor);
        } else {
          cppgres::visit_node(node, visitor);
        }
      }
    }
  }
};

template <covering_node T> struct raw_expr_node_walker {
  void operator()(T &node, auto &&visitor) {
    _walker(node, std::forward<decltype(visitor)>(visitor),
#if PG_MAJORVERSION_NUM < 16
            reinterpret_cast<bool (*)(::Node *, ::tree_walker_callback, void *)>(
                raw_expression_tree_walker)
#else
            ::raw_expression_tree_walker_impl
#endif
    );
  }

private:
  node_walker<T> _walker;
};

template <typename T> struct expr_node_walker {

  void operator()(T &node, auto &&visitor) {
    _walker(node, std::forward<decltype(visitor)>(visitor),
#if PG_MAJORVERSION_NUM < 16
            reinterpret_cast<bool (*)(::Node *, ::tree_walker_callback, void *)>(
                ::expression_tree_walker)
#else
            ::expression_tree_walker_impl
#endif
    );
  }

private:
  node_walker<T> _walker;
};

template <> struct expr_node_walker<nodes::Query> {
  expr_node_walker() {}
  explicit expr_node_walker(int flags) : _flags(flags) {}
  void operator()(nodes::Query &node, auto &&visitor) {
    _walker(node, std::forward<decltype(visitor)>(visitor),
            [this](auto query, auto cb, auto ctx) -> bool {
              return
#if PG_MAJORVERSION_NUM < 16
                  reinterpret_cast<bool (*)(::Query *, ::tree_walker_callback, void *, int)>(
                      ::query_tree_walker)(reinterpret_cast<::Query *>(query), cb, ctx, _flags);
#else
                  ::query_tree_walker_impl
                  (reinterpret_cast<::Query *>(query), cb, ctx, _flags);
#endif
            });
  }

private:
  node_walker<nodes::Query> _walker;
  int _flags = QTW_EXAMINE_SORTGROUP | QTW_EXAMINE_RTES_BEFORE;
};

#undef node_dispatch

} // namespace cppgres

#include <future>
#include <queue>
#include <type_traits>


#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#elif __APPLE__
#include <pthread.h>
#endif

namespace cppgres {

#if defined(__linux__)
static inline bool is_main_thread() { return gettid() == getpid(); }
#elif defined(__APPLE__)
static inline bool is_main_thread() { return pthread_main_np() != 0; }
#else
#warning "is_main_thread() not implemented"
static inline bool is_main_thread() { return false; }
#endif

/**
 * @brief Single-threaded Postgres workload worker
 *
 * @warning Use extreme caution and care when handling workload – ensure the worker does not
 *          outlive the intended lifetime – and receives no interference – that is, no other
 *          threads should be doing any Postgres workloads while this worker is alive.
 */
struct worker {
  worker() : done(false), terminated(false) {}

  ~worker() { terminate(); }

  void terminate() {
    {
      std::scoped_lock lock(mutex);
      if (terminated)
        return;
      done = true;
      cv.notify_one();
    }
  }

  template <typename F, typename... Args>
  auto post(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        [f = std::move(f), ... args = std::move(args)]() { return f(args...); });
    std::future<ReturnType> result = task->get_future();

    {
      std::scoped_lock lock(mutex);
      tasks.emplace([task]() { (*task)(); });
    }
    cv.notify_one();
    return result;
  }

  /**
   * @brief Run the worker
   *
   * @throws std::runtime_error if called on a secondary thread
   */
  void run() {
    if (!is_main_thread()) {
      throw std::runtime_error("Worker can only run on main thread");
    }
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]() { return done.load() || !tasks.empty(); });
        if (done.load() && tasks.empty())
          break;
        task = std::move(tasks.front());
        tasks.pop();
      }
      task();
    }
    terminated = true;
  }

private:
  std::mutex mutex;
  std::condition_variable cv;
  std::queue<std::function<void()>> tasks;
  std::atomic<bool> done;
  std::atomic<bool> terminated;
};

} // namespace cppgres

/**
 * @brief Export a C++ function as a Postgres function.
 *
 * Its argument types must conform to the @ref cppgres::convertible_from_nullable_datum concept and
 * its return type must conform to the @ref cppgres::convertible_into_nullable_datum or
 * @ref cppgres::datumable_iterator concepts. This requirement is inherited from @ref
 * cppgres::postgres_function.
 *
 * \arg name Name to export it under
 * \arg function C++ function or lambda
 *
 * \note You no longer need to use PG_FUNCTION_INFO_V1 macro.
 *
 */
#define postgres_function(name, function)                                                          \
  extern "C" {                                                                                     \
  PG_FUNCTION_INFO_V1(name);                                                                       \
  Datum name(PG_FUNCTION_ARGS) { return cppgres::postgres_function(function)(fcinfo); }            \
  }

#endif // cppgres_hpp
