import os
import sys
import numpy as np
from scipy.linalg import eigh

def parse_args():
    """Parse command line arguments for matrix dimensions."""
    if len(sys.argv) != 3:
        print("Error: missing arguments <M> <N>")
        print("Usage: python3 generator.py <M> <N>")
        sys.exit(1)

    try:
        M = int(sys.argv[1])
        N = int(sys.argv[2])
    except ValueError:
        print("Error: <M> and <N> must be integers")
        print("Usage: python3 generator.py <M> <N>")
        sys.exit(1)

    return M, N

def generate_matrix(rows, cols):
    """Generate a matrix of floats with given dimensions."""
    return np.random.randn(rows, cols)

def generate_vector(length):
    """Generate a vector of floats."""
    return np.random.randn(length)

def generate_scalar():
    """Generate a random scalar"""
    return np.random.rand()

def get_symm_mat(mat):
    return mat.T @ mat

def svd_jacobi(mat):
    """Computes the Singular Value Decomposition (SVD) of a symmetric matrix using the Jacobi algorithm"""
    eigvals, eigvecs = eigh(mat)
    idx = eigvals.argsort()[::-1]
    eigvals = eigvals[idx]
    eigvecs = eigvecs[:, idx]
    expected_matrix = np.diag(eigvals)
    vec_S = np.sqrt(np.maximum(eigvals, 0))
    mat_V = eigvecs
    return expected_matrix, vec_S, mat_V

def svd_lsv(mat, V, S):
    """Computes left singular vectors U from A, V, and S."""
    AV = np.dot(mat, V)
    S_inv = np.zeros_like(S)
    non_zero_S_mask = S > 1e-12
    S_inv[non_zero_S_mask] = 1.0 / S[non_zero_S_mask]
    U = AV * S_inv

    return U

def format_matrix(array):
    """Convert a NumPy matrix into a C-style array."""
    # Flatten the array for C representation
    flat_array = array.flatten()
    return "{ " + ", ".join(f"{x:f}" for x in flat_array) + " }"

def format_array(array):
    """Convert a NumPy array into a C-style array."""
    return "{ " + ", ".join(f"{x:f}" for x in array) + " }"

def format_float(value):
    """Convert a float value into a C-style representation."""
    return f"{value:f}"

def generate_header_file(M, N, mat, S, V, expected, filename="data.h"):
    """Generate a C header file containing the matrices and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define DIM_M {M}\n")
        f.write(f"#define DIM_N {N}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t mat[] = {format_matrix(mat)};\n")
        f.write(f"play_fmt_t V[] = {format_matrix(V)};\n")
        f.write(f"play_fmt_t S[] = {format_array(S)};\n")
        f.write(f"play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t mat[] = {format_matrix(mat)};\n")
        f.write(f"PI_L2 play_fmt_t V[] = {format_matrix(V)};\n")
        f.write(f"PI_L2 play_fmt_t S[] = {format_array(S)};\n")
        f.write(f"PI_L2 play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    M, N = parse_args()
    mat = generate_matrix(M, N)
    mat_sym = get_symm_mat(mat)

    _, S, V = svd_jacobi(mat_sym)
    expected = svd_lsv(mat, V, S)

    generate_header_file(M, N, mat, S, V, expected)
    print(f"File 'data.h' successfully generated for matrices of size {M}x{N}.")

if __name__ == "__main__":
    main()
