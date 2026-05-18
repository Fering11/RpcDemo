#include "operation.hpp"
namespace operation {

double div(int left, int right) {
	return left / right;
}

int add(int left, int right) {
	return left + right;
}

// 矩阵乘法辅助函数：n x n 方阵相乘
std::vector<std::vector<int>> multiply(
    const std::vector<std::vector<int>>& A,
    const std::vector<std::vector<int>>& B) {
    int n = A.size();
    std::vector<std::vector<int>> C(n, std::vector<int>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < n; ++k)  // 先固定 k 可提高缓存命中，但这里按直观写法
            for (int j = 0; j < n; ++j)
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