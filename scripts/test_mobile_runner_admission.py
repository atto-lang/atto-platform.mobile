#!/usr/bin/env python3
"""Offline negative matrix for the source-free GitHub runner metadata verifier."""
import copy
import unittest
from check_mobile_runner_admission import validate, REPO, WORKFLOW, PROOF_STEP

SHA = "a" * 40
RUN = {"id": 81, "run_attempt": 2, "name": WORKFLOW, "repository": {"full_name": REPO},
       "head_sha": SHA, "status": "completed", "conclusion": "success"}


def job(name, label):
    return {"id": 200 if name == "allocation-only" else 201,
            "run_id": 81, "run_attempt": 2, "name": name,
            "runner_id": 123456, "labels": [label], "status": "completed", "conclusion": "success",
            "steps": [{"name": PROOF_STEP[name], "status": "completed", "conclusion": "success",
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


    def test_malformed_job_name_as_array(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["name"] = ["allocation-only"]
        self.check_reject(jobs=d)

    def test_malformed_job_name_as_object(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["name"] = {"name": "allocation-only"}
        self.check_reject(jobs=d)

    def test_invalid_string_timestamp(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"][0]["started_at"] = "not-a-timestamp"
        self.check_reject(jobs=d)

    def test_timezone_free_timestamp(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"][0]["started_at"] = "2026-10-29T03:00:00"
        self.check_reject(jobs=d)

    def test_reversed_timestamp(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"][0]["started_at"] = "2026-10-29T03:00:02Z"
        self.check_reject(jobs=d)

    def test_github_housekeeping_steps(self):
        d = copy.deepcopy(JOBS)
        steps = d["jobs"][0]["steps"]
        setup = copy.deepcopy(steps[0]); setup["name"] = "Set up job"
        finish = copy.deepcopy(steps[0]); finish["name"] = "Complete job"
        d["jobs"][0]["steps"] = [setup, steps[0], finish]
        self.assertEqual(validate(RUN, d, SHA), [])

    def test_substituted_proof_step(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"][0]["name"] = "Checkout private source"
        self.check_reject(jobs=d)

    def test_extra_unexpected_step(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"].append(copy.deepcopy(d["jobs"][0]["steps"][0]))
        self.check_reject(jobs=d)

    def test_wrong_housekeeping_order(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][1]["steps"].insert(0, {**d["jobs"][1]["steps"][0], "name": "Complete job"})
        self.check_reject(jobs=d)

    def test_duplicate_job_ids(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][1]["id"] = d["jobs"][0]["id"]
        self.check_reject(jobs=d)

    def test_bool_runner_not_valid_id(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["runner_id"] = True
        self.check_reject(jobs=d)

    def test_bool_job_id_not_valid(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][1]["id"] = True
        self.check_reject(jobs=d)

    def test_unstructured_repository(self):
        d = copy.deepcopy(RUN)
        d["repository"] = ["not", "a", "repository", "object"]
        self.check_reject(run=d)

    def test_unstructured_step_timestamp(self):
        d = copy.deepcopy(JOBS)
        d["jobs"][0]["steps"][0]["started_at"] = 123
        self.check_reject(jobs=d)

    def test_github_housekeeping_step_failed(self):
        d = copy.deepcopy(JOBS)
        step = copy.deepcopy(d["jobs"][0]["steps"][0])
        step["name"] = "Set up job"
        step["conclusion"] = "failure"
        d["jobs"][0]["steps"].insert(0, step)
        self.check_reject(jobs=d)


if __name__ == "__main__":
    unittest.main()
