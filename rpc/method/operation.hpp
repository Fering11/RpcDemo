#pragma once

#include "../detail/predef.h"
#include "../detail/register.h"
#include "../proto/build/base.pb.h"
#include "../proto/build/method.pb.h"
#include <string>
#include <type_traits>
#include <vector>

namespace operation {
namespace impl {
using ProtoEnum = RPC::MethodType;
int div(int left, int right);
int add(int left, int right);
std::vector<std::vector<int>> calc_matrix(
	std::vector<std::vector<int>> left,
	std::vector<std::vector<int>> right, int times);
}

template<class>
inline constexpr bool dependent_false_v = false;

template<class T>
struct rpc_method_traits {
	static_assert(dependent_false_v<T>, "Method doesn't match traits.");
};

//标签
struct add_tag {};
inline constexpr add_tag add{};

template<>
struct rpc_method_traits<add_tag> {
	static constexpr impl::ProtoEnum id = impl::ProtoEnum::MET_ADD;
	using request_type = RPC::method::add;
	using response_type = typename request_type::result_type;
	using result_type = int;

	static request_type make_request(int left, int right);
	static response_type make_response(result_type value);
	static std::string serialize_result(result_type value);
	static result_type parse_result(const std::string& body);
	static result_type parse_response(const RPC::Response& resp);
	static result_type invoke(const request_type& req);
};

struct div_tag {};
inline constexpr div_tag div{};

template<>
struct rpc_method_traits<div_tag> {
	static constexpr impl::ProtoEnum id = impl::ProtoEnum::MET_DIV;
	using request_type = RPC::method::div;
	using response_type = typename request_type::result_type;
	using result_type = int;

	static request_type make_request(int left, int right);
	static response_type make_response(result_type value);
	static std::string serialize_result(result_type value);
	static result_type parse_result(const std::string& body);
	static result_type parse_response(const RPC::Response& resp);
	static result_type invoke(const request_type& req);
};

struct calc_matrix_tag {};
inline constexpr calc_matrix_tag calc_matrix{};

template<>
struct rpc_method_traits<calc_matrix_tag> {
	static constexpr impl::ProtoEnum id = impl::ProtoEnum::MET_CALC_MATRIX;
	using request_type = RPC::method::calc_matrix;
	using response_type = typename request_type::result_type;
	using result_type = std::vector<std::vector<int>>;

	static request_type make_request(
		const std::vector<std::vector<int>>& left,
		const std::vector<std::vector<int>>& right,
		int times);
	static response_type make_response(const result_type& value);
	static std::string serialize_result(const result_type& value);
	static result_type parse_result(const std::string& body);
	static result_type parse_response(const RPC::Response& resp);
	static result_type invoke(const request_type& req);
};



}
//RPC_RegisterInit(operation, operation::ProtoEnum);
//RPC_RegisterMethod(operation, operation::ProtoEnum::MET_DIV, operation::div);
//RPC_RegisterMethod(operation, operation::ProtoEnum::MET_ADD, operation::add);
//RPC_RegisterMethod(operation, operation::ProtoEnum::MET_CALC_MATRIX, operation::calc_matrix);
