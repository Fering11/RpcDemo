#include "operation.hpp"
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace operation {
namespace {

void fill_proto_matrix(
    const std::vector<std::vector<int>>& matrix,
    google::protobuf::RepeatedPtrField<RPC::method::calc_matrix::int_list>* rows) {
    for (const auto& row : matrix) {
        auto* proto_row = rows->Add();
        for (int value : row) {
            proto_row->add_edge(value);
        }
    }
}

std::vector<std::vector<int>> read_proto_matrix(
    const google::protobuf::RepeatedPtrField<RPC::method::calc_matrix::int_list>& rows) {
    std::vector<std::vector<int>> matrix;
    matrix.reserve(static_cast<std::size_t>(rows.size()));

    for (const auto& proto_row : rows) {
        std::vector<int> row;
        row.reserve(static_cast<std::size_t>(proto_row.edge_size()));

        for (int value : proto_row.edge()) {
            row.push_back(value);
        }

        matrix.push_back(std::move(row));
    }

    return matrix;
}

}

namespace impl {

int div(int left, int right) {
    return left / right;
}

int add(int left, int right) {
    return left + right;
}

// 矩阵乘法辅助函数：n x n 方阵相乘
std::vector<std::vector<int>> multiply(
    const std::vector<std::vector<int>>& A,
    const std::vector<std::vector<int>>& B) {
    auto n = A.size();
    std::vector<std::vector<int>> C(n, std::vector<int>(n, 0));
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t k = 0; k < n; ++k)  // 先固定 k 可提高缓存命中，但这里按直观写法
            for (std::size_t j = 0; j < n; ++j)
                C[i][j] += A[i][k] * B[k][j];
    return C;
}

std::vector<std::vector<int>> calc_matrix(
    std::vector<std::vector<int>> left,
    std::vector<std::vector<int>> right, int times) {
    // times = 0 时直接返回 left（即 left * right^0 = left * I = left）
    if (times <= 0) return left;

    // 计算 left * right^times
    std::vector<std::vector<int>> result = std::move(left);
    for (int t = 0; t < times; ++t) {
        result = multiply(result, right);
    }
    return result;
}
}

rpc_method_traits<add_tag>::request_type
rpc_method_traits<add_tag>::make_request(int left, int right) {
    request_type request;
    request.set_left(left);
    request.set_right(right);
    return request;
}

rpc_method_traits<add_tag>::response_type
rpc_method_traits<add_tag>::make_response(result_type value) {
    response_type response;
    response.set_ret(value);
    return response;
}

std::string rpc_method_traits<add_tag>::serialize_result(result_type value) {
    return make_response(value).SerializeAsString();
}

rpc_method_traits<add_tag>::result_type
rpc_method_traits<add_tag>::parse_result(const std::string& body) {
    response_type response;
    if (!response.ParseFromString(body)) {
        throw std::runtime_error("failed to parse add result");
    }
    return response.ret();
}

rpc_method_traits<add_tag>::result_type
rpc_method_traits<add_tag>::parse_response(const RPC::Response& resp) {
    return parse_result(resp.body());
}

rpc_method_traits<add_tag>::result_type
rpc_method_traits<add_tag>::invoke(const request_type& req) {
    return impl::add(req.left(), req.right());
}

rpc_method_traits<div_tag>::request_type
rpc_method_traits<div_tag>::make_request(int left, int right) {
    request_type request;
    request.set_left(left);
    request.set_right(right);
    return request;
}

rpc_method_traits<div_tag>::response_type
rpc_method_traits<div_tag>::make_response(result_type value) {
    response_type response;
    response.set_ret(value);
    return response;
}

std::string rpc_method_traits<div_tag>::serialize_result(result_type value) {
    return make_response(value).SerializeAsString();
}

rpc_method_traits<div_tag>::result_type
rpc_method_traits<div_tag>::parse_result(const std::string& body) {
    response_type response;
    if (!response.ParseFromString(body)) {
        throw std::runtime_error("failed to parse div result");
    }
    return response.ret();
}

rpc_method_traits<div_tag>::result_type
rpc_method_traits<div_tag>::parse_response(const RPC::Response& resp) {
    return parse_result(resp.body());
}

rpc_method_traits<div_tag>::result_type
rpc_method_traits<div_tag>::invoke(const request_type& req) {
    return impl::div(req.left(), req.right());
}

rpc_method_traits<calc_matrix_tag>::request_type
rpc_method_traits<calc_matrix_tag>::make_request(
    const std::vector<std::vector<int>>& left,
    const std::vector<std::vector<int>>& right,
    int times) {
    request_type request;
    fill_proto_matrix(left, request.mutable_left());
    fill_proto_matrix(right, request.mutable_right());
    request.set_times(static_cast<uint32_t>(times));
    return request;
}

rpc_method_traits<calc_matrix_tag>::response_type
rpc_method_traits<calc_matrix_tag>::make_response(const result_type& value) {
    response_type response;
    fill_proto_matrix(value, response.mutable_ret());
    return response;
}

std::string rpc_method_traits<calc_matrix_tag>::serialize_result(const result_type& value) {
    return make_response(value).SerializeAsString();
}

rpc_method_traits<calc_matrix_tag>::result_type
rpc_method_traits<calc_matrix_tag>::parse_result(const std::string& body) {
    response_type response;
    if (!response.ParseFromString(body)) {
        throw std::runtime_error("failed to parse calc_matrix result");
    }
    return read_proto_matrix(response.ret());
}

rpc_method_traits<calc_matrix_tag>::result_type
rpc_method_traits<calc_matrix_tag>::parse_response(const RPC::Response& resp) {
    return parse_result(resp.body());
}

rpc_method_traits<calc_matrix_tag>::result_type
rpc_method_traits<calc_matrix_tag>::invoke(const request_type& req) {
    return impl::calc_matrix(
        read_proto_matrix(req.left()),
        read_proto_matrix(req.right()),
        static_cast<int>(req.times()));
}

}
