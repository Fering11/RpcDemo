#include "proto/build/test.pb.h"

#include "threadpool/thpoolv5.hpp"
int main() {


	//poolv5::pool mythpool(12);
	//mythpool.post([]() {})
	
	test::Package package;
	test::SubArgument subarg;
	
	subarg.set_id(12);
	subarg.set_status(test::STAT_ERR_BAD_ALLOC);
	subarg.set_info("Hello I am the argument in the sub function which allow to do a operation.");
	
	subarg.mutable_num()->add_num(32);
	subarg.mutable_num()->add_num(31);
	subarg.mutable_num()->add_num(30);
	subarg.mutable_num()->add_num(33);

	package.set_body(subarg.SerializeAsString());
	package.set_type(test::TYPE_SUB);

	auto str = package.SerializeAsString();

	test::Package other_side;
	test::SubArgument other_sub;

	bool suc = other_side.ParseFromString(str);

	suc = other_side.type() == test::TYPE_SUB;
	suc = other_sub.ParseFromString(other_side.body());
	int id = other_sub.id();
	suc = other_sub.status() == test::STAT_ERR_BAD_ALLOC;
	auto myinfo = other_sub.info();
	auto numlist = other_sub.num();
	
	numlist.num().size();

	return 0;
}