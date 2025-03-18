// clang-format off
#ifndef __STD_GENERATOR_INCLUDED
#define __STD_GENERATOR_INCLUDED
///////////////////////////////////////////////////////////////////////////////
// Reference implementation of std::generator proposal P2168.
//
// See https://wg21.link/P2168 for details.
//
///////////////////////////////////////////////////////////////////////////////
// Copyright Lewis Baker, Corentin Jabot
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0.
// (See accompanying file LICENSE or http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if __has_include(<coroutine>)
#include <coroutine>
#else
// Fallback for older experimental implementations of coroutines.
#include <experimental/coroutine>
namespace std {
using std::experimental::coroutine_handle;
using std::experimental::coroutine_traits;
using std::experimental::noop_coroutine;
using std::experimental::suspend_always;
using std::experimental::suspend_never;
} // namespace std
#endif

#include <exception>
#include <iterator>
#include <new>
#include <type_traits>
#include <utility>
#include <concepts>
#include <cassert>

#if __has_include(<ranges>)
#  include <ranges>
#else

// Placeholder implementation of the bits we need from <ranges> header
// when we don't have the <ranges> header (e.g. Clang 12 and earlier).
namespace std {

// Don't create naming conflicts with recent libc++ which defines std::iter_reference_t
// in <iterator> but doesn't yet provide a <ranges> header.
template <typename _T>
using __iter_reference_t = decltype(*std::declval<_T&>());

template <typename _T>
using iter_value_t =
    typename std::iterator_traits<std::remove_cvref_t<_T>>::value_type;

namespace ranges {

namespace __begin {
void begin();

struct _fn {
    template <typename _Range>
    requires requires(_Range& __r) {
        __r.begin();
    }
    auto operator()(_Range&& __r) const
        noexcept(noexcept(__r.begin()))
        -> decltype(__r.begin()) {
        return __r.begin();
    }

    template <typename _Range>
    requires
        (!requires(_Range& __r) { __r.begin(); }) &&
        requires(_Range& __r) { begin(__r); }
    auto operator()(_Range&& __r) const
        noexcept(noexcept(begin(__r)))
        -> decltype(begin(__r)) {
        return begin(__r);
    }
};

} // namespace __begin

inline namespace __begin_cpo {
inline constexpr __begin::_fn begin = {};
}

namespace __end {
void end();

struct _fn {
    template <typename _Range>
    requires requires(_Range& __r) { __r.end(); }
    auto operator()(_Range&& __r) const
        noexcept(noexcept(__r.end()))
        -> decltype(__r.end()) {
        return __r.end();
    }

    template <typename _Range>
    requires
        (!requires(_Range& __r) { __r.end(); }) &&
        requires(_Range& __r) { end(__r); }
    auto operator()(_Range&& __r) const
        noexcept(noexcept(end(__r)))
        -> decltype(end(__r)) {
        return end(__r);
    }
};
} // namespace __end

inline namespace _end_cpo {
inline constexpr __end::_fn end = {};
}

template <typename _Range>
using iterator_t = decltype(begin(std::declval<_Range>()));

template <typename _Range>
using sentinel_t = decltype(end(std::declval<_Range>()));

template <typename _Range>
using range_reference_t = __iter_reference_t<iterator_t<_Range>>;

template <typename _Range>
using range_value_t = iter_value_t<iterator_t<_Range>>;

template <typename _T>
concept range = requires(_T& __t) {
    ranges::begin(__t);
    ranges::end(__t);
};

} // namespace ranges
} // namespace std

#endif // !__has_include(<ranges>)


namespace std {

template <typename _T>
class __manual_lifetime {
  public:
    __manual_lifetime() noexcept {}
    ~__manual_lifetime() {}

    template <typename... _Args>
    _T& construct(_Args&&... __args) noexcept(std::is_nothrow_constructible_v<_T, _Args...>) {
        return *::new (static_cast<void*>(std::addressof(__value_))) _T((_Args&&)__args...);
    }

