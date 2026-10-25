"""Optional guest milestones, correlated without relying on synchronized clocks."""

import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time


def mark(event):
    destination = os.environ.get("SEVENZIP_RUNNER_TIMING")
    token = os.environ.get("SEVENZIP_RUN_TOKEN")
    if not destination or not token:
        return
    # Nested tooling fixtures use their own source roots. Do not let their
    # configure/test phases contaminate the owning runner's measurements.
    try:
        path = Path(destination).resolve()
        if len(path.parents) > 1 and path.parents[1] == Path(__file__).resolve().parents[2]:
            with path.open("a", encoding="utf-8") as output:
                output.write(json.dumps({"token": token, "event": event,
                                         "monotonic": time.monotonic()}) + "\n")
    except (OSError, RuntimeError):
        # Measurements must not change the command's result or cancellation.
        return


def summarize(path, token, anchor, invocation_started):
    events = [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines()]
    events = [event for event in events if event["token"] == token]
    result = {"guest_seconds": {}}
    if anchor is not None:
        before, guest, after = anchor
        result["milestones_seconds"] = {}
        for event in events:
            result["milestones_seconds"].setdefault(
                event["event"], round((before + after) / 2 - invocation_started + event["monotonic"] - guest, 3))
    starts = {}
    for event in events:
        name, timestamp = event["event"], event["monotonic"]
        if name.endswith("_start"):
            starts[name[:-6]] = timestamp
        elif name.endswith("_end") and name[:-4] in starts:
            phase = name[:-4]
            result["guest_seconds"][phase] = round(timestamp - starts.pop(phase), 3)
    first = next((event for event in events if event["event"] == "tests_start"), None)
    if first and anchor is not None:
        before, guest, after = anchor
        lower = before - invocation_started + first["monotonic"] - guest
        upper = after - invocation_started + first["monotonic"] - guest
        result.update(to_tests_seconds=round((lower + upper) / 2, 3),
                      to_tests_bounds_seconds=[round(lower, 3), round(upper, 3)])
    return result


def ctest(arguments):
    started = False
    with subprocess.Popen(["ctest", *arguments], stdout=subprocess.PIPE, text=True) as process:
        for line in process.stdout:
            if not started and re.match(r"\s*Start\s+[0-9]+:", line):
                mark("tests_start")
                started = True
            print(line, end="", flush=True)
        return process.wait()


if __name__ == "__main__":
    if sys.argv[1] == "--ctest":
        raise SystemExit(ctest(sys.argv[2:]))
    mark(sys.argv[1])
