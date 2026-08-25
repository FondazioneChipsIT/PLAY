import argparse
import subprocess
import re
import csv
import os

from types import SimpleNamespace

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
    test_directories = [
        d for d in os.listdir(paths.test_root)
        if os.path.isdir(os.path.join(paths.test_root, d))
        and not d.startswith('_')
        and d not in ["common", "runners", "hello"]
    ]
    test_directories.sort()
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

def check_run_status(returncode, output, stats_saved):
    if returncode != 0:
        return f"MAKE FAILED (rc={returncode})"

    if "Test SUCCESS" not in output:
        return "TEST FAILED"

    if not stats_saved:
        return "NO STATS"

    return "PASS"

def run_test_case(args, paths, test_dir_name, num_cores):
    current_test_dir = os.path.join(paths.test_root, test_dir_name)

    make_command = f"make clean all run STATS=1 USE_CLUSTER=1 NUM_CORES={num_cores} TARGET=PULP_OPEN platform={args.platform}"

    print(f"\n--- Running {test_dir_name} with {num_cores} cores ---")
    print(f"Command: cd {current_test_dir} && {make_command}")

    status = "UNKNOWN"

    try:
        proc = subprocess.Popen(
        make_command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        cwd=current_test_dir
    )

        output_lines = []
        for line in proc.stdout:
            print(line, end='')
            output_lines.append(line)

        proc.wait()
        result_stdout = ''.join(output_lines)

        stats_saved = parse_and_save_stats(paths, test_dir_name, num_cores, result_stdout)
        status = check_run_status(proc.returncode, result_stdout, stats_saved)
    except subprocess.CalledProcessError as e:
        status = "MAKE ERROR"
        print(f"Error during compilation or execution for {test_dir_name} (cores={num_cores}):")
        print(f"STDOUT:\n{e.stdout}")
        print(f"STDERR:\n{e.stderr}")
    except Exception as e:
        status = "SCRIPT ERROR"
        print(f"An unexpected error occurred: {e}")

    print(f"--- Outcome for {test_dir_name} ({num_cores} cores): {status} ---")

    return {'test': test_dir_name, 'cores': num_cores, 'status': status}


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
        data_1_core = test_data[test_name][1]
        data_8_cores = test_data[test_name][8]
        headers = test_data[test_name]['headers']

        if not data_1_core or not data_8_cores:
            print(f"Skipping {test_name}: missing data for 1 or 8 cores.")
            continue

        markdown_content.append(f"## {test_name.replace('_', ' ').upper()}")
        markdown_content.append("")

        display_headers = ['Core', 'Problem Dimension'] + [h.replace('_', ' ') for h in headers if h != 'id']
        markdown_content.append("| " + " | ".join(display_headers) + " |")
        markdown_content.append("|-" + "|-".join(["" for _ in display_headers]) + "|")

        markdown_content.append("**Single-core Execution**")
        row_1_data = data_1_core[0]
        row_values = ['0']
        row_values.append(dimensions.get(test_name, ''))
        for header in headers:
            if header != 'id':
                row_values.append(row_1_data.get(header, ''))
        markdown_content.append("| " + " | ".join(row_values) + " |")

        markdown_content.append("**Multi-core Execution**")

        max_cycles_8_cores = 0
        for row_8_data in data_8_cores:
            row_values = [row_8_data.get('id', '')]
            row_values.append(dimensions.get(test_name, ''))

            current_cycles = float(row_8_data.get('cycles', 0))
            if current_cycles > max_cycles_8_cores:
                max_cycles_8_cores = current_cycles

            for header in headers:
                if header != 'id':
                    row_values.append(row_8_data.get(header, ''))
            markdown_content.append("| " + " | ".join(row_values) + " |")

        cycles_1 = float(row_1_data.get('cycles', 0))
        speedup = cycles_1 / max_cycles_8_cores if max_cycles_8_cores > 0 else 0

        speedup_row_values = ['**Speedup**', f"**{speedup:.2f}**"] + [''] * (len(display_headers) - 2)
        markdown_content.append("| " + " | ".join(speedup_row_values) + " |")
        markdown_content.append("")

    markdown_filename = os.path.join(platform_dir, "benchmarks.md")
    with open(markdown_filename, 'w') as mdfile:
        mdfile.write("\n".join(markdown_content))

    print(f"Markdown Report saved to {markdown_filename}")

def save_run_summary(paths, outcomes):
    summary_filename = os.path.join(paths.results_dir, "run_summary.csv")

    with open(summary_filename, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=['test', 'cores', 'status'], lineterminator='\n')
        writer.writeheader()
        writer.writerows(outcomes)

    print(f"Run summary saved to {summary_filename}")

def print_run_summary(outcomes):
    print("\n--- Run Summary ---")

    failed = [outcome for outcome in outcomes if outcome['status'] != "PASS"]

    print(f"Passed: {len(outcomes) - len(failed)}/{len(outcomes)}")

    for outcome in failed:
        print(f"  {outcome['status']:24} {outcome['test']} ({outcome['cores']} cores)")

def main():
    args = parse_args()
    paths = set_paths(args)
    test_dirs = set_test_dirs(paths)

    outcomes = []

    for test_dir_name in test_dirs:
        outcomes.append(run_test_case(args, paths, test_dir_name, 1))
        outcomes.append(run_test_case(args, paths, test_dir_name, 8))

    generate_markdown_report(paths.platform_dir, paths.results_dir)

    save_run_summary(paths, outcomes)
    print_run_summary(outcomes)

    print("\nAll tests and the report generation have been completed.")

if __name__ == "__main__":
    main()
