import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import crypto from "node:crypto";
import {fileURLToPath} from "node:url";
import {buildInventory} from "../tools/platform-inventory.mjs";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const manifest = fs.readFileSync(path.join(root, ".oreslang/integration.json"), "utf8");
function candidate(content, run) {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), "ores-platform-inventory-"));
  try {
    const file = path.join(dir, "integration.json");
    fs.writeFileSync(file, content);
    return run(file, dir);
  } finally {
    fs.rmSync(dir, {recursive:true, force:true});
  }
}

test("source manifest exports a non-attesting, exact-byte inventory", () => {
  const inventory = buildInventory();
  assert.equal(inventory.schema, "oreslang.platform-inventory/1");
  assert.equal(inventory.platform, "mobile");
  assert.equal(inventory.repository, "ores-truffle-oreslang/oreslang-platform.mobile");
  assert.equal(inventory.role, "host");
  assert.equal(inventory.evidence, "not-attested");
  assert.deepEqual(Object.values(inventory.claims), [false, false, false, false]);
  assert.match(inventory.integrationSha256, /^[0-9a-f]{64}$/);
  assert.equal(inventory.integrationSha256,
    crypto.createHash("sha256").update(manifest).digest("hex"));
});

test("rejects forged evidence, upstreams, repository and extra keys", () => {
  for (const source of [
    manifest.replace('"evidence": "not-attested"', '"evidence": "verified"'),
    manifest.replace('"repository": "ores-truffle-oreslang/', '"repository": "other/'),
    manifest.replace('"upstreams": [', '"upstreams": ["unknown-upstream",'),
    manifest.replace('"role": "host"', '"role": "guest"'),
    manifest.replace('"schemaVersion": 1,', '"schemaVersion": 1, "promoted": true,')
  ]) {
    assert.notEqual(source, manifest);
    candidate(source, file => assert.throws(() => buildInventory(file)));
  }
});

test("rejects duplicate JSON keys, noncanonical encoding and size overflow", () => {
  const duplicate = manifest.replace('"role": "host",', '"role": "host",\n  "role": "host",');
  assert.notEqual(duplicate, manifest);
  for (const source of [duplicate, manifest.replace(/\n/g, "\r\n"), manifest + " ", " ".repeat(8193)])
    candidate(source, file => assert.throws(() => buildInventory(file)));
});

test("rejects filesystem aliases of the manifest", () => {
  candidate(manifest, (file, dir) => {
    const symlink = path.join(dir, "symlink.json");
    fs.symlinkSync(file, symlink);
    assert.throws(() => buildInventory(symlink));
    const hardlink = path.join(dir, "hardlink.json");
    fs.linkSync(file, hardlink);
    assert.throws(() => buildInventory(file));
    assert.throws(() => buildInventory(hardlink));
  });
});
