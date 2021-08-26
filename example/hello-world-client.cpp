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

#include "protos/helloworld.grpc.pb.h"

#include <agrpc/asioGrpc.hpp>
#include <boost/asio/spawn.hpp>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

void read_initial_metadata(grpc::ClientAsyncResponseReader<helloworld::HelloReply>& reader,
                           boost::asio::yield_context& yield)
{
    // begin-snippet: read_initial_metadata-client-side
    bool read_ok = agrpc::read_initial_metadata(reader, yield);
    // end-snippet
}

int main()
{
    auto stub =
        helloworld::Greeter::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    // begin-snippet: create-grpc_context-client-side
    agrpc::GrpcContext grpc_context{std::make_unique<grpc::CompletionQueue>()};
    // end-snippet

    // begin-snippet: make-work-guard
    auto guard = boost::asio::make_work_guard(grpc_context);
    // end-snippet
    boost::asio::spawn(grpc_context,
                       [&](auto yield)
                       {
                           // begin-snippet: request-unary-client-side
                           grpc::ClientContext client_context;
                           helloworld::HelloRequest request;
                           request.set_name("world");
                           std::unique_ptr<grpc::ClientAsyncResponseReader<helloworld::HelloReply>> reader =
                               stub->AsyncSayHello(&client_context, request, agrpc::get_completion_queue(grpc_context));
                           // end-snippet
                           // begin-snippet: finish-client-side
                           helloworld::HelloReply response;
                           grpc::Status status;
                           bool finish_ok = agrpc::finish(*reader, response, status, yield);
                           // end-snippet
                       });

    grpc_context.run();
}