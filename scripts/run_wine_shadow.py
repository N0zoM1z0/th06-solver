#!/usr/bin/env python3
"""Run live native shadow MPC and one owned Wine game in a shared 2-GiB cgroup."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import shutil
import signal
import time
import uuid
import sys
from wine_runtime import apply_environment, render_environment, resolve_runtime


def worker(directory):
    output = Path(directory)
    config = json.loads((output / 'worker.json').read_text())
    actual = {name: os.environ.get(name) for name in config['expected_environment']}
    (output / 'effective-worker-environment.json').write_text(json.dumps(actual, indent=2) + '\n')
    if actual != config['expected_environment']:
        raise RuntimeError('owned worker did not receive the declared rendering environment')
    wine_env = apply_environment(os.environ, config['expected_wine_environment'])
    effective = {name: wine_env.get(name) for name in config['expected_wine_environment']}
    (output / 'effective-environment.json').write_text(json.dumps(effective, indent=2) + '\n')
    version = subprocess.run([config['wine'][0], '--version'], env=wine_env,
                             capture_output=True, text=True, check=True)
    identity = {'launcher': config['wine'][0], 'wineserver': config['wineserver'],
                'launcher_sha256': hashlib.sha256(Path(config['wine'][0]).read_bytes()).hexdigest(),
                'wineserver_sha256': hashlib.sha256(Path(config['wineserver']).read_bytes()).hexdigest(),
                'version': version.stdout.strip()}
    (output / 'wine-runtime.json').write_text(json.dumps(identity, indent=2) + '\n')
    if (identity['launcher_sha256'] != config['runtime_sha256']['loader'] or
            identity['wineserver_sha256'] != config['runtime_sha256']['server']):
        raise RuntimeError('selected Wine runtime changed after launch preparation')
    native_log = (output / 'controller.log').open('wb')
    controller = subprocess.Popen(config['controller'], stdout=native_log, stderr=subprocess.STDOUT)
    game = None
    capture = None
    display_monitor = None
    try:
        deadline = time.monotonic() + 15
        while not (output / 'bridge.bin').exists():
            if controller.poll() is not None or time.monotonic() > deadline:
                raise RuntimeError('native bridge did not initialize')
            time.sleep(.01)
        if config.get('record'):
            from shadow_capture import Capture
            capture = Capture(output)
            wine_env.update(capture.metadata['audio_environment'])
        game = subprocess.Popen(config['wine'], env=wine_env)
        if capture:
            capture.attach(game)
        while game.poll() is None:
            if config.get('display_timing') and display_monitor is None:
                windows = subprocess.run(['xdotool', 'search', '--onlyvisible', '--class', '^th06.exe$'],
                                         capture_output=True, text=True)
                if windows.stdout.split():
                    with (output / 'display-monitor.log').open('wb') as log:
                        display_monitor = subprocess.Popen(
                            [sys.executable, str(Path(__file__).with_name('replay_display.py')),
                             windows.stdout.split()[0], str(output)], stdout=log, stderr=subprocess.STDOUT)
            if display_monitor and display_monitor.poll() not in (None, 0):
                raise RuntimeError('owned display monitor failed; inspect display-monitor.log')
            if controller.poll() is not None:
                # Normal completion can race the final owner ExitProcess.
                try:
                    game.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    raise RuntimeError('controller exited while retail kept running')
            time.sleep(.05)
        native_status = controller.wait(timeout=15)
        return game.returncode or native_status
    finally:
        if capture:
            capture.close()
        if game is not None and game.poll() is None:
            game.terminate()
            try: game.wait(timeout=3)
            except subprocess.TimeoutExpired:
                game.kill()
                game.wait()
        if display_monitor and display_monitor.poll() is None:
            display_monitor.terminate()
            display_monitor.wait(timeout=3)
        subprocess.run([config['wineserver'], '-k'], env=wine_env, check=False)
        subprocess.run([config['wineserver'], '-w'], env=wine_env, timeout=10, check=False)
        if controller.poll() is None:
            controller.terminate()
            try: controller.wait(timeout=3)
            except subprocess.TimeoutExpired:
                controller.kill()
                controller.wait()
        if capture:
            if capture.video is not None:
                capture.mux()
        native_log.close()

if len(sys.argv) == 3 and sys.argv[1] == '--worker':
    raise SystemExit(worker(sys.argv[2]))

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', type=Path, required=True)
parser.add_argument('--build', type=Path, default=Path('build'))
parser.add_argument('--prefix', type=Path, required=True)
parser.add_argument('--wine', default='wine', help='owned Wine loader; private runtimes stay outside Git')
parser.add_argument('--wineserver', type=Path, help='matching server; otherwise use the loader sibling')
parser.add_argument('--wine-arch', choices=['win32', 'win64', 'wow64'], default='win32')
parser.add_argument('--dll-overrides', default='mscoree,mshtml=')
parser.add_argument('--renderer-threads', type=int, default=2)
parser.add_argument('--vblank', choices=['inherit', 'off'], default='inherit')
parser.add_argument('--shader-cache', type=Path)
parser.add_argument('--display-timing', action='store_true', help='record independent X11 window damage timing; requires render timing')
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--data', type=Path, required=True)
parser.add_argument('--stage', type=int, default=5)
parser.add_argument('--shot', type=int, default=1)
parser.add_argument('--story', action='store_true')
parser.add_argument('--pulse', action='store_true', help='try rotating native motion recovery before heuristic proposals')
parser.add_argument('--pulse-on-failure', action='store_true', help='try motion once after an unsuccessful ordinary search')
parser.add_argument('--laser-aware', action='store_true', help='guide native proposals with observed laser motion and orbital futures')
parser.add_argument('--anytime', action='store_true', help='retain the first complete native certificate if later ranking times out')
parser.add_argument('--warm-start', action='store_true', help='verify an extended nominal backup before fresh proposals; requires anytime')
parser.add_argument('--warm-horizon', type=int, default=0, help='cap separately verified nominal backups; requires warm start, zero follows the fresh horizon')
parser.add_argument('--save-certificates', action='store_true', help='save every published literal certificate and decision queue generation for exact window reconstruction')
parser.add_argument('--acceleration-aware', action='store_true', help='guide proposals with observed native acceleration and its expiry')
parser.add_argument('--damage-aware', action='store_true', help='score full native futures by actual boss damage and completed spell phases')
parser.add_argument('--power-aware', action='store_true', help='guide native proposals toward power pickups until full power')
parser.add_argument('--precise-limiter', action='store_true', help='scope the replay-tested long-uptime precision correction to the original live limiter')
parser.add_argument('--sleep-limiter', action='store_true', help='use coarse sleeps before final limiter checks; implies precise limiter')
parser.add_argument('--recorded', action='store_true', help='propose multi-turn paths from conditional native collision geometry')
parser.add_argument('--record', action='store_true', help='capture owned Xvfb video and private audio separately')
parser.add_argument('--render-timing', action='store_true', help='record actual online update/draw/Present API clocks with the shared replay timing journal')
parser.add_argument('--horizon', type=int, default=128)
parser.add_argument('--survival-horizon', type=int, default=0, help='use a longer certified horizon while all active spell bosses are non-interactable; zero disables')
parser.add_argument('--rescue-horizon', type=int, default=0, help='after a failed replacement and low queue, try a shorter complete native future; zero disables')
parser.add_argument('--lead', type=int, default=16)
parser.add_argument('--budget-ms', type=int, default=110)
parser.add_argument('--interval', type=int, default=16)
parser.add_argument('--proposals', type=int, default=1)
parser.add_argument('--fresh', action='store_true', help='search fresh native alternatives at every interval')
parser.add_argument('--diverse-first', action='store_true', help='rank distinct initial actions before duplicate tails for the same action')
parser.add_argument('--refresh-updates', type=int, default=0, help='reuse a fully reverified nominal continuation between fresh searches every N actual updates; incompatible with fresh')
parser.add_argument('--updates', type=int, default=180000)
parser.add_argument('--seconds', type=int, default=3000)
args = parser.parse_args()
if args.record:
    missing = [name for name in ['ffmpeg', 'xdotool', 'pulseaudio'] if shutil.which(name) is None]
    if missing:
        parser.error('recording tools unavailable: ' + ', '.join(missing))
if not 1 <= args.updates <= 1000000 or not 1 <= args.seconds <= 7200:
    parser.error('invalid frame/time budget')
if not 1 <= args.renderer_threads <= 16:
    parser.error('renderer threads require 1..16')
if args.display_timing and (not args.render_timing or shutil.which('xdotool') is None):
    parser.error('display timing requires --render-timing and xdotool')
if not 1 <= args.stage <= 7 or not 0 <= args.shot <= 3 or (args.story and args.stage != 1):
    parser.error('invalid Lunatic/Extra stage/shot/story start')
difficulty = 4 if args.stage == 7 else 3
if not (32 <= args.horizon <= 592 and 8 <= args.lead <= 64 and args.horizon + args.lead <= 600
        and 1 <= args.budget_ms <= 2000 and 1 <= args.interval <= args.horizon and 1 <= args.proposals <= 50):
    parser.error('invalid shadow search bounds')
if args.survival_horizon and not (args.horizon <= args.survival_horizon <= 600 - args.lead):
    parser.error('invalid survival horizon')
if args.rescue_horizon and not 32 <= args.rescue_horizon <= args.horizon:
    parser.error('rescue horizon requires 32..base horizon')
if args.recorded and max(args.horizon, args.survival_horizon) > 240:
    parser.error('recorded proposals require horizon <= 240')
if args.pulse and args.pulse_on_failure:
    parser.error('choose one pulse policy')
if args.warm_horizon and (not args.warm_start or not 32 <= args.warm_horizon <= args.horizon):
    parser.error('warm horizon requires warm start and 32..base horizon')
if args.warm_start and not args.anytime:
    parser.error('warm start requires anytime certificates')
if args.refresh_updates and (args.fresh or not args.interval <= args.refresh_updates <= args.horizon):
    parser.error('refresh interval requires non-fresh mode and interval..horizon')
args.data = args.data.resolve()
args.game = args.game.resolve()
args.build = args.build.resolve()
args.prefix = args.prefix.resolve()
if args.shader_cache:
    args.shader_cache = args.shader_cache.resolve()
try:
    wine_loader, wine_server = resolve_runtime(args.wine, args.wineserver, args.prefix, args.wine_arch)
except ValueError as error:
    parser.error(str(error))
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=False)
exe = args.game / 'th06.exe'
pinned = '9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245'
if hashlib.sha256(exe.read_bytes()).hexdigest() != pinned:
    parser.error('retail executable identity mismatch')
archives = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in args.data.iterdir() if p.is_file() and p.suffix.lower() == '.dat'}
retail_archives = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in args.game.iterdir() if p.is_file() and p.suffix.lower() == '.dat'}
if len(archives) != 6 or archives != retail_archives:
    parser.error('native and retail must use the same six DAT archives')
shared_lock = Path(__file__).resolve().parents[1] / 'reports/local/.retail.lock'
shared_lock.parent.mkdir(parents=True, exist_ok=True)
runtime_ownership = shared_lock.open('a')
ownership = (args.prefix.parent / '.retail.lock').open('a')
try:
    fcntl.flock(runtime_ownership, fcntl.LOCK_EX | fcntl.LOCK_NB)
    fcntl.flock(ownership, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    parser.error('another owned retail game is already running')
frozen = args.output / 'bin'
frozen.mkdir()
binary_hashes = {}
sources = {'launch.exe': args.build / 'wine-replay/launch.exe',
           'shadow.dll': args.build / 'wine-replay/shadow.dll',
           'th06_shadow_online': args.build / 'th06_shadow_online',
           'run_wine_shadow.py': Path(__file__).resolve(),
           'wine_runtime.py': Path(__file__).with_name('wine_runtime.py').resolve(),
           'replay_display.py': Path(__file__).with_name('replay_display.py').resolve(),
           'replay_pacing.py': Path(__file__).with_name('replay_pacing.py').resolve(),
           'shadow_capture.py': Path(__file__).with_name('shadow_capture.py').resolve()}
for name, source in sources.items():
    expected = hashlib.sha256(source.read_bytes()).hexdigest()
    shutil.copy2(source, frozen / name)
    if expected != hashlib.sha256((frozen / name).read_bytes()).hexdigest() or expected != hashlib.sha256(source.read_bytes()).hexdigest():
        parser.error('Wine observer changed while snapshotting; retry after the build')
    binary_hashes[name] = expected

def windows(path):
    return 'Z:' + str(path).replace('/', '\\')

unit = 'th06-shadow-' + uuid.uuid4().hex[:12]
command = ['systemd-run', '--user', '--unit=' + unit, '--wait', '--pipe',
           '-p', 'MemoryMax=2G', '-p', 'MemoryHigh=1536M', '-p', 'MemorySwapMax=0',
           '-p', 'TasksMax=256', '-p', 'RuntimeMaxSec=' + str(args.seconds),
           '-p', 'TimeoutStopSec=5', '-p', 'KillMode=control-group',
           '--working-directory=' + str(args.game), '/usr/bin/env',
           'WINEDEBUG=-all', 'LANG=ja_JP.UTF-8', 'LC_ALL=ja_JP.UTF-8',
           'TH06_RETAIL_UPDATES=' + str(args.updates),
           'TH06_SHADOW_STAGE=' + str(args.stage), 'TH06_SHADOW_SHOT=' + str(args.shot),
           'TH06_SHADOW_STORY=' + str(int(args.story)),
           'TH06_SHADOW_RENDER_TIMING=' + str(int(args.render_timing)),
           'TH06_SHADOW_PRECISE_LIMITER=' + str(int(args.precise_limiter or args.sleep_limiter)),
           'TH06_SHADOW_SLEEP_LIMITER=' + str(int(args.sleep_limiter)),
           '/usr/bin/xvfb-run', '-a', sys.executable,
           str(frozen / 'run_wine_shadow.py'), '--worker', str(args.output)]
wine_environment, worker_environment = render_environment(
    os.environ, args.prefix, args.wine_arch, wine_server, args.dll_overrides,
    args.renderer_threads, args.vblank, args.shader_cache)
runtime_hashes = {'loader': hashlib.sha256(wine_loader.read_bytes()).hexdigest(),
                  'server': hashlib.sha256(wine_server.read_bytes()).hexdigest()}
env_start = command.index('/usr/bin/env') + 1
command[env_start:env_start] = [item for name in worker_environment for item in ['-u', name]]
index = command.index('/usr/bin/xvfb-run')
command[index:index] = [name + '=' + value for name, value in worker_environment.items() if value is not None]
worker_config = {
    'expected_environment': worker_environment,
    'expected_wine_environment': wine_environment,
    'runtime_sha256': runtime_hashes,
    'display_timing': args.display_timing, 'wineserver': str(wine_server),
    'controller': [str(frozen / 'th06_shadow_online'), '--data', str(args.data),
                   '--directory', str(args.output), '--horizon', str(args.horizon),
                   '--lead', str(args.lead), '--budget-ms', str(args.budget_ms),
                   '--interval', str(args.interval), '--proposals', str(args.proposals)],
    'wine': [str(wine_loader), str(frozen / 'launch.exe'), 'th06.exe', windows(frozen / 'shadow.dll'),
             windows(args.output)], 'record': args.record}
if args.pulse:
    worker_config['controller'].append('--pulse')
if args.pulse_on_failure:
    worker_config['controller'].append('--pulse-on-failure')
if args.laser_aware:
    worker_config['controller'].append('--laser-aware')
if args.anytime:
    worker_config['controller'].append('--anytime')
if args.warm_start:
    worker_config['controller'].append('--warm-start')
if args.warm_horizon:
    worker_config['controller'].extend(['--warm-horizon', str(args.warm_horizon)])
if args.save_certificates:
    worker_config['controller'].append('--save-certificates')
if args.power_aware:
    worker_config['controller'].append('--power-aware')
if args.acceleration_aware:
    worker_config['controller'].append('--acceleration-aware')
if args.damage_aware:
    worker_config['controller'].append('--damage-aware')
if args.survival_horizon:
    worker_config['controller'].extend(['--survival-horizon', str(args.survival_horizon)])
if args.rescue_horizon:
    worker_config['controller'].extend(['--rescue-horizon', str(args.rescue_horizon)])
if args.recorded:
    worker_config['controller'].append('--recorded')
if args.fresh:
    worker_config['controller'].append('--fresh')
if args.diverse_first:
    worker_config['controller'].append('--diverse-first')
if args.refresh_updates:
    worker_config['controller'].extend(['--refresh-updates', str(args.refresh_updates)])
(args.output / 'worker.json').write_text(json.dumps(worker_config, indent=2) + '\n')
receipt = {'unit': unit, 'memory_max_bytes': 2 * 1024**3,
           'memory_high_bytes': 1536 * 1024**2, 'swap_max_bytes': 0,
           'peak_cgroup_bytes': 0, 'oom': 0, 'oom_kill': 0,
           'cgroup_cpu_usage_usec': 0,
           'cgroup_limits_verified': False,
           'retail_sha256': pinned, 'mode': 'live-shadow-mpc',
           'wine_loader': str(wine_loader), 'wineserver': str(wine_server), 'wine_arch': args.wine_arch,
           'runtime_sha256': runtime_hashes,
           'render_environment': wine_environment, 'display_timing': args.display_timing,
           'game_runtime_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in args.game.iterdir() if p.suffix.lower() in ('.dll', '.conf')},
           'host_load_average_start': os.getloadavg(),
           'host_cpu_pressure_start': Path('/proc/pressure/cpu').read_text(),
           'stage': args.stage, 'shot': args.shot, 'difficulty': difficulty, 'story': args.story,
           'survival_horizon': args.survival_horizon,
           'rescue_horizon': args.rescue_horizon,
           'warm_start': args.warm_start, 'warm_horizon': args.warm_horizon,
           'save_certificates': args.save_certificates,
           'render_timing': args.render_timing,
           'precise_limiter': args.precise_limiter or args.sleep_limiter,
           'sleep_limiter': args.sleep_limiter,
           'dat_sha256': archives,
           'config_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in args.game.iterdir() if p.suffix.lower() == '.cfg'},
           'binaries': binary_hashes}
(args.output / 'launch.json').write_text(json.dumps(receipt, indent=2) + '\n')

def interrupted(signum, _frame):
    raise SystemExit(128 + signum)

signal.signal(signal.SIGTERM, interrupted)
started = time.monotonic()
last_snapshot = started
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
                try:
                    usage = dict(line.split() for line in (cgroup / 'cpu.stat').read_text().splitlines())
                    receipt['cgroup_cpu_usage_usec'] = max(receipt['cgroup_cpu_usage_usec'], int(usage['usage_usec']))
                except FileNotFoundError:
                    pass
                if not receipt['cgroup_limits_verified']:
                    try:
                        actual_max = int((cgroup / 'memory.max').read_text())
                        actual_high = int((cgroup / 'memory.high').read_text())
                        actual_swap = int((cgroup / 'memory.swap.max').read_text())
                        if (actual_max != receipt['memory_max_bytes'] or
                                actual_high != receipt['memory_high_bytes'] or actual_swap != 0):
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
            # Preserve measured limits/peaks if the interactive parent vanishes.
            # Keep the final report distinct with an explicit completion flag.
            if time.monotonic() - last_snapshot >= 1:
                receipt['runner_finished'] = False
                temporary = args.output / 'resource-receipt.tmp'
                temporary.write_text(json.dumps(receipt, indent=2) + '\n')
                temporary.replace(args.output / 'resource-receipt.json')
                last_snapshot = time.monotonic()
            time.sleep(0.2)
    except BaseException:
        subprocess.run(['systemctl', '--user', 'stop', unit], check=False)
        child.wait()
        raise
receipt['process_exit'] = child.returncode
receipt['runner_finished'] = True
receipt['wall_seconds'] = time.monotonic() - started
receipt['host_load_average_end'] = os.getloadavg()
receipt['host_cpu_pressure_end'] = Path('/proc/pressure/cpu').read_text()
state = subprocess.run(['systemctl', '--user', 'show', unit, '-p', 'Result', '-p', 'ExecMainStatus'],
                       capture_output=True, text=True, check=True)
receipt['systemd'] = dict(line.split('=', 1) for line in state.stdout.splitlines())
if (args.output / 'outcome.json').exists():
    receipt['outcome'] = json.loads((args.output / 'outcome.json').read_text())
(args.output / 'resource-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
if (args.output / 'controller.json').exists():
    receipt['controller'] = json.loads((args.output / 'controller.json').read_text())
(args.output / 'resource-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt, indent=2), flush=True)
raise SystemExit(child.returncode or (1 if receipt['oom_kill'] or not receipt['cgroup_limits_verified'] else 0))
