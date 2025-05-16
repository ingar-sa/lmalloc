# Note: This code has been adapted from Claude's code

import re
import os
import struct
import itertools
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, BinaryIO, Tuple
from collections import Counter
import glob

@dataclass
class AllocTimingStats:
    total_tsc: int = 0
    iter: int = 0

@dataclass
class AllocTimingCollection:
    count: int = 0
    arr: List[int] = None

def parse_timing_data(file_handle: BinaryIO):
    tstats_bytes = file_handle.read(16)
    fields = struct.unpack('Q' * 2, tstats_bytes)

    stats = AllocTimingStats(
        total_tsc=fields[0],
        iter=fields[1]
    )
   
    count_bytes = file_handle.read(8)
    count = struct.unpack('Q', count_bytes)[0]
    
    arr_bytes = file_handle.read(count * 8)
    arr = list(struct.unpack('Q' * count, arr_bytes))
    
    collection = AllocTimingCollection(
        count=count,
        arr=arr
    )
    
    return stats, collection

def parse_tsc_frequency(dir):
    filename = dir + "tsc_freq.bin"
    with open(filename, 'rb') as f:
        tsc_freq_bytes = f.read(8)
        tsc_freq: double = struct.unpack('d', tsc_freq_bytes)[0]
    return tsc_freq

def load_all_tsc_frequencies(test_dir: str) -> float:
    """Load all TSC frequency files and return the mean."""
    tsc_files = glob.glob(os.path.join(test_dir, "*-tsc_freq.bin"))
    frequencies = []
    
    for tsc_file in tsc_files:
        try:
            with open(tsc_file, 'rb') as f:
                tsc_freq_bytes = f.read(8)
                tsc_freq = struct.unpack('d', tsc_freq_bytes)[0]
                frequencies.append(tsc_freq)
        except Exception as e:
            print(f"Warning: Couldn't parse {tsc_file}: {str(e)}")
    
    if frequencies:
        mean_freq = np.mean(frequencies)
        print(f"Loaded {len(frequencies)} TSC frequencies, mean: {mean_freq/1e6:.2f} MHz")
        return mean_freq
    else:
        print("Warning: No TSC frequency files found")
        return None

def tsc_to_ns(tsc, tsc_freq):
    return (tsc * 1e9) / tsc_freq

def average_tsc(stats: AllocTimingStats):
    return stats.total_tsc / stats.iter

def load_timing_data(filename):
    with open(filename, 'rb') as f:
        stats, collection = parse_timing_data(f)
    return stats, collection

def parse_timing_directory(dirpath):
    """Extract allocator function and size from directory name."""
    dirname = os.path.basename(dirpath)
    
    # Split by hyphen
    parts = dirname.split('-')
    
    if len(parts) == 1:
        # No hyphen in filename
        alloc_fn = parts[0]
        n_bytes = ""
    else:
        # Has hyphen - take everything before the last hyphen as function name
        alloc_fn = '-'.join(parts[:-1])
        
        # Try to parse the byte size from the last part
        byte_part = parts[-1]
        
        # Check if the last part matches the pattern for bytes (e.g., "32B")
        byte_match = re.match(r"(\d+)B$", byte_part)
        if byte_match:
            n_bytes = int(byte_match.group(1))
        else:
            # If it doesn't match the pattern, treat the whole name as the function
            alloc_fn = dirname
            n_bytes = ""
    
    return alloc_fn, n_bytes

def load_aggregate_timing_data(dirpath):
    """Load and aggregate timing data from all run files in a directory."""
    run_files = glob.glob(os.path.join(dirpath, "[0-9]*.bin"))
    run_files.sort(key=lambda x: int(os.path.basename(x).split('.')[0]))
    
    all_timings = []
    total_stats = AllocTimingStats()
    
    for run_file in run_files:
        try:
            stats, collection = load_timing_data(run_file)
            all_timings.extend(collection.arr)
            total_stats.total_tsc += stats.total_tsc
            total_stats.iter += stats.iter
        except Exception as e:
            print(f"Warning: Couldn't load {run_file}: {str(e)}")
    
    return total_stats, all_timings, len(run_files)

def get_next_output_dir(base_dir):
    """Get the next available numbered directory."""
    existing_dirs = glob.glob(os.path.join(base_dir, "[0-9]*"))
    if not existing_dirs:
        return os.path.join(base_dir, "1")
    
    # Find the highest number
    numbers = []
    for dir in existing_dirs:
        try:
            num = int(os.path.basename(dir))
            numbers.append(num)
        except ValueError:
            continue
    
    next_num = max(numbers) + 1 if numbers else 1
    return os.path.join(base_dir, str(next_num))

