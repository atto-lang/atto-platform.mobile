#!/usr/bin/env python3
"""Read-only, fail-closed verifier for source-free runner-admission job metadata.

Input files are saved responses to the GitHub REST endpoints for a workflow run
and its latest-attempt jobs. No GitHub token, network, checkout, or source transfer.
"""
import argparse
import json
import sys
from pathlib import Path

REPO = "ores-truffle-oreslang/oreslang-platform.mobile"
WORKFLOW = "Source-local runner allocation canary (metadata only)"
REQUIRED = {"allocation-only": "ubuntu-24.04", "allocation-macos-only": "macos-15"}


def validate(run, jobs, expected_sha):
    problems = []
    if not isinstance(run, dict) or not isinstance(jobs, dict):
        return ["invalid run/jobs metadata"]
    run_id = run.get("id")
    attempt = run.get("run_attempt")
    if not isinstance(run_id, int) or run_id <= 0:
        problems.append("invalid run ID")
    if not isinstance(attempt, int) or attempt <= 0:
        problems.append("invalid run attempt")
    if run.get("name") != WORKFLOW:
        problems.append("wrong workflow name")
    if (run.get("repository") or {}).get("full_name") != REPO:
        problems.append("wrong source repository")
    if run.get("head_sha") != expected_sha:
        problems.append("wrong source head SHA")
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
    for job in job_items:
        if not isinstance(job, dict):
            problems.append("malformed job entry")
            continue
        name = job.get("name")
        if name not in REQUIRED or name in seen:
            problems.append(f"unknown or duplicate job: {name}")
            continue
        seen.add(name)
        if job.get("run_id") != run_id or job.get("run_attempt") != attempt:
            problems.append(f"{name}: wrong run or attempt")
        labels = job.get("labels")
        if not isinstance(labels, list) or REQUIRED[name] not in labels:
            problems.append(f"{name}: required runner label absent")
        runner = job.get("runner_id")
        if not isinstance(runner, int) or runner <= 0:
            problems.append(f"{name}: no admitted runner")
        if job.get("status") != "completed" or job.get("conclusion") != "success":
            problems.append(f"{name}: job not completed successfully")
        steps = job.get("steps")
        if not isinstance(steps, list) or len(steps) != 1:
            problems.append(f"{name}: expected one executed source-free step")
        elif any(not isinstance(step, dict) or step.get("status") != "completed"
                 or step.get("conclusion") != "success"
                 or not step.get("started_at") or not step.get("completed_at")
                 for step in steps):
            problems.append(f"{name}: no completed successful step")
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
    print("SOURCE_CHECKOUT=false SOURCE_EXPORT=false PRODUCT_TESTS_EXECUTED=false")
    return 0


if __name__ == "__main__":
    sys.exit(main())
