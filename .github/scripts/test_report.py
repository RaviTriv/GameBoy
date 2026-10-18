import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

SUITES = [
    (r"^blargg\.\d\d-", "Blargg cpu_instrs"),
    (r"^blargg\.(instr_timing|mem_timing|mem_timing_2)$", "Blargg timing"),
    (r"^blargg\.halt_bug$", "Blargg halt_bug"),
    (r"^blargg\.oam_bug$", "Blargg PPU"),
    (r"^mooneye\.mbc1\.", "Mooneye MBC1"),
    (r"^mooneye\.mbc2\.", "Mooneye MBC2"),
    (r"^mooneye\.", "Mooneye acceptance"),
    (r"^blargg\.", "Blargg other"),
]


def suite_of(name):
    for pattern, suite in SUITES:
        if re.search(pattern, name):
            return suite
    return "Other"


def load(path, known_fail):
    if not path or not os.path.exists(path):
        return []
    cases = []
    for case in ET.parse(path).getroot().iter("testcase"):
        status = case.get("status", "run")
        if case.find("skipped") is not None or status in ("notrun", "disabled"):
            outcome = "skipped"
        elif case.find("failure") is not None or case.find("error") is not None or status == "fail":
            outcome = "fail"
        else:
            outcome = "pass"
        cases.append({
            "name": case.get("name"),
            "reason": first_line(case.findtext("system-out")) if outcome != "pass" else "",
            "time": float(case.get("time") or 0),
            "outcome": outcome,
            "known_fail": known_fail,
        })
    return cases


def first_line(text, limit=120):
    for line in (text or "").splitlines():
        line = line.strip()
        if line:
            line = line.replace("`", "'")
            return line if len(line) <= limit else line[:limit] + "..."
    return ""


def fmt_time(seconds):
    return f"{seconds:.1f}s"


def render(cases, commit):
    total = len(cases)
    passed = sum(c["outcome"] == "pass" for c in cases)
    known = [c for c in cases if c["known_fail"] and c["outcome"] != "pass"]
    regressions = [c for c in cases if not c["known_fail"] and c["outcome"] != "pass"]
    fixed = [c for c in cases if c["known_fail"] and c["outcome"] == "pass"]
    pct = 100 * passed // total if total else 0

    lines = [f"### Test ROM accuracy, commit `{commit[:7]}`", ""]
    lines.append(
        f"**{passed} of {total} passing ({pct}%)**. "
        f"{len(known)} known failures, {len(regressions)} regressions."
    )
    lines.append("")

    suites = {}
    for c in cases:
        suites.setdefault(suite_of(c["name"]), []).append(c)

    lines.append("| Suite | Pass | Known fail | Fail | Time |")
    lines.append("|---|---|---|---|---|")
    order = [suite for _, suite in SUITES] + ["Other"]
    for suite, group in sorted(suites.items(), key=lambda kv: order.index(kv[0])):
        ok = sum(c["outcome"] == "pass" for c in group)
        kf = sum(c["known_fail"] and c["outcome"] != "pass" for c in group)
        bad = sum(not c["known_fail"] and c["outcome"] != "pass" for c in group)
        ran = sum(c["time"] for c in group if c["outcome"] == "pass")
        time = fmt_time(ran) if ok else "-"
        lines.append(f"| {suite} | {ok} / {len(group)} | {kf} | {bad} | {time} |")
    lines.append("")

    def section(title, items, show_time=False, show_reason=False):
        if not items:
            return
        lines.append(f"<details><summary>{title} ({len(items)})</summary>")
        lines.append("")
        for c in items:
            mark = "[x]" if c["outcome"] == "pass" else "[ ]"
            suffix = f" {fmt_time(c['time'])}" if show_time else ""
            if show_reason and c["reason"]:
                suffix += f": `{c['reason']}`"
            lines.append(f"- {mark} `{c['name']}`{suffix}")
        lines.append("")
        lines.append("</details>")
        lines.append("")

    section("Regressions", regressions, show_reason=True)
    section("Newly passing, remove mark_known_fail", fixed)
    section("Known failures", known, show_reason=True)
    section("All tests", sorted(cases, key=lambda c: c["name"]), show_time=True)
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("results")
    parser.add_argument("--known-fail")
    parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", "local"))
    args = parser.parse_args()

    if not os.path.exists(args.results):
        print(f"Gating results not found: {args.results}", file=sys.stderr)
        return 1

    cases = load(args.results, False) + load(args.known_fail, True)
    if not cases:
        print("No test results found", file=sys.stderr)
        return 1

    report = render(cases, args.commit)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a") as f:
            f.write(report + "\n")
    print(report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