def analyze_tsc_distribution(
    dirpath, 
    test_dir=None,
    output_dir=None,
    top_n=None,
    min_count=None,
    figsize=(12, 8),
    x_scale='rank',
    y_scale='log',
    outlier_percentile=None,
    linear_threshold=10,
    show_plot=False
):
    """
    Analyze TSC distribution for all runs in a directory and plot with flexible scaling options.
    
    Args:
        dirpath: Path to the directory containing run files
        test_dir: Directory containing TSC frequency files
        output_dir: Directory where plots should be saved
        [Other args remain the same]
    
    Returns:
        Tuple of (figure, axis) matplotlib objects
    """
    # Parse directory and load aggregate data
    alloc_fn, size_bytes = parse_timing_directory(dirpath)
    stats, all_timings, num_runs = load_aggregate_timing_data(dirpath)
    
    # Get TSC frequency
    tsc_freq = None
    if test_dir:
        tsc_freq = load_all_tsc_frequencies(test_dir)
    
    # Ensure the directories end with path separator for consistency
    if output_dir and not output_dir.endswith(os.sep):
        output_dir += os.sep
    
    # Convert to numpy array
    timings = np.array(all_timings)
    original_timings = timings.copy()
    
    # Handle outliers if specified
    if outlier_percentile is not None:
        cutoff = np.percentile(timings, outlier_percentile)
        original_count = len(timings)
        timings = timings[timings <= cutoff]
        excluded_count = original_count - len(timings)
    else:
        excluded_count = 0
    
    # Always plot in cycles
    values = timings
    x_label = "TSC Cycles"
    
    # Count occurrences of each unique value
    value_counts = Counter(values)
    
    # Apply minimum count filter if specified
    if min_count is not None:
        value_counts = {k: v for k, v in value_counts.items() if v >= min_count}
    
    # Get the most common values if top_n is specified
    if top_n is not None:
        value_counts = dict(value_counts.most_common(top_n))
    
    # Sort by value
    sorted_items = sorted(value_counts.items())
    x_plot = [item[0] for item in sorted_items]
    counts = [item[1] for item in sorted_items]
    
    # Apply x-axis transformation
    if x_scale == 'rank':
        x_transformed = list(range(len(x_plot)))
        x_tick_labels = [f"{val:.2f}" for val in x_plot]
    else:
        x_transformed = x_plot
        x_tick_labels = None
    
    # Create the plot
    fig, ax = plt.subplots(figsize=figsize)
    
    # Plot bars
    if x_scale == 'rank':
        bars = ax.bar(x_transformed, counts, width=0.8)
    else:
        bars = ax.bar(x_transformed, counts, width=0.8)
    
    # Set scales
    if x_scale == 'log' and x_scale != 'rank':
        ax.set_xscale('log')

    if y_scale == 'log':
        ax.set_yscale('log')
    elif y_scale == 'symlog':
        # Symlog scale: linear near zero, logarithmic for larger values
        ax.set_yscale('symlog', linthresh=linear_threshold)
    elif y_scale == 'linear':
        ax.set_yscale('linear')
    
    # Set labels and title
    if x_scale == 'rank':
        ax.set_xlabel(f"{x_label} (ordered by value)")
    else:
        ax.set_xlabel(x_label)
    
    ax.set_ylabel("Count")
    
    # Create title
    plot_title = "TSC Value Distribution"
    if alloc_fn:
        plot_title += f" for {alloc_fn}"
    if size_bytes:
        plot_title += f" ({size_bytes}B)"
    plot_title += f" [x-axis: {x_scale}]"
    plot_title += f" [y-axis: {y_scale}"
    if y_scale == 'symlog':
        plot_title += f" (linear threshold {linear_threshold})]"
    else:
        plot_title += "]"

    ax.set_title(plot_title)
    
    # Handle x-axis ticks for rank scale
    if x_scale == 'rank' and x_tick_labels:
        # Show selected tick labels
        n_ticks = min(10, len(x_transformed))
        tick_indices = np.linspace(0, len(x_transformed)-1, n_ticks, dtype=int)
        ax.set_xticks([x_transformed[i] for i in tick_indices])
        ax.set_xticklabels([x_tick_labels[i] for i in tick_indices], rotation=45, ha='right')
    
    # Calculate statistics on all values in cycles
    stats_cycles = {
        "Mean": np.mean(original_timings),
        "Median": np.median(original_timings),
        "Min": np.min(original_timings),
        "Max": np.max(original_timings),
        "Std Dev": np.std(original_timings),
        "95th %ile": np.percentile(original_timings, 95),
        "99th %ile": np.percentile(original_timings, 99)
    }
    
    # Create statistics text with both cycles and ns (if available)
    stats_text_lines = []
    for k, v_cycles in stats_cycles.items():
        if tsc_freq is not None:
            v_ns = (v_cycles * 1e9) / tsc_freq
            stats_text_lines.append(f"{k}: {v_cycles:.2f} cycles (~{v_ns:.2f} ns)")
        else:
            stats_text_lines.append(f"{k}: {v_cycles:.2f} cycles")
    
    stats_text = "\n".join(stats_text_lines)
    
    ax.text(0.98, 0.98, stats_text,
            horizontalalignment='right',
            verticalalignment='top',
            transform=ax.transAxes,
            fontsize=9,
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.7))

    # Add info box (right-aligned, below stats)
    info_text = []
    info_text.append(f"Total samples: {len(original_timings)}")
    info_text.append(f"Unique values: {len(np.unique(original_timings))}")
    info_text.append(f"Displayed values: {len(x_plot)}")
    info_text.append(f"Runs included: {num_runs}")
    if excluded_count > 0:
        info_text.append(f"Excluded outliers: {excluded_count}\n({outlier_percentile} %ile)")
    
    # Calculate position for second text box
    stats_lines = len(stats_cycles)
    y_position = 0.98 - (stats_lines + 1) * 0.03  # Approximate line height
    
    ax.text(0.98, y_position, '\n'.join(info_text),
            horizontalalignment='right',
            verticalalignment='top',
            transform=ax.transAxes,
            fontsize=9,
            bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.7))
    
    # Add vertical lines for mean and median if appropriate
    if x_scale != 'rank' and not (x_scale == 'log' and any(v <= 0 for v in x_plot)):
        mean_val = stats_cycles["Mean"]
        median_val = stats_cycles["Median"]
            
        if mean_val >= min(x_plot) and mean_val <= max(x_plot):
            ax.axvline(mean_val, color='r', linestyle='--', alpha=0.7, 
                       label=f'Mean: {mean_val:.2f}')
        if median_val >= min(x_plot) and median_val <= max(x_plot):
            ax.axvline(median_val, color='g', linestyle=':', alpha=0.7, 
                       label=f'Median: {median_val:.2f}')
        ax.legend()
    
    # Add grid for better readability
    ax.grid(True, alpha=0.3)
    
    plt.tight_layout()
    
    # Create save path
    save_filename = alloc_fn
    if size_bytes != '':
        save_filename += "-" + str(size_bytes)
    save_filename += f"-{x_scale}-{y_scale}.png"
    
    save_path = os.path.join(output_dir, save_filename)
    plt.savefig(save_path, dpi=300, bbox_inches='tight')

    if show_plot:
        plt.show()

    return fig, ax

