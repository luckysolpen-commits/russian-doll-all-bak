<?php
// bridge.php - Simple File-Based IPC for GoDaddy/Web environments
header('Content-Type: application/json');

$action = $_GET['action'] ?? '';
$path = $_GET['path'] ?? '';
// TPM-Compliant: Dynamic root resolution relative to this script
$root = realpath(__DIR__ . '/../');

if (!$path || strpos(realpath($root . '/' . $path), $root) !== 0) {
    echo json_encode(['error' => 'Invalid path']);
    exit;
}

$full_path = $root . '/' . $path;

switch ($action) {
    case 'read':
        if (file_exists($full_path)) {
            echo json_encode(['content' => file_get_contents($full_path)]);
        } else {
            echo json_encode(['error' => 'File not found']);
        }
        break;

    case 'write':
    case 'append':
        $data = file_get_contents('php://input');
        $mode = ($action === 'append') ? 'a' : 'w';
        $f = fopen($full_path, $mode);
        if ($f) {
            flock($f, LOCK_EX);
            fwrite($f, $data);
            fflush($f);
            flock($f, LOCK_UN);
            fclose($f);
            
            // Pulse the changed file if needed
            if (strpos($path, 'current_frame.txt') !== false || strpos($path, 'history.txt') !== false) {
                 $changed_suffix = (strpos($path, 'history') !== false ? 'history_changed.txt' : 'frame_changed.txt');
                 file_put_contents(dirname($full_path) . '/' . $changed_suffix, "X\n", FILE_APPEND);
            }
            
            echo json_encode(['status' => 'ok']);
        } else {
            echo json_encode(['error' => 'Write/Append failed']);
        }
        break;

    case 'stat':
        if (file_exists($full_path)) {
            echo json_encode(['size' => filesize($full_path), 'mtime' => filemtime($full_path)]);
        } else {
            echo json_encode(['error' => 'File not found']);
        }
        break;

    default:
        echo json_encode(['error' => 'Invalid action']);
}
