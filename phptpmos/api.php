<?php
/**
 * Main API endpoint for GL-OS Desktop
 */

require_once 'pieces/system/bootstrap.php';

header('Content-Type: application/json');

// Parse request
$input = json_decode(file_get_contents('php://input'), true);
$action = $input['action'] ?? null;

// Initialize session directory
$sessionDir = dirname(__FILE__) . '/pieces/apps/gl_os/session';
if (!is_dir($sessionDir)) {
    mkdir($sessionDir, 0755, true);
}

// Create manager
$manager = new GLOSManager($sessionDir);

// Handle actions
$response = ['success' => false, 'error' => 'Unknown action'];

try {
    switch ($action) {
        case 'init_session':
            $sessionId = $manager->initSession();
            $response = [
                'success' => true,
                'sessionId' => $sessionId,
                'message' => 'Session initialized'
            ];
            break;
        
        case 'create_window':
            $response = $manager->createWindow(
                $input['sessionId'],
                $input['windowId'] ?? 'window_' . time(),
                $input['x'] ?? 50,
                $input['y'] ?? 50,
                $input['width'] ?? 600,
                $input['height'] ?? 400
            );
            break;
        
        case 'close_window':
            $response = $manager->closeWindow(
                $input['sessionId'],
                $input['windowId']
            );
            break;
        
        case 'poll_frame':
            $response = $manager->pollFrame($input['sessionId']);
            break;
        
        case 'send_input':
            $response = $manager->sendInput(
                $input['sessionId'],
                $input['windowId'],
                $input['input'] ?? ''
            );
            break;
        
        default:
            $response = ['success' => false, 'error' => 'Unknown action: ' . $action];
    }
} catch (Exception $e) {
    $response = ['success' => false, 'error' => $e->getMessage()];
}

// Send response
echo json_encode($response, JSON_PRETTY_PRINT);

?>