    void destruct() noexcept(std::is_nothrow_destructible_v<_T>) {
        __value_.~_T();
    }

    _T& get() & noexcept {
        return __value_;
    }
    _T&& get() && noexcept {
        return static_cast<_T&&>(__value_);
    }
    const _T& get() const & noexcept {
        return __value_;
    }
    const _T&& get() const && noexcept {
        return static_cast<const _T&&>(__value_);
    }

  private:
    union {
        std::remove_const_t<_T> __value_;
    };
};

template <typename _T>
class __manual_lifetime<_T&> {
  public:
    __manual_lifetime() noexcept : __value_(nullptr) {}
    ~__manual_lifetime() {}

    _T& construct(_T& __value) noexcept {
        __value_ = std::addressof(__value);
        return __value;
    }

    void destruct() noexcept {}

    _T& get() const noexcept {
        return *__value_;
    }

  private:
    _T* __value_;
};

template <typename _T>
class __manual_lifetime<_T&&> {
  public:
    __manual_lifetime() noexcept : __value_(nullptr) {}
    ~__manual_lifetime() {}

    _T&& construct(_T&& __value) noexcept {
        __value_ = std::addressof(__value);
        return static_cast<_T&&>(__value);
    }

    void destruct() noexcept {}

    _T&& get() const noexcept {
        return static_cast<_T&&>(*__value_);
    }

  private:
    _T* __value_;
};

struct use_allocator_arg {};

namespace ranges {

template <typename _Rng, typename _Allocator = use_allocator_arg>
struct elements_of {
    explicit constexpr elements_of(_Rng&& __rng) noexcept
    requires std::is_default_constructible_v<_Allocator>
    : __range(static_cast<_Rng&&>(__rng))
    {}

    constexpr elements_of(_Rng&& __rng, _Allocator&& __alloc) noexcept
    : __range((_Rng&&)__rng), __alloc((_Allocator&&)__alloc) {}

    constexpr elements_of(elements_of&&) noexcept = default;

    constexpr elements_of(const elements_of &) = delete;
    constexpr elements_of &operator=(const elements_of &) = delete;
    constexpr elements_of &operator=(elements_of &&) = delete;

    constexpr _Rng&& get() noexcept {
        return static_cast<_Rng&&>(__range);
    }

    constexpr _Allocator get_allocator() const noexcept {
        return __alloc;
    }

private:
    [[no_unique_address]] _Allocator __alloc; // \expos
    _Rng && __range; // \expos
};

template <typename _Rng>
elements_of(_Rng &&) -> elements_of<_Rng>;

template <typename _Rng, typename Allocator>
elements_of(_Rng &&, Allocator&&) -> elements_of<_Rng, Allocator>;

} // namespace ranges

template <typename _Alloc>
static constexpr bool __allocator_needs_to_be_stored =
    !std::allocator_traits<_Alloc>::is_always_equal::value ||
    !std::is_default_constructible_v<_Alloc>;

// Round s up to next multiple of a.
constexpr size_t __aligned_allocation_size(size_t s, size_t a) {
    return (s + a - 1) & ~(a - 1);
}


template <typename _Ref,
          typename _Value = std::remove_cvref_t<_Ref>,
          typename _Allocator = use_allocator_arg>
class generator;

template<typename _Alloc>
class __promise_base_alloc {
    static constexpr std::size_t __offset_of_allocator(std::size_t __frameSize) noexcept {
        return __aligned_allocation_size(__frameSize, alignof(_Alloc));
    }

    static constexpr std::size_t __padded_frame_size(std::size_t __frameSize) noexcept {
        return __offset_of_allocator(__frameSize) + sizeof(_Alloc);
    }

    static _Alloc& __get_allocator(void* __frame, std::size_t __frameSize) noexcept {
        return *reinterpret_cast<_Alloc*>(
            static_cast<char*>(__frame) + __offset_of_allocator(__frameSize));
    }

public:
    template<typename... _Args>
    static void* operator new(std::size_t __frameSize, std::allocator_arg_t, _Alloc __alloc, _Args&...) {
        void* __frame = __alloc.allocate(__padded_frame_size(__frameSize));

        // Store allocator at end of the coroutine frame.
        // Assuming the allocator's move constructor is non-throwing (a requirement for allocators)
        ::new (static_cast<void*>(std::addressof(__get_allocator(__frame, __frameSize)))) _Alloc(std::move(__alloc));

        return __frame;
    }

