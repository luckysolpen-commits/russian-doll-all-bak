#!/bin/bash
# test_trunk_delta.sh - Test trunk+delta storage and checkout reconstruction

cd "/home/no/Desktop/Piecemark-IT/中ϕ_00.00/0.sp-inniϕ©+16/TPMOS-TRAINING/#.reference/TPMOS_c_+rmmp_66.02/#.test"

GITLET="../projects/gitlet/ops/+x"
HASH="test-td4"

rm -rf ~/gitlet/$HASH

echo "=== INIT ==="
$GITLET/gitlet_init.+x $HASH

echo ""
echo "=== COMMIT 1: TRUNK ==="
echo "hello world" > testfile.txt
$GITLET/gitlet_add.+x $HASH testfile.txt
$GITLET/gitlet_commit.+x $HASH "Create testfile"
C1=$(cat ~/gitlet/$HASH/.gitlet/refs/heads/master)
echo "Commit 1: $C1"

echo ""
echo "=== COMMIT 2: DELTA ==="
echo "hello world modified" > testfile.txt
$GITLET/gitlet_add.+x $HASH testfile.txt
$GITLET/gitlet_commit.+x $HASH "Modify testfile"
C2=$(cat ~/gitlet/$HASH/.gitlet/refs/heads/master)
echo "Commit 2: $C2"

echo ""
echo "=== TREE (C2) ==="
cat ~/gitlet/$HASH/.gitlet/objects/$C2 | grep "^tree "

echo ""
echo "=== INDEX ==="
cat ~/gitlet/$HASH/.gitlet/index

echo ""
echo "=== STORAGE ==="
echo "Objects:"
ls ~/gitlet/$HASH/.gitlet/objects/
echo "Deltas:"
ls ~/gitlet/$HASH/.gitlet/deltas/ 2>/dev/null || echo "(none)"

echo ""
echo "=== CHECKOUT C1 (trunk) ==="
$GITLET/gitlet_checkout.+x $HASH $C1
echo "Content: $(cat testfile.txt)"

echo ""
echo "=== CHECKOUT C2 (delta) ==="
$GITLET/gitlet_checkout.+x $HASH $C2
echo "Content: $(cat testfile.txt)"

echo ""
echo "=== TEST COMPLETE ==="
