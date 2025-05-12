import os
import glob
import struct
import re
import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, BinaryIO

# Using the functions you provided
# (AllocTimingStats, AllocTimingCollection, etc.)

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

def main():
    # Base directories
    arena_dir = "logs/temparena/"
    # malloc_dir = "logs/malloc/"
    
    # Find all test directories
    arena_tests = glob.glob(os.path.join(arena_dir, "*-*"))
    # malloc_tests = glob.glob(os.path.join(malloc_dir, "*-*"))
    
    # Load TSC frequencies
    arena_tsc_freq = load_all_tsc_frequencies(arena_dir)
    # malloc_tsc_freq = load_all_tsc_frequencies(malloc_dir)
    
    # Use the average frequency if both are available
    # if arena_tsc_freq and malloc_tsc_freq:
    #     if abs(arena_tsc_freq - malloc_tsc_freq) > 1e6:
    #         print(f"Warning: Different TSC frequencies detected: arena={arena_tsc_freq/1e6:.2f}MHz, malloc={malloc_tsc_freq/1e6:.2f}MHz")
    #     tsc_freq = np.mean([arena_tsc_freq, malloc_tsc_freq])
    # else:
    tsc_freq = arena_tsc_freq or malloc_tsc_freq
    
    # Data structure to hold results: {allocator: {size: avg_time_ns}}
    allocator_data = {}
    
    # Process arena test directories
    for test_dir in arena_tests:
        alloc_fn, size = parse_timing_directory(test_dir)
        if not isinstance(size, int):
            continue
            
        total_stats, all_timings, num_runs = load_aggregate_timing_data(test_dir)
        avg_tsc = average_tsc(total_stats)
        avg_time_ns = tsc_to_ns(avg_tsc, tsc_freq)
        
        if alloc_fn not in allocator_data:
            allocator_data[alloc_fn] = {}
        allocator_data[alloc_fn][size] = avg_time_ns
        
        print(f"Processed {alloc_fn} - {size}B: {avg_time_ns:.2f} ns (from {num_runs} runs)")
    
    # Process malloc test directories
    # for test_dir in malloc_tests:
    #     alloc_fn, size = parse_timing_directory(test_dir)
    #     if not isinstance(size, int):
    #         continue
    #         
    #     total_stats, all_timings, num_runs = load_aggregate_timing_data(test_dir)
    #     avg_tsc = average_tsc(total_stats)
    #     avg_time_ns = tsc_to_ns(avg_tsc, tsc_freq)
    #     
    #     if alloc_fn not in allocator_data:
    #         allocator_data[alloc_fn] = {}
    #     allocator_data[alloc_fn][size] = avg_time_ns
    #     
    #     print(f"Processed {alloc_fn} - {size}B: {avg_time_ns:.2f} ns (from {num_runs} runs)")
    
    # Generate LaTeX plot
    generate_latex_plot(allocator_data)
    
def generate_latex_plot(allocator_data):
    # Colors for different allocators
    colors = {
        # 'ka_alloc': 'red',
        # 'oka_alloc': 'blue',
        'ua_alloc': 'green',
        # 'malloc': 'black'
    }
    
    # Prepare plot data
    plot_data = {}
    for allocator, size_data in allocator_data.items():
        sizes = sorted(size_data.keys())
        times = [size_data[size] for size in sizes]
        plot_data[allocator] = (sizes, times)
    
    # Generate LaTeX code
    latex_code = r'''
\documentclass{article}
\usepackage{pgfplots}
\usepackage{tikz}
\pgfplotsset{compat=1.17}

\begin{document}

\begin{figure}
    \centering
    \begin{tikzpicture}
        \begin{axis}[
            title={Memory Allocator Performance by Allocation Size},
            xlabel={Allocation Size (bytes)},
            ylabel={Average Time (ns)},
            grid=both,
            legend pos=north west,
            width=12cm,
            height=8cm,
            xmode=log,  % Logarithmic x-axis for better visualization of size ranges
        ]
'''
    
    # Add each allocator's data
    for allocator, (sizes, times) in plot_data.items():
        color = colors.get(allocator, 'black')
        data_points = ' '.join([f'({size}, {time})' for size, time in zip(sizes, times)])
        
        latex_code += f'''        
        \\addplot[
            color={color},
            mark=*,
        ] coordinates {{
            {data_points}
        }};
        \\addlegendentry{{{allocator}}}
'''
    
    # Close the LaTeX code
    latex_code += r'''        \end{axis}
    \end{tikzpicture}
    \caption{Comparison of memory allocator performance across different allocation sizes.}
    \label{fig:allocator-performance}
\end{figure}

\end{document}
'''
    
    # Save LaTeX code to file
    with open('allocator_performance.tex', 'w') as f:
        f.write(latex_code)
    print("LaTeX plot code saved to allocator_performance.tex")

if __name__ == "__main__":
    main()
