#!/bin/bash
# Regression suite for my_shell. Feeds scripted command sequences into the
# shell's non-interactive (piped-stdin) mode and checks the output against
# what's expected. Exits 0 if every test passes, 1 otherwise — CI-ready.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
SHELL_BIN="$ROOT_DIR/my_shell"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

pass_count=0
fail_count=0

# run_test NAME INPUT EXPECTED_SUBSTRING
run_test() {
    local name="$1" input="$2" expected="$3"
    local actual
    actual="$(printf '%s\n' "$input" | timeout 5 "$SHELL_BIN" 2>&1)"
    if printf '%s' "$actual" | grep -qF -- "$expected"; then
        printf 'PASS: %s\n' "$name"
        pass_count=$((pass_count + 1))
    else
        printf 'FAIL: %s\n' "$name"
        printf '  input:    %q\n' "$input"
        printf '  expected to contain: %q\n' "$expected"
        printf '  actual:   %q\n' "$actual"
        fail_count=$((fail_count + 1))
    fi
}

# run_test_not NAME INPUT FORBIDDEN_SUBSTRING
run_test_not() {
    local name="$1" input="$2" forbidden="$3"
    local actual
    actual="$(printf '%s\n' "$input" | timeout 5 "$SHELL_BIN" 2>&1)"
    if printf '%s' "$actual" | grep -qF -- "$forbidden"; then
        printf 'FAIL: %s\n' "$name"
        printf '  input:    %q\n' "$input"
        printf '  expected NOT to contain: %q\n' "$forbidden"
        printf '  actual:   %q\n' "$actual"
        fail_count=$((fail_count + 1))
    else
        printf 'PASS: %s\n' "$name"
        pass_count=$((pass_count + 1))
    fi
}

if [ ! -x "$SHELL_BIN" ]; then
    echo "Build my_shell first (run 'make' from the project root)." >&2
    exit 1
fi

# --- Built-ins -------------------------------------------------------------
run_test "echo basic"            'echo hello world'                    'hello world'
run_test "echo -n suppresses \\n" 'echo -n no newline'                  'no newline'
run_test "echo double-quoted"     'echo "hello   world"'                'hello   world'
run_test "echo glued quotes"      'echo foo"bar baz"qux'                'foobar bazqux'
run_test "pwd"                    'pwd'                                 "$ROOT_DIR"
run_test "cd then pwd"            $'cd /tmp\npwd'                       '/tmp'
run_test "setenv + expansion"     $'setenv FOO bar\necho $FOO'          'bar'
run_test "unsetenv removes var"   $'setenv FOO bar\nunsetenv FOO\necho [ $FOO ]' '[  ]'
run_test "which finds ls"         'which ls'                            '/ls'
run_test "which missing cmd"      'which not_a_real_cmd_xyz'            'not found'

# --- External commands / PATH ----------------------------------------------
run_test "external command runs"  'echo run-me > /dev/null; echo ok'   'ok'
run_test "unknown command"        'not_a_real_cmd_xyz'                 'command not found'

# --- Pipes -------------------------------------------------------------
run_test "single pipe"            'echo hello | cat'                   'hello'
run_test "three-stage pipe chain" 'echo hello | cat | cat'              'hello'

# --- Redirection -------------------------------------------------------
run_test "> creates file"         "echo hi > $TMP_DIR/out.txt; cat $TMP_DIR/out.txt" 'hi'
run_test ">> appends"             "echo a > $TMP_DIR/o2.txt; echo b >> $TMP_DIR/o2.txt; cat $TMP_DIR/o2.txt" $'a\nb'
run_test "< reads stdin"          "echo fromfile > $TMP_DIR/o3.txt; cat < $TMP_DIR/o3.txt" 'fromfile'

# --- Exit status / && / || --------------------------------------------
run_test "&& runs on success"     'true && echo yes-and'                'yes-and'
run_test_not "&& skips on failure" 'false && echo should-not-print'     'should-not-print'
run_test "|| runs on failure"     'false || echo yes-or'                'yes-or'
run_test '$? reflects last status' $'false\necho $?'                    '1'

# --- Parsing edge cases --------------------------------------------------
run_test "semicolon sequencing"   'echo a; echo b'                      $'a\nb'
run_test "trailing pipe is a syntax error" 'echo hi |'                  'syntax error'
run_test "leading pipe is a syntax error"  '| echo hi'                  'syntax error'

echo
echo "$pass_count passed, $fail_count failed"
[ "$fail_count" -eq 0 ]
