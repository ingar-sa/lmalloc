# Note: This code has been adapted from Claude's code

import re
import os
import struct
import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, Optional, BinaryIO, Tuple, Dict
from collections import Counter

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

def tsc_to_ns(tsc, tsc_freq):
    return (tsc * 1e9) / tsc_freq

def average_tsc(stats: AllocTimingStats):
    return stats.total_tsc / stats.iter

def load_timing_data(filename):
    with open(filename, 'rb') as f:
        stats, collection = parse_timing_data(f)
    return stats, collection

def parse_timing_filename(filepath):
    filename = os.path.basename(filepath)
    pattern = r"([a-zA-Z_]+)-(\d+)B\.bin"

    match = re.match(pattern, filename)
    if not match:
        raise ValueError(f"Filename '{filename}' does not match the expected format '<alloc_fn_name>-<n bytes>B.bin'")
    
    alloc_fn = match.group(1)
    n_bytes = int(match.group(2))
    
    return alloc_fn, n_bytes

def plot_timing_distribution(
    timing_arr: List[int], 
    tsc_freq: Optional[float] = None,
    use_wall_clock: bool = True,
    title: Optional[str] = None,
    alloc_fn: Optional[str] = None,
    size_bytes: Optional[int] = None,
    show_stats: bool = True,
    save_path: Optional[str] = None,
    bins: int = 50,
    figsize: Tuple[int, int] = (10, 6)
):
    """
    Plot the distribution of timing measurements.
    
    Args:
        timing_arr: List of TSC timing values
        tsc_freq: TSC frequency in Hz (required if use_wall_clock=True)
        use_wall_clock: If True, convert TSC cycles to nanoseconds (requires tsc_freq)
        title: Custom title for the plot (if None, a title will be generated)
        alloc_fn: Name of the allocation function
        size_bytes: Size in bytes for the allocation
        show_stats: Whether to display statistics on the plot
        save_path: If provided, save the plot to this path
        bins: Number of bins for the histogram
        figsize: Size of the figure (width, height) in inches
    """
    # Convert to numpy array for better handling
    timings = np.array(timing_arr)
    
    # Determine if we should convert to wall clock time
    convert_to_ns = use_wall_clock and tsc_freq is not None
    
    if convert_to_ns:
        x_values = (timings * 1e9) / tsc_freq
        x_label = "Time (ns)"
        time_unit = "ns"
    else:
        x_values = timings
        x_label = "TSC Cycles"
        time_unit = "cycles"
    
    # Create figure and axis
    fig, ax = plt.subplots(figsize=figsize)
    
    # Plot histogram
    n, bins, patches = ax.hist(x_values, bins=bins, alpha=0.7, color='skyblue', edgecolor='black')
    
    # Set labels and title
    ax.set_xlabel(x_label)
    ax.set_ylabel('Frequency')
    
    if title is None:
        title_parts = []
        if alloc_fn:
            title_parts.append(f"{alloc_fn}")
        if size_bytes:
            title_parts.append(f"{size_bytes}B")
        
        time_type = "Wall Clock Time" if convert_to_ns else "TSC Cycles"
        if title_parts:
            plot_title = f"{time_type} Distribution for {' '.join(title_parts)}"
        else:
            plot_title = f"{time_type} Distribution"
    else:
        plot_title = title
        
    ax.set_title(plot_title)
    
    # Calculate statistics
    if show_stats:
        stats = {
            "Mean": np.mean(x_values),
            "Median": np.median(x_values),
            "Min": np.min(x_values),
            "Max": np.max(x_values),
            "Std Dev": np.std(x_values),
            "95th %ile": np.percentile(x_values, 95),
            "99th %ile": np.percentile(x_values, 99)
        }
        
        # Add stats text box
        stats_text = "\n".join([f"{k}: {v:.2f} {time_unit}" for k, v in stats.items()])
        props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
        ax.text(0.05, 0.95, stats_text, transform=ax.transAxes, fontsize=10,
                verticalalignment='top', bbox=props)
        
        # Add vertical lines for mean and median
        ax.axvline(stats["Mean"], color='r', linestyle='--', alpha=0.7, 
                   label=f'Mean: {stats["Mean"]:.2f} {time_unit}')
        ax.axvline(stats["Median"], color='g', linestyle=':', alpha=0.7, 
                   label=f'Median: {stats["Median"]:.2f} {time_unit}')
        ax.legend()
    
    # Adjust layout
    plt.tight_layout()
    
    # Save if requested
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
    
    # Show plot
    plt.show()
    
    return fig, ax

