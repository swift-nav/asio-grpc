// Copyright 2022 Dennis Hezel
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef AGRPC_DETAIL_UTILITY_HPP
#define AGRPC_DETAIL_UTILITY_HPP

#include "agrpc/detail/config.hpp"

#include <type_traits>
#include <utility>

AGRPC_NAMESPACE_BEGIN()

namespace detail
{
template <class T>
using RemoveCvrefT = std::remove_cv_t<std::remove_reference_t<T>>;

struct Empty
{
    Empty() = default;
    Empty(const Empty&) = default;
    Empty(Empty&&) = default;
    Empty& operator=(const Empty&) = default;
    Empty& operator=(Empty&&) = default;
    ~Empty() = default;

    template <class Arg, class... Args>
    constexpr explicit Empty(Arg&&, Args&&...) noexcept
    {
    }
};

template <class First, class Second, bool = std::is_empty_v<Second> && !std::is_final_v<Second>>
class CompressedPair : private Second
{
  public:
    template <class T, class U>
    constexpr CompressedPair(T&& first, U&& second) noexcept(
        std::is_nothrow_constructible_v<First, T&&>&& std::is_nothrow_constructible_v<Second, U&&>)
        : Second(std::forward<U>(second)), first_(std::forward<T>(first))
    {
    }

    template <class T>
    constexpr explicit CompressedPair(T&& first) noexcept(
        std::is_nothrow_constructible_v<First, T&&>&& std::is_nothrow_default_constructible_v<Second>)
        : Second(), first_(std::forward<T>(first))
    {
    }

    constexpr auto& first() noexcept { return this->first_; }

    constexpr const auto& first() const noexcept { return this->first_; }

    constexpr Second& second() noexcept { return *this; }

    constexpr const Second& second() const noexcept { return *this; }

  private:
    First first_;
};

template <class First, class Second>
class CompressedPair<First, Second, false> final
{
  public:
    template <class T, class U>
    constexpr CompressedPair(T&& first, U&& second) noexcept(
        std::is_nothrow_constructible_v<First, T&&>&& std::is_nothrow_constructible_v<Second, U&&>)
        : first_(std::forward<T>(first)), second_(std::forward<U>(second))
    {
    }

    template <class T>
    constexpr explicit CompressedPair(T&& first) noexcept(
        std::is_nothrow_constructible_v<First, T&&>&& std::is_nothrow_default_constructible_v<Second>)
        : first_(std::forward<T>(first)), second_()
    {
    }

    constexpr auto& first() noexcept { return this->first_; }

    constexpr const auto& first() const noexcept { return this->first_; }

    constexpr auto& second() noexcept { return this->second_; }

    constexpr const auto& second() const noexcept { return this->second_; }

  private:
    First first_;
    Second second_;
};

template <class OnExit>
class ScopeGuard
{
  public:
    constexpr explicit ScopeGuard(OnExit on_exit) : on_exit(std::move(on_exit)) {}

    ScopeGuard(const ScopeGuard&) = delete;

    ScopeGuard(ScopeGuard&&) = delete;

    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ScopeGuard& operator=(ScopeGuard&&) = delete;

    ~ScopeGuard() noexcept
    {
        if (is_armed)
        {
            on_exit();
        }
    }

    constexpr void release() noexcept { is_armed = false; }

  private:
    OnExit on_exit;
    bool is_armed{true};
};

template <class T>
T forward_as(std::add_lvalue_reference_t<std::remove_reference_t<T>> u)
{
    if constexpr (std::is_rvalue_reference_v<T> || !std::is_reference_v<T>)
    {
        return std::move(u);
    }
    else
    {
        return u;
    }
}

template <class T, class... Args>
constexpr T* construct_at(T* ptr, Args&&... args)
{
    return ::new (const_cast<void*>(static_cast<const volatile void*>(ptr))) T{std::forward<Args>(args)...};
}

template <std::size_t MaxSize, std::size_t MaxAlignment, class Signature>
class TrivialFunction;

template <std::size_t MaxSize, std::size_t MaxAlignment, class ReturnType, class... Args>
class TrivialFunction<MaxSize, MaxAlignment, ReturnType(Args...)>
{
  public:
    template <class Function>
    explicit TrivialFunction(Function&& function)
        : function(
              [](void* self, Args... args)
              {
                  return std::invoke(*static_cast<detail::RemoveCvrefT<Function>*>(self),
                                     detail::forward_as<Args>(args)...);
              })
    {
        using DecayedFunction = detail::RemoveCvrefT<Function>;
        static_assert(MaxSize >= sizeof(DecayedFunction), "Function size exceeds storage capacity");
        static_assert(MaxAlignment >= alignof(DecayedFunction), "Function alignment exceeds storage capacity");
        static_assert(std::is_trivially_copyable_v<DecayedFunction>, "Function must be trivially copyable");
        detail::construct_at(reinterpret_cast<DecayedFunction*>(&storage), std::forward<Function>(function));
    }

    ReturnType operator()(Args... args) { return function(&storage, detail::forward_as<Args>(args)...); }

  private:
    using Storage = std::aligned_storage_t<MaxSize, MaxAlignment>;
    using Function = ReturnType (*)(void*, Args...);

    Storage storage;
    Function function;
};
}

AGRPC_NAMESPACE_END

#endif  // AGRPC_DETAIL_UTILITY_HPP
