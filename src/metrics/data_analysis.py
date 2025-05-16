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

@dataclass
class ProcessedResult:
    """Store processed timing data for LaTeX table generation"""
    allocator: str
    size: str
    mean_ns: float
    median_ns: float
    min_ns: float
    max_ns: float
    std_dev_ns: float
    p95_ns: float
    p99_ns: float
    total_samples: int
    
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

def load_all_tsc_frequencies(allocator_dir: str) -> float:
    """Load all TSC frequency files from the allocator directory and return the mean."""
    # Look for tsc_freq.bin files directly in the allocator directory
    tsc_files = glob.glob(os.path.join(allocator_dir, "*-tsc_freq.bin"))
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
        print(f"Loaded {len(frequencies)} TSC frequencies from {allocator_dir}, mean: {mean_freq/1e6:.2f} MHz")
        return mean_freq
    else:
        print(f"Warning: No TSC frequency files found in {allocator_dir}")
        return None

def tsc_to_ns(tsc, tsc_freq):
    return (tsc * 1e9) / tsc_freq

def average_tsc(stats: AllocTimingStats):
    return stats.total_tsc / stats.iter

def load_timing_data(filename):
    with open(filename, 'rb') as f:
        stats, collection = parse_timing_data(f)
    return stats, collection

def parse_allocator_directory(dirpath):
    """Extract allocator function and size from directory name."""
    dirname = os.path.basename(dirpath)
    
    # Split by the last hyphen to separate function name and size
    parts = dirname.rsplit('-', 1)
    
    if len(parts) < 2:
        # No hyphen in filename - this might be a SDHS directory
        parent_dir = os.path.basename(os.path.dirname(dirpath))
        if parent_dir == 'sdhs':
            return dirname, None
        else:
            # Unexpected format
            return dirname, None
    
    alloc_fn = parts[0]
    size = parts[1]
    
    return alloc_fn, size