# Example of comparing both visualizations for the same data
def compare_tsc_and_wall_clock(filepath, base_dir=None):
    """
    Create two plots for the same data - one in TSC cycles and one in wall clock time
    
    Args:
        filepath: Path to the timing file
        base_dir: Base directory where tsc_freq.bin is located
    """
    alloc_fn, size_bytes = parse_timing_filename(filepath)
    stats, collection = load_timing_data(filepath)
    
    if base_dir is None:
        base_dir = os.path.dirname(filepath) + "/"
    
    try:
        tsc_freq = parse_tsc_frequency(base_dir)
        print(f"TSC Frequency: {tsc_freq/1e6:.2f} MHz")
        
        # Plot using raw TSC cycles
        plot_timing_distribution(
            collection.arr,
            tsc_freq=tsc_freq,
            use_wall_clock=False,
            alloc_fn=alloc_fn,
            size_bytes=size_bytes,
            show_stats=True
        )
        
        # Plot using wall clock time (nanoseconds)
        plot_timing_distribution(
            collection.arr,
            tsc_freq=tsc_freq,
            use_wall_clock=True,
            alloc_fn=alloc_fn,
            size_bytes=size_bytes,
            show_stats=True
        )
    except Exception as e:
        print(f"Error: {str(e)}")
        print("Could not create comparison plots")

