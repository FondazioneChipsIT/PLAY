import os
import sys
import numpy as np

def parse_args():
    """Parse command line arguments and return the vector length."""
    if len(sys.argv) != 2:
        print("Error: missing argument <length>")
        print("Usage: python3 generator.py <length>")
        sys.exit(1)

    try:
        length = int(sys.argv[1])
    except ValueError:
        print("Error: <length> must be an integer")
        print("Usage: python3 generator.py <length>")
        sys.exit(1)

    return length

def generate_vector(length):
    """Generate a vector of floats."""
    return np.random.randn(length)

def vector_dot(vec_a, vec_b):
    """Compute the dot-product of two vectors."""
    return np.dot(vec_a, vec_b)

def format_array(array):
    """Convert a NumPy array into a C-style array."""
    return "{ " + ", ".join(f"{x:f}" for x in array) + " }"

def format_float(value):
    """Convert a float value into a C-style representation."""
    return f"{value:f}"

def generate_header_file(length, vec_a, vec_b, expected, filename="data.h"):
    """Generate a C header file containing the vectors and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define LEN {length}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t vec_a[] = {format_array(vec_a)};\n")
        f.write(f"play_fmt_t vec_b[] = {format_array(vec_b)};\n")
        f.write(f"play_fmt_t expected = {format_float(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t vec_a[] = {format_array(vec_a)};\n")
        f.write(f"PI_L2 play_fmt_t vec_b[] = {format_array(vec_b)};\n")
        f.write(f"PI_L2 play_fmt_t expected = {format_float(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    length = parse_args()
    vec_a = generate_vector(length)
    vec_b = generate_vector(length)
    expected = vector_dot(vec_a, vec_b)

    generate_header_file(length, vec_a, vec_b, expected)
    print(f"File 'data.h' successfully generated with {length} elements.")

if __name__ == "__main__":
    main()
