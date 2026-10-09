#!/usr/bin/env python3
# Review CI probe runner: builds and runs review/probes/*.cpp and compares the outcome
# with the prediction in the probe header. Review branch only, not for merging.
#
# Probe header tags (before the first non-comment line):
#   // probe: <id and what it checks>
#   // where: <job ids>
#   // std: <standards>
#   // expect: build-ok | build-fail ["substring"] | run-ok | run-fail
#   // expect c++23: <outcome for one standard>
#   // with: <extra translation unit next to the probe>
#   // meaning: <what each outcome means for the hypothesis>
#
# A prediction that does not hold is a normal result (the hypothesis is refuted); the script
# always exits 0, so only the project build and tests can fail a CI job.

import argparse
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile

MSVC_STD = {
    "c++14": "/std:c++14",
    "c++17": "/std:c++17",
    "c++20": "/std:c++20",
    "c++23": "/std:c++latest",
    "c++2c": "/std:c++latest",
}

STD_UNSUPPORTED = re.compile(r"unrecognized command-line option .-std=|invalid value .* in .-std=|unknown argument.*-std|D9002.*/std:")

TAG = re.compile(r"//\s*(probe|where|std|with|expect|meaning)(?:\s+(c\+\+\w+))?\s*:\s*(.*)$")


def parse(path):
    tags = {"probe": "", "where": [], "std": [], "with": [], "expect": {}, "meaning": ""}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = TAG.match(line)
        if not match:
            if line.strip() and not line.startswith("//"):
                break
            continue
        key, std, value = match.groups()
        value = value.strip()
        if key == "expect":
            tags["expect"][std or "*"] = value
        elif key in ("probe", "meaning"):
            tags[key] = value
        else:
            tags[key] = value.split()
    return tags


def key_line(text):
    lines = [line.strip() for line in text.splitlines() if line.strip()]
    for line in lines:
        if re.search(r"error|warning|LNK\d+|C\d{4}", line):
            return line
    return lines[0] if lines else ""


def build(args, src, extra, std, out):
    if args.style == "msvc":
        if std not in MSVC_STD:
            return None
        cmd = [args.cxx, "/nologo", "/EHsc", "/W4", "/WX", "/permissive-", MSVC_STD[std], "/I", args.include, str(src)]
        cmd += [str(e) for e in extra] + shlex.split(args.flags) + ["/Fe" + out]
    else:
        cmd = [args.cxx, "-std=" + std, "-Wall", "-Wextra", "-pedantic-errors", "-Werror", "-I", args.include, str(src)]
        cmd += [str(e) for e in extra] + shlex.split(args.flags) + ["-o", out]
    return cmd


def evaluate(expect, built, build_out, ran, run_out, run_rc):
    kind, _, rest = expect.partition(" ")
    needle = rest.strip().strip('"')
    if kind == "build-ok":
        ok = built
    elif kind == "build-fail":
        ok = not built and needle in build_out
    elif kind == "run-ok":
        ok = built and ran and run_rc == 0
    elif kind == "run-fail":
        ok = built and ran and run_rc != 0
    else:
        raise ValueError("unknown expectation: " + expect)
    return ok


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--job", required=True)
    parser.add_argument("--style", choices=["gcc", "msvc"], required=True)
    parser.add_argument("--cxx", required=True)
    parser.add_argument("--flags", default="")
    args = parser.parse_args()

    probes_dir = pathlib.Path(__file__).resolve().parent
    args.include = str(probes_dir.parent.parent / "include")
    rows = []
    for src in sorted(probes_dir.glob("*.cpp")):
        tags = parse(src)
        if not tags["probe"] or args.job not in tags["where"]:
            continue
        extra = [probes_dir / name for name in tags["with"]]
        for std in tags["std"]:
            expect = tags["expect"].get(std, tags["expect"].get("*"))
            with tempfile.TemporaryDirectory() as tmp:
                out = os.path.join(tmp, "probe.exe" if args.style == "msvc" else "probe")
                cmd = build(args, src, extra, std, out)
                if cmd is None:
                    continue
                result = subprocess.run(cmd, cwd=tmp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                built = result.returncode == 0
                build_out = result.stdout
                ran, run_out, run_rc = False, "", None
                if built and expect.startswith("run"):
                    run = subprocess.run([out], cwd=tmp, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
                    ran, run_out, run_rc = True, run.stdout, run.returncode
            if not built and STD_UNSUPPORTED.search(build_out):
                rows.append((tags["probe"].split()[0], src.name, std, expect, "std not supported", "skipped", key_line(build_out)))
                continue
            ok = evaluate(expect, built, build_out, ran, run_out, run_rc)
            actual = ("build-ok" if built else "build-fail") + (("; run rc=%d" % run_rc) if ran else "")
            detail = key_line(run_out) if ran else ("" if built else key_line(build_out))
            print("== %s [%s] %s" % (src.name, std, " ".join(cmd)))
            print(build_out + run_out)
            rows.append((tags["probe"].split()[0], src.name, std, expect, actual, "held" if ok else "NOT held", detail))

    table = ["| probe | file | std | predicted | actual | prediction | key line |", "|---|---|---|---|---|---|---|"]
    for row in rows:
        table.append("| " + " | ".join(cell.replace("|", "\\|")[:200] for cell in row) + " |")
    report = "### Probes: %s\n\n%s\n" % (args.job, "\n".join(table))
    print(report)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
