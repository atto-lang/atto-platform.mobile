#!/usr/bin/env node
// A source-local, non-attesting inventory. Do not substitute for build/CI receipts.
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import {fileURLToPath, pathToFileURL} from "node:url";

const platform = "mobile";
const repository = "ores-truffle-oreslang/oreslang-platform." + platform;
const upstreams = Object.freeze(["oreslang-source.llvm","oreslang-target-profiles"]);
const targetClass = "mobile-native";
const manifestPath = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../.oreslang/integration.json");
const maxBytes = 8192;
const keys = ["evidence", "note", "protocol", "repository", "role", "schemaVersion", "upstreams"];

function readBoundedRegularFile(filename) {
  const stat = fs.lstatSync(filename, {bigint:true});
  if (!stat.isFile() || stat.nlink !== 1n || stat.size > BigInt(maxBytes))
    throw new Error("integration manifest must be a bounded, singly linked regular file");
  const fd = fs.openSync(filename, fs.constants.O_RDONLY | (fs.constants.O_NOFOLLOW || 0));
  try {
    const before = fs.fstatSync(fd, {bigint:true});
    if (!before.isFile() || before.nlink !== 1n || before.dev !== stat.dev ||
        before.ino !== stat.ino || before.size !== stat.size)
      throw new Error("integration manifest changed before read");
    const buf = Buffer.alloc(maxBytes + 1);
    let length = 0;
    while (length < buf.length) {
      const count = fs.readSync(fd, buf, length, buf.length - length, null);
      if (count === 0) break;
      length += count;
    }
    if (length > maxBytes) throw new Error("integration manifest exceeds size budget");
    const after = fs.fstatSync(fd, {bigint:true});
    if (after.size !== before.size || after.mtimeNs !== before.mtimeNs ||
        after.ino !== before.ino || BigInt(length) !== before.size)
      throw new Error("integration manifest changed during read");
    return buf.subarray(0, length);
  } finally {
    fs.closeSync(fd);
  }
}

export function buildInventory(filename = manifestPath) {
  const bytes = readBoundedRegularFile(filename);
  const raw = bytes.toString("utf8");
  const parsed = JSON.parse(raw);
  if (!parsed || Array.isArray(parsed) || typeof parsed !== "object" ||
      JSON.stringify(Object.keys(parsed).sort()) !== JSON.stringify(keys) ||
      raw !== JSON.stringify(parsed, null, 2) + "\n" ||
      parsed.schemaVersion !== 1 || parsed.protocol !== "oreslang-ecosystem-integration" ||
      parsed.repository !== repository || parsed.role !== "host" ||
      parsed.evidence !== "not-attested" ||
      typeof parsed.note !== "string" ||
      JSON.stringify(parsed.upstreams) !== JSON.stringify(upstreams))
    throw new Error("integration manifest identity/shape mismatch");
  return {
    schema: "oreslang.platform-inventory/1",
    platform, repository, targetClass,
    integrationSha256: crypto.createHash("sha256").update(bytes).digest("hex"),
    role: "host",
    upstreams: [...upstreams],
    evidence: "not-attested",
    claims: {
      compilerGuest: false,
      targetExecuted: false,
      physicalHardware: false,
      releaseAuthorized: false
    }
  };
}

if (process.argv[1] && pathToFileURL(path.resolve(process.argv[1])).href === import.meta.url) {
  try {
    if (process.argv.length !== 2) throw new Error("usage: node tools/platform-inventory.mjs");
    process.stdout.write(JSON.stringify(buildInventory(), null, 2) + "\n");
  } catch (error) {
    console.error("platform inventory: " + error.message);
    process.exitCode = 1;
  }
}
