# pet_plugin.py - All pet actions

def feed(piece_id, amount=20):
    global _piece_manager_ref
    hunger = _piece_manager_ref.get_state(piece_id, 'hunger')
    new_hunger = max(0, hunger - amount)
    happiness_boost = min(20, amount // 2)
    happiness = _piece_manager_ref.get_state(piece_id, 'happiness') + happiness_boost
    return {
        'hunger': new_hunger,
        'happiness': min(100, happiness)
    }

def play(piece_id, intensity=30):
    global _piece_manager_ref
    energy = _piece_manager_ref.get_state(piece_id, 'energy')
    happiness = _piece_manager_ref.get_state(piece_id, 'happiness')
    new_energy = max(0, energy - intensity)
    new_happiness = min(100, happiness + intensity // 2)
    return {
        'energy': new_energy,
        'happiness': new_happiness
    }

def sleep(piece_id, hours=8):
    global _piece_manager_ref
    energy = _piece_manager_ref.get_state(piece_id, 'energy')
    new_energy = min(100, energy + hours * 10)
    return {'energy': new_energy}

def status(piece_id):
    global _piece_manager_ref
    state = _piece_manager_ref.get_state(piece_id)
    print(f"\n=== {state.get('name', 'Pet')} Status ===")
    print(f"Hunger:    {state.get('hunger', 0)}")
    print(f"Happiness: {state.get('happiness', 0)}")
    print(f"Energy:    {state.get('energy', 0)}")
    print(f"Level:     {state.get('level', 1)}")
    return None  # no state change

def evolve(piece_id):
    global _piece_manager_ref
    level = _piece_manager_ref.get_state(piece_id, 'level')
    if level >= 5:
        print("[EVOLVE] FuzzPet reached max level!")
        return None
    new_level = level + 1
    print(f"[EVOLVE] Level up! Now level {new_level}")
    return {'level': new_level}

# pet_plugin.py - Now reads personality from Piece data

# Global reference set by main module after initialization
_piece_manager_ref = None
_methods_manager_ref = None
_event_manager_ref = None

def set_managers(pm, mm, em):
    """Called by main module to set global references"""
    global _piece_manager_ref, _methods_manager_ref, _event_manager_ref
    _piece_manager_ref = pm
    _methods_manager_ref = mm
    _event_manager_ref = em

def respond_to_event(piece_id, event_name):
    """
    Generic listener response: Reads personality text from the Piece's 'responses' section.
    Falls back to default if no match.
    """
    global _piece_manager_ref
    # Use the cached reference to piece_manager
    responses = _piece_manager_ref.pieces[piece_id].get('responses', {})
    message = responses.get(event_name, responses.get('default', '*confused* ...what?'))
    print(f"[Pet] {message}")
    
    # Log the response to master ledger as well
    import time
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
    log_entry = f"[{timestamp}] Response: {piece_id} {event_name} | Message: {message}\n"
    with open('pieces/master_ledger/master_ledger.txt', 'a') as ledger:
        ledger.write(log_entry)
    
    return None  # no state change from personality

# Register the generic responder for all events we care about
# (We can register it multiple times or make one listener per event)
def respond_to_fed(piece_id):
    respond_to_event(piece_id, 'fed')

def respond_to_played(piece_id):
    respond_to_event(piece_id, 'played')

def respond_to_slept(piece_id):
    respond_to_event(piece_id, 'slept')

def respond_to_command_received(piece_id):
    respond_to_event(piece_id, 'command_received')

# Optional: Add evolve response too
def respond_to_evolve(piece_id):
    respond_to_event(piece_id, 'evolve')
