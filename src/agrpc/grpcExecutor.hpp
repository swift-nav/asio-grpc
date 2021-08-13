// Copyright 2021 Dennis Hezel
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

#ifndef AGRPC_AGRPC_GRPCEXECUTOR_HPP
#define AGRPC_AGRPC_GRPCEXECUTOR_HPP

#include "agrpc/detail/asioForward.hpp"
#include "agrpc/detail/grpcContextInteraction.hpp"
#include "agrpc/detail/grpcExecutorBase.hpp"
#include "agrpc/detail/grpcExecutorOperation.hpp"
#include "agrpc/detail/grpcExecutorOptions.hpp"
#include "agrpc/detail/memory.hpp"
#include "agrpc/grpcContext.hpp"

#include <memory_resource>
#include <utility>

namespace agrpc
{
template <class Allocator = std::allocator<void>, std::uint32_t Options = detail::GrpcExecutorOptions::DEFAULT>
class BasicGrpcExecutor
    : public std::conditional_t<detail::is_outstanding_work_tracked(Options),
                                detail::GrpcExecutorWorkTrackerBase<Allocator>, detail::GrpcExecutorBase<Allocator>>
{
  private:
    using Base =
        std::conditional_t<detail::is_outstanding_work_tracked(Options), detail::GrpcExecutorWorkTrackerBase<Allocator>,
                           detail::GrpcExecutorBase<Allocator>>;

  public:
    using allocator_type = Allocator;

    constexpr explicit BasicGrpcExecutor(agrpc::GrpcContext& grpc_context) noexcept(
        std::is_nothrow_default_constructible_v<allocator_type>)
        : BasicGrpcExecutor(grpc_context, allocator_type{})
    {
    }

    constexpr BasicGrpcExecutor(agrpc::GrpcContext& grpc_context, allocator_type allocator) noexcept
        : Base(std::addressof(grpc_context), std::move(allocator))
    {
    }

    [[nodiscard]] constexpr auto& context() const noexcept { return *this->grpc_context(); }

    [[nodiscard]] constexpr allocator_type get_allocator() const
        noexcept(std::is_nothrow_copy_constructible_v<allocator_type>)
    {
        return this->allocator();
    }

    template <class OtherAllocator, std::uint32_t OtherOptions>
    [[nodiscard]] constexpr bool operator==(
        const agrpc::BasicGrpcExecutor<OtherAllocator, OtherOptions>& rhs) const noexcept
    {
        return this->grpc_context() == rhs.grpc_context() && this->allocator() == rhs.allocator();
    }

    template <class OtherAllocator, std::uint32_t OtherOptions>
    [[nodiscard]] constexpr bool operator!=(
        const agrpc::BasicGrpcExecutor<OtherAllocator, OtherOptions>& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    [[nodiscard]] bool running_in_this_thread() const noexcept
    {
        return detail::GrpcContextImplementation::running_in_this_thread(this->context());
    }

    template <class Function, class OtherAllocator>
    void dispatch(Function&& function, const OtherAllocator& other_allocator) const
    {
        detail::create_work<detail::is_blocking_never(Options)>(this->context(), std::forward<Function>(function),
                                                                other_allocator);
    }

    template <class Function, class OtherAllocator>
    void post(Function&& function, const OtherAllocator& other_allocator) const
    {
        detail::create_work<detail::is_blocking_never(Options)>(this->context(), std::forward<Function>(function),
                                                                other_allocator);
    }

    template <class Function, class OtherAllocator>
    void defer(Function&& function, const OtherAllocator& other_allocator) const
    {
        detail::create_work<detail::is_blocking_never(Options)>(this->context(), std::forward<Function>(function),
                                                                other_allocator);
    }

    template <class Function>
    void execute(Function&& function) const
    {
        detail::create_work<detail::is_blocking_never(Options)>(this->context(), std::forward<Function>(function),
                                                                this->allocator());
    }

    [[nodiscard]] constexpr auto require(asio::execution::blocking_t::possibly_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_blocking_never(Options, false)>
    {
        return {this->context(), this->allocator()};
    }

    [[nodiscard]] constexpr auto require(asio::execution::blocking_t::never_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_blocking_never(Options, true)>
    {
        return {this->context(), this->allocator()};
    }

    [[nodiscard]] constexpr auto prefer(asio::execution::relationship_t::fork_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_relationship_continuation(Options, false)>
    {
        return {this->context(), this->allocator()};
    }

    [[nodiscard]] constexpr auto prefer(asio::execution::relationship_t::continuation_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_relationship_continuation(Options, true)>
    {
        return {this->context(), this->allocator()};
    }

    [[nodiscard]] constexpr auto require(asio::execution::outstanding_work_t::tracked_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_outstanding_work_tracked(Options, true)>
    {
        return {this->context(), this->allocator()};
    }

    [[nodiscard]] constexpr auto require(asio::execution::outstanding_work_t::untracked_t) const noexcept
        -> BasicGrpcExecutor<Allocator, detail::set_outstanding_work_tracked(Options, false)>
    {
        return {this->context(), this->allocator()};
    }

    template <class OtherAllocator>
    [[nodiscard]] constexpr auto require(asio::execution::allocator_t<OtherAllocator> other_allocator) const noexcept
        -> BasicGrpcExecutor<OtherAllocator, Options>
    {
        return {this->context(), other_allocator.value()};
    }

    [[nodiscard]] constexpr auto require(asio::execution::allocator_t<void>) const noexcept
        -> BasicGrpcExecutor<std::allocator<void>, Options>
    {
        return {this->context()};
    }

    [[nodiscard]] static constexpr auto query(asio::execution::blocking_t) noexcept
    {
        if constexpr (detail::is_blocking_never(Options))
        {
            return asio::execution::blocking.never;
        }
        else
        {
            return asio::execution::blocking.possibly;
        }
    }

    [[nodiscard]] static constexpr auto query(asio::execution::mapping_t) noexcept
    {
        return asio::execution::mapping.thread;
    }

    [[nodiscard]] constexpr auto& query(asio::execution::context_t) const noexcept { return this->context(); }

    [[nodiscard]] static constexpr auto query(asio::execution::relationship_t) noexcept
    {
        if constexpr (detail::is_relationship_continuation(Options))
        {
            return asio::execution::relationship.continuation;
        }
        else
        {
            return asio::execution::relationship.fork;
        }
    }

    [[nodiscard]] static constexpr auto query(asio::execution::outstanding_work_t) noexcept
    {
        if constexpr (detail::is_outstanding_work_tracked(Options))
        {
            return asio::execution::outstanding_work.tracked;
        }
        else
        {
            return asio::execution::outstanding_work.untracked;
        }
    }

    template <class OtherAllocator>
    [[nodiscard]] constexpr auto query(asio::execution::allocator_t<OtherAllocator>) const noexcept
    {
        return this->get_allocator();
    }
};

using GrpcExecutor = agrpc::BasicGrpcExecutor<>;

namespace pmr
{
using GrpcExecutor = agrpc::BasicGrpcExecutor<std::pmr::polymorphic_allocator<std::byte>>;
}
}  // namespace agrpc

#endif  // AGRPC_AGRPC_GRPCEXECUTOR_HPP
