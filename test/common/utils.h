#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>

#include "play_fmt.h"

/*
 * Tolerance of the comparisons: two values match when
 *     |val_a - val_b| <= TOLL_ABS + TOLL_REL * |val_b|
 * FP32 keeps the historical absolute tolerance. Both macros can be
 * overridden from the command line (-DTOLL_ABS=... -DTOLL_REL=...).
 */
#if PLAY_FORMAT == PLAY_FMT_FP16
#define TOLL_ABS_DEFAULT 0.0625f
#define TOLL_REL_DEFAULT 0.004f
#elif PLAY_FORMAT == PLAY_FMT_BF16
#define TOLL_ABS_DEFAULT 0.25f
#define TOLL_REL_DEFAULT 0.0625f
#else   /* PLAY_FMT_FP32 */
#define TOLL_ABS_DEFAULT 0.004f
#define TOLL_REL_DEFAULT 0.0f
#endif  /* PLAY_FORMAT */

#ifndef TOLL_ABS
#define TOLL_ABS TOLL_ABS_DEFAULT
#endif  /* TOLL_ABS */

#ifndef TOLL_REL
#define TOLL_REL TOLL_REL_DEFAULT
#endif  /* TOLL_REL */

#define TOLL TOLL_ABS

/**
 * @brief Allocates a memory buffer in L1 memory and returns its address.
 *
 * @param bytes Size of the memory region to allocate, in bytes.
 *
 * @return A pointer to the beginning of the allocated memory region, or
 *         `NULL` if the allocation fails.
 *
 * @note The caller is responsible for freeing the allocated memory using
 *       the appropriate deallocation function.
 */
void *my_alloc(const int bytes);

/**
 * @brief Synchronizes all cores in the cluster using a barrier
 *
 * @note This function only has effect when CLUSTER macro is defined.
 * In non-cluster configurations, this function does nothing.
 */
void barrier();

/**
 * @brief Checks if the current core is the master core, i.e. core ID 0
 *
 * @return true if the current core is the master core (core ID 0)
 * @return false if the current core is not the master core
 */
bool is_master_core();

/**
 * @brief Compares two floating-point values with a tollerance check
 *
 * @param[in] val_a First value
 * @param[in] val_b Second value
 *
 * @return true if the values differ within the tolerance (TOLL_ABS, TOLL_REL)
 * @return false if the difference exceeds the tolerance
 */
bool scalar_compare(const play_fmt_t val_a, const play_fmt_t val_b);

/**
 * @brief Compares two floating-point vectors with a tolerance check
 *
 * @param[in] vec_a Pointer to the first vector
 * @param[in] vec_b Pointer to the second vector
 * @param[in] len Length of the vectors to compare
 *
 * @return true if all elements differ within the tolerance (TOLL_ABS, TOLL_REL)
 * @return false if any element difference exceeds the tolerance
 *
 * @warning Both vectors must have at least 'len' elements allocated
 */
bool vector_compare(const play_fmt_t *vec_a, const play_fmt_t *vec_b, const int len);

/**
 * @brief Compares the absolute values of two floating-point vectors with a tolerance check
 *
 * @param[in] vec_a Pointer to the first vector
 * @param[in] vec_b Pointer to the second vector
 * @param[in] len Length of the vectors to compare
 *
 * @return true if all elements differ within the tolerance (TOLL_ABS, TOLL_REL)
 * @return false if any element difference exceeds the tolerance
 *
 * @warning Both vectors must have at least 'len' elements allocated
 */
bool vector_compare_abs(const play_fmt_t *vec_a, const play_fmt_t *vec_b, const int len);

/**
 * @brief Compares two floating-point matrices with a tollerance check
 *
 * @param[in] mat_a Pointer to the first matrix
 * @param[in] mat_b Pointer to the second matrix
 * @param[in] rows Number of rows for the two matrices
 * @param[in] cols Number of columts for the two matrices
 *
 *
 * @return true if all elements differ within the tolerance (TOLL_ABS, TOLL_REL)
 * @return false if any element difference exceeds the tolerance
 */
bool matrix_compare(const play_fmt_t *mat_a, const play_fmt_t *mat_b, const int rows, const int cols);

/**
 * @brief Compares the absolute value of two floating-point matrices with a tollerance check
 *
 * @param[in] mat_a Pointer to the first matrix
 * @param[in] mat_b Pointer to the second matrix
 * @param[in] rows Number of rows for the two matrices
 * @param[in] cols Number of columts for the two matrices
 *
 *
 * @return true if all elements differ within the tolerance (TOLL_ABS, TOLL_REL)
 * @return false if any element difference exceeds the tolerance
 */
bool matrix_compare_abs(const play_fmt_t *mat_a, const play_fmt_t *mat_b, const int rows, const int cols);

#ifdef  PRINT_DATA

/**
 * @brief Prints a scalar value to standard output
 *
 * @param[in] val The scalar to print
 * @param[in] str Descriptive string to identify the scalar in output
 *
 * @note This function only produces output when PRINT_DATA macro is defined.
 * In other configurations, this function has no effect.
 */
void scalar_print(const play_fmt_t val, const char *str);

/**
 * @brief Prints a vector to standard output
 *
 * @param[in] vec Pointer to the vector to print
 * @param[in] len Length of the vector
 * @param[in] str Descriptive string to identify the vector in output
 *
 * @note This function only produces output when PRINT_DATA macro is defined.
 * In other configurations, this function has no effect.
 */
void vector_print(const play_fmt_t *vec, const int len, const char *str);

/**
 * @brief Printa a matrix to standard output
 *
 * @param[in] mat Pointer to the matrix to print
 * @param[in] rows Number of rows of the matrix
 * @param[in] cols Number of cols of the matrix
 * @param[in] str Descriptive string to identify the matrix in output
 *
 * @note This function only produces output when PRINT_DATA macro is defined.
 * In other configurations, this function has no effect.
 */
void matrix_print(const play_fmt_t *mat, const int rows, const int cols, const char *str);

#endif  /* PRINT_DATA */

#if TARGET_IS_SPATZ

/**
 * @brief Returns the current CPU cycle count.
 *
 * @return The number of CPU cycles.
 */
unsigned long get_cycle();

#endif  /* TARGET_IS_SPATZ */

#endif  /* UTILS_H_ */
