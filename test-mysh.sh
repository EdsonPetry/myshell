#!/bin/bash

# colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0
TOTAL=0

TEST_DIR="/tmp/mysh_test_$$"
MYSH="./mysh"

setup() {
  echo "Setting up test environment..."

  if [ ! -x "$MYSH" ]; then
    echo -e "${RED}Error: mysh executable not found or not executable${NC}"
    echo "Please compile mysh first: make mysh"
    exit 1
  fi

  MYSH="$(pwd)/$MYSH"

  mkdir -p "$TEST_DIR"
  cd "$TEST_DIR" || exit 1
}

cleanup() {
  echo "Cleaning up test environment..."
  cd /
  rm -rf "$TEST_DIR"
}

assert_equal() {
  local name="$1"
  local expected="$2"
  local actual="$3"

  TOTAL=$((TOTAL + 1))
  if [ "$expected" = "$actual" ]; then
    echo -e "${GREEN}✓ PASS${NC}: $name"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: $name"
    echo "  Expected: '$expected'"
    echo "  Got:      '$actual'"
    FAIL=$((FAIL + 1))
  fi
}

assert_file_exists() {
  local name="$1"
  local file="$2"

  TOTAL=$((TOTAL + 1))
  if [ -f "$file" ]; then
    echo -e "${GREEN}✓ PASS${NC}: $name"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: $name (file '$file' not found)"
    FAIL=$((FAIL + 1))
  fi
}

assert_file_contains() {
  local name="$1"
  local file="$2"
  local expected="$3"

  TOTAL=$((TOTAL + 1))
  if [ -f "$file" ] && grep -q "$expected" "$file"; then
    echo -e "${GREEN}✓ PASS${NC}: $name"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: $name"
    if [ -f "$file" ]; then
      echo "  File contents: $(cat "$file")"
    else
      echo "  File not found: $file"
    fi
    FAIL=$((FAIL + 1))
  fi
}

test_basic_commands() {
  echo -e "\n${YELLOW}=== Testing Basic Commands ===${NC}"

  echo "echo hello world" | $MYSH >output.txt 2>&1
  assert_file_contains "echo command" "output.txt" "hello world"

  echo "pwd" | $MYSH >output.txt 2>&1
  assert_file_contains "pwd command" "output.txt" "$TEST_DIR"

  echo "true" | $MYSH >output.txt 2>&1
  local exit_code=$?
  assert_equal "true command exit code" "0" "$exit_code"

  echo "false" | $MYSH >output.txt 2>&1
}

test_builtin_cd() {
  echo -e "\n${YELLOW}=== Testing Built-in: cd ===${NC}"

  mkdir -p subdir
  cat >script.sh <<'EOF'
cd subdir
pwd
EOF

  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "cd to subdir" "output.txt" "subdir"

  echo "cd nonexistent_dir" | $MYSH >output.txt 2>&1
  assert_file_contains "cd to invalid dir" "output.txt" "No such file or directory"
}

test_builtin_which() {
  echo -e "\n${YELLOW}=== Testing Built-in: which ===${NC}"

  echo "which ls" | $MYSH >output.txt 2>&1
  TOTAL=$((TOTAL + 1))
  if grep -q "/ls" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: which ls"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: which ls (expected path containing /ls)"
    FAIL=$((FAIL + 1))
  fi

  echo "which cat" | $MYSH >output.txt 2>&1
  TOTAL=$((TOTAL + 1))
  if grep -q "/cat" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: which cat"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: which cat (expected path containing /cat)"
    FAIL=$((FAIL + 1))
  fi

  echo "which nonexistent_command_xyz" | $MYSH >output.txt 2>&1
  local
  content=$(cat output.txt)

  TOTAL=$((TOTAL + 1))
  if [ -z "$content" ]; then
    echo -e "${GREEN}✓ PASS${NC}: which with non-existent command (empty output)"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: which with non-existent command should produce no output"
    FAIL=$((FAIL + 1))
  fi
}

