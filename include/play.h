#ifndef PLAY_H_
#define PLAY_H_

#include "play_fmt.h"

/**
 * @brief Prints hello world message
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int hello_world();

/**
 * @brief Performs element-wise multiplication of two vectors.
 *
 * This function computes the element-wise product of two floating-point vectors (`src_a` and `src_b`)
 * and stores the result in the `dst` vector.
 *
 * @param [in] src_a A pointer to the first source vector.
 * @param [in] src_b A pointer to the second source vector.
 * @param [out] dst A pointer to the destination vector where the results will be stored.
 * @param [in] len The length of the vectors. All three vectors are assumed to have the same length.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_mul(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);

/**
 * @brief Performs element-wise subtraction of two vectors.
 *
 * This function computes the element-wise subtraction of two floating-point vectors(`src_a` and `src_b`)
 * and stores the result in the `dst` vector.
 *
 * @param [in] src_a A pointer to the first source vector.
 * @param [in] src_b A pointer to the second source vector.
 * @param [out] dst A pointer to the destination vector where the results will be stored.
 * @param [in] len The length of the vectors. All three vectors are assumed to have the same length.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_sub(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);

/**
 * @brief Performs element-wise addition of two vectors.
 *
 * This function computes the element-wise addition of two floating-point vectors(`src_a` and `src_b`)
 * and stores the result in the `dst` vector.
 *
 * @param [in] src_a A pointer to the first source vector.
 * @param [in] src_b A pointer to the second source vector.
 * @param [out] dst A pointer to the destination vector where the results will be stored.
 * @param [in] len The length of the vectors. All three vectors are assumed to have the same length.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_add(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);

/**
 * @brief Adds a scalar offset to each element of a vector.
 *
 * This function performs an element-wise addition of a constant scalar value (`offset`)
 * to a source vector (`src`) and stores the result in a destination vector (`dst`).
 *
 * @param [in] src A pointer to the source vector.
 * @param [in] offset The scalar value to add to each element of the vector.
 * @param [out] dst A pointer to the destination vector where the results will be stored.
 * @param [in] len The length of the vectors.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_offset(const play_fmt_t *src, const play_fmt_t offset, play_fmt_t *dst, const int len);

/**
 * @brief Performs element-wise scaling of a vector.
 *
 * This function multiplies each element of a source vector (`src`) by a constant
 * scalar value (`val`) and stores the result in a destination vector (`dst`).
 *
 * @param [in] src A pointer to the source vector.
 * @param [in] val The scalar value to multiply each element by.
 * @param [out] dst A pointer to the destination vector where the results will be stored.
 * @param [in] len The length of the vectors.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_scale(const play_fmt_t *src, const play_fmt_t val, play_fmt_t *dst, const int len);

/**
 * @brief Finds the minimum value in a vector.
 *
 * This function iterates through a vector of floating-point numbers (`src`) to find
 * and return its minimum value.
 *
 * @param [in] src A pointer to the source vector.
 * @param [out] min A pointer to the memory location where the minimum value will be stored.
 * @param [in] len The length of the vector.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_min(const play_fmt_t *src, play_fmt_t *min, const int len);

/**
 * @brief Computes the dot product of two vectors.
 *
 * This function calculates the dot product of two vectors, `src_a` and `src_b`, by
 * summing the products of their corresponding elements. The result is
 * a single scalar value which is stored in the memory location pointed to by `dst`.
 *
 * @param [in] src_a A pointer to the first source vector.
 * @param [in] src_b A pointer to the second source vector.
 * @param [out] dst A pointer to the memory location where the scalar result will be stored.
 * @param [in] len The length of the vectors.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_dot(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int len);

/**
 * @brief Computes the AXPY operation: dst = α·src_a + src_b.
 *
 * This function performs an element-wise multiplication of the source vector `src_a` by
 * a scalar `alpha`, and then adds the corresponding elements of the vector `src_b`.
 * The result is stored in a separate destination vector `dst`. The original source
 * vectors `src_a` and `src_b` are not modified.
 *
 * @param [in] src_a A pointer to the first source vector.
 * @param [in] src_b A pointer to the second source vector.
 * @param [in] alpha The scalar value (α) to multiply with each element of `src_a`.
 * @param [out] dst A pointer to the destination vector where the final result is stored.
 * @param [in] len The length of the vectors.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_axpy(const play_fmt_t *src_a, const play_fmt_t *src_b, const play_fmt_t alpha, play_fmt_t *dst, const int len);

/**
 * @brief Copies elements from a source vector to a destination vector.
 *
 * This function performs an element-by-element copy of floating-point values
 * from the source vector `src` to the destination vector `dst`.
 *
 * @param [in] src A pointer to the source vector.
 * @param [out] dst A pointer to the destination vector.
 * @param [in] len The number of elements to copy.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_memcpy(const play_fmt_t *src, play_fmt_t *dst, const int len);

/**
 * @brief Fills a vector with a value.
 *
 * This function iterates through each element of a vector and sets it to the
 * specified constant value `val`.
 *
 * @param [out] vec A pointer to the destination vector to be modified.
 * @param [in] val The constant value to set each element to.
 * @param [in] len The number of elements in the vector.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int vector_set_all(play_fmt_t *vec, const play_fmt_t val, const int len);

/**
 * @brief Performs matrix multiplication: dst = src_a * src_b.
 *
 * This function multiplies the matrix `src_a` by the matrix `src_b` and stores the
 * resulting matrix in `dst`. This operation is valid only if the number of columns
 * of `src_a` is equal to the number of rows of `src_b`.
 *
 * @param [in] src_a A pointer to the first source matrix, with dimensions dim_M x dim_N.
 * @param [in] src_b A pointer to the second source matrix, with dimensions dim_N x dim_P.
 * @param [out] dst A pointer to the destination matrix, with dimensions dim_M x dim_P.
 * @param [in] dim_M The number of rows in the `src_a` and `dst` matrices.
 * @param [in] dim_N The number of columns in the `src_a` matrix and the number of rows in the `src_b` matrix.
 * @param [in] dim_P The number of columns in the `src_b` and `dst` matrices.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_mul(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);

/**
 * @brief Performs matrix transposition.
 *
 * This function computes the transpose of a matrix `src` and stores the result
 * in a destination matrix `dst`. The transpose operation swaps the rows and
 * columns of the source matrix.
 *
 * @param [in] src A pointer to the source matrix with dimensions dim_M x dim_N.
 * @param [out] dst A pointer to the destination matrix, which will have dimensions dim_N x dim_M.
 * @param [in] dim_M The number of rows in the source matrix.
 * @param [in] dim_N The number of columns in the source matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_trans(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N);

/**
 * @brief Performs matrix multiplication with a transposed first matrix: dst = src_a^T * src_b
 *
 * @param [in] src_a A pointer to the first source matrix, with dimansion dim_M x dim_N
 * @param [in] src_b A pointer to the second source matric, with dimension dim_M x dim_P
 * @param [out] dst A pointer to the destination matrix, wich will have dimension dim_N x dim_P
 * @param [in] dim_M The number of rows in the `src_a` and `src_b` matrices.
 * @param [in] dim_N The number of cols in the `src_a` matrix and the number of rows in the `dst` matrix.
 * @param [in] dim_P The number of cols in the `src_b` and  `dst` matrices.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_mul_trans_A(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);

/**
 * @brief Performs matrix multiplication with a transposed second matrix: dst = src_a * src_b^T.
 *
 * This function multiplies a matrix `src_a` by the transpose of a matrix `src_b`.
 * The result is stored in `dst`.
 *
 * @param [in] src_a A pointer to the first source matrix, with dimensions dim_M x dim_N.
 * @param [in] src_b A pointer to the second source matrix, with dimensions dim_P x dim_N.
 * @param [out] dst A pointer to the destination matrix, which will have dimensions dim_M x dim_P.
 * @param [in] dim_M The number of rows in the `src_a` and `dst` matrices.
 * @param [in] dim_N The number of columns in the `src_a` and `src_b` matrices.
 * @param [in] dim_P The number of rows in the `src_b` matrix and the number of columns in the `dst` matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_mul_trans_B(const play_fmt_t *src_a, const play_fmt_t *src_b, play_fmt_t *dst, const int dim_M, const int dim_N, const int dim_P);

/**
 * @brief Copies elements from a source matrix to a destination matrix.
 *
 * This function performs an element-by-element copy of floating-point values
 * from the source matrix `src` to the destination matrix `dst`. The matrices
 * are treated as one-dimensional arrays in row-major order.
 *
 * @param [in] src A pointer to the source matrix.
 * @param [out] dst A pointer to the destination matrix.
 * @param [in] dim_M The number of rows in the matrices.
 * @param [in] dim_N The number of columns in the matrices.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_memcpy(const play_fmt_t *src, play_fmt_t *dst, const int dim_M, const int dim_N);

/**
 * @brief Fills a matrix with a constant value.
 *
 * This function iterates through each element of a matrix and sets it to the
 * specified constant value `val`. The matrix is treated as a one-dimensional
 * array in row-major order.
 *
 * @param [out] mat A pointer to the destination matrix to be modified.
 * @param [in] val The constant value to set each element to.
 * @param [in] dim_M The number of rows in the matrix.
 * @param [in] dim_N The number of columns in the matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_set_all(play_fmt_t *mat, const play_fmt_t val, const int dim_M, const int dim_N);

/**
 * @brief Swaps two rows of a matrix.
 *
 * This function swaps the contents of the specified rows, `row_a` and `row_b`,
 * within the matrix `mat`. The operation is performed in-place.
 *
 * @param [in, out] mat A pointer to the matrix where the rows will be swapped.
 * @param [in] row_a The index of the first row to be swapped (0-based).
 * @param [in] row_b The index of the second row to be swapped (0-based).
 * @param [in] dim_N The total number of columns in the matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int matrix_swap_rows(play_fmt_t *mat, const int row_a, const int row_b, const int dim_N);

/**
 * @brief Performs a generalized matrix-vector multiplication and addition: dst = α·mat·vec_x + β·vec_y.
 *
 * This function computes the product of a matrix and a vector, scales the result,
 * and adds it to another scaled vector.
 *
 * @param [in] mat A pointer to the input matrix.
 * @param [in] vec_x A pointer to the first input vector.
 * @param [in] vec_y A pointer to the second input vector.
 * @param [in] alpha A scalar multiplier for the matrix-vector product.
 * @param [in] beta A scalar multiplier for the vector vec_y.
 * @param [out] dst A pointer to the destination vector where the result is stored.
 * @param [in] dim_M The number of rows in the matrix.
 * @param [in] dim_N The number of columns in the matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_gemv(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N);

/**
 * @brief Performs a transposed general matrix-vector multiplication and addition: dst = α·matᵀ·vec_x + β·vec_y.
 *
 * @param [in] mat A pointer to the input matrix.
 * @param [in] vec_x A pointer to the input vector.
 * @param [in] vec_y A pointer to the input vector.
 * @param [in] alpha A scalar multiplier for the matrix-vector product.
 * @param [in] beta A scalar multiplier for the vector `vec_y`.
 * @param [out] dst A pointer to the destination vector where the result will be stored.
 * @param [in] dim_M The number of rows of the matrix `mat`.
 * @param [in] dim_N The number of columns of the matrix `mat`.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_gemv_trans(const play_fmt_t *mat, const play_fmt_t *vec_x, const play_fmt_t *vec_y, const play_fmt_t alpha, const play_fmt_t beta, play_fmt_t *dst, const int dim_M, const int dim_N);

/**
 * @brief Performs LU decomposition.
 *
 * This function factorizes an input matrix into its L and U components, storing the
 * result in-place. It also tracks row permutations resulting from partial pivoting.
 *
 * @param [in, out] mat A pointer to the input matrix to be decomposed.
 * @param [out] perm A pointer to an array that will store the permutation vector.
 * @param [in] dim_M The number of rows of the matrix.
 * @param [in] dim_N The number of columns of matrix.
 *
 * @return int Returns 0 on success, or -1 if the matrix is singular and cannot be decomposed.
 */