    template<typename _This, typename... _Args>
    static void* operator new(std::size_t __frameSize, _This&, std::allocator_arg_t, _Alloc __alloc, _Args&...) {
        return __promise_base_alloc::operator new(__frameSize, std::allocator_arg, std::move(__alloc));
    }

    static void operator delete(void* __ptr, std::size_t __frameSize) noexcept {
        _Alloc& __alloc = __get_allocator(__ptr, __frameSize);
        _Alloc __localAlloc(std::move(__alloc));
        __alloc.~Alloc();
        __localAlloc.deallocate(static_cast<std::byte*>(__ptr), __padded_frame_size(__frameSize));
    }
};

template<typename _Alloc>
    requires (!__allocator_needs_to_be_stored<_Alloc>)
class __promise_base_alloc<_Alloc> {
public:
    static void* operator new(std::size_t __size) {
        _Alloc __alloc;
        return __alloc.allocate(__size);
    }

    static void operator delete(void* __ptr, std::size_t __size) noexcept {
        _Alloc __alloc;
        __alloc.deallocate(static_cast<std::byte*>(__ptr), __size);
    }
};

template<typename _Ref>
struct __generator_promise_base
{
    template <typename _Ref2, typename _Value, typename _Alloc>
    friend class generator;

    __generator_promise_base* __root_;
    std::coroutine_handle<> __parentOrLeaf_;
    // Note: Using manual_lifetime here to avoid extra calls to exception_ptr
    // constructor/destructor in cases where it is not needed (i.e. where this
    // generator coroutine is not used as a nested coroutine).
    // This member is lazily constructed by the __yield_sequence_awaiter::await_suspend()
    // method if this generator is used as a nested generator.
    __manual_lifetime<std::exception_ptr> __exception_;
    __manual_lifetime<_Ref> __value_;

    explicit __generator_promise_base(std::coroutine_handle<> thisCoro) noexcept
        : __root_(this)
        , __parentOrLeaf_(thisCoro)
    {}

    ~__generator_promise_base() {
        if (__root_ != this) {
            // This coroutine was used as a nested generator and so will
            // have constructed its __exception_ member which needs to be
            // destroyed here.
            __exception_.destruct();
        }
    }

    std::suspend_always initial_suspend() noexcept {
        return {};
    }

    void return_void() noexcept {}

    void unhandled_exception() {
        if (__root_ != this) {
            __exception_.get() = std::current_exception();
        } else {
            throw;
        }
    }

    // Transfers control back to the parent of a nested coroutine
    struct __final_awaiter {
        bool await_ready() noexcept {
            return false;
        }

        template <typename _Promise>
        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<_Promise> __h) noexcept {
            _Promise& __promise = __h.promise();
            __generator_promise_base& __root = *__promise.__root_;
            if (&__root != &__promise) {
                auto __parent = __promise.__parentOrLeaf_;
                __root.__parentOrLeaf_ = __parent;
                return __parent;
            }
            return std::noop_coroutine();
        }

        void await_resume() noexcept {}
    };

    __final_awaiter final_suspend() noexcept {
        return {};
    }

    std::suspend_always yield_value(_Ref&& __x)
            noexcept(std::is_nothrow_move_constructible_v<_Ref>) {
        __root_->__value_.construct((_Ref&&)__x);
        return {};
    }

    template <typename _T>
    requires
        (!std::is_reference_v<_Ref>) &&
        std::is_convertible_v<_T, _Ref>
    std::suspend_always yield_value(_T&& __x)
            noexcept(std::is_nothrow_constructible_v<_Ref, _T>) {
        __root_->__value_.construct((_T&&)__x);
        return {};
    }

