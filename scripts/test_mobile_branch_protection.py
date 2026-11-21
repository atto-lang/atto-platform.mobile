#!/usr/bin/env python3
"""Synthetic, offline negative regression matrix for mobile branch protection."""
import copy
import unittest
from check_mobile_branch_protection import validate, REQUIRED_CHECKS, GITHUB_ACTIONS_APP_ID

SHA = "a" * 40
REPO = {"id": 1448719611, "full_name": "atto-lang/atto-platform.mobile",
        "private": True, "visibility": "private", "archived": False, "disabled": False}
BRANCH = {"name": "main", "protected": True, "commit": {"sha": SHA}}
PROTECTION = {
    "required_status_checks": {"strict": True, "checks": [
        {"context": name, "app_id": GITHUB_ACTIONS_APP_ID} for name in sorted(REQUIRED_CHECKS)
    ]},
    "required_pull_request_reviews": {
        "required_approving_review_count": 1, "require_code_owner_reviews": True,
        "dismiss_stale_reviews": True,
        "bypass_pull_request_allowances": {"users": [], "teams": [], "apps": []}
    },
    "enforce_admins": {"enabled": True},
    "allow_force_pushes": {"enabled": False},
    "allow_deletions": {"enabled": False}
}


class ProtectionAuditTests(unittest.TestCase):
    def assert_reject(self, mut):
        repo, branch, protection = copy.deepcopy((REPO, BRANCH, PROTECTION))
        mut(repo, branch, protection)
        self.assertTrue(validate(repo, branch, protection, SHA))

    def test_valid_snapshot(self):
        self.assertEqual(validate(REPO, BRANCH, PROTECTION, SHA), [])

    def test_wrong_repo_id(self):
        self.assert_reject(lambda r,b,p: r.update(id=1234))

    def test_boolean_repo_id(self):
        self.assert_reject(lambda r,b,p: r.update(id=True))

    def test_wrong_repo_name(self):
        self.assert_reject(lambda r,b,p: r.update(full_name="attacker/private"))

    def test_visibility_flip(self):
        self.assert_reject(lambda r,b,p: r.update(visibility="public"))

    def test_private_flag_false(self):
        self.assert_reject(lambda r,b,p: r.update(private=False))

    def test_archived(self):
        self.assert_reject(lambda r,b,p: r.update(archived=True))

    def test_disabled_missing(self):
        self.assert_reject(lambda r,b,p: r.pop("disabled"))

    def test_wrong_branch(self):
        self.assert_reject(lambda r,b,p: b.update(name="feature"))

    def test_branch_not_protected(self):
        self.assert_reject(lambda r,b,p: b.update(protected=False))

    def test_wrong_sha(self):
        self.assert_reject(lambda r,b,p: b["commit"].update(sha="b"*40))

    def test_missing_commit(self):
        self.assert_reject(lambda r,b,p: b.pop("commit"))

    def test_missing_required_checks(self):
        self.assert_reject(lambda r,b,p: p.pop("required_status_checks"))

    def test_non_strict_checks(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"].update(strict=False))

    def test_missing_one_check(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"].pop())

    def test_unsigned_check_app(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"][0].update(app_id=-1))

    def test_boolean_app_id(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"][0].update(app_id=True))

    def test_fake_check_context(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"][0].update(context="green"))

    def test_malformed_status_check_context(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"][0].update(context=[]))

    def test_duplicate_check(self):
        self.assert_reject(lambda r,b,p: p["required_status_checks"]["checks"].append(p["required_status_checks"]["checks"][0]))

    def test_reviews_missing(self):
        self.assert_reject(lambda r,b,p: p.pop("required_pull_request_reviews"))

    def test_zero_reviews(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"].update(required_approving_review_count=0))

    def test_boolean_review_count(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"].update(required_approving_review_count=True))

    def test_no_codeowner(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"].update(require_code_owner_reviews=False))

    def test_no_stale_dismissal(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"].update(dismiss_stale_reviews=False))

    def test_bypass_missing(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"].pop("bypass_pull_request_allowances"))

    def test_user_bypass(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"]["bypass_pull_request_allowances"]["users"].append({"login":"someuser"}))

    def test_team_bypass(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"]["bypass_pull_request_allowances"]["teams"].append({"slug":"some-team"}))

    def test_app_bypass(self):
        self.assert_reject(lambda r,b,p: p["required_pull_request_reviews"]["bypass_pull_request_allowances"]["apps"].append({"slug":"some-app"}))

    def test_admin_bypass(self):
        self.assert_reject(lambda r,b,p: p["enforce_admins"].update(enabled=False))

    def test_force_push(self):
        self.assert_reject(lambda r,b,p: p["allow_force_pushes"].update(enabled=True))

    def test_force_push_field_missing(self):
        self.assert_reject(lambda r,b,p: p.pop("allow_force_pushes"))

    def test_branch_deletion(self):
        self.assert_reject(lambda r,b,p: p["allow_deletions"].update(enabled=True))

    def test_branch_deletion_field_missing(self):
        self.assert_reject(lambda r,b,p: p.pop("allow_deletions"))

    def test_malformed_repo_object(self):
        self.assertTrue(validate([], BRANCH, PROTECTION, SHA))

    def test_malformed_protection_object(self):
        self.assertTrue(validate(REPO, BRANCH, [], SHA))


if __name__ == "__main__":
    unittest.main()
