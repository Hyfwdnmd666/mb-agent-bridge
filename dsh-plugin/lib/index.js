// dsh-mount-and-blade — Mount & Blade 1.011 as DSH model tools.
//
// Every tool here shells out to mb_agent.py, which owns the behaviour. That split is
// deliberate: the hard-won details (input transport, coordinate scaling, window stacking,
// save format) belong in Python next to the bridge, and this file stays thin enough to read.
//
// Three things this plugin gets right that an obvious implementation gets wrong, all of them
// learned by trying to play the game and failing:
//
//   * Input goes through a physical USB HID device, not SendInput. The engine reads pointer
//     state through DirectInput, which ignores synthetic events - proven twice, with
//     byte-identical frames before and after SendInput and PostMessage clicks.
//   * Coordinates are render-space, not client-space. The engine renders 1280x800 into an
//     853x533 client and scales pointer coordinates by 1.5006, so a raw client coordinate
//     lands about 50% too low.
//   * The host window is moved aside before every interaction. It used to sit on top of the
//     game, which made window-DC screenshots show the host and made clicks get eaten by it.
//
// IMPORTANT: child processes are spawned with FILE-backed stdio, never pipes. Under the DSH
// sandbox a child that opens an inherited named pipe fails with EPERM, so execFile() with
// default options cannot be used at all.

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { defineTool } from '@deepseek-ai/dsh-tools';

export const name = 'dsh-mount-and-blade';
export const inject = ['tools'];

// Both defaults are resolved relative to this file, so a clone works anywhere. Override with
// config.python / config.agent, or the MB_PYTHON / MB_AGENT environment variables.
const HERE = path.dirname(new URL(import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, '$1'));
const REPO_ROOT = path.resolve(HERE, '..', '..');

const DEFAULT_PYTHON = process.env.MB_PYTHON || 'python';
const DEFAULT_AGENT = process.env.MB_AGENT || path.join(REPO_ROOT, 'mb_agent.py');

const WORK = path.join(os.tmpdir(), 'dsh-mb-bridge');

function configOf(config = {}) {
  return {
    python: config.python || process.env.MB_PYTHON || DEFAULT_PYTHON,
    agent: config.agent || process.env.MB_AGENT || DEFAULT_AGENT,
    shots: config.shots || path.join(WORK, 'shots'),
  };
}

let seq = 0;

/** Run mb_agent.py and return { code, stdout, stderr }. Captured via temp files, not pipes. */
function runAgent(cfg, args, timeoutMs = 60000) {
  return new Promise((resolve) => {
    fs.mkdirSync(WORK, { recursive: true });
    const id = `${process.pid}-${Date.now()}-${seq++}`;
    const outPath = path.join(WORK, `${id}.out`);
    const errPath = path.join(WORK, `${id}.err`);

    let outFd;
    let errFd;
    try {
      outFd = fs.openSync(outPath, 'w');
      errFd = fs.openSync(errPath, 'w');
    } catch (error) {
      resolve({ code: -1, stdout: '', stderr: `cannot open capture files: ${error?.message ?? error}` });
      return;
    }

    let child;
    try {
      child = spawn(cfg.python, [cfg.agent, ...args], {
        stdio: ['ignore', outFd, errFd],
        windowsHide: true,
      });
    } catch (error) {
      try { fs.closeSync(outFd); } catch {}
      try { fs.closeSync(errFd); } catch {}
      resolve({ code: -1, stdout: '', stderr: `spawn failed: ${error?.message ?? error}` });
      return;
    }

    const finish = (code, extra = '') => {
      try { fs.closeSync(outFd); } catch {}
      try { fs.closeSync(errFd); } catch {}
      let stdout = '';
      let stderr = '';
      try { stdout = fs.readFileSync(outPath, 'utf8'); } catch {}
      try { stderr = fs.readFileSync(errPath, 'utf8'); } catch {}
      for (const p of [outPath, errPath]) { try { fs.unlinkSync(p); } catch {} }
      resolve({ code, stdout: stdout.trim(), stderr: (stderr + extra).trim() });
    };

    const timer = setTimeout(() => {
      try { child.kill(); } catch {}
      finish(-1, `\n[timed out after ${timeoutMs} ms]`);
    }, timeoutMs);

    child.on('error', (error) => { clearTimeout(timer); finish(-1, `\n${error?.message ?? error}`); });
    child.on('close', (code) => { clearTimeout(timer); finish(code ?? -1); });
  });
}

/** Parse the single JSON object mb_agent.py prints. */
function parseJson(result, what) {
  const line = (result.stdout || '').split('\n').filter(Boolean).pop() || '';
  try {
    return JSON.parse(line);
  } catch {
    const detail = (result.stderr || result.stdout || 'no output').slice(0, 400);
    throw new Error(`${what}: ${detail}`);
  }
}

