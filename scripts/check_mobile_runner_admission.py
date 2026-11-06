#!/usr/bin/env python3
"""Read-only, fail-closed verifier for source-free runner-admission job metadata.

Input files are saved responses to the GitHub REST endpoints for a workflow run
and its latest-attempt jobs. No GitHub token, network, checkout, or source transfer.
"""
import argparse
import json
import sys
from pathlib import Path
from datetime import datetime

REPO = "ores-truffle-oreslang/oreslang-platform.mobile"
REPO_ID = 1419865409  # Immutable GitHub repository identity; name alone can be reused.
WORKFLOW = "Source-local runner allocation canary (metadata only)"
REQUIRED = {"allocation-only": "ubuntu-24.04", "allocation-macos-only": "macos-15"}
PROOF_STEP = {
    "allocation-only": "Prove a GitHub-hosted runner executed",
    "allocation-macos-only": "Prove a macOS GitHub-hosted runner executed",
}
def parse_timestamp(value):
    """Parse a timezone-aware timestamp without trusting truthy non-strings."""
    if not isinstance(value, str) or not value:
        return None
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
        return parsed if parsed.tzinfo is not None and parsed.utcoffset() is not None else None
    except (ValueError, OverflowError):
        return None


def valid_timestamp_pair(step):
    """Require real timezone-aware GitHub timestamps in chronological order."""
    started = parse_timestamp(step.get("started_at"))
    completed = parse_timestamp(step.get("completed_at"))
    return started is not None and completed is not None and started <= completed


# GitHub includes its own housekeeping steps in the API for executed jobs.
# A safe canary contains only optional setup, exactly one proof, optional finish.
def valid_step_sequence(steps, proof):
    if not isinstance(steps, list):
        return False
    names = [step.get("name") if isinstance(step, dict) else None for step in steps]
    allowed = (
        [proof],
        ["Set up job", proof],
        [proof, "Complete job"],
        ["Set up job", proof, "Complete job"],
    )
    if names not in allowed:
        return False
    if not all(
        isinstance(step, dict)
        and step.get("status") == "completed"
        and step.get("conclusion") == "success"
        and valid_timestamp_pair(step)
        for step in steps
    ):
        return False
    # Array order must agree with actual time order, not just step names.
    return all(parse_timestamp(left["completed_at"]) <= parse_timestamp(right["started_at"])
               for left, right in zip(steps, steps[1:]))


