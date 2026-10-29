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
PROOF_STEP = {
    "allocation-only": "Prove a GitHub-hosted runner executed",
    "allocation-macos-only": "Prove a macOS GitHub-hosted runner executed",
}
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
    return all(
        isinstance(step, dict)
        and step.get("status") == "completed"
        and step.get("conclusion") == "success"
        and isinstance(step.get("started_at"), str) and bool(step["started_at"])
        and isinstance(step.get("completed_at"), str) and bool(step["completed_at"])
        for step in steps
    )


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
    job_ids = set()
    for job in job_items:
        if not isinstance(job, dict):
            problems.append("malformed job entry")
            continue
        name = job.get("name")
        if name not in REQUIRED or name in seen:
            problems.append(f"unknown or duplicate job: {name}")
            continue
        seen.add(name)
        job_id = job.get("id")
        if type(job_id) is not int or job_id <= 0 or job_id in job_ids:
            problems.append(f"{name}: missing or duplicate job ID")
        else:
            job_ids.add(job_id)
        if job.get("run_id") != run_id or job.get("run_attempt") != attempt:
            problems.append(f"{name}: wrong run or attempt")
        labels = job.get("labels")
        if not isinstance(labels, list) or REQUIRED[name] not in labels:
            problems.append(f"{name}: required runner label absent")
        runner = job.get("runner_id")
        if type(runner) is not int or runner <= 0:
            problems.append(f"{name}: no admitted runner")
        if job.get("status") != "completed" or job.get("conclusion") != "success":
            problems.append(f"{name}: job not completed successfully")
        steps = job.get("steps")
        if not valid_step_sequence(steps, PROOF_STEP[name]):
            problems.append(f"{name}: unexpected, missing or incomplete canary steps")
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