def plot_unique_tsc_counts(
    timing_arr: List[int],
    tsc_freq: Optional[float] = None,
    use_wall_clock: bool = True,
    top_n: Optional[int] = None,
    min_count: Optional[int] = None,
    title: Optional[str] = None,
    alloc_fn: Optional[str] = None,
    size_bytes: Optional[int] = None,
    save_path: Optional[str] = None,
    figsize: Tuple[int, int] = (12, 6),
    rotate_labels: bool = False
):
    """
    Plot bars representing the count of each unique TSC value in the array.
    
    Args:
        timing_arr: List of TSC timing values
        tsc_freq: TSC frequency in Hz (required if use_wall_clock=True)
        use_wall_clock: If True, convert TSC cycles to nanoseconds (requires tsc_freq)
        top_n: If provided, only show the top N most common values
        min_count: If provided, only show values that appear at least this many times
        title: Custom title for the plot (if None, a title will be generated)
        alloc_fn: Name of the allocation function
        size_bytes: Size in bytes for the allocation
        save_path: If provided, save the plot to this path
        figsize: Size of the figure (width, height) in inches
        rotate_labels: If True, rotate x-axis labels 90 degrees (helpful for many bars)
    
    Returns:
        Tuple of (figure, axis) matplotlib objects
    """
    # Convert to numpy array
    timings = np.array(timing_arr)
    
    # Determine if we should convert to wall clock time
    convert_to_ns = use_wall_clock and tsc_freq is not None
    
    if convert_to_ns:
        values = (timings * 1e9) / tsc_freq
        x_label = "Time (ns)"
        time_unit = "ns"
    else:
        values = timings
        x_label = "TSC Cycles"
        time_unit = "cycles"
    
    # Count occurrences of each unique value
    value_counts = Counter(values)
    
    # Apply minimum count filter if specified
    if min_count is not None:
        value_counts = {k: v for k, v in value_counts.items() if v >= min_count}
    
    # Get the most common values if top_n is specified
    if top_n is not None:
        value_counts = dict(value_counts.most_common(top_n))
    
    # Sort by value (for chronological order)
    sorted_items = sorted(value_counts.items())
    x_values = [item[0] for item in sorted_items]
    counts = [item[1] for item in sorted_items]
    
    # Check if we have any data to plot after filtering
    if len(x_values) == 0:
        print("No data to plot after applying filters")
        return None, None
    
    # Create the plot
    fig, ax = plt.subplots(figsize=figsize)
    bars = ax.bar(x_values, counts, alpha=0.7)
    
    # Set labels and title
    ax.set_xlabel(x_label)
    ax.set_ylabel("Count")
    
    if title:
        plot_title = title
    else:
        plot_title = f"Unique {time_unit.capitalize()} Value Counts"
        if alloc_fn:
            plot_title += f" for {alloc_fn}"
        if size_bytes:
            plot_title += f" ({size_bytes}B)"
        if top_n:
            plot_title += f" (Top {top_n})"
    
    ax.set_title(plot_title)
    
    # Add a note about the total number of unique values
    total_unique = len(value_counts)
    original_unique = len(np.unique(values))
    if total_unique != original_unique:
        ax.text(0.98, 0.98, 
                f"Showing {total_unique} of {original_unique} unique values",
                horizontalalignment='right',
                verticalalignment='top',
                transform=ax.transAxes,
                fontsize=9,
                bbox=dict(facecolor='white', alpha=0.7))
    
    # Handle x-axis labels if there are many bars
    if len(x_values) > 20 or rotate_labels:
        plt.xticks(rotation=90)
    
    # Add count labels above bars if there aren't too many
    if len(x_values) <= 30:
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height + 0.05*max(counts),
                   f'{int(height)}', ha='center', va='bottom', 
                   rotation=45 if len(x_values) > 15 else 0,
                   fontsize=8 if len(x_values) > 10 else 10)
    
    # Add summary statistics as a text box
    if len(counts) > 0:
        stats_text = (
            f"Total samples: {sum(counts)}\n"
            f"Unique values: {total_unique}\n"
            f"Most common: {sorted(value_counts.items(), key=lambda x: x[1], reverse=True)[0][0]:.2f} {time_unit} "
            f"({sorted(value_counts.items(), key=lambda x: x[1], reverse=True)[0][1]} occurrences)"
        )
        ax.text(0.02, 0.98, stats_text,
                horizontalalignment='left',
                verticalalignment='top',
                transform=ax.transAxes,
                fontsize=9,
                bbox=dict(facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=300, bbox_inches='tight')
    
    plt.show()
    return fig, ax

# Example usage function
def analyze_unique_tsc_values(filepath, base_dir=None, use_wall_clock=True, top_n=20, min_count=None):
    """
    Analyze and plot the unique TSC values in an allocation timing file.
    
    Args:
        filepath: Path to the timing file
        base_dir: Base directory where tsc_freq.bin is located (if None, use the directory of filepath)
        use_wall_clock: If True, convert TSC cycles to nanoseconds, otherwise use raw TSC cycles
        top_n: Show only the top N most frequent values (None to show all)
        min_count: Only show values that appear at least this many times (None to show all)
    """
    alloc_fn, size_bytes = parse_timing_filename(filepath)
    stats, collection = load_timing_data(filepath)
    
    if base_dir is None:
        base_dir = os.path.dirname(filepath) + "/"
    
    tsc_freq = None
    if use_wall_clock:
        try:
            tsc_freq = parse_tsc_frequency(base_dir)
            print(f"TSC Frequency: {tsc_freq/1e6:.2f} MHz")
        except Exception as e:
            print(f"Warning: Couldn't find or parse tsc_freq.bin ({str(e)})")
            print("Falling back to raw TSC cycles")
            use_wall_clock = False
    
    # Check the number of unique values to provide info to the user
    unique_count = len(np.unique(collection.arr))
    print(f"Total timing samples: {len(collection.arr)}")
    print(f"Unique timing values: {unique_count}")
    
    if unique_count > 1000 and top_n is None:
        print(f"Warning: There are {unique_count} unique values. Consider using top_n to limit the display.")
    
    # Plot the unique TSC value counts
    plot_unique_tsc_counts(
        collection.arr,
        tsc_freq=tsc_freq,
        use_wall_clock=use_wall_clock,
        top_n=top_n,
        min_count=min_count,
        alloc_fn=alloc_fn,
        size_bytes=size_bytes
    )
    
    # Also display the frequency distribution for comparison
    print("\nAlso showing regular distribution histogram for comparison:")
    plot_timing_distribution(
        collection.arr,
        tsc_freq=tsc_freq,
        use_wall_clock=use_wall_clock,
        alloc_fn=alloc_fn,
        size_bytes=size_bytes,
        show_stats=True
    )

def main():
    directory = "./logs/malloc/1/"
    filepath = directory + "malloc-128B.bin"
    #compare_tsc_and_wall_clock(filepath)
    #analyze_allocation_file(filepath, use_wall_clock=False)
    analyze_unique_tsc_values(filepath, None, False, None, None)

    alloc_fn, n_bytes = parse_timing_filename(filepath)
    stats, collection = load_timing_data(filepath)
    tsc_freq = parse_tsc_frequency(directory)
    # tsc_freq = parse_tsc_frequency("./logs/ua_nmc/21/")

    print("TSC frequency: ", tsc_freq, '\n')

    print(f"Timing data for {n_bytes}B allocations with {alloc_fn}")
    print("\tTotal time:  ", stats.total_tsc)
    print("\tTotal iter:  ", stats.iter)
    print("\tAverage TSC: ", average_tsc(stats))
    print("\tAverage ns:  ", tsc_to_ns(average_tsc(stats), tsc_freq))

    filtered_arr = sorted(collection.arr)[:-1]
    print("\nTiming data collection:")
    print("\tCount:    ", collection.count)
    print("\tSum:      ", sum(filtered_arr))
    print("\tAvg TSC:  ", sum(filtered_arr) / collection.count, tsc_freq)
    print("\tAvg ns:   ", tsc_to_ns(sum(filtered_arr) / collection.count, tsc_freq))
    print("\tSmallest: ", sorted(collection.arr)[:10])
    print("\tLargest:  ", sorted(collection.arr)[-10:])

main()