    template <typename _Gen>
    struct __yield_sequence_awaiter {
        _Gen __gen_;

        __yield_sequence_awaiter(_Gen&& __g) noexcept
            // Taking ownership of the generator ensures frame are destroyed
            // in the reverse order of their execution.
            : __gen_((_Gen&&)__g) {
        }

        bool await_ready() noexcept {
            return false;
        }

        // set the parent, root and exceptions pointer and
        // resume the nested
        template<typename _Promise>
        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<_Promise> __h) noexcept {
            __generator_promise_base& __current = __h.promise();
            __generator_promise_base& __nested = *__gen_.__get_promise();
            __generator_promise_base& __root = *__current.__root_;

            __nested.__root_ = __current.__root_;
            __nested.__parentOrLeaf_ = __h;

            // Lazily construct the __exception_ member here now that we
            // know it will be used as a nested generator. This will be
            // destroyed by the promise destructor.
            __nested.__exception_.construct();
            __root.__parentOrLeaf_ = __gen_.__get_coro();

            // Immediately resume the nested coroutine (nested generator)
            return __gen_.__get_coro();
        }

        void await_resume() {
            __generator_promise_base& __nestedPromise = *__gen_.__get_promise();
            if (__nestedPromise.__exception_.get()) {
                std::rethrow_exception(std::move(__nestedPromise.__exception_.get()));
            }
        }
    };

    template <typename _OValue, typename _OAlloc>
    __yield_sequence_awaiter<generator<_Ref, _OValue, _OAlloc>>
    yield_value(std::ranges::elements_of<generator<_Ref, _OValue, _OAlloc>> __g) noexcept {
        return std::move(__g).get();
    }

    template <std::ranges::range _Rng, typename _Allocator>
    __yield_sequence_awaiter<generator<_Ref, std::remove_cvref_t<_Ref>, _Allocator>>
    yield_value(std::ranges::elements_of<_Rng, _Allocator> && __x) {
        return [](allocator_arg_t, _Allocator alloc, auto && __rng) -> generator<_Ref, std::remove_cvref_t<_Ref>, _Allocator> {
            for(auto && e: __rng)
                co_yield static_cast<decltype(e)>(e);
        }(std::allocator_arg, __x.get_allocator(), std::forward<_Rng>(__x.get()));
    }

    void resume() {
        __parentOrLeaf_.resume();
    }

    // Disable use of co_await within this coroutine.
    void await_transform() = delete;
};

template<typename _Generator, typename _ByteAllocator, bool _ExplicitAllocator = false>
struct __generator_promise;

template<typename _Ref, typename _Value, typename _Alloc, typename _ByteAllocator, bool _ExplicitAllocator>
struct __generator_promise<generator<_Ref, _Value, _Alloc>, _ByteAllocator, _ExplicitAllocator> final
    : public __generator_promise_base<_Ref>
    , public __promise_base_alloc<_ByteAllocator> {
    __generator_promise() noexcept
    : __generator_promise_base<_Ref>(std::coroutine_handle<__generator_promise>::from_promise(*this))
    {}

    generator<_Ref, _Value, _Alloc> get_return_object() noexcept {
        return generator<_Ref, _Value, _Alloc>{
            std::coroutine_handle<__generator_promise>::from_promise(*this)
        };
    }

    using __generator_promise_base<_Ref>::yield_value;

    template <std::ranges::range _Rng>
    typename __generator_promise_base<_Ref>::template __yield_sequence_awaiter<generator<_Ref, _Value, _Alloc>>
    yield_value(std::ranges::elements_of<_Rng> && __x) {
        static_assert (!_ExplicitAllocator,
        "This coroutine has an explicit allocator specified with std::allocator_arg so an allocator needs to be passed "
        "explicitely to std::elements_of");
        return [](auto && __rng) -> generator<_Ref, _Value, _Alloc> {
            for(auto && e: __rng)
                co_yield static_cast<decltype(e)>(e);
        }(std::forward<_Rng>(__x.get()));
    }
};

