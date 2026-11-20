#ifndef ORES_PHONE_POLICY_H
#define ORES_PHONE_POLICY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Only trusted system code may create an ores_phone_app_policy.  No value in
 * this structure is evidence that a kernel sandbox was installed. */
enum ores_phone_capability {
    ORES_PHONE_CAP_NETWORK = 1u << 0,
    ORES_PHONE_CAP_CAMERA = 1u << 1,
    ORES_PHONE_CAP_MICROPHONE = 1u << 2,
    ORES_PHONE_CAP_LOCATION = 1u << 3
};
#define ORES_PHONE_ALL_CAPABILITIES ((uint32_t)0x0fu)

typedef struct ores_phone_app_policy {
    const char *app_id;
    uint32_t uid;
    const char *executable_path;
    const char *private_data_path;
    uint64_t memory_max_bytes;
    uint32_t cpu_quota_us;
    uint32_t cpu_period_us;
    uint32_t requested_capabilities;
    uint32_t granted_capabilities;
} ores_phone_app_policy;

typedef enum ores_phone_policy_status {
    ORES_PHONE_POLICY_OK = 0,
    ORES_PHONE_POLICY_INVALID_ARGUMENT = 1,
    ORES_PHONE_POLICY_INVALID_APP_ID = 2,
    ORES_PHONE_POLICY_UNSAFE_UID = 3,
    ORES_PHONE_POLICY_INVALID_PATH = 4,
    ORES_PHONE_POLICY_INVALID_BUDGET = 5,
    ORES_PHONE_POLICY_DENIED_CAPABILITY = 6,
    ORES_PHONE_POLICY_DUPLICATE_SECURITY_DOMAIN = 7
} ores_phone_policy_status;

/* Lexical admission only.  Before exec the privileged launcher MUST securely
 * open/verify binaries and directory roots, create real kernel restrictions,
 * and verify their application.  No process launch occurs here. */
ores_phone_policy_status ores_phone_policy_validate(const ores_phone_app_policy *policy);
ores_phone_policy_status ores_phone_policy_validate_set(
    const ores_phone_app_policy *policies, size_t count);
const char *ores_phone_policy_status_name(ores_phone_policy_status status);

#ifdef __cplusplus
}
#endif
#endif