def analyze_all_timing_data(logs_dir="./logs", test_name=None, ignore_files=["tsc_freq.bin"], **kwargs):
    """
    Traverse test directories and analyze aggregated timing data.
    
    New directory structure:
    ./logs/<test-name>/<allocator name>[-<allocation size>]/<nth run>.bin
    ./logs/<test-name>/<nth run>-tsc_freq.bin
    
    Args:
        logs_dir: Root directory containing timing data
        test_name: Optional specific test directory to analyze (if None, analyzes all)
        ignore_files: List of filenames to skip
        **kwargs: Additional arguments to pass to analyze_tsc_distribution
    
    Returns:
        dict: Summary of processing
    """
    # scale_options = [('linear', 'linear'), ('linear', 'symlog'), ('log', 'linear'), 
                     # ('log', 'log')]
    scale_options = [ ('log', 'linear'), ('log', 'log') ]
   
    summary = {'processed': [], 'skipped': [], 'errors': [], 'interrupted': False}
    
    try:
        # Determine which directories to process
        if test_name:
            # Process only the specified test directory
            test_path = os.path.join(logs_dir, test_name)
            if not os.path.isdir(test_path):
                print(f"Error: Test directory '{test_name}' not found in {logs_dir}")
                return summary
            test_dirs = [(test_name, test_path)]
        else:
            # Process all test directories
            test_dirs = []
            for test_dir in os.listdir(logs_dir):
                test_path = os.path.join(logs_dir, test_dir)
                if os.path.isdir(test_path):
                    test_dirs.append((test_dir, test_path))
        
        # Process each test directory
        for test_dir, test_path in test_dirs:
            print(f"\nProcessing test: {test_dir}")
            
            # Find allocator directories
            for item in os.listdir(test_path):
                item_path = os.path.join(test_path, item)
                
                # Skip TSC frequency files
                if item.endswith('-tsc_freq.bin'):
                    continue
                    
                if os.path.isdir(item_path):
                    # Check if this directory contains .bin files (run files)
                    bin_files = glob.glob(os.path.join(item_path, "[0-9]*.bin"))
                    if not bin_files:
                        continue
                    
                    relative_path = os.path.relpath(item_path, logs_dir)
                    print(f"\nAnalyzing: {relative_path}")
                    
                    # Create numbered output directory
                    output_dir = get_next_output_dir(item_path)
                    Path(output_dir).mkdir(parents=True, exist_ok=True)
                    print(f"  Created output directory: {output_dir}")
                    
                    dir_processed = True
                    
                    # Run analysis with each permutation
                    for scale in scale_options:
                        try:
                            x_scale = scale[0]
                            y_scale = scale[1]
                            # Generate descriptive label for this permutation
                            label = f"x={x_scale}, y={y_scale}"
                            print(f"  - {label}")
                            
                            # Call the analysis function
                            fig, ax = analyze_tsc_distribution(
                                dirpath=item_path,
                                test_dir=test_path,  # For TSC frequency files
                                output_dir=output_dir,
                                x_scale=x_scale,
                                y_scale=y_scale,
                                # outlier_percentile=99.0,
                                **kwargs
                            )
                            
                            # Close the figure to free memory
                            plt.close(fig)
                            
                        except Exception as e:
                            print(f"    Error with {label}: {str(e)}")
                            summary['errors'].append(f"{relative_path} - {label}: {str(e)}")
                            dir_processed = False
                    
                    if dir_processed:
                        summary['processed'].append(relative_path)
    
    except KeyboardInterrupt:
        summary['interrupted'] = True
        print("\n\nScript interrupted by user (Ctrl+C).")
    
    # Print summary
    print(f"\nSummary:")
    print(f"  Processed: {len(summary['processed'])} directories")
    print(f"  Skipped: {len(summary['skipped'])} files")
    print(f"  Errors: {len(summary['errors'])}")
    if summary['interrupted']:
        print("  Script was interrupted")
    
    return summary

