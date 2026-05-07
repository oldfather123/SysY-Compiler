#!/usr/bin/env python3
"""
Enhanced diff script for comparing timing and pass configurations between test runs.
Usage:
  diff.py <log_dir1> <log_dir2>
  diff.py <log_file1.log> <log_file2.log>  (legacy mode)
"""

import re
import sys
import os
import json
from colorama import Fore, Style, init

# Initialize colorama
init(autoreset=True)


def parse_time_from_last_column(line):
    # 尝试匹配最后一列的时间格式（0H-0M-0S-108537us）
    time_match = re.search(r'(\d+)H-(\d+)M-(\d+)S-(\d+)us$', line)
    if time_match:
        hours = int(time_match.group(1))
        minutes = int(time_match.group(2))
        seconds = int(time_match.group(3))
        microseconds = int(time_match.group(4))

        # 转换为毫秒
        total_ms = ((hours * 3600 + minutes * 60 + seconds) * 1000_000 + microseconds) / 1000
        return total_ms

    # 如果没有匹配到时间格式，返回0
    return 0.0


def parse_log(file_path):
    """Parse legacy log file format"""
    test_times = {}
    with open(file_path, 'r') as f:
        for line in f:
            if line.startswith('[PASS]'):
                # 提取测试名称
                test_name = line.split()[1]

                # 提取最后一列的时间值
                last_column_time = parse_time_from_last_column(line)

                # 如果最后一列为0，使用第二列的毫秒时间
                if last_column_time == 0.0:
                    match = re.search(r'(\d+\.\d+)\s+ms', line)
                    if match:
                        last_column_time = float(match.group(1))

                test_times[test_name] = last_column_time
    return test_times


def load_timing_log(log_dir):
    """Load timing data from timestamped log directory"""
    time_log_path = os.path.join(log_dir, "time.log")
    test_times = {}

    if not os.path.exists(time_log_path):
        print(f"Warning: {time_log_path} not found")
        return test_times

    with open(time_log_path, 'r') as f:
        for line in f:
            line = line.strip()
            if ':' in line:
                test_name, time_us = line.split(':', 1)
                test_times[test_name.strip()] = int(time_us.strip())

    return test_times


def load_passes_log(log_dir):
    """Load pass configuration from timestamped log directory"""
    pass_log_path = os.path.join(log_dir, "passes.log")
    passes = {}

    if not os.path.exists(pass_log_path):
        print(f"Warning: {pass_log_path} not found")
        return passes

    with open(pass_log_path, 'r') as f:
        passes = json.load(f)

    return passes


