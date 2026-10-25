#!/usr/bin/env python3
"""Run one owned retail replay in a bounded user-systemd cgroup."""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import subprocess
import shutil
import signal
import time
import uuid

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', type=Path, required=True)
parser.add_argument('--build', type=Path, default=Path('build'))
parser.add_argument('--prefix', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--replay', default='replay/th6_01.rpy')
parser.add_argument('--updates', type=int, default=180000)
parser.add_argument('--seconds', type=int, default=3000)
args = parser.parse_args()
if not 1 <= args.updates <= 1000000 or not 1 <= args.seconds <= 7200:
    parser.error('invalid frame/time budget')
args.game = args.game.resolve()
args.build = args.build.resolve()
args.prefix = args.prefix.resolve()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=False)
exe = args.game / 'th06.exe'
pinned = '9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245'
if hashlib.sha256(exe.read_bytes()).hexdigest() != pinned:
    parser.error('retail executable identity mismatch')
if not (args.prefix / 'system.reg').exists():
    parser.error('initialize the dedicated 32-bit Wine prefix first')
if not (args.game / args.replay).is_file():
    parser.error('replay does not exist under the game directory')
ownership = (args.prefix.parent / '.retail.lock').open('a')
try:
    fcntl.flock(ownership, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    parser.error('another owned retail replay is already running')
frozen = args.output / 'bin'
frozen.mkdir()
binary_hashes = {}
for name in ['launch.exe', 'observer.dll']:
    source = args.build / 'wine-replay' / name
    expected = hashlib.sha256(source.read_bytes()).hexdigest()
    shutil.copy2(source, frozen / name)
    if expected != hashlib.sha256((frozen / name).read_bytes()).hexdigest() or expected != hashlib.sha256(source.read_bytes()).hexdigest():
        parser.error('Wine observer changed while snapshotting; retry after the build')
    binary_hashes[name] = expected

def windows(path):
    return 'Z:' + str(path).replace('/', '\\')

unit = 'th06-replay-' + uuid.uuid4().hex[:12]
command = ['systemd-run', '--user', '--unit=' + unit, '--wait', '--pipe',
           '-p', 'MemoryMax=2G', '-p', 'MemoryHigh=1536M', '-p', 'MemorySwapMax=0',
           '-p', 'TasksMax=256', '-p', 'RuntimeMaxSec=' + str(args.seconds),
           '-p', 'TimeoutStopSec=5', '-p', 'KillMode=control-group',
           '--working-directory=' + str(args.game), '/usr/bin/env',
           'WINEPREFIX=' + str(args.prefix), 'WINEARCH=win32', 'WINEDEBUG=-all',
           'WINEDLLOVERRIDES=mscoree,mshtml=', 'LANG=ja_JP.UTF-8', 'LC_ALL=ja_JP.UTF-8',
           'LP_NUM_THREADS=2', 'LIBGL_ALWAYS_SOFTWARE=1', 'MESA_SHADER_CACHE_DISABLE=true',
           'TH06_RETAIL_REPLAY=' + args.replay, 'TH06_RETAIL_UPDATES=' + str(args.updates),
           # Keep X available until the owned prefix's background services exit.
           # Arguments remain separate; the static shell program never expands
           # a pathname or replay filename as shell source.
           '/usr/bin/xvfb-run', '-a', '/bin/sh', '-c',
           'wine "$@"; result=$?; wineserver -k; wineserver -w; exit "$result"',
           'th06-owned-replay',
           str(frozen / 'launch.exe'), 'th06.exe',
           windows(frozen / 'observer.dll'), windows(args.output)]
receipt = {'unit': unit, 'memory_max_bytes': 2 * 1024**3, 'swap_max_bytes': 0,
           'peak_cgroup_bytes': 0, 'oom': 0, 'oom_kill': 0,
           'cgroup_limits_verified': False,
           'retail_sha256': pinned, 'replay_sha256': hashlib.sha256((args.game / args.replay).read_bytes()).hexdigest(),
           'binaries': binary_hashes}
(args.output / 'start.json').write_text(json.dumps(receipt, indent=2) + '\n')

def interrupted(signum, _frame):
    raise SystemExit(128 + signum)

signal.signal(signal.SIGTERM, interrupted)
started = time.monotonic()
with (args.output / 'process.log').open('wb') as log:
    child = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    cgroup = None
    try:
        while child.poll() is None:
            if cgroup is None:
                result = subprocess.run(['systemctl', '--user', 'show', unit, '-p', 'ControlGroup', '--value'],
                                        capture_output=True, text=True)
                if result.returncode == 0 and result.stdout.strip():
                    cgroup = Path('/sys/fs/cgroup') / result.stdout.strip().lstrip('/')
            if cgroup is not None:
                if not receipt['cgroup_limits_verified']:
                    try:
                        actual_max = int((cgroup / 'memory.max').read_text())
                        actual_swap = int((cgroup / 'memory.swap.max').read_text())
                        if actual_max != receipt['memory_max_bytes'] or actual_swap != 0:
                            raise RuntimeError('owned Wine cgroup memory limits were not applied')
                        receipt['cgroup_limits_verified'] = True
                    except FileNotFoundError:
                        pass
                for filename in ['memory.current', 'memory.peak']:
                    try:
                        receipt['peak_cgroup_bytes'] = max(receipt['peak_cgroup_bytes'], int((cgroup / filename).read_text()))
                    except (FileNotFoundError, ValueError):
                        pass
                try:
                    events = dict(line.split() for line in (cgroup / 'memory.events').read_text().splitlines())
                    for name in ['oom', 'oom_kill']:
                        receipt[name] = max(receipt[name], int(events.get(name, 0)))
                except FileNotFoundError:
                    pass
            time.sleep(0.2)
    except BaseException:
        subprocess.run(['systemctl', '--user', 'stop', unit], check=False)
        child.wait()
        raise
receipt['process_exit'] = child.returncode
receipt['wall_seconds'] = time.monotonic() - started
state = subprocess.run(['systemctl', '--user', 'show', unit, '-p', 'Result', '-p', 'ExecMainStatus'],
                       capture_output=True, text=True, check=True)
receipt['systemd'] = dict(line.split('=', 1) for line in state.stdout.splitlines())
if (args.output / 'outcome.json').exists():
    receipt['outcome'] = json.loads((args.output / 'outcome.json').read_text())
(args.output / 'resource-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt, indent=2), flush=True)
raise SystemExit(child.returncode or (1 if receipt['oom_kill'] or not receipt['cgroup_limits_verified'] else 0))
