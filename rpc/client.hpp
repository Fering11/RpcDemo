#ifndef RPC_CLIENT_HEAD
#define RPC_CLIENT_HEAD
#include "detail/client_stub.hpp"
RPC_BEGIN
class client:public client_stub {
};

RPC_END
#endif