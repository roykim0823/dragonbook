#!/usr/bin/env bash
# Build and run the numbered programs in code/src/, printing their output, and check the verdicts.
#
#   ./run.sh              every program, in order (01 .. 16, then 90 = fuzzer)
#   ./run.sh 5            only the program whose file name starts with 05_
#   ./run.sh 5 13 90      several
#
# Output goes to stdout (the README quotes it from there). Uses exactly the build line
# documented in each .cpp (g++ -std=c++17 -O2 -Wall -Wextra -I code/include). Exit status is non-zero if a
# compile fails, a program exits non-zero, or its output contains "MISMATCH" or a non-zero
# mismatch / bound-loss count. The programs themselves always exit 0, so this grep is the real
# check. CMakeLists.txt offers the same via ctest.

set -euo pipefail
cd "$(dirname "$0")"

srcs=()
if [ $# -eq 0 ]; then
  srcs=(code/src/[0-9][0-9]_*.cpp)
else
  for n in "$@"; do
    nn=$(printf "%02d" "$n")
    m=(code/src/${nn}_*.cpp)
    if [ ! -e "${m[0]}" ]; then echo "!! no program numbered $nn in code/src/"; exit 2; fi
    srcs+=("${m[@]}")
  done
fi

mkdir -p build
status=0
for src in "${srcs[@]}"; do
  name=$(basename "$src" .cpp)
  echo "================================================================ $name"
  if ! g++ -std=c++17 -O2 -Wall -Wextra -I code/include -o "build/$name" "$src"; then
    echo "!! $name: compile failed"; status=1; continue
  fi
  log="build/$name.log"
  if ! "./build/$name" 2>&1 | tee "$log"; then
    echo "!! $name: exited non-zero"; status=1
  fi
  if grep -Eq 'MISMATCH|[1-9][0-9]* mismatches|[1-9][0-9]* bound losses' "$log"; then
    echo "!! $name: verification FAILED"; status=1
  fi
done

echo "================================================================"
[ $status -eq 0 ] && echo "ALL OK" || echo "SOME CHECKS FAILED"
exit $status
