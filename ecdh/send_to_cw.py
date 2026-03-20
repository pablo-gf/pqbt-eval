"""Flash ECDH benchmark to ChipWhisperer and retrieve results"""

import chipwhisperer as cw
import time
import os

def flash_and_run():
    """Flash hex and capture output"""
    
    scope = cw.scope()
    target = cw.target(scope, cw.targets.SimpleSerial)
    scope.default_setup()
    
    # Set baud rate explicitly 
    target.baud = 38400
    
    print(f"Flashing ecdh_benchmark.hex...")
    prog = cw.programmers.STM32FProgrammer
    cw.program_target(scope, prog, "ecdh_benchmark.hex", baud=38400)
    print("Flashing complete!")
    
    print("\nResetting target...")
    
    # Flush any pending data in buffers before reset
    target.flush()
    
    scope.io.nrst = 'low'
    time.sleep(0.1)
    scope.io.nrst = 'high_z'
    time.sleep(0.5)
    
    results = {}
    raw_data = {}  # Store raw measurements
    current_scheme = None
    current_operation = None
    current_stat = None
    collecting_raw = None  # 'keygen' or 'sharedsecret' when collecting raw data
    timeout = 600
    start_time = time.time()
    line_buffer = ""
    
    while True:
        if time.time() - start_time > timeout:
            print("\n[!] Timeout waiting for results")
            break
        
        try:
            data = target.read(timeout=10)
            if data:
                line_buffer += data
                
                while '\n' in line_buffer:
                    line, line_buffer = line_buffer.split('\n', 1)
                    line = line.strip()
                    if line:
                        print(line)
                        
                        # Parse results
                        if "ECDH P-192" in line:
                            current_scheme = "P-192"
                            results[current_scheme] = {}
                            raw_data[current_scheme] = {}
                            collecting_raw = None
                        elif "ECDH P-256" in line:
                            current_scheme = "P-256"
                            results[current_scheme] = {}
                            raw_data[current_scheme] = {}
                            collecting_raw = None
                        elif "keygen cycles:" in line:
                            current_operation = "keygen"
                            current_stat = "avg"
                            collecting_raw = None
                            if current_scheme:
                                results[current_scheme][current_operation] = {}
                        elif "sharedsecret cycles:" in line:
                            current_operation = "sharedsecret"
                            current_stat = "avg"
                            collecting_raw = None
                            if current_scheme:
                                results[current_scheme][current_operation] = {}
                        elif line == "min:":
                            current_stat = "min"
                            collecting_raw = None
                        elif line == "max:":
                            current_stat = "max"
                            collecting_raw = None
                        elif line == "keygen_raw:":
                            collecting_raw = "keygen"
                            if current_scheme:
                                raw_data[current_scheme]["keygen"] = []
                        elif line == "sharedsecret_raw:":
                            collecting_raw = "sharedsecret"
                            if current_scheme:
                                raw_data[current_scheme]["sharedsecret"] = []
                        elif line == "#":
                            collecting_raw = None
                        elif collecting_raw and current_scheme:
                            # Collecting raw measurements
                            try:
                                cycles = int(line)
                                raw_data[current_scheme][collecting_raw].append(cycles)
                            except ValueError:
                                pass
                        elif current_scheme and current_operation and current_stat:
                            # Parse summary statistics
                            try:
                                cycles = int(line)
                                results[current_scheme][current_operation][current_stat] = cycles
                            except ValueError:
                                pass
                        elif "DONE" in line:
                            print("=" * 60)
                            print("[*] Benchmark complete!")
                            scope.dis()
                            return results, raw_data
        except Exception as e:
            pass
    
    scope.dis()
    return results, raw_data

def save_results(results, raw_data, clock_mhz=36):
    """Save results to files in pqm4 style"""
    
    # Create benchmarks directory if it doesn't exist
    os.makedirs("benchmarks", exist_ok=True)
    
    # Calculate statistics from raw data if not already present
    for scheme, operations in raw_data.items():
        if scheme not in results:
            results[scheme] = {}
        for op, cycles_list in operations.items():
            if cycles_list:
                if op not in results[scheme]:
                    results[scheme][op] = {}
                results[scheme][op]['avg'] = sum(cycles_list) // len(cycles_list)
                results[scheme][op]['min'] = min(cycles_list)
                results[scheme][op]['max'] = max(cycles_list)
    
    # Save raw data for each scheme in separate files
    for scheme, operations in raw_data.items():
        scheme_name = scheme.lower().replace("-", "")  # "p-192" -> "p192"
        
        # Save keygen measurements
        if "keygen" in operations:
            filename = f"benchmarks/ecdh_{scheme_name}_keygen.txt"
            with open(filename, 'w') as f:
                for cycles in operations["keygen"]:
                    f.write(f"{cycles}\n")
            print(f"Saved raw keygen data to {filename}")
        
        # Save shared secret measurements
        if "sharedsecret" in operations:
            filename = f"benchmarks/ecdh_{scheme_name}_sharedsecret.txt"
            with open(filename, 'w') as f:
                for cycles in operations["sharedsecret"]:
                    f.write(f"{cycles}\n")
            print(f"Saved raw sharedsecret data to {filename}")
    
    # Save summary statistics
    with open('benchmarks/ecdh_summary.txt', 'w') as f:
        f.write(f"ECDH Benchmark Results - Summary\n")
        f.write(f"Clock: {clock_mhz} MHz\n")
        f.write(f"{'='*60}\n\n")
        
        for scheme, ops in results.items():
            f.write(f"{scheme}:\n")
            for op, stats in ops.items():
                if 'avg' in stats:
                    cycles = stats['avg']
                    time_ms = (cycles / (clock_mhz * 1_000_000)) * 1000
                    f.write(f"  {op:15s}: {cycles:>12,} cycles ({time_ms:>8.3f} ms)\n")
                if 'min' in stats:
                    f.write(f"    min: {stats['min']:>12,} cycles\n")
                if 'max' in stats:
                    f.write(f"    max: {stats['max']:>12,} cycles\n")
            f.write("\n")
    
    print("Saved summary to benchmarks/ecdh_summary.txt")
    
    # Print summary to console
    print("\n" + "=" * 60)
    print("ECDH Benchmark Results - Summary")
    print("=" * 60)
    
    for scheme, ops in results.items():
        print(f"\n{scheme}:")
        for op, stats in ops.items():
            if 'avg' in stats:
                cycles = stats['avg']
                time_ms = (cycles / (clock_mhz * 1_000_000)) * 1000
                print(f"  {op:15s}: {cycles:>12,} cycles ({time_ms:>8.3f} ms)")

if __name__ == "__main__":
    
    print("ECDH Benchmark")
    print("=" * 60)
    
    results, raw_data = flash_and_run()
    
    if results:
        save_results(results, raw_data)
    else:
        print("[!] No results captured")