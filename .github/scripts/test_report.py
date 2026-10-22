import argparse
import os
import re
import sys
import xml.etree.ElementTree as ET

SUITES = [
    (r"^blargg\.", "Blargg"),
    (r"^mooneye\.mbc\d\.", "Mooneye MBC"),
    (r"^mooneye\.", "Mooneye"),
    (r"^wilbertpol\.", "Mooneye (wilbertpol)"),
    (r"^gbmicrotest\.", "GBMicrotest"),
    (r"^age\.", "AGE"),
    (r"^mealybug\.", "Mealybug Tearoom"),
    (r"^acid2\.", "dmg-acid2"),
    (r"^scribbltests\.", "Scribbltests"),
    (r"^turtle\.", "TurtleTests"),
    (r"^strikethrough\.", "Strikethrough"),
    (r"^little-things\.", "little-things-gb"),
    (r"^(mbc3_tester|rtc3test\.)", "MBC3 and RTC"),
    (r"^savestate\.", "Save states"),
]


def suite_of(name):
    for pattern, suite in SUITES:
        if re.search(pattern, name):
            return suite
    return "Other"


def load(path):
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


def cell(text):
    return text.replace("|", "\\|")


def render(cases, commit):
    total = len(cases)
    passed = sum(c["outcome"] == "pass" for c in cases)
    failed = total - passed
    pct = 100 * passed // total if total else 0

    lines = [f"### Test ROM accuracy, commit `{commit[:7]}`", ""]
    lines.append(f"**{passed} of {total} passing ({pct}%)**, {failed} failing.")
    lines.append("")

    suites = {}
    for c in cases:
        suites.setdefault(suite_of(c["name"]), []).append(c)

    lines.append("| Suite | Pass | Fail | Time |")
    lines.append("|---|---|---|---|")
    order = [suite for _, suite in SUITES] + ["Other"]
    for suite, group in sorted(suites.items(), key=lambda kv: order.index(kv[0])):
        ok = sum(c["outcome"] == "pass" for c in group)
        bad = len(group) - ok
        time = fmt_time(sum(c["time"] for c in group))
        lines.append(f"| {suite} | {ok} / {len(group)} | {bad} | {time} |")
    lines.append("")

    lines.append(f"<details><summary>All tests ({total})</summary>")
    lines.append("")
    lines.append("| Test | Suite | Result | Time | Output |")
    lines.append("|---|---|---|---|---|")
    for c in sorted(cases, key=lambda c: (c["outcome"] == "pass", suite_of(c["name"]), c["name"])):
        result = "Pass" if c["outcome"] == "pass" else "**Fail**"
        reason = f"`{cell(c['reason'])}`" if c["reason"] else ""
        lines.append(f"| `{cell(c['name'])}` | {suite_of(c['name'])} | {result} | {fmt_time(c['time'])} | {reason} |")
    lines.append("")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("results")
    parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", "local"))
    args = parser.parse_args()

    if not os.path.exists(args.results):
        print(f"Test results not found: {args.results}", file=sys.stderr)
        return 1

    cases = load(args.results)
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
