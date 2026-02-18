#!/usr/bin/env python3

import subprocess
import os
import sys

def main():
    """Main execution script for the Python TPM application."""
    print("Starting TPM Python application...")
    
    # Ensure the current working directory is the base of the project
    # This might be important if run_fuzzpet.py is executed from elsewhere
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    # Launch the orchestrator in the background
    print("Launching orchestrator.py...")
    # Using python -u for unbuffered output to see prints immediately
    orchestrator_process = subprocess.Popen(
        [sys.executable, "-u", "plugins/orchestrator.py"],
        stdout=sys.stdout,
        stderr=sys.stderr
    )
    print(f"Orchestrator started with PID: {orchestrator_process.pid}")

    try:
        # Wait for the orchestrator to finish (or handle its lifecycle)
        orchestrator_process.wait()
    except KeyboardInterrupt:
        print("\nTermination signal received. Shutting down orchestrator...")
        orchestrator_process.terminate()
        orchestrator_process.wait()
    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        print("TPM Python application finished.")

if __name__ == "__main__":
    main()