function textBlock(text) {
  return [{ type: 'text', text }];
}

const objectSchema = (properties) => ({ type: 'object', properties, additionalProperties: false });

export function apply(ctx, config = {}) {
  const cfg = configOf(config);
  const tools = ctx.get('tools');
  if (tools === undefined) {
    console.log('[dsh-mount-and-blade] tools service unavailable; plugin idle');
    return;
  }

  // ---- mb_state ---------------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_state',
    description:
      'Report whether Mount & Blade is running, its pid, the client rectangle, the render scale ' +
      'clicks use, and the player gold if a campaign is loaded. Call this first.',
    parameters: {},
    output: {
      schema: objectSchema({
        running: { type: 'boolean' },
        pid: { type: 'integer' },
        width: { type: 'integer' },
        height: { type: 'integer' },
        render_scale: { type: 'number' },
        gold: { type: 'integer' },
        note: { type: 'string' },
      }),
      render: (args, v) => textBlock(v.running
        ? `Mount & Blade pid=${v.pid} client=${v.width}x${v.height} scale=${v.render_scale} ` +
          `gold=${v.gold === null ? 'n/a (not in a campaign)' : v.gold}`
        : `Mount & Blade is not running. ${v.note}`),
    },
    async execute() {
      const r = await runAgent(cfg, ['state'], 30000);
      const d = parseJson(r, 'state');
      if (!d.ok) {
        return { running: false, pid: 0, width: 0, height: 0, render_scale: 0, gold: -1,
                 note: String(d.error ?? 'window not found').slice(0, 300) };
      }
      return {
        running: true,
        pid: d.pid ?? 0,
        width: d.client?.w ?? 0,
        height: d.client?.h ?? 0,
        render_scale: d.render_scale ?? 0,
        gold: typeof d.gold === 'number' ? d.gold : -1,
        note: d.gold_error ? 'no campaign loaded' : '',
      };
    },
  }));

  // ---- mb_look ----------------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_look',
    description:
      'Capture the Mount & Blade client area to a PNG from the composited screen and return its ' +
      'path. Read that path as an image to see the game. The image is the client area, so a pixel ' +
      'in it maps to an mb_click render coordinate by multiplying by 1280/width.',
    parameters: {
      label: { type: 'string', description: 'Optional short label used in the filename.' },
      scale: { type: 'integer', description: 'Integer downscale for latency. Default 2.' },
    },
    output: {
      schema: objectSchema({
        path: { type: 'string' },
        width: { type: 'integer' },
        height: { type: 'integer' },
      }),
      render: (args, v) => textBlock(`Captured client ${v.width}x${v.height} -> ${v.path}`),
    },
    async execute(args) {
      fs.mkdirSync(cfg.shots, { recursive: true });
      const safe = String(args?.label ?? 'look').replace(/[^A-Za-z0-9_-]/g, '').slice(0, 24) || 'look';
      const file = path.join(cfg.shots, `${safe}-${Date.now()}.png`);
      const scale = Number.isFinite(args?.scale) ? args.scale : 2;
      const r = await runAgent(cfg, ['look', '--out', file, '--scale', String(scale)], 40000);
      const d = parseJson(r, 'look');
      if (!d.ok) throw new Error(String(d.error ?? 'screenshot failed'));
      return { path: d.path, width: d.client?.w ?? 0, height: d.client?.h ?? 0 };
    },
  }));

  // ---- mb_click ---------------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_click',
    description:
      'Click inside Mount & Blade using a physical USB HID mouse, which is the only input the ' +
      'engine accepts. Coordinates are RENDER-space: take a pixel from an mb_look image and ' +
      'multiply by 1280/width. (The server converts to screen coordinates itself; passing raw ' +
      'client pixels lands roughly 50% too low.)',
    parameters: {
      x: { type: 'integer', description: 'Render-space X = image_x * (1280 / image_width).', required: true },
      y: { type: 'integer', description: 'Render-space Y = image_y * (1280 / image_width).', required: true },
      button: { type: 'string', description: 'left (default) or right.' },
    },
    output: {
      schema: objectSchema({
        x: { type: 'integer' }, y: { type: 'integer' },
        client_x: { type: 'integer' }, client_y: { type: 'integer' },
      }),
      render: (args, v) => textBlock(
        `Clicked render (${v.x},${v.y}) -> client (${v.client_x},${v.client_y})`),
    },
    async execute(args) {
      const button = args?.button === 'right' ? 'right' : 'left';
      const r = await runAgent(cfg, ['click', String(args.x), String(args.y), '--button', button], 40000);
      const d = parseJson(r, 'click');
      if (!d.ok) throw new Error(String(d.error ?? 'click failed'));
      return { x: args.x, y: args.y, client_x: d.client_xy?.[0] ?? 0, client_y: d.client_xy?.[1] ?? 0 };
    },
  }));

  // ---- mb_key -----------------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_key',
    description:
      'Tap one key through the HID keyboard. Useful values: 0x1B ESC (does not dismiss every ' +
      'screen in this game), 0x0D Enter, 0x20 Space, 0x09 Tab (leave/back in city and menu ' +
      'views), arrows 0x25-0x28, W/A/S/D 0x57/0x41/0x53/0x44.',
    parameters: {
      vk: { type: 'integer', description: 'Virtual-key code, e.g. 27 for ESC.', required: true },
    },
    output: {
      schema: objectSchema({ vk: { type: 'integer' } }),
      render: (args, v) => textBlock(`Sent vk=0x${(v.vk >>> 0).toString(16).toUpperCase()}`),
    },
    async execute(args) {
      const r = await runAgent(cfg, ['key', String(args.vk)], 30000);
      const d = parseJson(r, 'key');
      if (!d.ok) throw new Error(String(d.error ?? 'key failed'));
      return { vk: args.vk };
    },
  }));

  // ---- mb_save_state ----------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_save_state',
    description:
      'Read structured campaign state straight out of the save file - no running game, no input, ' +
      'no focus needed. Returns the player name, game version, and every party on the map with ' +
      'its position, facing, troop stacks and total men. This is the reliable way to observe the ' +
      'world; prefer it over reading pixels.',
    parameters: {
      save: { type: 'string', description: 'Optional path to a .sav. Defaults to the Native slot 0.' },
    },
    output: {
      schema: objectSchema({
        player: { type: 'string' },
        version: { type: 'integer' },
        party_count: { type: 'integer' },
        json: { type: 'string' },
      }),
      render: (args, v) => textBlock(
        `Save: player=${v.player} version=${v.version} parties=${v.party_count}\n${v.json}`),
    },
    async execute(args) {
      const argv = ['save-state'];
      if (args?.save) argv.push('--save', String(args.save));
      const r = await runAgent(cfg, argv, 60000);
      const d = parseJson(r, 'save-state');
      if (!d.ok) throw new Error(String(d.error ?? 'save read failed'));
      return {
        player: d.header?.player ?? '',
        version: d.header?.version ?? 0,
        party_count: d.party_count ?? 0,
        json: JSON.stringify(d.parties ?? []),
      };
    },
  }));

  // ---- mb_save_move -----------------------------------------------------------------
  tools.register(defineTool({
    name: 'mb_save_move',
    description:
      'Move a party in the save file. This is the action channel that works: the engine rejects ' +
      'synthetic input and the save is a documented, offline structure, so a write either verifies ' +
      'or fails. Every write takes a timestamped backup first. The change takes effect the next ' +
      'time that save is loaded, so it is a strategic tool, not a realtime one. Use ' +
      'mb_save_state to find the index, and dry_run to preview.',
    parameters: {
      party: { type: 'integer', description: 'Party index from mb_save_state.', required: true },
      x: { type: 'number', description: 'Target map x.', required: true },
      y: { type: 'number', description: 'Target map y.', required: true },
      dry_run: { type: 'boolean', description: 'Preview the byte-level change without writing.' },
      save: { type: 'string', description: 'Optional path to a .sav.' },
    },
    output: {
      schema: objectSchema({
        dry_run: { type: 'boolean' },
        offset: { type: 'string' },
        from: { type: 'string' },
        to: { type: 'string' },
        backup: { type: 'string' },
      }),
      render: (args, v) => textBlock(v.dry_run
        ? `DRY RUN party ${args.party} at ${v.offset}: ${v.from} -> ${v.to}`
        : `Moved party ${args.party} at ${v.offset}: ${v.from} -> ${v.to}\nbackup: ${v.backup}\n` +
          'Load this save in game for the change to take effect.'),
    },
    async execute(args) {
      const argv = ['save-move', '--party', String(args.party),
                    '--x', String(args.x), '--y', String(args.y)];
      if (args?.dry_run) argv.push('--dry-run');
      if (args?.save) argv.push('--save', String(args.save));
      const r = await runAgent(cfg, argv, 60000);
      const d = parseJson(r, 'save-move');
      if (!d.ok) throw new Error(String(d.error ?? 'save write failed'));
      return {
        dry_run: Boolean(d.dry_run),
        offset: String(d.offset ?? ''),
        from: `(${(d.from ?? [])[0]}, ${(d.from ?? [])[1]})`,
        to: `(${(d.to ?? [])[0]}, ${(d.to ?? [])[1]})`,
        backup: String(d.backup ?? ''),
      };
    },
  }));

  console.log(`[dsh-mount-and-blade] registered 6 tools via ${cfg.agent}`);
}
