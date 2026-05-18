#include "proto/build/test.pb.h"

#include "threadpool/thpoolv5.hpp"
#include "method/operation.hpp"
int main() {
	operation_traits<operation::ProtoEnum::MET_ADD>;
	
	return 0;
}