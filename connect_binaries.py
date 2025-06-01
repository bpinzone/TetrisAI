#!/usr/bin/python3

# ./connect_binaries.py ./main ./python_ui/tetris_ui.py --args1 t 0 4 6 10000 20 e

import subprocess
import threading
import sys
import argparse

def pipe_stream(source, target, prefix=''):
    """Pipe data from source stream to target stream"""
    try:
        for line in source:
            # Print the intercepted communication (optional)
            print(f"{prefix}: {line.strip()}")
            target.write(line)
            target.flush()
    except BrokenPipeError:
        print(f"Pipe closed for {prefix}")

def main():
    parser = argparse.ArgumentParser(description='Connect two executables with bidirectional pipes')
    parser.add_argument('executable1', help='Path to first executable')
    parser.add_argument('executable2', help='Path to second executable')
    parser.add_argument('--args1', nargs='*', default=[], help='Arguments for first executable')
    parser.add_argument('--args2', nargs='*', default=[], help='Arguments for second executable')
    
    args = parser.parse_args()

    # Start both programs with pipes
    program1 = subprocess.Popen(
        [args.executable1] + args.args1,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=1
    )
    
    program2 = subprocess.Popen(
        [args.executable2] + args.args2,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=1
    )
    
    # Create threads to handle the piping in both directions
    thread_1_to_2 = threading.Thread(
        target=pipe_stream, 
        args=(program1.stdout, program2.stdin, "1->2"),
        daemon=True
    )
    
    thread_2_to_1 = threading.Thread(
        target=pipe_stream, 
        args=(program2.stdout, program1.stdin, "2->1"),
        daemon=True
    )
    
    # Start the piping threads
    thread_1_to_2.start()
    thread_2_to_1.start()
    
    try:
        # Wait for the programs to finish
        program1.wait()
        program2.wait()
    except KeyboardInterrupt:
        print("\nShutting down...")
        program1.terminate()
        program2.terminate()
    finally:
        # Clean up
        program1.stdout.close()
        program1.stdin.close()
        program2.stdout.close()
        program2.stdin.close()

if __name__ == "__main__":
    main() 