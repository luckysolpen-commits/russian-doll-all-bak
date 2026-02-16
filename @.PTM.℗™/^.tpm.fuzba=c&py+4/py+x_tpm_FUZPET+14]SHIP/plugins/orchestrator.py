# This piece is the orchestrator.
# It launches and manages all other pieces as separate processes.

import sys
import subprocess
import time

# --- Pieces to Launch ---
# The order can matter for initialization.
# e.g., game_logic should run to create the first frame before renderer starts.
PIECES = [
    'plugins/fuzzpet.py',
    'plugins/game_logic.py',
    'plugins/master_ledger_monitor.py',
    'plugins/renderer.py',
    'plugins/keyboard.py',
    'plugins/response_handler.py',
]

def main():
    """Launches all pieces as subprocesses and manages them."""
    print("Orchestrator starting...", file=sys.stderr)
    
    processes = []
    try:
        # Launch each piece
        for piece_script in PIECES:
            print(f"  -> Launching {piece_script}...", file=sys.stderr)
            # We use sys.executable to ensure we use the same python interpreter
            # that is running the orchestrator. -u is for unbuffered output.
            # Allow renderer and keyboard to access terminal for display/input,
            # but redirect other processes
            if "renderer.py" in piece_script or "keyboard.py" in piece_script:
                # Renderer needs to output to terminal for display
                # Keyboard needs to access terminal for input
                proc = subprocess.Popen([sys.executable, "-u", piece_script])
            else:
                # Other processes should not interfere with display/input
                proc = subprocess.Popen([sys.executable, "-u", piece_script],
                                      stdout=subprocess.DEVNULL,
                                      stderr=subprocess.DEVNULL)
            processes.append(proc)
            print(f"     {piece_script} started with PID: {proc.pid}", file=sys.stderr)
            # A small delay to let pieces initialize in order
            time.sleep(0.5)

        print("\nAll pieces launched. System is running.", file=sys.stderr)
        print("Press Ctrl+C in the terminal running 'run_fuzzpet.py' to stop.", file=sys.stderr)

        # Monitor child processes
        while True:
            # Check if any process has terminated
            for proc in processes:
                if proc.poll() is not None: # Process has terminated
                    if proc.returncode != 0:
                        print(f"\nERROR: Piece '{PIECES[processes.index(proc)]}' (PID: {proc.pid}) exited with error code {proc.returncode}.", file=sys.stderr)
                        # Trigger shutdown of other processes
                        raise KeyboardInterrupt # Use KeyboardInterrupt to trigger the finally block
                    else:
                        print(f"\nPiece '{PIECES[processes.index(proc)]}' (PID: {proc.pid}) terminated gracefully.", file=sys.stderr)
                        # Remove from active processes
                        processes.remove(proc)
                        if not processes: # All processes are done
                            print("All pieces terminated gracefully. Orchestrator exiting.", file=sys.stderr)
                            return
            time.sleep(0.5) # Check every 0.5 seconds

    except KeyboardInterrupt:
        print("\nOrchestrator received shutdown signal.", file=sys.stderr)
    except Exception as e:
        print(f"\nAn unexpected error occurred in the orchestrator: {e}", file=sys.stderr)
    finally:
        print("Terminating all piece processes...", file=sys.stderr)
        for proc in reversed(processes):
            if proc.poll() is None: # Check if process is still running
                try:
                    print(f"  -> Terminating PID {proc.pid}...", file=sys.stderr)
                    proc.terminate()
                    proc.wait(timeout=2) # Wait for graceful exit
                except subprocess.TimeoutExpired:
                    print(f"     PID {proc.pid} did not terminate, killing.", file=sys.stderr)
                    proc.kill()
                except Exception as e:
                    print(f"     Error terminating PID {proc.pid}: {e}", file=sys.stderr)
        print("All pieces terminated. Orchestrator shutting down.", file=sys.stderr)


if __name__ == "__main__":
    main()
