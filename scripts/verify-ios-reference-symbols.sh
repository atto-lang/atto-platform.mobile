#!/usr/bin/env bash
# Fail closed when any required C ABI v1 symbol is absent from a thin iOS archive.
# xcrun nm -gUj lists only defined, externally visible Mach-O symbol names.
# The Oreslang ABI v1 public symbol set is CLOSED: no duplicates or extra exports.
set -euo pipefail
if [[ "$#" -ne 3 ]]; then
  echo 'expected device arm64, simulator arm64 and simulator x86_64 archives' >&2
  exit 2
fi
required=(
  _ores_mobile_abi_version
  _ores_mobile_copy_message
  _ores_mobile_clear_message
  _ores_mobile_handoff_message
  _ores_mobile_dispatch
  _ores_mobile_dispatch_copy
)
expected_set="$(printf '%s\n' "${required[@]}" | LC_ALL=C sort)"
for archive in "$@"; do
  if [[ ! -s "$archive" ]]; then
    echo "ABI_ARCHIVE_MISSING: $archive" >&2
    exit 2
  fi
  if ! symbols="$(xcrun nm -gUj "$archive")"; then
    echo "ABI_NM_FAILED: $archive" >&2
    exit 2
  fi
  for symbol in "${required[@]}"; do
    if ! grep -Fqx -- "$symbol" <<<"$symbols"; then
      echo "ABI_SYMBOL_MISSING: $symbol in $archive" >&2
      exit 2
    fi
  done
  # Requiring presence alone permits duplicate definitions and unreviewed
  # _ores_mobile_* exports. Compare the entire sorted ABI namespace including
  # duplicate occurrences. Other system/toolchain symbols may be present.
  actual_set="$(awk 'index($0, "_ores_mobile_") == 1' <<<"$symbols" | LC_ALL=C sort)"
  if [[ "$actual_set" != "$expected_set" ]]; then
    echo "ABI_PUBLIC_SYMBOL_SET_MISMATCH: $archive" >&2
    exit 2
  fi
done
