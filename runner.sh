#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LLVM_BIN=/data2/ben/alive-decomp/tools/llvm-project/build/bin
BUILD=$SCRIPT_DIR/build

# Compile to unoptimized IR (disable optnone so opt can run passes)
$LLVM_BIN/clang -O0 -Xclang -disable-O0-optnone -emit-llvm -S \
    -o /tmp/example.ll $SCRIPT_DIR/example.c

# Run instcombine, capturing IR before and after (module scope for complete IR with metadata)
$LLVM_BIN/opt -passes=instcombine \
    --print-before=instcombine --print-after=instcombine --print-module-scope \
    /tmp/example.ll -o /dev/null 2>/tmp/opt_dump.txt

# Extract pre and post IR sections, filtering tagDebugMarkers noise (lines ending with ;<digits>)
awk '
    /^; \*\*\* IR Dump Before InstCombinePass/ { mode="pre"; next }
    /^; \*\*\* IR Dump After InstCombinePass/  { mode="post"; next }
    /^; \*\*\*/                                { mode="" }
    /;[0-9]+$/                                 { next }
    mode=="pre"  { print > "/tmp/pre.ll" }
    mode=="post" { print > "/tmp/post.ll" }
' /tmp/opt_dump.txt

$BUILD/touched-diff /tmp/pre.ll /tmp/post.ll foo