test_comments() {
  echo -e "\n${YELLOW}=== Testing Comments ===${NC}"

  echo "# this is a comment" | $MYSH >output.txt 2>&1
  local
  size=$(stat -c%s output.txt 2>/dev/null || stat -f%z output.txt 2>/dev/null)
  assert_equal "full line comment produces no output" "0" "$size"

  echo "echo hello # this is a comment" | $MYSH >output.txt 2>&1
  assert_file_contains "inline comment" "output.txt" "hello"
  TOTAL=$((TOTAL + 1))
  if ! grep -q "comment" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: comment text not in output"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: comment text should not appear in output"
    FAIL=$((FAIL + 1))
  fi
}

test_input_redirection() {
  echo -e "\n${YELLOW}=== Testing Input Redirection ===${NC}"

  echo "test content" >input.txt
  echo "cat < input.txt" | $MYSH >output.txt 2>&1
  assert_file_contains "input redirection" "output.txt" "test content"

  echo "cat < nonexistent.txt" | $MYSH >output.txt 2>&1
  assert_file_contains "input redirection with missing file" "output.txt" "No such file"
}

test_output_redirection() {
  echo -e "\n${YELLOW}=== Testing Output Redirection ===${NC}"

  echo "echo hello > output.txt" | $MYSH
  assert_file_exists "output redirection creates file" "output.txt"
  assert_file_contains "output redirection content" "output.txt" "hello"

  echo "existing content" >output.txt
  echo "echo new content > output.txt" | $MYSH
  assert_file_contains "output redirection truncates" "output.txt" "new content"
  TOTAL=$((TOTAL + 1))
  if ! grep -q "existing" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: old content was truncated"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: old content should be truncated"
    FAIL=$((FAIL + 1))
  fi
}

test_pipelines() {
  echo -e "\n${YELLOW}=== Testing Pipelines ===${NC}"

  echo "echo hello | cat" | $MYSH >output.txt 2>&1
  assert_file_contains "simple pipeline" "output.txt" "hello"

  echo "echo hello world | cat | cat" | $MYSH >output.txt 2>&1
  assert_file_contains "three-command pipeline" "output.txt" "hello world"

  cat >input.txt <<'EOF'
line1
line2
target
line4
EOF
  echo "cat input.txt | grep target" | $MYSH >output.txt 2>&1
  assert_file_contains "pipeline with grep" "output.txt" "target"

  echo "echo test | cat > output.txt" | $MYSH
  assert_file_contains "pipeline with output redirect" "output.txt" "test"
}

test_conditionals_and() {
  echo -e "\n${YELLOW}=== Testing Conditionals: and ===${NC}"

  cat >script.sh <<'EOF'
true
and echo success
EOF
  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "and after true" "output.txt" "success"

  cat >script.sh <<'EOF'
false
and echo should_not_appear
EOF
  $MYSH script.sh >output.txt 2>&1
  TOTAL=$((TOTAL + 1))
  if ! grep -q "should_not_appear" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: and after false (command skipped)"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: and should skip after false"
    FAIL=$((FAIL + 1))
  fi
}

test_conditionals_or() {
  echo -e "\n${YELLOW}=== Testing Conditionals: or ===${NC}"

  cat >script.sh <<'EOF'
false
or echo executed
EOF
  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "or after false" "output.txt" "executed"

  cat >script.sh <<'EOF'
true
or echo should_not_appear
EOF
  $MYSH script.sh >output.txt 2>&1
  TOTAL=$((TOTAL + 1))
  if ! grep -q "should_not_appear" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: or after true (command skipped)"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: or should skip after true"
    FAIL=$((FAIL + 1))
  fi
}

test_batch_mode() {
  echo -e "\n${YELLOW}=== Testing Batch Mode ===${NC}"

  cat >script.sh <<'EOF'
echo line1
echo line2
echo line3
EOF

  $MYSH script.sh >output.txt 2>&1

  TOTAL=$((TOTAL + 1))
  if ! grep -q "mysh>" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: batch mode has no prompts"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: batch mode should not show prompts"
    FAIL=$((FAIL + 1))
  fi

  assert_file_contains "batch mode output line 1" "output.txt" "line1"
  assert_file_contains "batch mode output line 2" "output.txt" "line2"
  assert_file_contains "batch mode output line 3" "output.txt" "line3"
}

