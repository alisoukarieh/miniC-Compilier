# Implementation Plan: minicc Compiler

## Overview
Implementation of command-line argument parsing and AST memory management for the minicc compiler, according to Section 8.1 of the project specification.

---

## Status Summary

| Task | Status |
|------|--------|
| `parse_args()` implementation | Done |
| `free_nodes()` implementation | Done |
| Build verification | Pending |
| Testing | Pending |

---

## What Has Been Done

### 1. `parse_args()` - Command-line Argument Parsing

**File:** `common.c` (lines 24-122)

**Implemented features:**

| Flag | Argument | Description | Default |
|------|----------|-------------|---------|
| `-b` | none | Display banner (compiler name + team members) | - |
| `-o` | `<filename>` | Output assembly file | `out.s` |
| `-t` | `<int>` | Trace level (0-5) | 0 |
| `-r` | `<int>` | Max registers (4-8) | 8 |
| `-s` | none | Stop after syntax analysis | false |
| `-v` | none | Stop after verification (passe_1) | false |
| `-h` | none | Display help message | - |

**Validation rules implemented:**
- `-s` and `-v` are mutually exclusive (error if both used)
- `-b` must be used alone (no other options or input file)
- `-t` value must be in range [0, 5]
- `-r` value must be in range [4, 8]
- Input file is required unless `-b` or `-h` is used

**Helper functions added:**
- `print_banner()` - Displays compiler name and team members
- `print_help()` - Displays usage information

### 2. `free_nodes()` - AST Memory Deallocation

**File:** `common.c` (lines 126-151)

**Implementation:**
- Recursively frees all child nodes via `opr[]` array
- Frees the `opr` array itself
- Frees dynamically allocated strings (`ident`, `str`)
- Frees the node structure

---

## What Should Be Done Next

### 1. Build Verification
```bash
make clean
make
```

### 2. Test Command-line Options
```bash
# Test help
./minicc -h

# Test banner
./minicc -b

# Test invalid combinations
./minicc -s -v test.c    # Should error: mutually exclusive

# Test invalid values
./minicc -t 10 test.c    # Should error: out of range
./minicc -r 2 test.c     # Should error: out of range

# Test valid compilation
./minicc -t 3 -o output.s test.c
./minicc -r 5 test.c
./minicc -s test.c       # Stop after syntax
./minicc -v test.c       # Stop after verification
```

### 3. Memory Leak Testing (Optional)
```bash
valgrind --leak-check=full ./minicc test.c
```

---

## Expected Final Result

### Successful Build
- No compilation errors or warnings
- `minicc` executable created

### Expected Behavior

| Command | Expected Output |
|---------|-----------------|
| `./minicc -h` | Usage message, exit 0 |
| `./minicc -b` | Banner with team names, exit 0 |
| `./minicc -s -v file.c` | Error message, exit 1 |
| `./minicc -t 6 file.c` | Error: trace level out of range, exit 1 |
| `./minicc -r 3 file.c` | Error: registers out of range, exit 1 |
| `./minicc` | Error: input file required, exit 1 |
| `./minicc file.c` | Compile file.c to out.s |
| `./minicc -o test.s file.c` | Compile file.c to test.s |

### Code Quality
- No memory leaks when `free_nodes()` is called on the AST
- Clean exit codes (0 for success, 1 for errors)
- Proper error messages to stderr

---

## File Changes Summary

### Modified Files
- `common.c` - Added implementations for `parse_args()` and `free_nodes()`

### Functions Added
| Function | Location | Purpose |
|----------|----------|---------|
| `print_banner()` | common.c:24 | Display compiler banner |
| `print_help()` | common.c:30 | Display usage help |

### Functions Modified
| Function | Location | Change |
|----------|----------|--------|
| `parse_args()` | common.c:42 | Full implementation with getopt |
| `free_nodes()` | common.c:126 | Full recursive implementation |

---

## Architecture Reference

### Node Structure (from `defs.h`)
```c
typedef struct _node_s {
    node_nature nature;
    node_type type;
    int64_t value;
    int32_t offset;
    bool global_decl;
    int32_t lineno;
    int32_t nops;
    struct _node_s ** opr;    // Child nodes array
    struct _node_s * decl_node;
    char * ident;             // Dynamically allocated
    char * str;               // Dynamically allocated
    int32_t node_num;
} node_s;
```

### Global Variables (defined in `lexico.l`)
```c
char * infile = NULL;
char * outfile = DEFAULT_OUTFILE;  // "out.s"
bool stop_after_syntax = false;
bool stop_after_verif = false;
```

### External Functions Used
- `set_max_registers(int32_t n)` from `arch.h`
