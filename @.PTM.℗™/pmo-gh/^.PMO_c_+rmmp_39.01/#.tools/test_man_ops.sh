#!/bin/bash
# test_man_ops.sh - Quick test for man-ops and man-pal

echo "=== MAN-OPS / MAN-PAL TEST ==="
echo ""

# Check project structures
echo "1. Checking project structures..."
[ -d "projects/man-ops" ] && echo "   ✓ projects/man-ops exists" || echo "   ✗ projects/man-ops MISSING"
[ -d "projects/man-pal" ] && echo "   ✓ projects/man-pal exists" || echo "   ✗ projects/man-pal MISSING"

# Check project.pdl files
echo ""
echo "2. Checking project.pdl files..."
[ -f "projects/man-ops/project.pdl" ] && echo "   ✓ man-ops/project.pdl exists" || echo "   ✗ man-ops/project.pdl MISSING"
[ -f "projects/man-pal/project.pdl" ] && echo "   ✓ man-pal/project.pdl exists" || echo "   ✗ man-pal/project.pdl MISSING"

# Check maps
echo ""
echo "3. Checking map files..."
[ -f "projects/man-ops/maps/map_0001_z0.txt" ] && echo "   ✓ man-ops map exists" || echo "   ✗ man-ops map MISSING"
[ -f "projects/man-pal/maps/map_0001_z0.txt" ] && echo "   ✓ man-pal map exists" || echo "   ✗ man-pal map MISSING"

# Check selector state
echo ""
echo "4. Checking selector state files..."
[ -f "projects/man-ops/pieces/selector/state.txt" ] && echo "   ✓ man-ops selector state exists" || echo "   ✗ man-ops selector state MISSING"
[ -f "projects/man-pal/pieces/selector/state.txt" ] && echo "   ✓ man-pal selector state exists" || echo "   ✗ man-pal selector state MISSING"

# Check layouts
echo ""
echo "5. Checking layout files..."
[ -f "pieces/apps/man-ops/layouts/man-ops.chtpm" ] && echo "   ✓ man-ops.chtpm exists" || echo "   ✗ man-ops.chtpm MISSING"
[ -f "pieces/apps/man-pal/layouts/man-pal.chtpm" ] && echo "   ✓ man-pal.chtpm exists" || echo "   ✗ man-pal.chtpm MISSING"

# Check modules
echo ""
echo "6. Checking module files..."
[ -f "pieces/apps/man-ops/plugins/man-ops_module.c" ] && echo "   ✓ man-ops_module.c exists" || echo "   ✗ man-ops_module.c MISSING"
[ -f "pieces/apps/man-pal/plugins/man-pal_module.c" ] && echo "   ✓ man-pal_module.c exists" || echo "   ✗ man-pal_module.c MISSING"
[ -x "pieces/apps/man-ops/plugins/+x/man-ops_module.+x" ] && echo "   ✓ man-ops_module.+x COMPILED" || echo "   ✗ man-ops_module.+x NOT COMPILED"
[ -x "pieces/apps/man-pal/plugins/+x/man-pal_module.+x" ] && echo "   ✓ man-pal_module.+x COMPILED" || echo "   ✗ man-pal_module.+x NOT COMPILED"

# Check PAL script
echo ""
echo "7. Checking PAL script..."
[ -f "projects/man-pal/scripts/move.asm" ] && echo "   ✓ man-pal move.asm exists" || echo "   ✗ man-pal move.asm MISSING"

# Check parser routing
echo ""
echo "8. Checking parser routing..."
grep -q "man-ops" pieces/chtpm/plugins/chtpm_parser.c && echo "   ✓ Parser has man-ops routing" || echo "   ✗ Parser MISSING man-ops routing"
grep -q "man-pal" pieces/chtpm/plugins/chtpm_parser.c && echo "   ✓ Parser has man-pal routing" || echo "   ✗ Parser MISSING man-pal routing"

# Show selector state content
echo ""
echo "9. Selector state content (man-ops):"
cat projects/man-ops/pieces/selector/state.txt 2>/dev/null || echo "   (file not found)"

echo ""
echo "10. Selector state content (man-pal):"
cat projects/man-pal/pieces/selector/state.txt 2>/dev/null || echo "   (file not found)"

echo ""
echo "=== TEST COMPLETE ==="
echo ""
echo "To test manually:"
echo "  1. Run: ./run_chtpm.sh"
echo "  2. Navigate to Project Manager"
echo "  3. Select 'man-ops' or 'man-pal'"
echo "  4. Select 'Control Map' and press ENTER"
echo "  5. Press arrow keys to move selector"
echo ""
