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

#ifndef AGRPC_DETAIL_RECIEVER_HPP
#define AGRPC_DETAIL_RECIEVER_HPP

#include "agrpc/detail/asioForward.hpp"

namespace agrpc::detail
{
template <class Receiver, class... Args>
void satisfy_receiver(Receiver&& receiver, Args&&... args) noexcept
{
    if constexpr (noexcept(detail::set_value(std::move(receiver), std::forward<Args>(args)...)))
    {
        detail::set_value(std::move(receiver), std::forward<Args>(args)...);
    }
    else
    {
        AGRPC_TRY { detail::set_value(std::move(receiver), std::forward<Args>(args)...); }
        AGRPC_CATCH(...) { detail::set_error(std::move(receiver), std::current_exception()); }
    }
}
}  // namespace agrpc::detail

#endif  // AGRPC_DETAIL_RECIEVER_HPP