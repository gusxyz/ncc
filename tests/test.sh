#!/bin/bash

# Run from the project root. Requires Bash 5.1+ for wait -n -p.
# Nonzero program exits are test results, so do not enable set -e.
set -uo pipefail

if (( BASH_VERSINFO[0] < 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] < 1) )); then
  printf 'This harness requires Bash 5.1 or newer.\n' >&2
  exit 1
fi

JOBS=${JOBS:-$(nproc)} # fast as lightning baby
NCC=${NCC:-./build/ncc}
CC=${CC:-cc}
if [[ ! "$JOBS" =~ ^[1-9][0-9]{0,3}$ ]]; then
  printf 'JOBS must be an integer from 1 to 9999.\n' >&2
  exit 1
fi
JOBS=$((10#$JOBS))
mkdir -p build || exit 1
work_root=$(mktemp -d build/test.XXXXXX) || exit 1
passed=0
failed=0
submitted=0
cleanup_success=0
declare -A running=()
declare -A durations=() labels=()
total_us=0
suite_start=${EPOCHREALTIME/[.,]/}

# fmt_ms VAR MICROSECONDS -> sets VAR to e.g. "12.3"
fmt_ms() { printf -v "$1" '%d.%d' $(( $2 / 1000 )) $(( ($2 % 1000) / 100 )); }

cleanup() {
  if (( cleanup_success )); then
    rm -rf -- "$work_root"
  else
    printf 'Test files retained in %s\n' "$work_root" >&2
  fi
}
trap cleanup EXIT
trap 'trap "" INT TERM; for pid in "${!running[@]}"; do kill "$pid" 2>/dev/null || :; done; wait; exit 130' INT TERM

run_program() {
  local dir="$1" expected="$2" input="$3" actual
  printf '%s\n' "$input" > "$dir/input.ncc"

  if ! "$NCC" -e "$input" > "$dir/program.s"; then
    printf 'FAIL: compilation failed\nSource: %s\n' "$input" >&2
    return 1
  fi
  if ! "$CC" -nostdlib -no-pie -o "$dir/program" "$dir/program.s"; then
    printf 'FAIL: assembly/linking failed\nSource: %s\n' "$input" >&2
    return 1
  fi

  "$dir/program"
  actual=$?
  if (( actual != expected )); then
    printf 'FAIL: expected %s, got %s\nSource: %s\n' "$expected" "$actual" "$input" >&2
    return 1
  fi
  printf '%s => %s\n' "$input" "$actual"
}

run_import_program() {
  local dir="$1" expected="$2" actual

  if ! "$NCC" "lib/main.nl" > "$dir/program.s"; then
    printf 'FAIL: import compilation failed\n' >&2
    return 1
  fi

  if ! "$CC" -nostdlib -no-pie -o "$dir/program" "$dir/program.s"; then
    printf 'FAIL: assembly/linking failed\n' >&2
    return 1
  fi

  "$dir/program"
  actual=$?

  if (( actual != expected )); then
    printf 'FAIL: expected %s, got %s\n' "$expected" "$actual" >&2
    return 1
  fi

  printf 'import => %s\n' "$actual"
}

run_error() {
  local dir="$1" expected="$2" input="$3" status
  printf '%s\n' "$input" > "$dir/input.ncc"
  "$NCC" -e "$input" > "$dir/program.s" 2> "$dir/compiler.err"
  status=$?

  if (( status == 0 || status >= 128 )); then
    printf 'FAIL: expected compiler rejection, got exit status %s\nSource: %s\n' "$status" "$input" >&2
    cat "$dir/compiler.err" >&2
    return 1
  fi
  if ! grep -Fq -- "$expected" "$dir/compiler.err"; then
    printf 'FAIL: expected diagnostic containing: %s\nSource: %s\n' "$expected" "$input" >&2
    cat "$dir/compiler.err" >&2
    return 1
  fi
  printf 'Rejected as expected: %s\n' "$input"
}

# Only the parent updates counters and prints complete worker logs.
reap_one() {
  local completed status id us ms
  wait -n -p completed "${!running[@]}"
  status=$?
  if [[ -z "${completed:-}" ]]; then
    printf 'FAIL: could not collect a worker result\n' >&2
    exit 1
  fi
  id=${running[$completed]}
  unset 'running[$completed]'
  cat "$work_root/$id/output.log"

  us=0
  read -r us < "$work_root/$id/time_us" 2>/dev/null || us=0
  durations[$id]=$us
  total_us=$((total_us + us))
  fmt_ms ms "$us"
  printf '  [test #%s: %s ms]\n' "$id" "$ms"

  if (( status == 0 )); then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    printf 'Failed test artifacts: %s/%s\n' "$work_root" "$id" >&2
  fi
}

submit() {
  local runner="$1" id pid
  shift
  while (( ${#running[@]} >= JOBS )); do
    reap_one
  done
  submitted=$((submitted + 1))
  id=$submitted
  mkdir "$work_root/$id" || exit 1
  labels[$id]="${*: -1}"   # last arg is the source/program text
  (
    start=${EPOCHREALTIME/[.,]/}
    "$runner" "$work_root/$id" "$@"
    rc=$?
    printf '%s\n' $(( ${EPOCHREALTIME/[.,]/} - start )) > "$work_root/$id/time_us"
    exit "$rc"
  ) > "$work_root/$id/output.log" 2>&1 &
  pid=$!
  running[$pid]=$id
}

assert_program() { submit run_program "$1" "$2"; }
assert_import_program() { submit run_import_program "$1"; }
assert() { assert_program "$1" "int main() { $2 }"; }
assert_error() { submit run_error "$1" "$2"; }

print_timing() {
  local wall_us=$(( ${EPOCHREALTIME/[.,]/} - suite_start ))
  local wall sum avg tps speedup id ms
  (( wall_us > 0 )) || wall_us=1
  fmt_ms wall "$wall_us"
  fmt_ms sum "$total_us"
  fmt_ms avg $(( submitted ? total_us / submitted : 0 ))
  tps=$(( submitted * 10000000 / wall_us ))   # tests/sec x10
  speedup=$(( total_us * 10 / wall_us ))      # x10

  printf '\n--- Timing ---\n'
  printf 'Wall time:      %s ms\n' "$wall"
  printf 'Sum of tests:   %s ms (avg %s ms/test)\n' "$sum" "$avg"
  printf 'Throughput:     %d.%d tests/sec\n' $((tps / 10)) $((tps % 10))
  printf 'Parallelism:    %d.%dx effective (%s jobs)\n' $((speedup / 10)) $((speedup % 10)) "$JOBS"
  printf 'Slowest 5:\n'
  for id in "${!durations[@]}"; do
    printf '%s\t%s\n' "${durations[$id]}" "${labels[$id]}"
  done | sort -t$'\t' -k1,1nr | head -5 | while IFS=$'\t' read -r us src; do
    fmt_ms ms "$us"
    printf '  %8s ms  %s\n' "$ms" "$src"
  done
}

finish() {
  while (( ${#running[@]} )); do
    reap_one
  done
  print_timing
  if (( failed )); then
    printf '\n%s passed, %s failed (%s total).\n' "$passed" "$failed" "$submitted" >&2
    return 1
  fi
  cleanup_success=1
  printf '\nAll %s tests passed (%s workers).\n' "$passed" "$JOBS"
}

# Addition
assert 0 'return 0;'
assert 42 'return 42;'
assert 10 'return 5+5;'
assert 100 'return 50+50;'
assert 21 'return 1+2+3+4+5+6;'

# Subtraction
assert 5 'return 10-5;'
assert 0 'return 5-5;'
assert 10 'return 20-5-5;'
assert 3 'return 10-4-3;'

# Multiplication
assert 42 'return 6*7;'
assert 100 'return 10*10;'
assert 24 'return 2*3*4;'
assert 0 'return 100*0;'

# Division
assert 5 'return 10/2;'
assert 3 'return 10/3;'
assert 2 'return 8/4;'
assert 2 'return 20/5/2;'

# Operator precedence
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 14 'return 2+3*4;'
assert 20 'return (2+3)*4;'
assert 7 'return 1+2*3;'
assert 9 'return 1+2*3+2;'
assert 26 'return 2*3+4*5;'
assert 14 'return 20-3*2;'
assert 10 'return 100/5/2;'
assert 17 'return 2+3*5;'

# Nested parentheses
assert 21 'return (1+2)*(3+4);'
assert 20 'return ((2+3)*4);'
assert 10 'return (((10)));'
assert 14 'return 2*(3+(4));'
assert 45 'return (2+3)*(4+5);'
assert 3 'return (10-(4+3));'

# Unary operators; process exit statuses wrap to 0..255.
assert 5 'return +5;'
assert 5 'return -(-5);'
assert 5 'return - -5;'
assert 251 'return +-(+(-(-5)));'

# Equality
assert 1 'return 5==5;'
assert 0 'return 5==6;'
assert 1 'return 5!=6;'
assert 0 'return 5!=5;'
assert 1 'return 2+3==5;'
assert 1 'return 2*3==3*2;'
assert 0 'return 2+3!=5;'

# Less than
assert 1 'return 3<5;'
assert 0 'return 5<3;'
assert 0 'return 5<5;'
assert 1 'return 3<=5;'
assert 1 'return 5<=5;'
assert 0 'return 6<=5;'

# Greater than
assert 1 'return 5>3;'
assert 0 'return 3>5;'
assert 0 'return 5>5;'
assert 1 'return 5>=3;'
assert 1 'return 5>=5;'
assert 0 'return 3>=5;'

# Comparison precedence
assert 1 'return 2+3==5;'
assert 1 'return 2*3>5;'
assert 1 'return 10/2<=5;'
assert 1 'return 2+3*4==14;'
assert 1 'return (2+3)*4>15;'
assert 0 'return 2+3*4<10;'
assert 1 'return 5+5>=2*5;'

# Chained comparisons (left-associative)
assert 1 'return 1<2<3;'
assert 1 'return 3<2<1;'
assert 1 'return 3>2>0;'
assert 1 'return 1==1==1;'
assert 0 'return 1==1==0;'
assert 1 'return 1<2==1;'
assert 1 'return 5>3!=0;'

# Negative values in comparisons
assert 1 'return -5<3;'
assert 1 'return -5<-3;'
assert 1 'return -3>-5;'
assert 1 'return -5<=-5;'
assert 1 'return -3>=-5;'
assert 0 'return -5>3;'

# Integer division truncation
assert 3 'return 10/3;'
assert 2 'return 8/3;'
assert 0 'return 1/2;'
assert 0 'return 0/5;'
assert 5 'return 11/2;'
assert 1 'return 5/3;'

# Multiple statements
assert 3 '1; 2; return 3;'
assert 15 '5; 10; return 15;'
assert 42 '1+2; return 6*7;'
assert 9 '2+3; return 4+5;'
assert 20 '10; return 5*4;'

# Local variables (explicit declarations required)
assert 5 'a: int; a=5; return a;'
assert 10 'a: int; a=10; return a;'
assert 15 'a: int; b: int; a=5; b=10; return a+b;'
assert 20 'a: int; b: int; a=10; b=2; return a*b;'
assert 5 'a: int; b: int; a=20; b=4; return a/b;'
assert 1 'a: int; b: int; a=5; b=5; return a==b;'
assert 1 'a: int; b: int; a=3; b=7; return a<b;'
assert 1 'a: int; b: int; a=7; b=3; return a>b;'

# Reassignment
assert 10 'a: int; a=5; a=10; return a;'
assert 15 'a: int; a=5; a=a+10; return a;'
assert 25 'a: int; a=5; a=a*a; return a;'
assert 3 'a: int; a=10; a=a-7; return a;'

# Chained assignments
assert 14 'a: int; b: int; a=b=7; return a+b;'
assert 30 'a: int; b: int; c: int; a=b=c=10; return a+b+c;'
assert 42 'a: int; b: int; a=b=21; return a+b;'
assert 5 'a: int; b: int; a=b=5; return a;'

# Assignment expressions
assert 10 'a: int; return (a=5)+a;'
assert 15 'a: int; b: int; a=5; b=(a=10); return a+b-5;'
assert 1 'a: int; return (a=5)==5;'

# Variable precedence
assert 14 'a: int; b: int; a=2; b=3; return a+b*4;'
assert 20 'a: int; b: int; a=2; b=3; return (a+b)*4;'
assert 1 'a: int; b: int; a=5; b=10; return a*2==b;'
assert 1 'a: int; b: int; a=5; b=10; return a<b;'

# Multi-character variable names
assert 10 'foo: int; bar: int; foo=5; bar=5; return foo+bar;'

# If statements
assert 10 'if (1) return 10; return 20;'
assert 20 'foo: int; bar: int; foo=5; bar=2; if (foo<bar) return 10; return 20;'
assert 10 'if (1) return 10; else return 20;'
assert 20 'if (0) return 10; else return 20;'
assert 30 'if (1) if (0) return 10; else return 30; return 20;'
assert 40 'if (0) return 10; else if (1) return 40; else return 20;'

# While loops
assert 5 'a: int; a=0; while(a<5) a=a+1; return a;'
assert 10 'a: int; a=0; while(a<10) a=a+1; return a;'
assert 0 'a: int; a=0; while(0) a=a+1; return a;'
assert 10 'a: int; a=10; while(a<5) a=a+1; return a;'
assert 0 'a: int; a=10; while(a>0) a=a-1; return a;'
assert 6 'a: int; b: int; a=0; while(a<3) a=a+1; b=0; while(b<3) b=b+1; return a+b;'
assert 10 'a: int; a=0; while(a<10) if(a<5) a=a+2; else a=a+1; return a;'

# Blocks
assert 10 '{ return 10; }'
assert 20 'a: int; b: int; { a=10; b=10; return a+b; }'
assert 42 'a: int; a=1; { a=42; } return a;'
assert 55 'a: int; sum: int; a=1; sum=0; while(a<=10) { sum=sum+a; a=a+1; } return sum;'
assert 30 'a: int; a=0; if(1) { a=10; a=a+20; } else { a=100; } return a;'
assert 100 'a: int; a=0; if(0) { a=10; } else { a=100; } return a;'
assert 15 'a: int; b: int; { a=5; { b=10; } return a+b; }'

# Prefix increment
assert 6 'i: int; i=5; return ++i;'
assert 6 'i: int; i=5; ++i; return i;'
assert 12 'i: int; i=5; return ++i*2;'
assert 7 'i: int; a: int; i=5; a=++i; return a+1;'
assert 3 'i: int; i=0; ++i; ++i; ++i; return i;'

# Postfix increment
assert 5 'i: int; i=5; return i++;'
assert 6 'i: int; i=5; i++; return i;'
assert 10 'i: int; i=5; return i++*2;'
assert 11 'i: int; a: int; i=5; a=i++; return a+i;'
assert 16 'i: int; a: int; i=5; a=i++*2; return a+i;'
assert 3 'i: int; i=0; i++; i++; i++; return i;'

# Prefix decrement
assert 4 'i: int; i=5; return --i;'
assert 4 'i: int; i=5; --i; return i;'
assert 8 'i: int; i=5; return --i*2;'
assert 8 'i: int; a: int; i=5; a=--i; return a+i;'
assert 2 'i: int; i=5; --i; --i; --i; return i;'

# Postfix decrement
assert 5 'i: int; i=5; return i--;'
assert 4 'i: int; i=5; i--; return i;'
assert 10 'i: int; i=5; return i--*2;'
assert 9 'i: int; a: int; i=5; a=i--; return a+i;'
assert 14 'i: int; a: int; i=5; a=i--*2; return a+i;'
assert 2 'i: int; i=5; i--; i--; i--; return i;'

# Mixed increment/decrement
assert 5 'i: int; i=5; i++; i--; return i;'
assert 5 'i: int; i=5; ++i; --i; return i;'
assert 6 'i: int; i=5; ++i; i--; i++; return i;'
assert 10 'i: int; a: int; b: int; i=5; a=i++; b=++i; return a+b-2;'
assert 0 'i: int; i=0; i++; i--; return i;'

# Basic for loops
assert 5 'i: int; for(i=0;i<5;i++) {} return i;'
assert 10 'i: int; for(i=0;i<10;++i) {} return i;'
assert 10 'a: int; i: int; a=0; for(i=0;i<10;i++) a++; return a;'
assert 45 'sum: int; i: int; sum=0; for(i=0;i<10;i++) {sum=sum+i;} return sum;'
assert 55 'sum: int; i: int; sum=0; for(i=1;i<=10;i++) {sum=sum+i;} return sum;'

# For loops with decrement
assert 0 'i: int; for(i=10;i>0;i--) {} return i;'
assert 0 'i: int; for(i=10;i>0;--i) {} return i;'
assert 55 'sum: int; i: int; sum=0; for(i=10;i>0;i--) {sum=sum+i;} return sum;'
assert 5 'i: int; for(i=10;i>5;i--) {} return i;'

# Optional for expressions
assert 5 'i: int; i=0; for(;i<5;i++) {} return i;'
assert 5 'i: int; i=0; for(;i<5;) i++; return i;'
assert 5 'i: int; i=0; for(;;) {if(i==5) return i; i++;}'
assert 42 'for(;;) return 42;'
assert 0 'i: int; for(i=0;i<0;i++) {} return i;'

# Nested for loops
assert 9 'sum: int; i: int; j: int; sum=0; for(i=0;i<3;i++) {for(j=0;j<3;j++) sum++;} return sum;'
assert 25 'sum: int; i: int; j: int; sum=0; for(i=0;i<5;i++) {for(j=0;j<5;j++) sum++;} return sum;'
assert 100 'sum: int; i: int; j: int; sum=0; for(i=0;i<10;i++) {for(j=0;j<10;j++) sum++;} return sum;'

# For loops with conditionals
assert 5 'i: int; for(i=0;i<10;i++) {if(i==5) return i;} return 0;'
assert 7 'i: int; for(i=0;i<10;i++) {if(i==7) return i;} return 0;'
assert 10 'a: int; i: int; a=0; for(i=0;i<10;i++) {if(i<5) a++; else a++;} return a;'

# Loop/stack balance
assert 100 'i: int; j: int; i=0; for(j=0;j<100;j++) {i++;} return i;'
assert 200 'i: int; j: int; i=0; for(j=0;j<200;j++) {++i;} return i;'
assert 50 'i: int; j: int; i=100; for(j=0;j<50;j++) {i--;} return i;'
assert 42 'i: int; j: int; i=0; for(j=0;j<42;j++) {i=i+1;} return i;'

# Break and continue
assert 5 'i: int; for(i=0;i<10;i++) {if(i==5) break;} return i;'
assert 3 'i: int; i=0; while(i<10) {if(i==3) break; i++;} return i;'
assert 5 'i: int; sum: int; i=0; sum=0; while(i<5) {i++; if(i<5) continue; sum=sum+i;} return sum;'
assert 5 'sum: int; i: int; sum=0; for(i=0;i<10;i++) {if(i<5) continue; sum++;} return sum;'

# Function definitions and calls (no parameters yet)
assert_program 42 'int answer() { return 42; } int main() { return answer(); }'
assert_program 42 'int main() { return answer(); } int answer() { return 42; }'
assert_program 42 'int six() { return 6; } int seven() { return 7; } int main() { return six()*seven(); }'
assert_program 47 'int answer() { return 42; } int main() { return 5+answer(); }'
assert_program 47 'int answer() { return 42; } int main() { return answer()+5; }'
assert_program 84 'int answer() { return 42; } int main() { return answer()+answer(); }'
assert_program 42 'int leaf() { return 42; } int middle() { return leaf(); } int main() { return middle(); }'

# Caller variables survive calls; each function has its own frame.
assert_program 12 'int helper() { a: int; a=7; return a; } int main() { a: int; b: int; a=5; b=helper(); return a+b; }'
assert_program 5 'int helper() { a: int; a=99; return a; } int main() { a: int; a=5; helper(); return a; }'
assert_program 10 'int one() { return 1; } int main() { sum: int; i: int; sum=0; for(i=0;i<10;i++) sum=sum+one(); return sum; }'

# Control-flow labels must remain unique across functions.
assert_program 3 'int first() { if(1) return 1; return 0; } int second() { if(1) return 2; return 0; } int main() { return first()+second(); }'

# Falling off the end returns zero in the current NCC implementation.
assert_program 0 'int main() {}'
assert_program 0 'int helper() {} int main() { return helper(); }'

# A program must explicitly define main.
assert_error 'No defined main function' 'int helper() { return 42; }'
assert_error 'No defined main function' ''

# One argument
assert_program 42 'int identity(int a) { return a; } int main() { return identity(42); }'

# Argument order
assert_program 7 'int subtract(int a, int b) { return a-b; } int main() { return subtract(10,3); }'

# Expressions as arguments
assert_program 21 'int multiply(int a, int b) { return a*b; } int main() { return multiply(1+2,10-3); }'

# Local variables as arguments
assert_program 15 'int add(int a, int b) { return a+b; } int main() { x: int; y: int; x=5; y=10; return add(x,y); }'

# All six registers; subtraction makes argument order matter
assert_program 43 'int six(int a, int b, int c, int d, int e, int f) { return a-b-c-d-e-f; } int main() { return six(63,1,2,4,5,8); }'

# Nested calls must preserve already evaluated arguments
assert_program 42 'int identity(int a) { return a; } int add(int a, int b) { return a+b; } int main() { return add(identity(20),identity(22)); }'

# Preserve a pending expression value across a call
assert_program 47 'int add(int a, int b) { return a+b; } int main() { return 5+add(20,22); }'

# Changing a parameter must not change the caller's variable
assert_program 5 'int change(int a) { a=99; return a; } int main() { a: int=5; change(a); return a; }'

# Recursion: every call needs its own parameter storage
assert_program 120 'int factorial(int n) { if(n<=1) return 1; return n*factorial(n-1); } int main() { return factorial(5); }'

# Explicit declarations
assert 5 'a: int=5; return a;'
assert 10 'a: int; a=10; return a;'
assert 15 'a: int=5; b: int=10; return a+b;'
assert 7 'a: int=5; b: int=a+2; return b;'
assert 10 'a: int=5; a=10; return a;'
assert 6 'a: int=5; return ++a;'

# Parameters are already declared locals
assert_program 6 'int increment(int a) { b: int=a+1; return b; } int main() { return increment(5); }'

# Invalid variable uses
assert_error 'Undeclared variable' 'int main() { a=5; return a; }'
assert_error 'Undeclared variable' 'int main() { return missing; }'
assert_error 'Variable already declared' 'int main() { a: int; a: int; return 0; }'

# u8 storage and conversion
assert_program 255 'int main() { x: u8 = 255; return x; }'
assert_program 233 'int main() { x: u8 = 233; return x; }'
assert_program 254 'int main() { x: u8 = 253; return x+1; }'

# u8 increment and decrement
assert_program 0 'int main() { x: u8 = 255; ++x; return x; }'
assert_program 255 'int main() { x: u8 = 255; old: int=x++; return old+x; }'
assert_program 255 'int main() { x: u8 = 0; --x; return x; }'
assert_program 0 'int main() { x: u8 = 0; old: int=x--; return old; }'

# Mixed local sizes: try both stack layouts
assert_program 43 'int main() { a: int=42; b: u8=1; return a+b; }'
assert_program 43 'int main() { b: u8=1; a: int=42; return a+b; }'

# u8 function parameter
assert_program 1 'int get(u8 x) { return x; } int main() { return get(257); }'

# Pointers: address-of and dereference
assert 42 'x: int=42; p: int*; p=&x; return *p;'
assert 9 'x: int; *(&x)=9; return x;'
assert 7 'x: int=1; p: int*; p=&x; *p=7; return x;'

# Pointer aliases and reassignment
assert 42 'x: int=42; p: int*; q: int*; p=&x; q=p; return *q;'
assert 7 'x: int=1; y: int=7; p: int*; p=&x; p=&y; return *p;'

# Pointers to u8
assert 255 'x: u8=255; p: u8*; p=&x; return *p;'
assert 9 'x: u8=1; p: u8*; p=&x; *p=9; return x;'

# Char literals
assert 65 "return 'A';"
assert 65 "c: char='A'; return c;"

# String literals
assert 65 'return *"A";'
assert 104 's: char*="hello"; return *s;'
assert 0 's: char*=""; return *s;'

# Separate literals need separate labels
assert 195 'a: char*="a"; b: char*="b"; return *a+*b;'

# Passing a string to a function
assert_program 90 'int first(char *s) { return *s; } int main() { return first("Z"); }'

# Character escapes
assert 10 "return '\n';"
assert 9 "return '\t';"
assert 13 "return '\r';"
assert 0 "return '\0';"

# String escapes
assert 10 'return *"\n";'
assert 9 'return *"\t";'
assert 92 'return *"\\";'
assert 34 'return *"\"";'

# Preserve bytes after an embedded zero
assert 98 's: char*="a\0b"; s++; s++; return *s;'

# Pointer addition and subtraction
assert 98 'p: char*="abc"; return *(p+1);'
assert 99 'p: char*="abc"; return *(2+p);'
assert 97 'p: char*="abc"; return *((p+2)-2);'

# Pointer differences count elements
assert 3 'p: char*="abc"; return (p+3)-p;'
assert 1 'p: char*="abc"; return ((p+1)-(p+3)) == -2;'
assert 1 'x: int; p: int*=&x; return (p+1)-p;'

# Shared helper must preserve integer subtraction
assert 7 'return 10-3;'
assert 1 'return 3-5 == -2;'

# Invalid pointer arithmetic
assert_error 'Can not add two pointers together' \
  'int main() { x: int; p: int*=&x; return p+p; }'

assert_error 'Cannot subtract a pointer from an integer' \
  'int main() { x: int; p: int*=&x; return 1-p; }'

assert_error 'Cannot subtract pointers to different types' \
  'int main() { x: int; y: char; return &x-&y; }'

# Recursive type compatibility: int ** versus char **
assert_error 'Cannot subtract pointers to different types' \
  'int main() { p: int*; q: char*; return &p-&q; }'

# Pointer Indexing
assert 104 's: char*="hello"; return s[0];'
assert 101 's: char*="hello"; return s[1];'
assert 9 'x: int=1; p: int*=&x; p[0]=9; return x;'
assert 3 'x: int=1; p: int*=&x; p[0]++; return x+1;'

# Block scopes: an inner block can read and update an outer variable
assert 3 'x: int=1; { y: int=2; x=x+y; } return x;'

# A nested declaration shadows the outer variable, then the outer value remains
assert 1 'x: int=1; { x: int=2; if(x!=2) return 0; } return x;'

# Inner block can read a variable from its parent scope
assert 1 'x: int=1; { y: int=x+1; if(y!=2) return 0; } return x;'

# A variable declared in a block is unavailable after leaving it
assert_error 'Undeclared variable' 'int main() { { x: int=2; } return x; }'

# Same-scope duplicates are still rejected
assert_error 'Variable already declared' 'int main() { x: int=1; x: int=2; return x; }'

# A function body shares the parameter scope, so a local cannot redeclare a parameter
assert_error 'Variable already declared' 'int f(int x) { x: int=2; return x; } int main() { return f(1); }'

# Global Variable
assert_program 5 'int x; int main() { x = 5; return x; }'
assert_program 37 'int counter; int set() { counter = 37; return 0; } int main() { set(); return counter; }'

# System Calls
# getpid: syscall number and return value
assert 1 'return syscall(39)>0;'

# close(-1) returns raw -EBADF (-9), rather than libc's -1
assert 1 'return syscall(3,-1)==-9;'

# kill(getpid(), 0) checks two syscall arguments and a nested syscall
assert 1 'return syscall(62,syscall(39),0)==0;'

# write(fd, buffer, length) checks three arguments; prints OK
assert 3 'return syscall(1,1,"OK\n",3);'

# exit(status) should terminate with status 42
assert 42 'return syscall(60,42);'

# runtime tests
assert_program 3 'int linux_write(int fd, char *buffer, int length) { return syscall(1, fd, buffer, length); } int main() { return linux_write(1, "OK\n", 3); }'

# Basic member reads and writes
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; p.x=20; p.y=22; return p.x+p.y; }'

# Members retain independent values
assert_program 42 'struct Pair { int a; int b; }; int main() { p: struct Pair; p.a=10; p.b=32; p.a=p.a+1; return p.a+p.b-1; }'

# Mixed widths and alignment
assert_program 42 'struct Mixed { char a; int b; char c; }; int main() { m: struct Mixed; m.a=1; m.b=39; m.c=2; return m.a+m.b+m.c; }'

# Byte stores must not overwrite neighboring fields
assert_program 42 'struct Bytes { u8 a; u8 b; }; int main() { v: struct Bytes; v.b=42; v.a=257; if(v.a!=1) return 0; return v.b; }'

# Nested structs
assert_program 42 'struct Inner { int x; }; struct Outer { char tag; struct Inner inner; int y; }; int main() { o: struct Outer; o.tag=1; o.inner.x=20; o.y=21; return o.tag+o.inner.x+o.y; }'

# Address of a member
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; x: int*=&p.x; *x=42; return p.x; }'

# Arrow access
assert_program 42 'struct Point { int x; int y; }; int main() { p: struct Point; ptr: struct Point*=&p; ptr->x=20; ptr->y=22; return p.x+p.y; }'

# Struct pointers passed to functions
assert_program 42 'struct Point { int x; }; int set(struct Point *p) { p->x=42; return 0; } int main() { p: struct Point; set(&p); return p.x; }'

# Pointer-valued members
assert_program 42 'struct Holder { int *value; }; int main() { x: int=42; h: struct Holder; h.value=&x; return *h.value; }'

# Self-referencing struct type and chained access
assert_program 42 'struct Node { int value; struct Node *next; }; int main() { a: struct Node; b: struct Node; a.next=&b; b.value=42; return a.next->value; }'

# Global struct storage
assert_program 42 'struct Point { int x; int y; }; struct Point p; int main() { p.x=20; p.y=22; return p.x+p.y; }'

# Prefix and postfix operations on members
assert_program 42 'struct Counter { int value; }; int main() { c: struct Counter; c.value=41; return ++c.value; }'
assert_program 41 'struct Counter { int value; }; int main() { c: struct Counter; c.value=20; old: int=c.value++; return old+c.value; }'

# Struct test errors
assert_error 'Unkown member of struct' \
  'struct Point { int x; }; int main() { p: struct Point; return p.y; }'

assert_error 'Member access requires a struct' \
  'int main() { x: int=1; return x.y; }'

assert_error 'Arrow access requires a pointer to a struct' \
  'struct Point { int x; }; int main() { p: struct Point; return p->x; }'

assert_error 'Duplicate member name' \
  'struct Point { int x; int x; }; int main() { return 0; }'

assert_error 'Member requires a complete type' \
  'struct Node { struct Node next; }; int main() { return 0; }'

# import program test
# assert_import_program 42

finish || exit 1