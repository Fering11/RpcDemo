# Demo RPC

A small C++ RPC framework built on Boost.Asio and Protocol Buffers.

The current version implements a minimal synchronous request-response flow:

- TCP message framing
- client-side blocking `call`
- protobuf `Request` / `Response` envelope
- compile-time method traits
- server-side method dispatch

## Build

From a Visual Studio developer shell:

```powershell
cmake --build out\build\x64-debug --target server
cmake --build out\build\x64-debug --target client
```

If the build directory does not exist yet:

```powershell
cmake --preset x64-debug
cmake --build out\build\x64-debug --target server client
```

## Usage

Start the server:

```powershell
.\out\build\x64-debug\rpc\server.exe
```

Run the client in another terminal:

```powershell
.\out\build\x64-debug\rpc\client.exe
```

Example client call:

```cpp
RPC::client client;
client.connect("127.0.0.1", 8888);

auto result = client.call(operation::add, 12, 21);
```

## Adding A Business Function

1. Add the method id in `rpc/proto/base.proto`.

```proto
enum MethodType {
    MET_UNKNOW = 0;
    MET_ADD = 1;
    MET_DIV = 2;
    MET_CALC_MATRIX = 3;
    MET_MY_METHOD = 4;
}
```

2. Add the request and result message in `rpc/proto/method.proto`.

```proto
message my_method {
    int32 value = 1;

    message result_type {
        int32 ret = 1;
    }
}
```

3. Regenerate protobuf files.

```powershell
.\rpc\proto\build.ps1
```

4. Add a method tag and specialize `rpc_method_traits` in `rpc/method/operation.hpp`.

```cpp
struct my_method_tag {};
inline constexpr my_method_tag my_method{};

template<>
struct rpc_method_traits<my_method_tag> {
    static constexpr impl::ProtoEnum id = impl::ProtoEnum::MET_MY_METHOD;
    using request_type = RPC::method::my_method;
    using response_type = typename request_type::result_type;
    using result_type = int;

    static request_type make_request(int value);
    static std::string serialize_result(result_type value);
    static result_type parse_response(const RPC::Response& resp);
    static result_type invoke(const request_type& req);
};
```

5. Implement the real function and trait adapter in `rpc/method/operation.cpp`.

6. Add one dispatch case in `rpc/detail/server_stub.hpp`.

```cpp
case MET_MY_METHOD:
    return dispatch_one<operation::my_method_tag>(request);
```

Then the client can call:

```cpp
auto result = client.call(operation::my_method, 123);
```
