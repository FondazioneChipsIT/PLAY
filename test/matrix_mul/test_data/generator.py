import os
import sys
import numpy as np

def parse_args():
    """Parse command line arguments for matrix dimensions."""
    if len(sys.argv) != 4:
        print("Error: missing arguments <M> <N> <P>")
        print("Usage: python3 generator.py <M> <N> <P>")
        sys.exit(1)

    try:
        M = int(sys.argv[1])
        N = int(sys.argv[2])
        P = int(sys.argv[3])
    except ValueError:
        print("Error: <M>, <N>, and <P> must be integers")
        print("Usage: python3 generator.py <M> <N> <P>")
        sys.exit(1)

    return M, N, P

def generate_matrix(rows, cols):
    """Generate a matrix of floats with given dimensions."""
    return np.random.randn(rows, cols)

def matrix_mul(mat_a, mat_b):
    """Performa matrix multiplication."""
    return np.matmul(mat_a, mat_b)

def format_matrix(array):
    """Convert a NumPy matrix into a C-style array."""
    # Flatten the array for C representation
    flat_array = array.flatten()
    return "{ " + ", ".join(f"{x:f}" for x in flat_array) + " }"

def generate_header_file(M, N, P, mat_a, mat_b, expected, filename="data.h"):
    """Generate a C header file containing the matrices and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define DIM_M {M}\n")
        f.write(f"#define DIM_N {N}\n")
        f.write(f"#define DIM_P {P}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t mat_a[] = {format_matrix(mat_a)};\n")
        f.write(f"play_fmt_t mat_b[] = {format_matrix(mat_b)};\n")
        f.write(f"play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t mat_a[] = {format_matrix(mat_a)};\n")
        f.write(f"PI_L2 play_fmt_t mat_b[] = {format_matrix(mat_b)};\n")
        f.write(f"PI_L2 play_fmt_t expected[] = {format_matrix(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    M, N, P = parse_args()
    mat_a = generate_matrix(M, N)
    mat_b = generate_matrix(N, P)
    expected = matrix_mul(mat_a, mat_b)

    generate_header_file(M, N, P, mat_a, mat_b, expected)
    print(f"File 'data.h' successfully generated for matrices of size {M}x{N} and {N}x{P}.")

if __name__ == "__main__":
    main()