def validate(run, jobs, expected_sha):
    problems = []
    if not isinstance(run, dict) or not isinstance(jobs, dict):
        return ["invalid run/jobs metadata"]
    run_id = run.get("id")
    attempt = run.get("run_attempt")
    if type(run_id) is not int or run_id <= 0:
        problems.append("invalid run ID")
    if type(attempt) is not int or attempt <= 0:
        problems.append("invalid run attempt")
    if run.get("name") != WORKFLOW:
        problems.append("wrong workflow name")
    repository = run.get("repository")
    if not isinstance(repository, dict) or repository.get("full_name") != REPO:
        problems.append("wrong source repository")
    if not isinstance(repository, dict) or type(repository.get("id")) is not int or repository.get("id") != REPO_ID:
        problems.append("wrong immutable source repository ID")
    if not isinstance(repository, dict) or repository.get("private") is not True:
        problems.append("source repository privacy not verified")
    if (not isinstance(expected_sha, str) or len(expected_sha) != 40 or
            any(c not in "0123456789abcdef" for c in expected_sha)):
        problems.append("invalid expected Git SHA")
    if run.get("head_sha") != expected_sha:
        problems.append("wrong source head SHA")
    if run.get("event") not in ("pull_request", "workflow_dispatch"):
        problems.append("unapproved canary trigger")
    source_head = run.get("head_repository")
    if (not isinstance(source_head, dict) or source_head.get("full_name") != REPO or
            type(source_head.get("id")) is not int or source_head.get("id") != REPO_ID or
            source_head.get("private") is not True):
        problems.append("forked or untrusted workflow head repository")
    if run.get("status") != "completed" or run.get("conclusion") != "success":
        problems.append("workflow not completed successfully")
    job_items = jobs.get("jobs")
    if not isinstance(job_items, list):
        return problems + ["jobs list absent"]
    if jobs.get("total_count") != len(job_items):
        problems.append("job listing incomplete or inconsistent")
    if len(job_items) != len(REQUIRED):
        problems.append("unexpected number of jobs")
    seen = set()
    job_ids = set()
    for job in job_items:
        if not isinstance(job, dict):
            problems.append("malformed job entry")
            continue
        name = job.get("name")
        if not isinstance(name, str) or name not in REQUIRED or name in seen:
            problems.append(f"unknown or duplicate job: {name}")
            continue
        seen.add(name)
        job_id = job.get("id")
        if type(job_id) is not int or job_id <= 0 or job_id in job_ids:
            problems.append(f"{name}: missing or duplicate job ID")
        else:
            job_ids.add(job_id)
        # bool == 1 is true in Python. IDs and attempts must be actual integers.
        if (type(job.get("run_id")) is not int or job.get("run_id") != run_id or
                type(job.get("run_attempt")) is not int or job.get("run_attempt") != attempt):
            problems.append(f"{name}: wrong run or attempt")
        if job.get("head_sha") != expected_sha:
            problems.append(f"{name}: job checkout source head mismatch")
        job_start = parse_timestamp(job.get("started_at"))
        job_end = parse_timestamp(job.get("completed_at"))
        if job_start is None or job_end is None or job_start > job_end:
            problems.append(f"{name}: invalid job timing")
        labels = job.get("labels")
        if (not isinstance(labels, list) or
                any(type(label) is not str or not label for label in labels) or
                len(labels) != len(set(labels)) or
                REQUIRED[name] not in labels or "self-hosted" in labels or
                any(other != REQUIRED[name] and other in labels for other in REQUIRED.values())):
            # A self-hosted or conflicting-OS label cannot establish a GitHub-hosted canary.
            problems.append(f"{name}: missing, malformed or untrusted runner labels")
        runner = job.get("runner_id")
        if type(runner) is not int or runner <= 0:
            problems.append(f"{name}: no admitted runner")
        if job.get("status") != "completed" or job.get("conclusion") != "success":
            problems.append(f"{name}: job not completed successfully")
        steps = job.get("steps")
        if not valid_step_sequence(steps, PROOF_STEP[name]):
            problems.append(f"{name}: unexpected, missing or incomplete canary steps")
        elif job_start is not None and job_end is not None:
            if (job_start > parse_timestamp(steps[0]["started_at"]) or
                    parse_timestamp(steps[-1]["completed_at"]) > job_end):
                problems.append(f"{name}: steps fall outside job interval")
    if seen != set(REQUIRED):
        problems.append("required jobs missing")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-json", required=True, type=Path)
    parser.add_argument("--jobs-json", required=True, type=Path)
    parser.add_argument("--expected-sha", required=True)
    args = parser.parse_args()
    if len(args.expected_sha) != 40 or any(ch not in "0123456789abcdef" for ch in args.expected_sha):
        parser.error("--expected-sha must be a full lowercase 40-character Git commit SHA")
    try:
        run = json.loads(args.run_json.read_text(encoding="utf-8"))
        jobs = json.loads(args.jobs_json.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        print(f"ADMISSION_FAIL: unable to load input: {type(exc).__name__}", file=sys.stderr)
        return 1
    problems = validate(run, jobs, args.expected_sha)
    if problems:
        for message in problems:
            print(f"ADMISSION_FAIL: {message}", file=sys.stderr)
        return 1
    print(f"ADMISSION_PASS: exact-head source-free ubuntu-24.04 and macos-15 jobs ran successfully (run {run['id']}, attempt {run['run_attempt']})")
    print("ADMISSION_METADATA_ONLY: verify workflow contents independently; no product or source-transfer proof")
    return 0


if __name__ == "__main__":
    sys.exit(main())
