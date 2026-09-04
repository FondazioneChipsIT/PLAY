import argparse
import subprocess
import re
import csv
import os
import threading

from concurrent.futures import ThreadPoolExecutor, as_completed
from types import SimpleNamespace

print_lock = threading.Lock()

# Explicit list of PULP-Open benchmark tests to run, instead of auto-discovering
# every non-underscore directory under test/ (that used to also pick up e.g.
# "hello"). Keep in sync with the `dimensions` table in generate_markdown_report().
TEST_DIRS = [
    'linalg_cholesky_decomp',
    'linalg_gemv',
    'linalg_gemv_trans',
    'linalg_lu_decomp',
    'linalg_lu_solve',
    'linalg_svd',
    'linalg_svd_jacobi',
    'linalg_svd_lsv',
    'matrix_memcpy',
    'matrix_mul',
    'matrix_mul_trans_A',
    'matrix_mul_trans_B',
    'matrix_set_all',
    'matrix_swap_rows',
    'matrix_trans',
    'vector_add',
    'vector_axpy',
    'vector_dot',
    'vector_memcpy',
    'vector_min',
    'vector_mul',
    'vector_offset',
    'vector_scale',
    'vector_set_all',
    'vector_sub',
]

def parse_args():
    parser = argparse.ArgumentParser(
        description="Script for running tests on different platforms."
    )

    parser.add_argument(
        "--platform",
        type=str,
        choices=["gvsoc", "rtl"],
        required=False,
        default="gvsoc",
        help="Specifies the execution platform. Allowed values: 'gvsoc' or 'rtl'. Default is 'gvsoc'."
    )

    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=4,
        help="Number of tests to run in parallel (each test's 1-core and 8-core runs stay sequential within it, since they share the same build directory). Default is 1 (sequential)."
    )

    args = parser.parse_args()
    return args

def set_paths(args):
    paths = SimpleNamespace()

    paths.script_dir = os.path.dirname(os.path.abspath(__file__))
    paths.project_root = os.path.abspath(os.path.join(paths.script_dir, "../../../../"))
    paths.runners_dir = os.path.abspath(os.path.join(paths.script_dir, "../../"))
    paths.test_root = os.path.join(paths.project_root, "test")
    paths.benchmarks_dir = os.path.join(paths.runners_dir, "pulp-open/benchmarks")
    paths.platform_dir = os.path.join(paths.benchmarks_dir, f"{args.platform}")
    paths.results_dir = os.path.join(paths.platform_dir, "results")

    os.makedirs(paths.results_dir, exist_ok=True)

    return paths

def set_test_dirs(paths):
    test_directories = []
    for d in TEST_DIRS:
        if os.path.isdir(os.path.join(paths.test_root, d)):
            test_directories.append(d)
        else:
            print(f"Warning: test '{d}' listed in TEST_DIRS but not found under {paths.test_root}, skipping.")
    return test_directories


def parse_and_save_stats(paths, test_dir_name, num_cores, output):
    start_info_line = f"Printing statistics:"
    if start_info_line not in output:
        print(f"Warning: Could not find the header for test {test_dir_name} ({num_cores} cores).")
        return False

    stats_start_index = output.find(start_info_line)
    stats_block = output[stats_start_index:]

    all_cores_data = {core_id: {} for core_id in range(num_cores)}

    pattern = re.compile(r"\[(\d+)\]\s+([\w\s]+):\s+([\d.]+)")

    for line in stats_block.splitlines():
        match = pattern.search(line)
        if match:
            core_id = int(match.group(1))
            key = match.group(2).strip().replace(" ", "_")
            value = match.group(3)
            all_cores_data[core_id][key] = value

    fieldnames = list(all_cores_data[0].keys()) if all_cores_data[0] else []
    fieldnames.insert(0, 'id')

    csv_filename = os.path.join(paths.results_dir, f"{test_dir_name}_CL_{num_cores}.csv")

    with open(csv_filename, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames, lineterminator='\n')
        writer.writeheader()

        for core_id in range(num_cores):
            row_data = all_cores_data[core_id]
            row_to_write = {field: row_data.get(field, '') for field in fieldnames}
            row_to_write['id'] = core_id
            writer.writerow(row_to_write)

    print(f"Statistics data saved to {csv_filename}")
    return True

