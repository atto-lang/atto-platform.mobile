#!/usr/bin/env bash
# No-Xcode regression of symbol admission logic. Does not qualify iOS binaries.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
gate="$root/scripts/verify-ios-reference-symbols.sh"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/bin" "$tmp/symbols" "$tmp/archives"
cat >"$tmp/bin/xcrun" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
[[ "$#" -eq 3 && "$1" == nm && "$2" == -gUj ]] || exit 99
if [[ "${MOCK_NM_FAIL:-0}" == 1 ]]; then exit 9; fi
archive="$3"
cat "$MOCK_SYMBOL_DIR/$(basename "$archive").txt"
MOCK
chmod +x "$tmp/bin/xcrun"
export PATH="$tmp/bin:$PATH" MOCK_SYMBOL_DIR="$tmp/symbols"
all_symbols=$(printf '%s\n' _ores_mobile_abi_version _ores_mobile_copy_message _ores_mobile_clear_message _ores_mobile_dispatch _ores_mobile_dispatch_copy)
archives=("$tmp/archives/device.a" "$tmp/archives/simulator-arm64.a" "$tmp/archives/simulator-x86_64.a")
for a in "${archives[@]}"; do
  printf dummy >"$a"
  printf '%s\n' "$all_symbols" >"$tmp/symbols/$(basename "$a").txt"
done
bash "$gate" "${archives[@]}" || { echo 'expected complete ABI success' >&2; exit 1; }
expect_failure() {
  local label="$1"; shift
  if "$@" >"$tmp/stdout" 2>"$tmp/stderr"; then
    echo "REJECTION_MISSING: $label" >&2
    exit 1
  fi
}
for index in 0 1 2; do
  archive="${archives[$index]}"
  for symbol in $all_symbols; do
    grep -Fvx -- "$symbol" <<<"$all_symbols" >"$tmp/symbols/$(basename "$archive").txt"
    expect_failure "missing $symbol in $(basename "$archive")" bash "$gate" "${archives[@]}"
    printf '%s\n' "$all_symbols" >"$tmp/symbols/$(basename "$archive").txt"
  done
done
# A similarly prefixed name cannot satisfy the exact _ores_mobile_dispatch gate.
grep -Fvx _ores_mobile_dispatch <<<"$all_symbols" >"$tmp/symbols/device.a.txt"
expect_failure 'dispatch-prefix confusion' bash "$gate" "${archives[@]}"
printf '%s\n' "$all_symbols" >"$tmp/symbols/device.a.txt"
# ABI v1 is a closed public namespace. A present-but-duplicated public
# definition, or an extra public API, must fail on every thin architecture.
for index in 0 1 2; do
  archive="${archives[$index]}"
  symbol_file="$tmp/symbols/$(basename "$archive").txt"
  printf '%s\n' "$all_symbols" _ores_mobile_dispatch >"$symbol_file"
  expect_failure "duplicate exported ABI symbol in $(basename "$archive")" bash "$gate" "${archives[@]}"
  printf '%s\n' "$all_symbols" _ores_mobile_unreviewed >"$symbol_file"
  expect_failure "unexpected ABI export in $(basename "$archive")" bash "$gate" "${archives[@]}"
  printf '%s\n' "$all_symbols" >"$symbol_file"
done
# Unrelated external symbols are allowed; only _ores_mobile_* is frozen.
printf '%s\n' "$all_symbols" _other_toolchain_helper >"$tmp/symbols/device.a.txt"
bash "$gate" "${archives[@]}" || { echo 'unrelated symbol falsely rejected' >&2; exit 1; }
printf '%s\n' "$all_symbols" >"$tmp/symbols/device.a.txt"
MOCK_NM_FAIL=1 expect_failure 'nm subprocess failure' bash "$gate" "${archives[@]}"
rm -f "${archives[2]}"
expect_failure 'missing simulator archive' bash "$gate" "${archives[@]}"
expect_failure 'wrong archive count' bash "$gate" "${archives[0]}" "${archives[1]}"
echo 'PASS: complete C ABI v1 symbol gate and 25 deterministic rejections (synthetic nm; not iOS execution)'
