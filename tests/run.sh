#!/bin/sh
# Conformance-style test runner.
#   pass/     must compile, exit 0, and print exactly NAME.expected
#   reject/   must be refused by the compiler with exit status 1
#   runtime/  must compile, then stop with a reported run-time error
# usage: tests/run.sh path/to/pasc
set -u
PASC=${1:?usage: run.sh path/to/pasc}
DIR=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
pass=0
fail=0

ok()  { pass=$((pass + 1)); }
bad() { fail=$((fail + 1)); echo "FAIL $1: $2"; }

for src in "$DIR"/pass/*.pas; do
  name=pass/$(basename "$src" .pas)
  if ! "$PASC" -o "$TMP/a.out" "$src" 2> "$TMP/err"; then
    bad "$name" "did not compile"; cat "$TMP/err"; continue
  fi
  if ! "$TMP/a.out" > "$TMP/out" 2> "$TMP/err"; then
    bad "$name" "non-zero exit"; cat "$TMP/err"; continue
  fi
  if diff -u "${src%.pas}.expected" "$TMP/out" > "$TMP/diff"; then ok
  else bad "$name" "wrong output"; cat "$TMP/diff"; fi
done

for src in "$DIR"/reject/*.pas; do
  name=reject/$(basename "$src" .pas)
  "$PASC" -o "$TMP/a.out" "$src" > /dev/null 2> "$TMP/err"
  status=$?
  if [ "$status" -ne 1 ]; then bad "$name" "compiler exit status $status, want 1"
  elif ! grep -q "error:" "$TMP/err"; then bad "$name" "no diagnostic"
  else ok; fi
done

for src in "$DIR"/runtime/*.pas; do
  name=runtime/$(basename "$src" .pas)
  if ! "$PASC" -o "$TMP/a.out" "$src" 2> "$TMP/err"; then
    bad "$name" "did not compile"; cat "$TMP/err"; continue
  fi
  "$TMP/a.out" > /dev/null 2> "$TMP/err"
  status=$?
  if [ "$status" -ne 1 ]; then bad "$name" "exit status $status, want 1"
  elif ! grep -q "runtime error at line" "$TMP/err"; then bad "$name" "no report"
  else ok; fi
done

echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
