#!/usr/bin/env python3
"""Run touched-diff across all O3 passes for a single function .ll file.

Usage: run_acyclic.py --opt <path> --touched-diff <path> --funcs-dir <path> <relative-path>

Output: one JSON line to stdout.
"""

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

NOISE_RE  = re.compile(r';\d+$')
HEADER_RE = re.compile(r'^\s*;\s*\*\*\* IR Dump (Before|After) (.+?) on ')
STAR_RE   = re.compile(r'^\s*;\s*\*\*\*')
COUNT_RE  = re.compile(r'(\w+): \{([^}]*)\}')


def count_instructions(ir_text, func_name):
    inside = False
    count = 0
    for line in ir_text.splitlines():
        if not inside:
            if 'define' in line and f'@{func_name}' in line:
                inside = True
        else:
            if line.rstrip() == '}':
                break
            if line.startswith(' ') or line.startswith('\t'):
                count += 1
    return count


def parse_sections(dump_text):
    sections = []
    current_dir = None
    current_pass = None
    current_lines = []

    for line in dump_text.splitlines():
        m = HEADER_RE.match(line)
        if m:
            if current_dir is not None:
                sections.append((current_dir, current_pass, '\n'.join(current_lines)))
            current_dir  = m.group(1)
            current_pass = m.group(2)
            current_lines = []
        elif STAR_RE.match(line):
            if current_dir is not None:
                sections.append((current_dir, current_pass, '\n'.join(current_lines)))
            current_dir = current_pass = None
            current_lines = []
        elif NOISE_RE.search(line):
            continue
        elif current_dir is not None:
            current_lines.append(line)

    if current_dir is not None:
        sections.append((current_dir, current_pass, '\n'.join(current_lines)))

    return sections


def pair_sections(sections):
    pairs = []
    i = 0
    while i < len(sections):
        if sections[i][0] == 'Before' and i + 1 < len(sections) and sections[i+1][0] == 'After':
            _, pass_name, before_ir = sections[i]
            _, _,         after_ir  = sections[i+1]
            pairs.append((pass_name, before_ir, after_ir))
            i += 2
        else:
            i += 1
    return pairs


def run_touched_diff(touched_diff, pre_ir, post_ir, func_name, tmpdir):
    pre_path  = Path(tmpdir) / "pre.ll"
    post_path = Path(tmpdir) / "post.ll"
    pre_path.write_text(pre_ir)
    post_path.write_text(post_ir)

    result = subprocess.run(
        [touched_diff, str(pre_path), str(post_path), func_name],
        capture_output=True, text=True
    )

    if result.returncode != 0:
        return None, result.stderr.strip()

    counts = {}
    for line in result.stdout.splitlines():
        m = COUNT_RE.match(line)
        if m:
            items = [x for x in m.group(2).split(',') if x.strip()]
            counts[m.group(1).lower()] = len(items)
    return counts, None


def run_function(opt, touched_diff, ll_path, func_name):
    dump = subprocess.run(
        [opt, "-passes=default<O3>",
         "--print-before-all", "--print-after-all", "--print-module-scope",
         str(ll_path), "-o", "/dev/null"],
        capture_output=True, text=True
    )

    sections = parse_sections(dump.stderr)
    pairs    = pair_sections(sections)

    pass_results = []
    with tempfile.TemporaryDirectory() as tmpdir:
        for pass_name, before_ir, after_ir in pairs:
            n_input = count_instructions(before_ir, func_name)
            if before_ir == after_ir:
                pass_results.append({"name": pass_name, "n_input": n_input, "removed": 0, "added": 0, "modified": 0})
                continue

            counts, err = run_touched_diff(touched_diff, before_ir, after_ir, func_name, tmpdir)
            if counts is not None:
                pass_results.append({"name": pass_name, "n_input": n_input, **counts})
            else:
                pass_results.append({"name": pass_name, "n_input": n_input, "error": err})

    return pass_results


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("rel_path", help="Path to .ll file relative to --funcs-dir")
    parser.add_argument("--opt",          required=True, help="Path to opt binary")
    parser.add_argument("--touched-diff", required=True, help="Path to touched-diff binary")
    parser.add_argument("--funcs-dir",    required=True, help="Path to bench/funcs directory")
    args = parser.parse_args()

    ll_path   = Path(args.funcs_dir) / args.rel_path
    func_name = Path(args.rel_path).stem

    passes = run_function(args.opt, args.touched_diff, ll_path, func_name)
    record = {"file": args.rel_path, "function": func_name, "passes": passes}
    print(json.dumps(record), flush=True)
    print(f"done: {func_name}", file=sys.stderr, flush=True)


if __name__ == "__main__":
    main()
