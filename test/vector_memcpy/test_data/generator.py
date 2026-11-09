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

def elementwise_add(vec_a, vec_b):
    """Compute the element-wise addition of two vectors."""
    return vec_a + vec_b

def format_array(array):
    """Convert a NumPy array into a C-style array."""
    return "{ " + ", ".join(f"{x:f}" for x in array) + " }"

def generate_header_file(length, vec, filename="data.h"):
    """Generate a C header file containing the vectors and their expected result."""

    script_dir = os.path.dirname(os.path.abspath(__file__))
    filepath = os.path.join(script_dir, filename)

    with open(filepath, "w") as f:
        f.write("#ifndef DATA_H_\n")
        f.write("#define DATA_H_\n\n#include \"play.h\"\n\n")

        f.write(f"#define LEN {length}\n\n")

        f.write("#if TARGET_IS_SPATZ\n\n")

        f.write(f"play_fmt_t vec[] = {format_array(vec)};\n\n")

        f.write("#elif TARGET_IS_PULP_OPEN\n\n")

        f.write('#include "pmsis.h"\n\n')

        f.write(f"PI_L2 play_fmt_t vec[] = {format_array(vec)};\n\n")

        f.write("#endif /* TARGET_IS_ */\n\n")

        f.write("#endif  /* DATA_H_ */\n")

def main():
    length = parse_args()
    vec = generate_vector(length)

    generate_header_file(length, vec)
    print(f"File 'data.h' successfully generated with {length} elements.")

if __name__ == "__main__":
    main()
