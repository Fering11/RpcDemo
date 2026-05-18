#ifndef RPC_CLIENT_STUB_HEAD
#define RPC_CLIENT_STUB_HEAD

#include "network.hpp"
#include "server_stub.hpp"
RPC_BEGIN

class client_stub:public tcp_client {
public:
	template<operation::function_type type, class... _Ax>
	auto call(_Ax&&... _args) 
		-> std::invoke_result_t<
		typename operation::operator_traits<type>::func> {

	}

};


RPC_END


#endif