test_exit_command() {
  echo -e "\n${YELLOW}=== Testing Exit Command ===${NC}"

  cat >script.sh <<'EOF'
echo before exit
exit
echo after exit
EOF

  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "exit command - before" "output.txt" "before exit"

  TOTAL=$((TOTAL + 1))
  if ! grep -q "after exit" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: exit stops execution"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: commands after exit should not execute"
    FAIL=$((FAIL + 1))
  fi
}

test_die_command() {
  echo -e "\n${YELLOW}=== Testing Die Command ===${NC}"

  cat >script.sh <<'EOF'
echo before die
die error message here
echo after die
EOF

  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "die command - before" "output.txt" "before die"
  assert_file_contains "die command - message" "output.txt" "error message here"

  TOTAL=$((TOTAL + 1))
  if ! grep -q "after die" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: die stops execution"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: commands after die should not execute"
    FAIL=$((FAIL + 1))
  fi
}

test_path_resolution() {
  echo -e "\n${YELLOW}=== Testing Path Resolution ===${NC}"

  echo "/bin/echo hello" | $MYSH >output.txt 2>&1
  assert_file_contains "absolute path" "output.txt" "hello"

  cat >./test_script.sh <<'EOF'
#!/bin/bash
echo "script executed"
EOF
  chmod +x ./test_script.sh

  echo "./test_script.sh" | $MYSH >output.txt 2>&1
  assert_file_contains "relative path" "output.txt" "script executed"

  echo "nonexistent_command_xyz" | $MYSH >output.txt 2>&1
}

test_complex_scenarios() {
  echo -e "\n${YELLOW}=== Testing Complex Scenarios ===${NC}"

  echo "test input" >input.txt
  echo "cat < input.txt > output.txt" | $MYSH
  assert_file_contains "both redirections" "output.txt" "test input"

  cat >script.sh <<'EOF'
true
and echo hello | cat
EOF
  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "conditional with pipeline" "output.txt" "hello"

  cat >script.sh <<'EOF'
true
and echo first
or echo second
and echo third
EOF
  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "conditional chain - first" "output.txt" "first"
  assert_file_contains "conditional chain - third" "output.txt" "third"
  TOTAL=$((TOTAL + 1))
  if ! grep -q "second" output.txt; then
    echo -e "${GREEN}✓ PASS${NC}: conditional chain - second skipped"
    PASS=$((PASS + 1))
  else
    echo -e "${RED}✗ FAIL${NC}: conditional chain - second should be skipped"
    FAIL=$((FAIL + 1))
  fi
}

test_edge_cases() {
  echo -e "\n${YELLOW}=== Testing Edge Cases ===${NC}"

  cat >script.sh <<'EOF'

echo test

EOF
  $MYSH script.sh >output.txt 2>&1
  assert_file_contains "empty lines" "output.txt" "test"

  echo "   " | $MYSH >output.txt 2>&1
  local
  size=$(stat -c%s output.txt 2>/dev/null || stat -f%z output.txt 2>/dev/null)
  assert_equal "whitespace only line" "0" "$size"

  echo "echo    hello    world" | $MYSH >output.txt 2>&1
  assert_file_contains "multiple spaces" "output.txt" "hello"
  assert_file_contains "multiple spaces" "output.txt" "world"
}

main() {
  echo "  MyShell Integration Test Suite"

  setup

  test_basic_commands
  test_builtin_cd
  test_builtin_which
  test_comments
  test_input_redirection
  test_output_redirection
  test_pipelines
  test_conditionals_and
  test_conditionals_or
  test_batch_mode
  test_exit_command
  test_die_command
  test_path_resolution
  test_complex_scenarios
  test_edge_cases

  cleanup

  # Print summary
  echo ""
  echo "  Test Summary"
  echo "=========================================="
  echo -e "Total:  $TOTAL"
  echo -e "${GREEN}Passed: $PASS${NC}"
  echo -e "${RED}Failed: $FAIL${NC}"
  echo "=========================================="

  if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
  else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
  fi
}

# Run tests
main "$@"
