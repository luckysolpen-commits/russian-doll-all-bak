"""
Event Manager plugin - Same as game's event manager but for menu system
Handles scheduled events based on clock time
Works with events piece to execute timed methods
"""

from plugin_interface import PluginInterface


class Plugin(PluginInterface):
    def __init__(self, game_engine):
        super().__init__(game_engine)
        self.piece_manager = None
        self.events_piece_name = 'clock_events'
        self.clock_piece_name = 'clock'
        self._last_checked_time = 0
        
    def initialize(self):
        """Initialize event manager plugin"""
        print("Event Manager plugin initialized")
        
        # Find the piece manager plugin
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, 'get_piece_state'):
                self.piece_manager = plugin
                break
    
    def check_events(self):
        """Check for and execute any events that should trigger now"""
        if not self.piece_manager:
            return
            
        # Get current clock time
        clock_state = self.piece_manager.get_piece_state(self.clock_piece_name)
        if not clock_state:
            return
            
        current_time = clock_state.get('current_time', 0)
        
        # Check if this is a new turn (time has increased)
        if hasattr(self, '_last_checked_time'):
            if current_time > self._last_checked_time:
                # A turn has ended, trigger turn_end events
                self._trigger_event_by_type('turn_end')
        else:
            self._last_checked_time = current_time
        
        self._last_checked_time = current_time
        
        # Get event definitions from events piece
        events_state = self.piece_manager.get_piece_state(self.events_piece_name)
        if not events_state:
            return
            
        # Check for events scheduled at this time
        executed_events = 0
        
        # Check for registered events (event_0_time, event_1_time, etc.)
        i = 0
        while True:
            event_time_key = f'event_{i}_time'
            event_method_key = f'event_{i}_method'
            event_target_key = f'event_{i}_target'
            
            if event_time_key not in events_state:
                break
                
            event_time = events_state[event_time_key]
            
            # Check if this event should trigger now
            if current_time == event_time or (events_state.get('recurring_interval', 0) > 0 and 
                                             current_time % events_state['recurring_interval'] == 0 and
                                             event_time <= current_time):
                
                event_method = events_state.get(event_method_key)
                event_target = events_state.get(event_target_key)
                
                if event_method and event_target:
                    self.execute_event(event_method, event_target)
                    executed_events += 1
                    
            i += 1
        
        if executed_events > 0:
            print(f"Executed {executed_events} scheduled events at time {current_time}")
    
    def _trigger_event_by_type(self, event_type):
        """Trigger methods on pieces that listen to the specified event type"""
        if not self.piece_manager:
            return
            
        # Look through all pieces to find those that listen to this event type
        triggered_count = 0
        for piece_name, piece_data in self.piece_manager.pieces.items():
            # Check if this piece has event listeners
            if 'state' in piece_data and 'event_listeners' in piece_data['state']:
                event_listeners = piece_data['state']['event_listeners']
                
                # Convert string to list if it's in string format
                if isinstance(event_listeners, str):
                    # Remove brackets and split
                    listeners_str = event_listeners.strip('[]')
                    event_listener_list = [item.strip() for item in listeners_str.split(',')] if ',' in listeners_str else [listeners_str]
                elif isinstance(event_listeners, list):
                    event_listener_list = event_listeners
                else:
                    continue
                
                # Check if this piece listens for the event type
                if event_type in event_listener_list:
                    # Try to call the corresponding method
                    method_name = f"respond_to_{event_type}"
                    self._execute_response_method(piece_name, method_name)
                    triggered_count += 1
        
        if triggered_count > 0:
            print(f"Triggered {triggered_count} responses for '{event_type}' event")
    
    def _execute_response_method(self, piece_name, method_name):
        """Execute a specific response method on a piece"""
        # Find the appropriate plugin to handle this method
        # First, specifically look for the NPC plugin for NPC-related methods
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            # Check if this is an NPC piece and use the NPC plugin
            piece_state = self.piece_manager.get_piece_state(piece_name)
            if piece_state and piece_state.get('type') == 'npc' and method_name.startswith('respond_to_'):
                if hasattr(plugin, '__class__') and 'npc' in plugin.__class__.__name__.lower():
                    if hasattr(plugin, method_name):
                        # Create a temporary object to represent the piece state
                        if piece_state:
                            temp_obj = type('TempObj', (), {})()
                            temp_obj.state = piece_state
                            temp_obj.piece_name = piece_name
                            try:
                                result = getattr(plugin, method_name)(temp_obj)
                                # Update the piece state if it was modified
                                if hasattr(temp_obj, 'state'):
                                    self.piece_manager.update_piece_state(piece_name, temp_obj.state)
                                return result
                            except Exception as e:
                                print(f"Error executing {method_name} for {piece_name}: {e}")
                                return False
            
            # Otherwise, try to find any plugin that has the method
            if hasattr(plugin, method_name):
                # Create a temporary object to represent the piece state
                piece_state = self.piece_manager.get_piece_state(piece_name)
                if piece_state:
                    temp_obj = type('TempObj', (), {})()
                    temp_obj.state = piece_state
                    temp_obj.piece_name = piece_name
                    try:
                        result = getattr(plugin, method_name)(temp_obj)
                        # Update the piece state if it was modified
                        if hasattr(temp_obj, 'state'):
                            self.piece_manager.update_piece_state(piece_name, temp_obj.state)
                        return result
                    except Exception as e:
                        print(f"Error executing {method_name} for {piece_name}: {e}")
                        return False
        return False
    
    def execute_event(self, method_name, target_piece):
        """Execute a specific event by calling a method on a target piece"""
        if not self.piece_manager:
            return False
            
        # Find the methods plugin to execute the method
        methods_plugin = None
        for plugin in self.game_engine.plugin_manager.plugin_instances:
            if hasattr(plugin, f'method_{method_name}'):
                methods_plugin = plugin
                break
        
        if methods_plugin:
            # Call the appropriate method with the target piece
            method_func = getattr(methods_plugin, f'method_{method_name}')
            
            if target_piece == 'clock':
                # For clock targets, pass the clock state or just call the method directly
                clock_state = self.piece_manager.get_piece_state(target_piece)
                if clock_state and hasattr(method_func, '__call__'):
                    try:
                        method_func(target_piece)
                    except:
                        method_func(None)  # Some methods might accept None
                        
            elif target_piece == 'player1':
                # For player targets, get player state and use it
                player_state = self.piece_manager.get_piece_state(target_piece)
                if player_state and hasattr(method_func, '__call__'):
                    # Create a temporary object with the state for the method
                    temp_obj = type('TempObj', (), {})()
                    temp_obj.state = player_state
                    success = method_func(temp_obj)
                    if success:
                        # Update the piece state after the method modifies it
                        self.piece_manager.update_piece_state(target_piece, temp_obj.state)
            
            else:
                # For other targets, try calling the method directly
                try:
                    method_func(target_piece)
                except:
                    # If method expects a state object, try creating one
                    target_state = self.piece_manager.get_piece_state(target_piece)
                    if target_state:
                        temp_obj = type('TempObj', (), {})()
                        temp_obj.state = target_state
                        try:
                            success = method_func(temp_obj)
                            if success and hasattr(temp_obj, 'state'):
                                self.piece_manager.update_piece_state(target_piece, temp_obj.state)
                        except:
                            pass
            
            return True
        
        return False
    
    def register_event(self, time, method, target, recurring=False):
        """Register a new event (not used in basic implementation)"""
        pass
    
    def execute_at_time(self, time, method, target):
        """Execute a method at a specific time (not used in basic implementation)"""
        pass