#!/usr/bin/env bash
# Fail closed when any required C ABI v1 symbol is absent from a thin iOS archive.
# xcrun nm -gUj lists only defined, externally visible Mach-O symbol names.
set -euo pipefail
if [[ "$#" -ne 3 ]]; then
  echo 'expected device arm64, simulator arm64 and simulator x86_64 archives' >&2
  exit 2
fi
for archive in "$@"; do
  if [[ ! -s "$archive" ]]; then
    echo "ABI_ARCHIVE_MISSING: $archive" >&2
    exit 2
  fi
  if ! symbols="$(xcrun nm -gUj "$archive")"; then
    echo "ABI_NM_FAILED: $archive" >&2
    exit 2
  fi
  for symbol in _ores_mobile_abi_version _ores_mobile_copy_message _ores_mobile_clear_message _ores_mobile_dispatch _ores_mobile_dispatch_copy; do
    if ! grep -Fqx -- "$symbol" <<<"$symbols"; then
      echo "ABI_SYMBOL_MISSING: $symbol in $archive" >&2
      exit 2
    fi
  done
done
