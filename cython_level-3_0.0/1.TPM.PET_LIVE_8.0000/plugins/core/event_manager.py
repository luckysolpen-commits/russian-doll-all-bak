"""
Event Manager plugin for the Pet Philosophy project
Handles events based on entries in the master ledger following TPM principles
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.events_piece_name = 'events'
        self.master_ledger_file = 'pieces/master_ledger/ledger.txt'
        self._last_checked_position = 0
        
    def initialize(self):
        """Initialize event manager plugin"""
        print("Event Manager plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break

    def process_recent_events(self):
        """Process any new events from the master ledger file"""
        if not self.piece_manager:
            return

        try:
            with open(self.master_ledger_file, 'r') as ledger:
                ledger.seek(self._last_checked_position)
                new_lines = ledger.readlines()
                self._last_checked_position = ledger.tell()
                
            for line in new_lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                self.process_event_line(line)
        except FileNotFoundError:
            pass  # Wait for file to be created

    def process_event_line(self, line):
        """Process a single event line from the master ledger"""
        if 'EventFire:' in line:
            self._handle_event_fire(line)
        elif 'MethodCall:' in line:
            self._handle_method_call(line)
        elif 'MovementEvent:' in line:
            self._handle_movement_event(line)
        elif 'CommandExecuted:' in line:
            self._handle_command_executed(line)
        elif 'Response:' in line:
            self._handle_response_event(line)

    def _handle_event_fire(self, line):
        """Handle an event fire from the master ledger"""
        try:
            # Parse: [TIMESTAMP] EventFire: event_name on piece_id | Trigger: source
            parts = line.split('EventFire: ')[1].split(' |')[0]
            event_and_target = parts.split(' on ')
            if len(event_and_target) >= 2:
                event_name = event_and_target[0].strip()
                piece_id = event_and_target[1].strip()
                
                # Trigger response methods based on piece event listeners
                self._trigger_response_for_event(piece_id, event_name)
        except (IndexError, AttributeError):
            pass

    def _handle_method_call(self, line):
        """Handle a method call from the master ledger"""
        try:
            # Parse: [TIMESTAMP] MethodCall: method_name on piece_id | Caller: source
            parts = line.split('MethodCall: ')[1].split(' |')[0]
            method_and_target = parts.split(' on ')
            if len(method_and_target) >= 2:
                method_name = method_and_target[0].strip()
                piece_id = method_and_target[1].strip()
                
                # Execute the method call on the piece
                self._execute_method_on_piece(piece_id, method_name)
        except (IndexError, AttributeError):
            pass

    def _handle_movement_event(self, line):
        """Handle movement events to update display"""
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'update_display'):
                plugin.update_display()

    def _handle_command_executed(self, line):
        """Handle command execution events"""
        # Just ensure display updates after command execution
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'update_display'):
                plugin.update_display()

    def _handle_response_event(self, line):
        """Handle response events - print them to console"""
        try:
            response_part = line.split('Response:')[1].split(' | Message:')
            if len(response_part) >= 2:
                piece_and_event = response_part[0].strip()
                message = response_part[1].strip().strip('"')
                
                print(f"[PET] {message}")
        except (IndexError, AttributeError):
            pass

    def _trigger_response_for_event(self, piece_id, event_name):
        """Trigger the appropriate response for an event on a piece"""
        if not self.piece_manager:
            return

        piece_state = self.piece_manager.get_piece_state(piece_id)
        if not piece_state:
            return

        # Look for responses defined in the piece
        responses = piece_state.get('responses', {})
        if isinstance(responses, dict):
            message = responses.get(event_name, responses.get('default', f'Default response for {event_name} on {piece_id}'))
            print(f"[{piece_id.upper()}] {message}")

    def _execute_method_on_piece(self, piece_id, method_name):
        """Execute a method on a specific piece (simplified for this example)"""
        # In a full implementation, this would call the appropriate plugin method
        # For now, we'll just log that the method was called
        print(f"[EVENT_MANAGER] Method {method_name} called on {piece_id}")

    def update(self, dt):
        """Update method checks for new events in the master ledger"""
        self.process_recent_events()