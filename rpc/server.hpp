#ifndef RPC_SERVER_HEAD
#define RPC_SERVER_HEAD

#include "detail/server_stub.hpp"
#include <cstdint>

RPC_BEGIN

class server: public server_stub {
public:
	explicit server(uint16_t port)
		: server_stub(port) {
	}
};

RPC_END

#endif