def load_command_log(log_dir):
    """Load command information from timestamped log directory"""
    command_log_path = os.path.join(log_dir, "command.log")
    command_info = {}

    if not os.path.exists(command_log_path):
        return command_info

    with open(command_log_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("python3 test.py"):
                command_info['command'] = line.strip()
            elif line.startswith("# Timestamp:"):
                command_info['timestamp'] = line.replace("# Timestamp:", "").strip()

    return command_info


def compare_passes(passes1, passes2):
    """Compare two pass configurations and return differences"""
    all_passes = set(passes1.keys()) | set(passes2.keys())
    differences = {}

    for pass_name in all_passes:
        val1 = passes1.get(pass_name, 0)
        val2 = passes2.get(pass_name, 0)

        if val1 != val2:
            differences[pass_name] = (val1, val2)

    return differences


def print_pass_differences(differences, name1, name2):
    """Print pass configuration differences with colors"""
    if not differences:
        print(f"\n[2] {Fore.GREEN}✓ Pass configurations are identical{Style.RESET_ALL}")
        return

    print(f"\n[2] {Fore.YELLOW}Pass Configuration Differences:{Style.RESET_ALL}")
    print(f"{'Pass Name':<30} {name1:<15} {name2:<15}")
    print("-" * 60)

    for pass_name, (val1, val2) in differences.items():
        color = Fore.RED if val1 > val2 else Fore.GREEN
        print(f"{color}{pass_name:<30} {val1:<15} {val2:<15}{Style.RESET_ALL}")


def compare_and_display_results(times1, times2, name1, name2):
    """Compare timing results and display with colors"""
    # 只显示比例和时间，不显示统计信息，比例为后/前（time2/time1）
    results = []
    for test_name in times1:
        if test_name in times2:
            time1 = times1[test_name]
            time2 = times2[test_name]
            # 避免除以0
            if time1 > 0:
                ratio = time2 / time1
            else:
                ratio = float('inf') if time2 > 0 else 1.0
            results.append((test_name, ratio, time1, time2))

    # 按测试名称排序输出
    results.sort(key=lambda x: x[0])

    print(f"\n[3] {Fore.CYAN}Timing Comparison Results:{Style.RESET_ALL}")
    print(f"{'Test Case':<35} {'Ratio':<10} {f'{name1} (μs)':<15} {f'{name2} (μs)':<15}")
    print("-" * 85)

    for test_name, ratio, time1, time2 in results:
        # 只有超过阈值才算提升或下降
        thred = getattr(compare_and_display_results, 'thred', 0.1)
        if ratio > 1.0 + thred:
            color = Fore.GREEN  # 增长标绿色
            trend = "↑"
        elif ratio < 1.0 - thred:
            color = Fore.RED    # 减少标红色
            trend = "↓"
        else:
            color = Style.RESET_ALL
            trend = "="
        print(f"{color}{test_name:<35} {ratio:.4f}{trend:<5} {time1:<15} {time2:<15}{Style.RESET_ALL}")


def main():
    if len(sys.argv) != 3:
        print("Usage:")
        print("  diff.py <log_dir1> <log_dir2>  (compare timestamped log directories)")
        print("  diff.py <log_file1.log> <log_file2.log>  (legacy mode)")
        sys.exit(1)

    path1, path2 = sys.argv[1], sys.argv[2]

    # Determine if we're dealing with directories or files
    if os.path.isdir(path1) and os.path.isdir(path2):
        # New mode: compare log directories
        print(f"[-] Comparing log directories: {path1} vs {path2}")

        times1 = load_timing_log(path1)
        times2 = load_timing_log(path2)
        passes1 = load_passes_log(path1)
        passes2 = load_passes_log(path2)
        command1 = load_command_log(path1)
        command2 = load_command_log(path2)

        name1 = os.path.basename(path1)
        name2 = os.path.basename(path2)

        # Display command information
        print(f"\n[1] {Fore.CYAN}Command Information:{Style.RESET_ALL}")
        if command1.get('command'):
            print(f"{name1}: {command1['timestamp']}\n >>>>>> {command1['command']}")
        else:
            print(f"{name1}: No command information available")

        if command2.get('command'):
            print(f"{name2}: {command2['timestamp']}\n >>>>>> {command2['command']}")
        else:
            print(f"{name2}: No command information available")

        # Compare pass configurations
        pass_differences = compare_passes(passes1, passes2)
        print_pass_differences(pass_differences, name1, name2)

        # 支持命令行参数 --thred
        thred = 0.1
        for i, arg in enumerate(sys.argv):
            if arg == '--thred' and i + 1 < len(sys.argv):
                try:
                    thred = float(sys.argv[i + 1])
                except:
                    pass
        compare_and_display_results.thred = thred
        # Compare timing results
        compare_and_display_results(times1, times2, name1, name2)

    elif os.path.isfile(path1) and os.path.isfile(path2):
        # Legacy mode: compare log files
        print(f"[0] Comparing log files: {path1} vs {path2}")

        times1 = parse_log(path1)
        times2 = parse_log(path2)

        name1 = os.path.basename(path1)
        name2 = os.path.basename(path2)

        # Convert ms to μs for consistency
        times1_us = {k: v * 1000 for k, v in times1.items()}
        times2_us = {k: v * 1000 for k, v in times2.items()}

        compare_and_display_results(times1_us, times2_us, name1, name2)

    else:
        print("Error: Both arguments must be either directories or files")
        sys.exit(1)


if __name__ == "__main__":
    main()
