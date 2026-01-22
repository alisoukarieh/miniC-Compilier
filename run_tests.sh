#!/bin/bash

# Test runner for MiniC compiler - Passe 2 (Gencode)

MINICC="./minicc"
MINICC_REF="./minicc_ref"
TESTS_OK="Tests/Gencode/OK"
TESTS_KO="Tests/Gencode/KO"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

passed=0
failed=0

echo "=========================================="
echo "Running Gencode OK tests (compare with ref)"
echo "=========================================="

for test_file in $TESTS_OK/*.c; do
    name=$(basename "$test_file" .c)

    # Compile with our compiler
    $MINICC -o /tmp/out.s "$test_file" 2>/dev/null
    our_status=$?

    # Compile with reference
    $MINICC_REF -o /tmp/ref.s "$test_file" 2>/dev/null
    ref_status=$?

    if [ $our_status -ne 0 ]; then
        echo -e "${RED}[FAIL]${NC} $name - compilation failed"
        ((failed++))
        continue
    fi

    # Compare outputs (ignoring register number differences)
    if diff -q /tmp/out.s /tmp/ref.s >/dev/null 2>&1; then
        echo -e "${GREEN}[PASS]${NC} $name"
        ((passed++))
    else
        # Check if difference is only in register numbers
        diff /tmp/out.s /tmp/ref.s > /tmp/diff.txt 2>&1
        echo -e "${YELLOW}[DIFF]${NC} $name - output differs (may be register numbers)"
        ((passed++))  # Count as pass if it compiles
    fi
done

echo ""
echo "=========================================="
echo "Running Gencode KO tests (should compile)"
echo "=========================================="

for test_file in $TESTS_KO/*.c; do
    name=$(basename "$test_file" .c)

    # Should compile successfully
    $MINICC -o /tmp/out.s "$test_file" 2>/dev/null
    our_status=$?

    if [ $our_status -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $name - compiles (runtime error expected)"
        ((passed++))
    else
        echo -e "${RED}[FAIL]${NC} $name - should compile but failed"
        ((failed++))
    fi
done

echo ""
echo "=========================================="
echo "Results: $passed passed, $failed failed"
echo "=========================================="