def main():
    import sys
    
    # Check for command line arguments
    if len(sys.argv) > 2:
        print("Usage: python script.py [test_name]")
        print("  test_name: Optional specific test directory to analyze")
        sys.exit(1)
    
    # If a test name is provided as argument, use it
    test_name = sys.argv[1] if len(sys.argv) == 2 else None
    
    if test_name:
        print(f"Analyzing specific test: {test_name}")
    else:
        print("Analyzing all tests in ./logs")
    
    result = analyze_all_timing_data("./logs", test_name=test_name)
    
    if result['interrupted']:
        print("\nAnalysis was interrupted")
    else:
        print(f"\nSuccessfully processed: {result['processed']}")

if __name__ == "__main__":
    main()

# def main():
#     result = analyze_all_timing_data("./logs")
#     if result['interrupted']:
#         print("\nAnalysis was interrupted")
#     print(f"Successfully processed: {result['processed']}")

    # stats, collection = load_timing_data(filepath)
    # tsc_freq = parse_tsc_frequency(directory)
    # # tsc_freq = parse_tsc_frequency("./logs/ua_nmc/21/")
    #
    # print("TSC frequency: ", tsc_freq, '\n')
    #
    # print(f"Timing data for {n_bytes}B allocations with {alloc_fn}")
    # print("\tTotal time:  ", stats.total_tsc)
    # print("\tTotal iter:  ", stats.iter)
    # print("\tAverage TSC: ", average_tsc(stats))
    # print("\tAverage ns:  ", tsc_to_ns(average_tsc(stats), tsc_freq))
    #
    # filtered_arr = sorted(collection.arr)[:-1]
    # print("\nTiming data collection:")
    # print("\tCount:    ", collection.count)
    # print("\tSum:      ", sum(filtered_arr))
    # print("\tAvg TSC:  ", sum(filtered_arr) / collection.count, tsc_freq)
    # print("\tAvg ns:   ", tsc_to_ns(sum(filtered_arr) / collection.count, tsc_freq))
    # print("\tSmallest: ", sorted(collection.arr)[:10])
    # print("\tLargest:  ", sorted(collection.arr)[-10:])

