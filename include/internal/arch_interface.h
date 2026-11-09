#ifndef ARCH_INTERFACE_H_
#define ARCH_INTERFACE_H_

#include "play.h"
#include "internal/fmt_math.h"

int linalg_cholesky_decomp_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim);
int linalg_gemv_impl(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N);
int linalg_gemv_trans_impl(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N);
int linalg_lu_decomp_impl(play_fmt_t *mat, int *perm, const int dim_M, const int dim_N);
int linalg_lu_solve_impl(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N);
int linalg_svd_impl(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N);
int linalg_svd_jacobi_impl(play_fmt_t *mat, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M);
int linalg_svd_lsv_impl(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N);

int matrix_memcpy_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N);
int matrix_mul_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);
int matrix_mul_trans_A_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);
int matrix_mul_trans_B_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);
int matrix_set_all_impl(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N);
int matrix_swap_rows_impl(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N);
int matrix_trans_impl(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N);

int vector_add_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);
int vector_axpy_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, const play_fmt_t alpha, play_fmt_t *dst, const int len);
int vector_dot_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);
int vector_memcpy_impl(const play_fmt_t *src, play_fmt_t *dst, const int len);
int vector_min_impl(const play_fmt_t *src, play_fmt_t *min, const int len);
int vector_mul_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);
int vector_offset_impl(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len);
int vector_scale_impl(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len);
int vector_set_all_impl(play_fmt_t *vec, const play_fmt_t val, const int len);
int vector_sub_impl(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);

#endif /* ARCH_INTERFACE_H_*/
