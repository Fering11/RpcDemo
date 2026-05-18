#pragma once

#include "../detail/predef.h"
#include "../detail/register.h"
#include "../proto/build/base.pb.h"

#include <vector>
namespace operation {
using ProtoEnum = rpc_detail_v0_1::MethodType;
double div(int left, int right);
int add(int left, int right);
std::vector<std::vector<int>> calc_matrix(
	std::vector<std::vector<int>> left,
	std::vector<std::vector<int>> right,int times);
}
RPC_RegisterInit(operation, operation::ProtoEnum);
RPC_RegisterMethod(operation, operation::ProtoEnum::MET_DIV, operation::div);
RPC_RegisterMethod(operation, operation::ProtoEnum::MET_ADD, operation::add);
RPC_RegisterMethod(operation, operation::ProtoEnum::MET_CALC_MATRIX, operation::calc_matrix);