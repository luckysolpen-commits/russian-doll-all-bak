"""
Menu System Entry Point
This file serves as the main entry point for the TPM-compliant menu system
"""

from main_menu_engine import MenuEngine

def main():
    """Main entry point for the menu system"""
    print("Starting TPM Compliant Menu System...")
    print("This system uses the same plugin architecture as the game but for menu navigation.")
    print()
    engine = MenuEngine()
    engine.run()

if __name__ == "__main__":
    main()