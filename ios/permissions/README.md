# iOS permissions and entitlements

Separate signed entitlements, current OS authorization and runtime actor capability. Re-evaluate them immediately before side effects; do not cache permission answers across prompts/async hops or grant capabilities from guest-provided fields. Revocation invalidates pending work.
