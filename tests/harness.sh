#!/bin/bash

# Shared assertions and parallel execution for test.sh.
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

  if ! as -o "$dir/program.o" "$dir/program.s"; then
    printf 'FAIL: assembly failed\nSource: %s\n' "$input" >&2
    return 1
  fi

  if ! ld -o "$dir/program" "$dir/program.o"; then
    printf 'FAIL: linking failed\nSource: %s\n' "$input" >&2
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

  if ! as -o "$dir/program.o" "$dir/program.s"; then
    printf 'FAIL: import assembly failed\n' >&2
    return 1
  fi

  if ! ld -o "$dir/program" "$dir/program.o"; then
    printf 'FAIL: import linking failed\n' >&2
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
  printf '  [%s: %s ms]\n' "${labels[$id]}" "$ms"

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
  suite_case=$((suite_case + 1))
  if (( selected_case != 0 && suite_case != selected_case )); then
    return 0
  fi
  while (( ${#running[@]} >= JOBS )); do
    reap_one
  done
  submitted=$((submitted + 1))
  id=$submitted
  mkdir "$work_root/$id" || exit 1
  labels[$id]="$suite:$suite_case"
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

