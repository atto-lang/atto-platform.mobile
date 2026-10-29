#!/usr/bin/env python3
"""Offline negative matrix for the source-free GitHub runner metadata verifier."""
import copy
import unittest
from check_mobile_runner_admission import validate, REPO, WORKFLOW

SHA = "a" * 40
RUN = {"id": 81, "run_attempt": 2, "name": WORKFLOW, "repository": {"full_name": REPO},
       "head_sha": SHA, "status": "completed", "conclusion": "success"}


def job(name, label):
    return {"id": 200, "run_id": 81, "run_attempt": 2, "name": name,
            "runner_id": 123456, "labels": [label], "status": "completed", "conclusion": "success",
            "steps": [{"name": "Prove runner", "status": "completed", "conclusion": "success",
                       "started_at": "2026-10-29T03:00:00Z", "completed_at": "2026-10-29T03:00:01Z"}]}


JOBS = {"total_count": 2, "jobs": [job("allocation-only", "ubuntu-24.04"),
                                     job("allocation-macos-only", "macos-15")]}


class RunnerAdmissionTests(unittest.TestCase):
    def check_reject(self, run=None, jobs=None):
        self.assertTrue(validate(copy.deepcopy(run if run is not None else RUN),
                                 copy.deepcopy(jobs if jobs is not None else JOBS), SHA))

    def test_valid(self):
        self.assertEqual(validate(RUN, JOBS, SHA), [])

    def test_runner_zero(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["runner_id"] = 0; self.check_reject(jobs=d)

    def test_runner_null(self):
        d = copy.deepcopy(JOBS); d["jobs"][1]["runner_id"] = None; self.check_reject(jobs=d)

    def test_steps_empty(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["steps"] = []; self.check_reject(jobs=d)

    def test_steps_unfinished(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["steps"][0]["completed_at"] = None; self.check_reject(jobs=d)

    def test_steps_failed(self):
        d = copy.deepcopy(JOBS); d["jobs"][1]["steps"][0]["conclusion"] = "failure"; self.check_reject(jobs=d)

    def test_job_failed(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["conclusion"] = "failure"; self.check_reject(jobs=d)

    def test_run_failed(self):
        d = copy.deepcopy(RUN); d["conclusion"] = "failure"; self.check_reject(run=d)

    def test_missing_job(self):
        d = copy.deepcopy(JOBS); d["jobs"].pop(); self.check_reject(jobs=d)

    def test_partial_pagination(self):
        d = copy.deepcopy(JOBS); d["total_count"] = 3; self.check_reject(jobs=d)

    def test_wrong_attempt(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["run_attempt"] = 1; self.check_reject(jobs=d)

    def test_wrong_run_id(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["run_id"] = 82; self.check_reject(jobs=d)

    def test_wrong_label(self):
        d = copy.deepcopy(JOBS); d["jobs"][0]["labels"] = ["ubuntu-latest"]; self.check_reject(jobs=d)

    def test_untrusted_repo(self):
        d = copy.deepcopy(RUN); d["repository"]["full_name"] = "elsewhere/repo"; self.check_reject(run=d)

    def test_wrong_head(self):
        d = copy.deepcopy(RUN); d["head_sha"] = "b" * 40; self.check_reject(run=d)

    def test_wrong_workflow(self):
        d = copy.deepcopy(RUN); d["name"] = "Fake green check"; self.check_reject(run=d)

    def test_duplicate_job(self):
        d = copy.deepcopy(JOBS); d["jobs"][1]["name"] = d["jobs"][0]["name"]; self.check_reject(jobs=d)


if __name__ == "__main__":
    unittest.main()
