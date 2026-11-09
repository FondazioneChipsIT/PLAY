import os
import sys
import numpy as np
from scipy.linalg import eigh

def parse_args():
    """Parse command line arguments for matrix dimensions."""
    if len(sys.argv) != 2:
        print("Error: missing arguments <M>")
        print("Usage: python3 generator.py <M>")
        sys.exit(1)

    try:
        M = int(sys.argv[1])
    except ValueError:
        print("Error: <M> must be integer")
        print("Usage: python3 generator.py <M>")
        sys.exit(1)

    return M

def generate_matrix(dim):
    """Generate a square matrix of floats with given dimensions."""
    return np.random.randn(dim, dim)

def get_sdp_mat(mat, eps=1e-6):
    """Generate a symmetric positive definite matrix"""
    mat =  mat @ mat.T
    mat += np.eye(mat.shape[0]) * eps
    mat = (mat + mat.T) / 2
    return mat

def cholesky_decomp(mat):
    """Computes the decomposition of a symmetric and definite positive matrix using the Cholesky algorithm"""
    return np.linalg.cholesky(mat)

def format_matrix(array):
    """Convert a NumPy matrix into a C-style array."""
    # Flatten the array for C representation
    flat_array = array.flatten()
    return "{ " + ", ".join(f"{x:f}" for x in flat_array) + " }"

def generate_header_file(M, mat, expected, filename="data.h"):
    """Generate a C header file containing the matrices and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define DIM_M {M}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t mat[] = {format_matrix(mat)};\n")
        f.write(f"play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t mat[] = {format_matrix(mat)};\n")
        f.write(f"PI_L2 play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    M = parse_args()
    mat = generate_matrix(M)
    mat_sdp = get_sdp_mat(mat)

    expected = cholesky_decomp(mat_sdp)

    generate_header_file(M, mat_sdp, expected)
    print(f"File 'data.h' successfully generated for matrices of size {M}x{M}.")

if __name__ == "__main__":
    main()
