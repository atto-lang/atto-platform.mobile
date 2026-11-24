// Shared, source-only parity admission: not a compiler, backend or target execution receipt.
import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import {lstatSync, readFileSync} from 'node:fs';

const expectedDigest = 'a5cc72dcbc58f51f007ba956f6892b6485a9316d3ffa4410e157c95d7f3fe68a';
const root = new URL('../', import.meta.url);
const fixture = new URL('./test_platform_portable_core_v1.ores', import.meta.url);
const member = new URL('../.oreslang/integration.json', import.meta.url);
const note = 'Declared integration touchpoints only; no CI or runtime parity is asserted.';
const upstreamsByRepo = Object.freeze({
  'ores-truffle-oreslang/oreslang-platform.mobile': ['oreslang-source.llvm', 'oreslang-target-profiles'],
  'ores-truffle-oreslang/oreslang-platform.iot': ['oreslang-platform.embedded', 'oreslang-source.llvm'],
  'ores-truffle-oreslang/oreslang-platform.asic': ['oreslang-source.hdl'],
  'ores-truffle-oreslang/oreslang-platform.fpga': ['oreslang-source.hdl'],
  'ores-truffle-oreslang/oreslang-platform.desktop': ['oreslang-source.llvm', 'oreslang-target-profiles'],
  'ores-truffle-oreslang/oreslang-platform.embedded': ['oreslang-source.llvm', 'oreslang-target-profiles'],
  'ores-truffle-oreslang/oreslang-platform-conformance': ['oreslang-common-source', 'oreslang-target-profiles']
});

function readRegular(path, limit) {
  const st = lstatSync(path);
  assert.ok(st.isFile() && !st.isSymbolicLink() && st.nlink === 1, 'regular single-linked source required');
  assert.ok(st.size > 0 && st.size <= limit, 'bounded nonempty source required');
  return readFileSync(path);
}
function validateMember(bytes, expectedRepo) {
  assert.ok(Buffer.isBuffer(bytes) && bytes.length > 0 && bytes.length <= 65536, 'bounded manifest bytes');
  const raw = new TextDecoder('utf-8', {fatal:true, ignoreBOM:true}).decode(bytes);
  assert.ok(!raw.startsWith('\uFEFF'), 'BOM forbidden');
  const value = JSON.parse(raw);
  assert.equal(raw, JSON.stringify(value, null, 2) + '\n', 'noncanonical JSON or duplicate keys');
  assert.ok(Object.hasOwn(upstreamsByRepo, expectedRepo), 'unknown repository');
  assert.deepEqual(value, {
    schemaVersion:1,
    protocol:'oreslang-ecosystem-integration',
    repository:expectedRepo,
    role:expectedRepo.endsWith('-conformance') ? 'conformance' : 'host',
    upstreams:upstreamsByRepo[expectedRepo],
    evidence:'not-attested',
    note
  }, 'mismatched identity/upstreams or unauthorized promotion');
  return value;
}
function validateFixture(bytes) {
  assert.ok(Buffer.isBuffer(bytes) && bytes.length > 0 && bytes.length <= 65536, 'bounded fixture bytes');
  assert.equal(createHash('sha256').update(bytes).digest('hex'), expectedDigest, 'portable fixture drift');
}

test('portable .ores corpus is pinned to the same SHA-256 in all seven platform repositories', () => {
  validateFixture(readRegular(fixture,65536));
});

test('platform integration manifest remains unpromoted and matches canonical dependency edges', () => {
  const bytes = readRegular(member,65536);
  const value = JSON.parse(bytes.toString('utf8'));
  validateMember(bytes,value.repository);
  if (process.env.GITHUB_REPOSITORY) assert.equal(value.repository,process.env.GITHUB_REPOSITORY);
  assert.ok(root.href.startsWith('file:'), 'local files only');
});

test('mutation controls deny altered source, missing platforms, wrong edges and fake execution', () => {
  const source = readRegular(fixture,65536);
  const wrong = Buffer.from(source);
  wrong[0] ^= 1;
  assert.throws(() => validateFixture(wrong), /fixture drift/);
  const repo = 'ores-truffle-oreslang/oreslang-platform.embedded';
  const expected = {schemaVersion:1,protocol:'oreslang-ecosystem-integration',repository:repo,
    role:'host',upstreams:upstreamsByRepo[repo],evidence:'not-attested',note};
  const encoded = x => Buffer.from(JSON.stringify(x,null,2)+'\n');
  validateMember(encoded(expected),repo);
  for (const bad of [
    {...expected, evidence:'target-execution'},
    {...expected, guestExecuted:true},
    {...expected, upstreams:['oreslang-source.java']},
    {...expected, repository:'ores-truffle-oreslang/oreslang-platform.mobile'}
  ]) assert.throws(() => validateMember(encoded(bad),repo));
  assert.throws(() => validateMember(Buffer.from('{"schemaVersion":1,"schemaVersion":2}\n'),repo));
  assert.throws(() => validateMember(Buffer.concat([Buffer.from([0xef,0xbb,0xbf]),encoded(expected)]),repo));
  assert.throws(() => validateMember(Buffer.from([0xc3,0x28]),repo));
});