template<typename _Alloc>
using __byte_allocator_t = typename std::allocator_traits<std::remove_cvref_t<_Alloc>>::template rebind_alloc<std::byte>;


// Type-erased allocator with default allocator behaviour.
template<typename _Ref, typename _Value, typename... _Args>
struct coroutine_traits<generator<_Ref, _Value>, _Args...> {
    using promise_type = __generator_promise<generator<_Ref, _Value>, std::allocator<std::byte>>;
};

// Type-erased allocator with std::allocator_arg parameter
template<typename _Ref, typename _Value, typename _Alloc, typename... _Args>
struct coroutine_traits<generator<_Ref, _Value>, allocator_arg_t, _Alloc, _Args...> {
private:
    using __byte_allocator = __byte_allocator_t<_Alloc>;
public:
    using promise_type = __generator_promise<generator<_Ref, _Value>, __byte_allocator, true /*explicit Allocator*/>;
};

// Type-erased allocator with std::allocator_arg parameter (non-static member functions)
template<typename _Ref, typename _Value, typename _This, typename _Alloc, typename... _Args>
struct coroutine_traits<generator<_Ref, _Value>, _This, allocator_arg_t, _Alloc, _Args...> {
private:
    using __byte_allocator = __byte_allocator_t<_Alloc>;
public:
    using promise_type = __generator_promise<generator<_Ref, _Value>, __byte_allocator,  true /*explicit Allocator*/>;
};

// Generator with specified allocator type
template<typename _Ref, typename _Value, typename _Alloc, typename... _Args>
struct coroutine_traits<generator<_Ref, _Value, _Alloc>, _Args...> {
    using __byte_allocator = __byte_allocator_t<_Alloc>;
public:
    using promise_type = __generator_promise<generator<_Ref, _Value, _Alloc>, __byte_allocator>;
};


// TODO :  make layout compatible promise casts possible
template <typename _Ref, typename _Value, typename _Alloc>
class generator {
    using __byte_allocator = __byte_allocator_t<_Alloc>;
public:
    using promise_type = __generator_promise<generator<_Ref, _Value, _Alloc>, __byte_allocator>;
    friend promise_type;
private:
    using __coroutine_handle = std::coroutine_handle<promise_type>;
public:

    generator() noexcept = default;

    generator(generator&& __other) noexcept
        : __coro_(std::exchange(__other.__coro_, {}))
        , __started_(std::exchange(__other.__started_, false)) {
    }

    ~generator() noexcept {
        if (__coro_) {
            if (__started_ && !__coro_.done()) {
                __coro_.promise().__value_.destruct();
            }
            __coro_.destroy();
        }
    }

    generator& operator=(generator && g) noexcept {
        swap(g);
        return *this;
    }

    void swap(generator& __other) noexcept {
        std::swap(__coro_, __other.__coro_);
        std::swap(__started_, __other.__started_);
    }

    struct sentinel {};

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = _Value;
        using reference = _Ref;
        using pointer = std::add_pointer_t<_Ref>;

        iterator() noexcept = default;
        iterator(const iterator &) = delete;

        iterator(iterator&& __other) noexcept
        : __coro_(std::exchange(__other.__coro_, {})) {
        }

        iterator& operator=(iterator&& __other) {
            std::swap(__coro_, __other.__coro_);
            return *this;
        }

        ~iterator() {
        }

        friend bool operator==(const iterator &it, sentinel) noexcept {
            return it.__coro_.done();
        }

        iterator &operator++() {
            __coro_.promise().__value_.destruct();
            __coro_.promise().resume();
            return *this;
        }
        void operator++(int) {
            (void)operator++();
        }

        reference operator*() const noexcept {
            return static_cast<reference>(__coro_.promise().__value_.get());
        }

      private:
        friend generator;

        explicit iterator(__coroutine_handle __coro) noexcept
        : __coro_(__coro) {}

        __coroutine_handle __coro_;
    };

    iterator begin() {
        assert(__coro_);
        assert(!__started_);
        __started_ = true;
        __coro_.resume();
        return iterator{__coro_};
    }

    sentinel end() noexcept {
        return {};
    }