def should_process_directory(allocator_type, alloc_fn, size):
    """Check if this allocator/size combination should be processed."""
    # For arena and malloc, we want small, medium, large sizes
    if allocator_type in ['arena', 'malloc']:
        return size in ['small', 'medium', 'large']
    
    # For sdhs, we process all directories (size will be None)
    if allocator_type == 'sdhs':
        return True
    
    return False

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
    allocator_type,
    tsc_freq,
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
    Now always converts to nanoseconds for display.
    
    Args:
        dirpath: Path to the directory containing run files
        allocator_type: The type of allocator (arena, malloc, sdhs)
        tsc_freq: TSC frequency for conversion to nanoseconds
        output_dir: Directory where plots should be saved
        [Other args remain the same]
    
    Returns:
        Tuple of (figure, axis, ProcessedResult) matplotlib objects and processed data
    """
    # Parse directory and load aggregate data
    alloc_fn, size = parse_allocator_directory(dirpath)
    stats, all_timings, num_runs = load_aggregate_timing_data(dirpath)
    
    if tsc_freq is None:
        raise ValueError(f"TSC frequency not found for {dirpath}")
    
    # Ensure the directories end with path separator for consistency
    if output_dir and not output_dir.endswith(os.sep):
        output_dir += os.sep
    
    # Convert to numpy array and then to nanoseconds
    timings_tsc = np.array(all_timings)
    original_timings_tsc = timings_tsc.copy()
    
    # Convert to nanoseconds
    timings_ns = tsc_to_ns(timings_tsc, tsc_freq)
    original_timings_ns = tsc_to_ns(original_timings_tsc, tsc_freq)
    
    # Handle outliers if specified (working in nanoseconds)
    if outlier_percentile is not None:
        cutoff = np.percentile(timings_ns, outlier_percentile)
        original_count = len(timings_ns)
        timings_ns = timings_ns[timings_ns <= cutoff]
        excluded_count = original_count - len(timings_ns)
    else:
        excluded_count = 0
    
    # Use nanoseconds for plotting
    values = timings_ns
    x_label = "Time (nanoseconds)"
    
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
    
    # Create title with allocator type and function
    plot_title = f"Time Distribution"
    if alloc_fn:
        plot_title += f" for {alloc_fn}"
        if size:
            plot_title += f" ({size})"
    else:
        plot_title += f" for {allocator_type}"
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
    
    # Calculate statistics on all values in nanoseconds
    stats_ns = {
        "Mean": np.mean(original_timings_ns),
        "Median": np.median(original_timings_ns),
        "Min": np.min(original_timings_ns),
        "Max": np.max(original_timings_ns),
        "Std Dev": np.std(original_timings_ns),
        "95th %ile": np.percentile(original_timings_ns, 95),
        "99th %ile": np.percentile(original_timings_ns, 99)
    }
    
    # Create statistics text
    stats_text_lines = []
    for k, v_ns in stats_ns.items():
        stats_text_lines.append(f"{k}: {v_ns:.2f} ns")
    
    stats_text = "\n".join(stats_text_lines)
    
    ax.text(0.98, 0.98, stats_text,
            horizontalalignment='right',
            verticalalignment='top',
            transform=ax.transAxes,
            fontsize=9,
            bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.7))

    # Add info box (right-aligned, below stats)
    info_text = []
    info_text.append(f"Total samples: {len(original_timings_ns)}")
    info_text.append(f"Unique values: {len(np.unique(original_timings_ns))}")
    info_text.append(f"Displayed values: {len(x_plot)}")
    info_text.append(f"Runs included: {num_runs}")
    if excluded_count > 0:
        info_text.append(f"Excluded outliers: {excluded_count}\n({outlier_percentile} %ile)")
    
    # Calculate position for second text box
    stats_lines = len(stats_ns)
    y_position = 0.98 - (stats_lines + 1) * 0.03  # Approximate line height
    
    ax.text(0.98, y_position, '\n'.join(info_text),
            horizontalalignment='right',
            verticalalignment='top',
            transform=ax.transAxes,
            fontsize=9,
            bbox=dict(boxstyle='round', facecolor='lightblue', alpha=0.7))
    
    # Add vertical lines for mean and median if appropriate
    if x_scale != 'rank' and not (x_scale == 'log' and any(v <= 0 for v in x_plot)):
        mean_val = stats_ns["Mean"]
        median_val = stats_ns["Median"]
            
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
    save_filename = f"{alloc_fn}"
    if size:
        save_filename += f"-{size}"
    save_filename += f"-{x_scale}-{y_scale}.png"
    
    save_path = os.path.join(output_dir, save_filename)
    plt.savefig(save_path, dpi=300, bbox_inches='tight')

    if show_plot:
        plt.show()
    
    # Create ProcessedResult for LaTeX table
    # For SDHS, use the alloc_fn as the size since there's no separate size
    display_size = size if size is not None else "all"
    
    processed_result = ProcessedResult(
        allocator=alloc_fn if allocator_type == 'sdhs' else allocator_type,
        size=display_size,
        mean_ns=stats_ns["Mean"],
        median_ns=stats_ns["Median"],
        min_ns=stats_ns["Min"],
        max_ns=stats_ns["Max"],
        std_dev_ns=stats_ns["Std Dev"],
        p95_ns=stats_ns["95th %ile"],
        p99_ns=stats_ns["99th %ile"],
        total_samples=len(original_timings_ns)
    )

    return fig, ax, processed_result

def generate_latex_table(results: List[ProcessedResult], output_file: str):
    """Generate a LaTeX table with the timing statistics."""
    
    # Sort results by allocator and then by size
    def sort_key(result):
        # Define custom ordering for allocators
        allocator_order = {'arena': 0, 'malloc': 1}
        
        # Define custom ordering for sizes
        size_order = {'small': 0, 'medium': 1, 'large': 2, 'all': 3}
        
        # For arena and malloc, use the allocator order
        if result.allocator in ['arena', 'malloc']:
            alloc_idx = allocator_order.get(result.allocator, 999)
            size_idx = size_order.get(result.size, 999)
        else:
            # For SDHS, sort alphabetically by function name
            alloc_idx = 1000  # Put after arena/malloc
            size_idx = 0  # All SDHS entries have same "size"
        
        return (alloc_idx, size_idx, result.allocator, result.size)
    
    results.sort(key=sort_key)
    
    latex_content = r"""
\begin{table}[htbp]
\centering
\caption{Memory Allocator Performance Statistics (nanoseconds)}
\label{tab:allocator_stats}
\begin{tabular}{|l|l|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r@{\hspace{2pt}}|r|}
\hline
\textbf{Allocator} & \textbf{Size} & \textbf{Mean} & \textbf{Median} & \textbf{Min} & \textbf{Max} & \textbf{Std Dev} & \textbf{95th \%} & \textbf{99th \%} & \textbf{Samples} \\
\hline"""

    for result in results:
        # Format numbers with appropriate precision
        latex_content += f"\\hline\n"
        latex_content += f"{result.allocator} & {result.size} & "
        latex_content += f"{result.mean_ns:.2f} & {result.median_ns:.2f} & "
        latex_content += f"{result.min_ns:.2f} & {result.max_ns:.2f} & "
        latex_content += f"{result.std_dev_ns:.2f} & {result.p95_ns:.2f} & "
        latex_content += f"{result.p99_ns:.2f} & {result.total_samples:,} \\\\\n"

    latex_content += r"""