def run_test_case(args, paths, test_dir_name, num_cores):
    current_test_dir = os.path.join(paths.test_root, test_dir_name)

    make_command = f"make clean all run STATS=1 USE_CLUSTER=1 NUM_CORES={num_cores} TARGET=PULP_OPEN platform={args.platform}"

    header = f"\n--- Running {test_dir_name} with {num_cores} cores ---\nCommand: cd {current_test_dir} && {make_command}\n"

    try:
        proc = subprocess.Popen(
            make_command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            cwd=current_test_dir
        )

        result_stdout = proc.communicate()[0]
        proc.wait()

        # Printed as one block (rather than line-by-line) so parallel runs
        # don't interleave each other's output.
        with print_lock:
            print(header, end='')
            print(result_stdout, end='')

        parse_and_save_stats(paths, test_dir_name, num_cores, result_stdout)
    except Exception as e:
        with print_lock:
            print(f"An unexpected error occurred for {test_dir_name} (cores={num_cores}): {e}")


def run_test_series(args, paths, test_dir_name):
    # 8-core only: no single-core baseline is run anymore.
    run_test_case(args, paths, test_dir_name, 8)


def generate_markdown_report(platform_dir, results_dir):
    print("\n--- Generating Markdown Report ---")

    dimensions = {
        'linalg_cholesky_decomp': '64x64',
        'linalg_gemv': '64x32',
        'linalg_gemv_trans': '64x32',
        'linalg_lu_decomp': '64x32',
        'linalg_lu_solve': '64x64',
        'linalg_svd': '64x32',
        'linalg_svd_jacobi': '64x64',
        'linalg_svd_lsv': '64x32',
        'matrix_memcpy': '64x32',
        'matrix_mul': '64x32 <br> 32x64',
        'matrix_mul_trans_A': '64x32 <br> 48x32',
        'matrix_mul_trans_B': '32x64 <br> 32x48',
        'matrix_set_all': '64x32',
        'matrix_swap_rows': '16x256',
        'matrix_trans': '64x32',
        'vector_add': '2048',
        'vector_axpy': '2048',
        'vector_dot': '2048',
        'vector_memcpy': '2048',
        'vector_min': '2048',
        'vector_mul': '2048',
        'vector_offset': '2048',
        'vector_scale': '2048',
        'vector_set_all': '2048',
        'vector_sub': '2048',
    }

    test_data = {}

    for filename in os.listdir(results_dir):
        if filename.endswith('.csv'):
            file_path = os.path.join(results_dir, filename)
            parts = filename.split('_CL_')
            test_name = parts[0]
            cores = int(parts[1].replace('.csv', ''))

            with open(file_path, 'r') as csvfile:
                reader = csv.DictReader(csvfile)

                if test_name not in test_data:
                    test_data[test_name] = {'headers': reader.fieldnames, 1: [], 8: []}

                if cores == 1:
                    test_data[test_name][1] = list(reader)
                elif cores == 8:
                    test_data[test_name][8] = list(reader)

    markdown_content = []
    sorted_test_names = sorted(test_data.keys())

    for test_name in sorted_test_names:
        data_8_cores = test_data[test_name][8]
        headers = test_data[test_name]['headers']

        if not data_8_cores:
            print(f"Skipping {test_name}: missing data for 8 cores.")
            continue

        markdown_content.append(f"## {test_name.replace('_', ' ').upper()}")
        markdown_content.append("")

        display_headers = ['Core', 'Problem Dimension'] + [h.replace('_', ' ') for h in headers if h != 'id']
        markdown_content.append("| " + " | ".join(display_headers) + " |")
        markdown_content.append("|-" + "|-".join(["" for _ in display_headers]) + "|")

        for row_8_data in data_8_cores:
            row_values = [row_8_data.get('id', '')]
            row_values.append(dimensions.get(test_name, ''))

            for header in headers:
                if header != 'id':
                    row_values.append(row_8_data.get(header, ''))
            markdown_content.append("| " + " | ".join(row_values) + " |")

        markdown_content.append("")

    markdown_filename = os.path.join(platform_dir, "benchmarks.md")
    with open(markdown_filename, 'w') as mdfile:
        mdfile.write("\n".join(markdown_content))

    print(f"Markdown Report saved to {markdown_filename}")

def main():
    args = parse_args()
    paths = set_paths(args)
    test_dirs = set_test_dirs(paths)

    if args.jobs > 1:
        print(f"Running {len(test_dirs)} tests, up to {args.jobs} in parallel...")
        with ThreadPoolExecutor(max_workers=args.jobs) as executor:
            futures = {
                executor.submit(run_test_series, args, paths, test_dir_name): test_dir_name
                for test_dir_name in test_dirs
            }
            for future in as_completed(futures):
                test_dir_name = futures[future]
                try:
                    future.result()
                except Exception as e:
                    print(f"Test {test_dir_name} raised an exception: {e}")
    else:
        for test_dir_name in test_dirs:
            run_test_series(args, paths, test_dir_name)

    generate_markdown_report(paths.platform_dir, paths.results_dir)

    print("\nAll tests and the report generation have been completed.")

if __name__ == "__main__":
    main()