private:
    explicit generator(__coroutine_handle __coro) noexcept
        : __coro_(__coro) {
    }

public: // to get around access restrictions for __yield_sequence_awaitable
    std::coroutine_handle<> __get_coro() noexcept { return __coro_; }
    promise_type* __get_promise() noexcept { return std::addressof(__coro_.promise()); }

private:
    __coroutine_handle __coro_;
    bool __started_ = false;
};

// Specialisation for type-erased allocator implementation.
template <typename _Ref, typename _Value>
class generator<_Ref, _Value, use_allocator_arg> {
    using __promise_base = __generator_promise_base<_Ref>;
public:

    generator() noexcept
        : __promise_(nullptr)
        , __coro_()
        , __started_(false)
    {}

    generator(generator&& __other) noexcept
        : __promise_(std::exchange(__other.__promise_, nullptr))
        , __coro_(std::exchange(__other.__coro_, {}))
        , __started_(std::exchange(__other.__started_, false)) {
    }

    ~generator() noexcept {
        if (__coro_) {
            if (__started_ && !__coro_.done()) {
                __promise_->__value_.destruct();
            }
            __coro_.destroy();
        }
    }

    generator& operator=(generator g) noexcept {
        swap(g);
        return *this;
    }

    void swap(generator& __other) noexcept {
        std::swap(__promise_, __other.__promise_);
        std::swap(__coro_, __other.__coro_);
        std::swap(__started_, __other.__started_);
    }

    struct sentinel {};

    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = _Value;
        using reference = _Ref;
        using pointer = std::add_pointer_t<_Ref>;

        iterator() noexcept = default;
        iterator(const iterator &) = delete;

        iterator(iterator&& __other) noexcept
        : __promise_(std::exchange(__other.__promise_, nullptr))
        , __coro_(std::exchange(__other.__coro_, {}))
        {}

        iterator& operator=(iterator&& __other) {
            __promise_ = std::exchange(__other.__promise_, nullptr);
            __coro_ = std::exchange(__other.__coro_, {});
            return *this;
        }

        ~iterator() = default;

        friend bool operator==(const iterator &it, sentinel) noexcept {
            return it.__coro_.done();
        }

        iterator& operator++() {
            __promise_->__value_.destruct();
            __promise_->resume();
            return *this;
        }

        void operator++(int) {
            (void)operator++();
        }

        reference operator*() const noexcept {
            return static_cast<reference>(__promise_->__value_.get());
        }

      private:
        friend generator;

        explicit iterator(__promise_base* __promise, std::coroutine_handle<> __coro) noexcept
        : __promise_(__promise)
        , __coro_(__coro)
        {}

        __promise_base* __promise_;
        std::coroutine_handle<> __coro_;
    };

    iterator begin() {
        assert(__coro_);
        assert(!__started_);
        __started_ = true;
        __coro_.resume();
        return iterator{__promise_, __coro_};
    }

    sentinel end() noexcept {
        return {};
    }

private:
    template<typename _Generator, typename _ByteAllocator, bool _ExplicitAllocator>
    friend struct __generator_promise;

    template<typename _Promise>
    explicit generator(std::coroutine_handle<_Promise> __coro) noexcept
        : __promise_(std::addressof(__coro.promise()))
        , __coro_(__coro)
    {}

public: // to get around access restrictions for __yield_sequence_awaitable
    std::coroutine_handle<> __get_coro() noexcept { return __coro_; }
    __promise_base* __get_promise() noexcept { return __promise_; }

private:
    __promise_base* __promise_;
    std::coroutine_handle<> __coro_;
    bool __started_ = false;
};

#if __has_include(<ranges>)
namespace ranges {

template <typename _T, typename _U, typename _Alloc>
constexpr inline bool enable_view<generator<_T, _U, _Alloc>> = true;

} // namespace ranges
#endif

} // namespace std

#endif // __STD_GENERATOR_INCLUDED
// clang-format on