\hline
\end{tabular}
\end{table}
"""
    
    # Write to file
    with open(output_file, 'w') as f:
        f.write(latex_content)
    
    print(f"LaTeX table saved to: {output_file}")

def analyze_all_timing_data(logs_dir="./logs", ignore_files=["tsc_freq.bin"], **kwargs):
    """
    Traverse the logs directory structure and analyze allocator timing data.
    
    Directory structure:
    ./logs/arena/{alloc_fn}-{size}/ (where size is small, medium, large)
    ./logs/malloc/{alloc_fn}-{size}/ (where size is small, medium, large)  
    ./logs/sdhs/{alloc_fn}/
    
    TSC frequency files are located directly in the allocator directories:
    ./logs/arena/*-tsc_freq.bin
    ./logs/malloc/*-tsc_freq.bin
    ./logs/sdhs/*-tsc_freq.bin
    
    Args:
        logs_dir: Root directory containing timing data
        ignore_files: List of filenames to skip
        **kwargs: Additional arguments to pass to analyze_tsc_distribution
    
    Returns:
        dict: Summary of processing
    """
    scale_options = [('log', 'linear'), ('log', 'log')]
   
    summary = {'processed': [], 'skipped': [], 'errors': [], 'interrupted': False}
    all_results = []  # Store all ProcessedResult objects for LaTeX table
    
    try:
        # Process each allocator type
        allocator_types = ['arena', 'malloc', 'sdhs']
        
        for allocator_type in allocator_types:
            allocator_dir = os.path.join(logs_dir, allocator_type)
            if not os.path.isdir(allocator_dir):
                print(f"Warning: {allocator_type} directory not found in {logs_dir}")
                continue
            
            print(f"\nProcessing allocator: {allocator_type}")
            
            # Load TSC frequency for this allocator type
            tsc_freq = load_all_tsc_frequencies(allocator_dir)
            if tsc_freq is None:
                print(f"    Error: No TSC frequency found for {allocator_type}")
                summary['errors'].append(f"{allocator_type}: No TSC frequency")
                continue
            
            # Find all subdirectories within the allocator directory
            for item in os.listdir(allocator_dir):
                item_path = os.path.join(allocator_dir, item)
                
                # Skip TSC frequency files
                if item.endswith('-tsc_freq.bin'):
                    continue
                    
                if os.path.isdir(item_path):
                    # Check if this directory contains .bin files (run files)
                    bin_files = glob.glob(os.path.join(item_path, "[0-9]*.bin"))
                    if not bin_files:
                        continue
                    
                    # Parse the directory name to get allocator function and size
                    alloc_fn, size = parse_allocator_directory(item_path)
                    
                    # Check if we should process this combination
                    if not should_process_directory(allocator_type, alloc_fn, size):
                        print(f"  Skipping {item} (not in target list)")
                        summary['skipped'].append(f"{allocator_type}/{item}")
                        continue
                    
                    print(f"  Analyzing: {item}")
                    
                    # Create numbered output directory
                    output_dir = get_next_output_dir(item_path)
                    Path(output_dir).mkdir(parents=True, exist_ok=True)
                    print(f"    Created output directory: {output_dir}")
                    
                    dir_processed = True
                    
                    # Run analysis with each scale combination
                    for scale in scale_options:
                        try:
                            x_scale = scale[0]
                            y_scale = scale[1]
                            # Generate descriptive label for this permutation
                            label = f"x={x_scale}, y={y_scale}"
                            print(f"    - {label}")
                            
                            # Call the analysis function
                            fig, ax, processed_result = analyze_tsc_distribution(
                                dirpath=item_path,
                                allocator_type=allocator_type,
                                tsc_freq=tsc_freq,
                                output_dir=output_dir,
                                x_scale=x_scale,
                                y_scale=y_scale,
                                **kwargs
                            )
                            
                            # Store the result for the first scale option (avoid duplicates)
                            if scale == scale_options[0]:
                                all_results.append(processed_result)
                            
                            # Close the figure to free memory
                            plt.close(fig)
                            
                        except Exception as e:
                            print(f"      Error with {label}: {str(e)}")
                            summary['errors'].append(f"{allocator_type}/{item} - {label}: {str(e)}")
                            dir_processed = False
                    
                    if dir_processed:
                        summary['processed'].append(f"{allocator_type}/{item}")
    
    except KeyboardInterrupt:
        summary['interrupted'] = True
        print("\n\nScript interrupted by user (Ctrl+C).")
    
    # Generate LaTeX table if we have results
    if all_results:
        latex_output_file = os.path.join(logs_dir, "allocator_statistics.tex")
        generate_latex_table(all_results, latex_output_file)
    
    # Print summary
    print(f"\nSummary:")
    print(f"  Processed: {len(summary['processed'])} directories")
    print(f"  Skipped: {len(summary['skipped'])} directories")
    print(f"  Errors: {len(summary['errors'])}")
    if summary['interrupted']:
        print("  Script was interrupted")
    
    return summary

def main():
    import sys
    
    # No longer support specifying test_name since we process all allocators
    if len(sys.argv) > 1:
        print("Usage: python script.py")
        print("This script processes all allocator data in ./logs/")
        sys.exit(1)
    
    print("Analyzing all allocator data in ./logs")
    
    result = analyze_all_timing_data("./logs")
    
    if result['interrupted']:
        print("\nAnalysis was interrupted")
    else:
        print(f"\nSuccessfully processed: {result['processed']}")

if __name__ == "__main__":
    main()
