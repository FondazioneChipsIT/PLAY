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

def generate_scalar():
    """Generate a random scalar"""
    return np.random.rand()

def elementwise_offset(vec, offset):
    """Adds a scalar offset to each element of a vector."""
    return vec + offset

def format_array(array):
    """Convert a NumPy array into a C-style array."""
    return "{ " + ", ".join(f"{x:f}" for x in array) + " }"

def format_float(value):
    """Convert a float value into a C-style representation."""
    return f"{value:f}"

def generate_header_file(length, vec, offset, expected, filename="data.h"):
    """Generate a C header file containing the vectors and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define LEN {length}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t vec[] = {format_array(vec)};\n")
        f.write(f"play_fmt_t offset = {format_float(offset)};\n")
        f.write(f"play_fmt_t expected[] = {format_array(expected)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t vec[] = {format_array(vec)};\n")
        f.write(f"PI_L2 play_fmt_t offset = {format_float(offset)};\n")
        f.write(f"PI_L2 play_fmt_t expected[] = {format_array(expected)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    length = parse_args()
    vec = generate_vector(length)
    offset = generate_scalar()
    expected = elementwise_offset(vec, offset)

    generate_header_file(length, vec, offset, expected)
    print(f"File 'data.h' successfully generated with {length} elements.")

if __name__ == "__main__":
    main()
