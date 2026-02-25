#!/usr/bin/env python3

import subprocess
import os
import sys

def main():
    """Main execution script for the Python TPM application."""
    print("Starting TPM Python application...")
    
    # Ensure the current working directory is the base of the project
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)
    
    # Set project root in location_kvp (for dynamic path resolution)
    os.makedirs(os.path.join(script_dir, 'pieces/locations'), exist_ok=True)
    with open(os.path.join(script_dir, 'pieces/locations/location_kvp'), 'w') as f:
        f.write(f"project_root={script_dir}\n")
        f.write(f"pieces_dir={script_dir}/pieces\n")

    # Launch the orchestrator from the system piece
    print("Launching orchestrator from pieces/system/plugins/...")
    orchestrator_process = subprocess.Popen(
        [sys.executable, "-u", "pieces/system/plugins/orchestrator.py"],
        stdout=sys.stdout,
        stderr=sys.stderr
    )
    print(f"Orchestrator started with PID: {orchestrator_process.pid}")

    try:
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
