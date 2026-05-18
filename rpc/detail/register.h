#ifndef RPC_REGISTER_HEAD
#define RPC_REGISTER_HEAD

#define RPC_RegisterInit(package,enum_type) template<enum_type>\
struct package##_traits{};

#define RPC_RegisterMethod(package,enum_tag,rpc_fun)    template<>	\
struct package##_traits<enum_tag> {	\
	using inv_type = decltype(rpc_fun);	\
	constexpr static inv_type* inv = rpc_fun;	\
};	



#endif