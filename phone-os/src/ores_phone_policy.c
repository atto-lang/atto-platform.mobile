#include "ores_phone_policy.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define ORES_PHONE_MAX_APP_ID_BYTES 127u
#define ORES_PHONE_MAX_APPS 1024u
#define ORES_PHONE_MIN_UID 100000u
#define ORES_PHONE_MAX_UID 999999u
#define ORES_PHONE_MIN_MEMORY (16ull * 1024ull * 1024ull)
#define ORES_PHONE_MAX_MEMORY (4ull * 1024ull * 1024ull * 1024ull)

static int valid_app_id(const char *id) {
    size_t i = 0;
    unsigned segment_length = 0;
    unsigned segments = 1;
    if (id == NULL) return 0;
    for (; i <= ORES_PHONE_MAX_APP_ID_BYTES && id[i] != '\0'; ++i) {
        char ch = id[i];
        if (ch == '.') {
            if (segment_length == 0 || id[i - 1] == '-') return 0;
            segment_length = 0;
            ++segments;
            continue;
        }
        if (segment_length == 0) {
            if (ch < 'a' || ch > 'z') return 0;
        } else if (!((ch >= 'a' && ch <= 'z') ||
                     (ch >= '0' && ch <= '9') || ch == '-')) {
            return 0;
        }
        ++segment_length;
    }
    return i <= ORES_PHONE_MAX_APP_ID_BYTES && segments >= 2 &&
           segment_length != 0 && id[i - 1] != '-';
}

static int matches_managed_path(const char *actual, const char *prefix,
                                const char *id, const char *suffix) {
    char expected[512];
    int written;
    if (actual == NULL) return 0;
    written = snprintf(expected, sizeof expected, "%s%s%s", prefix, id, suffix);
    return written >= 0 && (size_t)written < sizeof expected &&
           strcmp(expected, actual) == 0;
}

ores_phone_policy_status ores_phone_policy_validate(const ores_phone_app_policy *p) {
    if (p == NULL) return ORES_PHONE_POLICY_INVALID_ARGUMENT;
    if (!valid_app_id(p->app_id)) return ORES_PHONE_POLICY_INVALID_APP_ID;
    if (p->uid < ORES_PHONE_MIN_UID || p->uid > ORES_PHONE_MAX_UID)
        return ORES_PHONE_POLICY_UNSAFE_UID;
    if (!matches_managed_path(p->executable_path, "/opt/ores-phone/apps/",
                              p->app_id, "/bin/app") ||
        !matches_managed_path(p->private_data_path, "/var/lib/ores-phone/apps/",
                              p->app_id, ""))
        return ORES_PHONE_POLICY_INVALID_PATH;
    if (p->memory_max_bytes < ORES_PHONE_MIN_MEMORY ||
        p->memory_max_bytes > ORES_PHONE_MAX_MEMORY ||
        p->cpu_period_us < 10000u || p->cpu_period_us > 1000000u ||
        p->cpu_quota_us < 1000u || p->cpu_quota_us > p->cpu_period_us)
        return ORES_PHONE_POLICY_INVALID_BUDGET;
    if (((p->requested_capabilities | p->granted_capabilities) &
         ~ORES_PHONE_ALL_CAPABILITIES) != 0u ||
        (p->requested_capabilities & ~p->granted_capabilities) != 0u)
        return ORES_PHONE_POLICY_DENIED_CAPABILITY;
    return ORES_PHONE_POLICY_OK;
}

ores_phone_policy_status ores_phone_policy_validate_set(
    const ores_phone_app_policy *policies, size_t count) {
    size_t i, j;
    ores_phone_policy_status status;
    if (policies == NULL || count == 0 || count > ORES_PHONE_MAX_APPS)
        return ORES_PHONE_POLICY_INVALID_ARGUMENT;
    for (i = 0; i < count; ++i) {
        status = ores_phone_policy_validate(&policies[i]);
        if (status != ORES_PHONE_POLICY_OK) return status;
        for (j = 0; j < i; ++j) {
            if (policies[i].uid == policies[j].uid ||
                strcmp(policies[i].app_id, policies[j].app_id) == 0 ||
                strcmp(policies[i].private_data_path, policies[j].private_data_path) == 0 ||
                strcmp(policies[i].executable_path, policies[j].executable_path) == 0)
                return ORES_PHONE_POLICY_DUPLICATE_SECURITY_DOMAIN;
        }
    }
    return ORES_PHONE_POLICY_OK;
}

const char *ores_phone_policy_status_name(ores_phone_policy_status status) {
    switch (status) {
        case ORES_PHONE_POLICY_OK: return "ok";
        case ORES_PHONE_POLICY_INVALID_ARGUMENT: return "invalid_argument";
        case ORES_PHONE_POLICY_INVALID_APP_ID: return "invalid_app_id";
        case ORES_PHONE_POLICY_UNSAFE_UID: return "unsafe_uid";
        case ORES_PHONE_POLICY_INVALID_PATH: return "invalid_path";
        case ORES_PHONE_POLICY_INVALID_BUDGET: return "invalid_budget";
        case ORES_PHONE_POLICY_DENIED_CAPABILITY: return "denied_capability";
        case ORES_PHONE_POLICY_DUPLICATE_SECURITY_DOMAIN: return "duplicate_security_domain";
    }
    return "unknown";
}
