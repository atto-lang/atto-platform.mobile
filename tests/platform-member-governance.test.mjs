// Source-free per-member guard. Not a compiler, guest, CI or hardware attestation.
import test from "node:test";
import assert from "node:assert/strict";
import {readFileSync, lstatSync} from "node:fs";

const expected = {
  "schemaVersion": 1,
  "protocol": "oreslang-ecosystem-integration",
  "repository": "ores-truffle-oreslang/oreslang-platform.mobile",
  "role": "host",
  "upstreams": [
    "oreslang-source.llvm",
    "oreslang-target-profiles"
  ],
  "evidence": "not-attested",
  "note": "Declared integration touchpoints only; no CI or runtime parity is asserted."
};
const manifest = new URL("../.oreslang/integration.json", import.meta.url);

function admit(bytes) {
  assert.ok(Buffer.isBuffer(bytes), "input must be bytes");
  assert.ok(bytes.length > 0 && bytes.length <= 65536, "bounded manifest size");
  const raw = new TextDecoder("utf-8", {fatal:true, ignoreBOM:true}).decode(bytes);
  assert.ok(!raw.startsWith("\uFEFF"), "BOM disallowed");
  const data = JSON.parse(raw);
  // Canonical byte equality rejects duplicate authority keys, including
  // Unicode-escaped aliases that JSON.parse would otherwise last-win.
  assert.equal(raw, JSON.stringify(data, null, 2) + "\n", "noncanonical or duplicate keys");
  assert.deepEqual(data, expected, "identity, typed upstreams and anti-attestation must match");
  return {verified:false, parity:false, execution:false, evidence:"metadata-only"};
}

test("committed member manifest is a regular, single-linked canonical file", () => {
  const stat = lstatSync(manifest);
  assert.ok(stat.isFile() && !stat.isSymbolicLink() && stat.nlink === 1, "unsafe manifest file type or hard link");
  assert.deepEqual(admit(readFileSync(manifest)), {
    verified:false, parity:false, execution:false, evidence:"metadata-only"
  });
});

test("reject missing, substituted and duplicate authority fields", () => {
  const altered = structuredClone(expected);
  delete altered.repository;
  assert.throws(() => admit(Buffer.from(JSON.stringify(altered,null,2)+"\n")));
  altered.repository = "attacker/substituted-repo";
  assert.throws(() => admit(Buffer.from(JSON.stringify(altered,null,2)+"\n")));
  assert.throws(() => admit(Buffer.from(JSON.stringify(expected,null,2).replace('"role": "host",','"role": "host",\n  "role": "compiler",')+"\n")));
  assert.throws(() => admit(Buffer.from(JSON.stringify(expected,null,2).replace('"role": "host",','"\\u0072ole": "compiler",\n  "role": "host",')+"\n")));
  assert.throws(() => admit(Buffer.from(JSON.stringify({...expected, attested:true},null,2)+"\n")));
});

test("reject upstream edge substitution, missing edges and duplicate edges", () => {
  for (const candidate of [[],["oreslang-source.js"],expected.upstreams.concat(expected.upstreams[0]),[...expected.upstreams].reverse()]) {
    if (JSON.stringify(candidate) === JSON.stringify(expected.upstreams)) continue;
    assert.throws(() => admit(Buffer.from(JSON.stringify({...expected,upstreams:candidate},null,2)+"\n")));
  }
});

test("reject false release and hardware/guest execution promotion", () => {
  for (const evidence of ["verified","passing","compiler-guest","physical-hardware",true,null]) {
    assert.throws(() => admit(Buffer.from(JSON.stringify({...expected,evidence},null,2)+"\n")));
  }
  assert.throws(() => admit(Buffer.from(JSON.stringify({...expected,role:"compiler"},null,2)+"\n")));
  assert.throws(() => admit(Buffer.from(JSON.stringify({...expected,note:"all tests passed"},null,2)+"\n")));
});

test("reject invalid encodings, size and noncanonical bytes", () => {
  const valid = Buffer.from(JSON.stringify(expected,null,2)+"\n");
  assert.throws(() => admit(Buffer.concat([Buffer.from([0xEF,0xBB,0xBF]),valid])));
  assert.throws(() => admit(Buffer.from([0xC3,0x28])));
  assert.throws(() => admit(Buffer.alloc(65537,0x20)));
  assert.throws(() => admit(Buffer.concat([valid,Buffer.from(" ")])));
  assert.throws(() => admit(Buffer.from(JSON.stringify(expected))));
});
