#!/bin/bash
# web_chtpm/php_server.sh
cd "$(dirname "$0")"
../setup_pieces.sh
echo "Starting CHTPM Web Server at http://localhost:8001"
php -S localhost:8001