int linalg_lu_decomp(play_fmt_t *mat, int *perm, const int dim_M, const int dim_N);

/**
 * @brief Solves a linear system Ax = b using a precomputed LU factorization.
 *
 * This function solves the system by performing two sequential steps:
 * 1. Forward substitution to solve Ly = Pb.
 * 2. Backward substitution to solve Ux = y.
 * The LU factors, the permutation vector, and the right-hand side vector are required as input.
 *
 * @param [in] mat A pointer to the matrix containing the combined L/U factors from a previous LU decomposition.
 * @param [in] vec A pointer to the right-hand side vector `b`.
 * @param [in] perm A pointer to the permutation vector `p`.
 * @param [out] result A pointer to the destination vector where the solution `x` will be stored.
 * @param [in] dim_M The number of rows of the matrix.
 * @param [in] dim_N The number of columns of the matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_lu_solve(const play_fmt_t *mat, const play_fmt_t *vec, const int *perm, play_fmt_t *result, const int dim_M, const int dim_N);

/**
 * @brief Computes the Singular Value Decomposition (SVD) of a symmetric matrix using the Jacobi algorithm.
 *
 * This function diagonalizes the input matrix 'src' in-place, transforming it into a diagonal matrix.
 * The diagonal elements of the resulting matrix are the singular values squared, and the accumulated rotations form
 * the matrix of right singular vectors. The singular values are then extracted and stored in a separate vector.
 *
 * @param [in, out] src A pointer to the input symmetric square matrix (M x M) to be decomposed.
 * @param [out] mat_V A pointer to the destination matrix where the right singular vectors will be stored as columns.
 * @param [out] vec_S A pointer to the destination vector where the singular values (square roots of eigenvalues) will be stored.
 * @param [in] dim_M The dimension of the square matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_svd_jacobi(play_fmt_t *mat, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M);

/**
 * @brief Computes left singular vectors U from A, V, and S
 *
 * This function calculates the left singular vectors (U) of a matrix A using
 * the right singular vectors (V) and the singular values (S). The computation
 * is based on the formula U = AVΣ⁻¹, where Σ is the diagonal matrix of
 * singular values and Σ⁻¹ is its pseudo-inverse.
 *
 * @param [in] src Original input matrix A (M x N).
 * @param [in] mat_V Right singular vectors V (N x N).
 * @param [in] vec_S Singular values S (length N).
 * @param [out] dst Output matrix for the left singular vectors U (M x N).
 * @param [in] dim_M The number of rows (M) in the input matrices.
 * @param [in] dim_N The number of columns (N) in the input matrices.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_svd_lsv(const play_fmt_t *src, play_fmt_t *mat_V, play_fmt_t *vec_S, play_fmt_t *dst, const int dim_M, const int dim_N);

/**
 * @brief Computes Singular Value Decomposition (SVD) of a matrix
 *
 * Decomposes an input matrix `src` into its SVD components:
 * U (left singular vectors), S (singular values), and V (right singular vectors).
 * The decomposition follows the formula: A = UΣVᵀ.
 *
 * @param [in] src Input matrix A (M x N).
 * @param [out] dst Output matrix for the left singular vectors U (M x N).
 * @param [out] mat_V Output matrix for the right singular vectors V (N x N).
 * @param [out] vec_S Output vector for the singular values (length N).
 * @param [in] dim_M The number of rows (M) of the input matrix.
 * @param [in] dim_N The number of columns (N) of the input matrix.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_svd(const play_fmt_t *src, play_fmt_t *dst, play_fmt_t *mat_V, play_fmt_t *vec_S, const int dim_M, const int dim_N);

/**
 * @brief Computes the Cholesky Decomposition of a symmetric positive definite matrix
 *
 * Decomposes a square, symmetric, and positive definite input matrix `src` (A)
 * into a lower triangular matrix `dst` (L).
 * The decomposition follows the formula: A = L Lᵀ.
 *
 * @param [in] src Input matrix A (dim x dim). Must be symmetric and positive definite.
 * @param [out] dst Output matrix for the lower triangular Cholesky factor L (dim x dim).
 * @param [in] dim The dimension (N) of the square input and output matrices.
 *
 * @return int Returns 0 on success, non-zero on error.
 */
int linalg_cholesky_decomp(const play_fmt_t *src, play_fmt_t *dst, const int dim);

#endif  /* PLAY_H_ */
