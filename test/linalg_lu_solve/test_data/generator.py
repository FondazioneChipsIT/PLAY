import os
import sys
import numpy as np
from scipy.linalg import lu, solve_triangular

def parse_args():
    """Parse command line arguments for matrix dimensions."""
    if len(sys.argv) != 2:
        print("Error: missing arguments <M>")
        print("Usage: python3 generator.py <M>")
        sys.exit(1)

    try:
        M = int(sys.argv[1])
    except ValueError:
        print("Error: <M> must be integers")
        print("Usage: python3 generator.py <M>")
        sys.exit(1)

    return M

def generate_solvable_system(n):
    """Generate a a solvable linear system Ax = b with a square matrix."""
    A = np.random.randn(n, n)
    A += np.diag(np.full(n, 50.0))  # Adds a dominant diagonal for better stability
    x = np.random.randn(n)
    b = A @ x
    return A, x, b

def lu_decomp(mat):
    """Performs LU decomposition of matix mat with partial pivoting"""

    rows, cols = mat.shape
    dim_min = min(rows, cols)

    perm, L, U = lu(mat, permute_l=False, p_indices=True)

    dst = np.copy(mat)

    dst[:dim_min, :dim_min] = U
    for i in range(rows):
        for j in range(dim_min):
            if (i > j):
                dst[i, j] = L[i, j]

    return dst, perm, L, U

def lu_solve(L, U, b, perm):
    """Solves linear system Ax = b using LU factorization"""
    b_perm = b[perm]
    y = solve_triangular(L, b_perm, lower=True)
    x = solve_triangular(U, y, lower=False)
    return x


def format_matrix(array):
    """Convert a NumPy matrix into a C-style array."""
    # Flatten the array for C representation
    flat_array = array.flatten()
    return "{ " + ", ".join(f"{x:f}" for x in flat_array) + " }"

def format_array(array):
    """Convert a NumPy array into a C-style array."""
    return "{ " + ", ".join(f"{x}" for x in array) + " }"

def format_float(value):
    """Convert a float value into a C-style representation."""
    return f"{value:f}"

def generate_header_file(M, mat, b, perm, expected, filename="data.h"):
    """Generate a C header file containing the matrices and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define DIM_M {M}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t LU[] = {format_matrix(mat)};\n")
        f.write(f"play_fmt_t b[] = {format_array(b)};\n")
        f.write(f"int p[] = {format_array(perm)};\n")
        f.write(f"play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t LU[] = {format_matrix(mat)};\n")
        f.write(f"PI_L2 play_fmt_t b[] = {format_array(b)};\n")
        f.write(f"PI_L2 int p[] = {format_array(perm)};\n")
        f.write(f"PI_L2 play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    M = parse_args()
    A, x, b = generate_solvable_system(M)

    lu, perm, L, U = lu_decomp(A)
    expected = lu_solve(L, U, b, perm)

    generate_header_file(M, lu, b, perm, expected)
    print(f"File 'data.h' successfully generated for matrices of size {M}x{M}.")

if __name__ == "__main__":
    main()
