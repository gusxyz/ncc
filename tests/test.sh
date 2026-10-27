#!/bin/bash

# Run all suites, named suites, or a single case (e.g. pointers:3).
# Requires Bash 5.1+ for the parallel harness.
set -uo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." || exit 1

usage() {
  printf 'Usage: %s [--list | SUITE[:CASE] ...]\n' "$0"
  printf 'No arguments runs all suites. CASE is the assertion number within a suite.\n'
  printf 'Examples: make test-pointers; make test TEST="pointers:3 functions"\n'
}

suites=(tests/cases/*.sh)
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
  usage
  exit 0
fi
if [[ "${1:-}" == --list ]]; then
  for file in "${suites[@]}"; do
    name=${file##*/}
    printf '%s\n' "${name%.sh}"
  done
  exit 0
fi
if (( $# == 0 )); then
  for file in "${suites[@]}"; do
    name=${file##*/}
    set -- "$@" "${name%.sh}"
  done
fi

# Validate every selector before starting any workers.
for selector in "$@"; do
  if [[ ! "$selector" =~ ^([a-z][a-z0-9_]*)(:([1-9][0-9]{0,5}))?$ ]] ||
     [[ ! -f "tests/cases/${selector%%:*}.sh" ]]; then
    printf 'Unknown suite or invalid case: %s\n' "$selector" >&2
    usage >&2
    exit 1
  fi
done

source tests/harness.sh
for selector in "$@"; do
  suite=${selector%%:*}
  selected_case=0
  if [[ "$selector" == *:* ]]; then
    selected_case=${selector#*:}
  fi
  suite_case=0
  source "tests/cases/$suite.sh"
  if (( selected_case > suite_case )); then
    printf 'FAIL: %s has %s cases; requested case %s\n' "$suite" "$suite_case" "$selected_case" >&2
    # Drain any workers from earlier selectors before reporting failure.
    while (( ${#running[@]} )); do reap_one; done
    exit 1
  fi
done
finish || exit 1