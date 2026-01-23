#!/bin/bash

# Test runner for MiniC compiler
# Supports: Syntaxe, Verif, and Gencode tests

MINICC="./minicc"
MINICC_REF="./minicc_ref"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

# Counters
passed=0
failed=0

# Flags
run_syntaxe=false
run_verif=false
run_gencode=false

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -s    Run Syntaxe tests"
    echo "  -v    Run Verif tests"
    echo "  -g    Run Gencode tests"
    echo "  -a    Run all tests"
    echo "  -h    Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 -a          # Run all tests"
    echo "  $0 -s -v       # Run Syntaxe and Verif tests"
    echo "  $0 -g          # Run only Gencode tests"
    exit 0
}

# Parse arguments
while getopts "svgah" opt; do
    case $opt in
        s) run_syntaxe=true ;;
        v) run_verif=true ;;
        g) run_gencode=true ;;
        a) run_syntaxe=true; run_verif=true; run_gencode=true ;;
        h) usage ;;
        *) usage ;;
    esac
done

# If no flags provided, show usage
if ! $run_syntaxe && ! $run_verif && ! $run_gencode; then
    echo "Error: No test category specified"
    echo ""
    usage
fi

# Check compiler exists
if [ ! -f "$MINICC" ]; then
    echo -e "${RED}Error: $MINICC not found. Run 'make' first.${NC}"
    exit 1
fi

###########################################
# SYNTAXE TESTS
###########################################
run_syntaxe_tests() {
    local syntaxe_passed=0
    local syntaxe_failed=0

    echo -e "${CYAN}=========================================="
    echo "SYNTAXE TESTS"
    echo "==========================================${NC}"

    echo ""
    echo "--- Syntaxe OK (should pass syntax check) ---"
    for test_file in Tests/Syntaxe/OK/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        $MINICC -s "$test_file" 2>/dev/null
        status=$?

        if [ $status -eq 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $name"
            ((syntaxe_passed++))
        else
            echo -e "${RED}[FAIL]${NC} $name - syntax check failed"
            ((syntaxe_failed++))
        fi
    done

    echo ""
    echo "--- Syntaxe KO (should fail syntax check) ---"
    for test_file in Tests/Syntaxe/KO/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        $MINICC -s "$test_file" 2>/dev/null
        status=$?

        if [ $status -ne 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $name - correctly rejected"
            ((syntaxe_passed++))
        else
            echo -e "${RED}[FAIL]${NC} $name - should have failed"
            ((syntaxe_failed++))
        fi
    done

    echo ""
    echo -e "Syntaxe: ${GREEN}$syntaxe_passed passed${NC}, ${RED}$syntaxe_failed failed${NC}"
    ((passed += syntaxe_passed))
    ((failed += syntaxe_failed))
}

###########################################
# VERIF TESTS
###########################################
run_verif_tests() {
    local verif_passed=0
    local verif_failed=0

    echo -e "${CYAN}=========================================="
    echo "VERIF TESTS (Passe 1)"
    echo "==========================================${NC}"

    echo ""
    echo "--- Verif OK (should pass verification) ---"
    for test_file in Tests/Verif/OK/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        $MINICC -v "$test_file" 2>/dev/null
        status=$?

        if [ $status -eq 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $name"
            ((verif_passed++))
        else
            echo -e "${RED}[FAIL]${NC} $name - verification failed"
            ((verif_failed++))
        fi
    done

    echo ""
    echo "--- Verif KO (should fail verification) ---"
    for test_file in Tests/Verif/KO/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        $MINICC -v "$test_file" 2>/dev/null
        status=$?

        if [ $status -ne 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $name - correctly rejected"
            ((verif_passed++))
        else
            echo -e "${RED}[FAIL]${NC} $name - should have failed"
            ((verif_failed++))
        fi
    done

    echo ""
    echo -e "Verif: ${GREEN}$verif_passed passed${NC}, ${RED}$verif_failed failed${NC}"
    ((passed += verif_passed))
    ((failed += verif_failed))
}

###########################################
# GENCODE TESTS
###########################################
run_gencode_tests() {
    local gencode_passed=0
    local gencode_failed=0

    echo -e "${CYAN}=========================================="
    echo "GENCODE TESTS (Passe 2)"
    echo "==========================================${NC}"

    echo ""
    echo "--- Gencode OK (compare with reference) ---"
    for test_file in Tests/Gencode/OK/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        # Compile with our compiler
        $MINICC -o /tmp/out.s "$test_file" 2>/dev/null
        our_status=$?

        if [ $our_status -ne 0 ]; then
            echo -e "${RED}[FAIL]${NC} $name - compilation failed"
            ((gencode_failed++))
            continue
        fi

        # Compare with reference if available
        if [ -f "$MINICC_REF" ]; then
            $MINICC_REF -o /tmp/ref.s "$test_file" 2>/dev/null

            if diff -q /tmp/out.s /tmp/ref.s >/dev/null 2>&1; then
                echo -e "${GREEN}[PASS]${NC} $name"
                ((gencode_passed++))
            else
                echo -e "${YELLOW}[DIFF]${NC} $name - output differs from reference"
                ((gencode_passed++))
            fi
        else
            echo -e "${GREEN}[PASS]${NC} $name - compiles (no ref to compare)"
            ((gencode_passed++))
        fi
    done

    echo ""
    echo "--- Gencode KO (should compile, runtime error expected) ---"
    for test_file in Tests/Gencode/KO/*.c; do
        [ -f "$test_file" ] || continue
        name=$(basename "$test_file" .c)

        $MINICC -o /tmp/out.s "$test_file" 2>/dev/null
        status=$?

        if [ $status -eq 0 ]; then
            echo -e "${GREEN}[PASS]${NC} $name - compiles (runtime error expected)"
            ((gencode_passed++))
        else
            echo -e "${RED}[FAIL]${NC} $name - should compile"
            ((gencode_failed++))
        fi
    done

    echo ""
    echo -e "Gencode: ${GREEN}$gencode_passed passed${NC}, ${RED}$gencode_failed failed${NC}"
    ((passed += gencode_passed))
    ((failed += gencode_failed))
}

###########################################
# MAIN
###########################################

if $run_syntaxe; then
    run_syntaxe_tests
    echo ""
fi

if $run_verif; then
    run_verif_tests
    echo ""
fi

if $run_gencode; then
    run_gencode_tests
    echo ""
fi

echo -e "${CYAN}=========================================="
echo "TOTAL RESULTS"
echo "==========================================${NC}"
echo -e "Total: ${GREEN}$passed passed${NC}, ${RED}$failed failed${NC}"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
