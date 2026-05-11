#include <pack.pb.h>

int main() {

	default_rpc::User user;
	user.set_id(12);
	user.set_name("Name");
	default_rpc::Request request;
	//request.compact().insert();
	request.mutable_client()->set_code(default_rpc::error::ERR_BAD_REQUEST);
	request.mutable_client()->mutable_html()->assign("HE");


	return 0;
}