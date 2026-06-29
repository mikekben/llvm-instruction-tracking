#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: run_functions.sh <functions.txt> <funcs-dir> <llvm-bin-dir> <touched-diff> <output.jsonl>"
    exit 1
}

[ $# -eq 5 ] || usage

FUNCS_LIST="$1"
FUNCS_DIR="$2"
LLVM_BIN="$3"
TOUCHED_DIFF="$4"
OUTPUT="$5"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

parallel -j 32 \
    python3 "$SCRIPT_DIR/run_functions.py" \
        --opt "$LLVM_BIN/opt" \
        --touched-diff "$TOUCHED_DIFF" \
        --funcs-dir "$FUNCS_DIR" \
        {} \
    :::: "$FUNCS_LIST" \
    > "$OUTPUT"

echo "Written $OUTPUT"
