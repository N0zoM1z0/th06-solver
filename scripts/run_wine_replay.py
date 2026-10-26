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
import os
import sys
from replay_pacing import analyze, compare_native


def cache_state(directory):
    if directory is None:
        return None
    entries = [(str(path.relative_to(directory)), path.stat().st_size)
               for path in sorted(directory.rglob('*')) if path.is_file()]
    return {'files': len(entries), 'bytes': sum(size for _, size in entries),
            'file_size_manifest_sha256': hashlib.sha256(json.dumps(entries).encode()).hexdigest()}


def parse_cpus(value):
    available = os.sched_getaffinity(0)
    selected = set()
    try:
        for item in value.split(','):
            bounds = [int(v) for v in item.split('-')]
            if len(bounds) not in (1, 2):
                raise ValueError()
            first, last = bounds[0], bounds[-1]
            if not 0 <= first <= last <= max(available):
                raise ValueError()
            selected.update(range(first, last + 1))
        if not selected or not selected <= available:
            raise ValueError()
    except ValueError:
        raise argparse.ArgumentTypeError('CPU list is outside the available affinity')
    return sorted(selected)


def graphics_libraries():
    relative = next(line.split(':', 2)[2] for line in Path('/proc/self/cgroup').read_text().splitlines()
                    if line.startswith('0::'))
    group = Path('/sys/fs/cgroup') / relative.lstrip('/')
    for pid in (group / 'cgroup.procs').read_text().split():
        process = Path('/proc') / pid
        try:
            if (process / 'comm').read_text().strip() != 'th06.exe':
                continue
            paths = sorted({line.split()[-1] for line in (process / 'maps').read_text().splitlines()
                            if any(name in line for name in ['libGLX_mesa', 'swrast_dri.so', 'libgallium-', 'libvulkan_lvp.so', '/d3d8.dll'])})
            if paths:
                with (process / 'exe').open('rb') as executable:
                    header = executable.read(5)
                return {'pid': int(pid), 'unix_executable': str((process / 'exe').resolve()),
                        'nice': os.getpriority(os.PRIO_PROCESS, int(pid)),
                        'cpus': sorted(os.sched_getaffinity(int(pid))),
                        'thread_nice': {task.name: os.getpriority(os.PRIO_PROCESS, int(task.name))
                                        for task in (process / 'task').iterdir()},
                        'unix_elf_bits': 64 if header == b'\x7fELF\x02' else 32 if header == b'\x7fELF\x01' else None,
                        'libraries': {path: hashlib.sha256(Path(path).read_bytes()).hexdigest() for path in paths}}
        except (FileNotFoundError, ProcessLookupError):
            continue
    return None


def worker(directory):
    output = Path(directory)
    config = json.loads((output / 'worker.json').read_text())
    actual = {name: os.environ.get(name) for name in config['expected_environment']}
    (output / 'effective-worker-environment.json').write_text(json.dumps(actual, indent=2) + '\n')
    if actual != config['expected_environment']:
        (output / 'effective-environment.json').write_text(json.dumps(actual, indent=2) + '\n')
        raise RuntimeError('owned worker did not receive the declared rendering environment')
    priority = {'nice': os.getpriority(os.PRIO_PROCESS, 0),
                'cpus': sorted(os.sched_getaffinity(0))}
    (output / 'effective-worker-priority.json').write_text(json.dumps(priority, indent=2) + '\n')
    if config.get('display_cpus') and priority['cpus'] != config['display_cpus']:
        raise RuntimeError('owned display worker did not receive the declared CPU affinity')
    wine_env = os.environ.copy()
    for name, value in config.get('wine_environment', {}).items():
        if value is None:
            wine_env.pop(name, None)
        else:
            wine_env[name] = value
    effective = {name: wine_env.get(name) for name in config.get('expected_wine_environment', actual)}
    (output / 'effective-environment.json').write_text(json.dumps(effective, indent=2) + '\n')
    if effective != config.get('expected_wine_environment', actual):
        raise RuntimeError('owned Wine child did not receive the declared rendering environment')
    wine_path = Path(shutil.which(config['wine'][0])).resolve()
    version = subprocess.run([str(wine_path), '--version'], capture_output=True, text=True, check=True)
    (output / 'wine-runtime.json').write_text(json.dumps({'launcher': str(wine_path),
        'launcher_sha256': hashlib.sha256(wine_path.read_bytes()).hexdigest(), 'version': version.stdout.strip()}, indent=2) + '\n')
    wine_command = config['wine']
    if config.get('wine_nice', 0):
        wine_command = ['/usr/bin/nice', '-n', str(config['wine_nice']), *wine_command]
    if config.get('wine_cpus'):
        wine_command = ['/usr/bin/taskset', '--cpu-list',
                        ','.join(map(str, config['wine_cpus'])), *wine_command]
    game = subprocess.Popen(wine_command, env=wine_env)
    snapshot = False
    display_monitor = None
    libraries_recorded = False
    try:
        while game.poll() is None:
            if not libraries_recorded and (output / 'update-times.bin').exists() and (output / 'update-times.bin').stat().st_size >= 40:
                libraries = graphics_libraries()
                if libraries:
                    (output / 'graphics-libraries.json').write_text(json.dumps(libraries, indent=2) + '\n')
                    libraries_recorded = True
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
            clock = output / 'update-times.bin'
            if config.get('snapshot') and not snapshot and clock.exists() and clock.stat().st_size >= 16 + 60 * 24:
                windows = subprocess.run(['xdotool', 'search', '--onlyvisible', '--class', '^th06.exe$'],
                                         capture_output=True, text=True)
                for window in windows.stdout.split():
                    geometry = subprocess.run(['xdotool', 'getwindowgeometry', '--shell', window],
                                              capture_output=True, text=True)
                    fields = dict(line.split('=', 1) for line in geometry.stdout.splitlines() if '=' in line)
                    if fields.get('WIDTH') == '640' and fields.get('HEIGHT') == '480':
                        source = os.environ['DISPLAY'] + '+' + fields['X'] + ',' + fields['Y']
                        subprocess.run(['ffmpeg', '-nostdin', '-hide_banner', '-loglevel', 'error', '-y',
                                        '-f', 'x11grab', '-video_size', '640x480', '-i', source,
                                        '-frames:v', '1', '-threads:v', '1', '-update', '1',
                                        str(output / 'screenshot.png')], check=True, timeout=15)
                        snapshot = True
                        break
            time.sleep(.05)
        return game.returncode
    finally:
        if game.poll() is None:
            game.terminate()
            try:
                game.wait(timeout=3)
            except subprocess.TimeoutExpired:
                game.kill()
                game.wait()
        if display_monitor and display_monitor.poll() is None:
            display_monitor.terminate()
            display_monitor.wait(timeout=3)
        subprocess.run([config.get('wineserver', 'wineserver'), '-k'], check=False)
        subprocess.run([config.get('wineserver', 'wineserver'), '-w'], check=False, timeout=10)


if len(sys.argv) == 3 and sys.argv[1] == '--worker':
    raise SystemExit(worker(sys.argv[2]))

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game', type=Path, required=True)
parser.add_argument('--build', type=Path, default=Path('build'))
parser.add_argument('--prefix', type=Path, required=True)
parser.add_argument('--wine', default='wine', help='Wine loader; private runtimes remain outside the repository history')
parser.add_argument('--wineserver', type=Path, help='matching server; otherwise use the loader sibling')
parser.add_argument('--wine-arch', choices=['win32', 'win64', 'wow64'], default='win32')
parser.add_argument('--dll-overrides', default='mscoree,mshtml=', help='explicit Wine DLL selection for the owned replay process')
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--replay', default='replay/th6_01.rpy')
parser.add_argument('--updates', type=int, default=180000)
parser.add_argument('--seconds', type=int, default=3000)
parser.add_argument('--start-stage', type=int, default=1)
parser.add_argument('--single-stage', action='store_true', help='stop at the selected stage natural clear')
parser.add_argument('--render-timing', action='store_true')
parser.add_argument('--precise-limiter', action='store_true', help='restore double precision only in the original frame limiter')
parser.add_argument('--sleep-limiter', action='store_true', help='sleep 1 ms only outside the original final 4-ms deadline checks; implies --precise-limiter')
parser.add_argument('--software-bullets', action='store_true', help='original CPU bullet draw path only inside its render callback')
parser.add_argument('--batch-bullets', action='store_true', help='batch adjacent CPU-transformed bullet quads; implies --software-bullets')
parser.add_argument('--renderer-threads', type=int, default=2)
parser.add_argument('--wine-nice', type=int, default=0, help='lower Wine CPU priority relative to the owned display server (0..19)')
parser.add_argument('--wine-cpus', type=parse_cpus, help='owned Wine CPU affinity, e.g. 0-3,8-11')
parser.add_argument('--display-cpus', type=parse_cpus, help='owned display/observer affinity; use separate physical cores')
parser.add_argument('--vblank', choices=['inherit', 'off'], default='inherit')
parser.add_argument('--shader-cache', type=Path)
parser.add_argument('--native-trace', type=Path, help='compare with a fresh native export CSV')
parser.add_argument('--snapshot', action='store_true', help='one screenshot during initial warmup, inside the capped group')
parser.add_argument('--display-timing', action='store_true', help='independent XDamage window update clock inside the same cap')
parser.add_argument('--require-pacing', action='store_true', help='also fail acceptance if the declared presentation gate fails')
args = parser.parse_args()
if args.sleep_limiter:
    args.precise_limiter = True
if args.batch_bullets:
    args.software_bullets = True
if args.precise_limiter or args.snapshot or args.require_pacing or args.display_timing or args.software_bullets:
    args.render_timing = True
if not 1 <= args.updates <= 1000000 or not 1 <= args.seconds <= 7200:
    parser.error('invalid frame/time budget')
if not 1 <= args.start_stage <= 6 or not 1 <= args.renderer_threads <= 8:
    parser.error('invalid stage/render threads')
if not 0 <= args.wine_nice <= 19:
    parser.error('invalid Wine nice value')
if args.wine_cpus and args.display_cpus and set(args.wine_cpus) & set(args.display_cpus):
    parser.error('Wine and display CPU affinity must not overlap')
if args.shader_cache:
    args.shader_cache = args.shader_cache.resolve()
    args.shader_cache.mkdir(parents=True, exist_ok=True)
if args.native_trace:
    args.native_trace = args.native_trace.resolve()
if args.snapshot and any(shutil.which(name) is None for name in ['ffmpeg', 'xdotool']):
    parser.error('snapshot requires ffmpeg and xdotool')
if args.display_timing and shutil.which('xdotool') is None:
    parser.error('display timing requires xdotool')
args.game = args.game.resolve()
args.build = args.build.resolve()
args.prefix = args.prefix.resolve()
wine_loader = shutil.which(args.wine)
if wine_loader is None:
    parser.error('Wine loader is unavailable')
wine_loader = Path(wine_loader).resolve()
wine_server = args.wineserver or wine_loader.parent / 'wineserver'
if not wine_server.is_file():
    parser.error('provide --wineserver matching the selected Wine loader')
wine_server = wine_server.resolve()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=False)
exe = args.game / 'th06.exe'
pinned = '9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245'
if hashlib.sha256(exe.read_bytes()).hexdigest() != pinned:
    parser.error('retail executable identity mismatch')
if not (args.prefix / 'system.reg').exists():
    parser.error('initialize the dedicated Wine prefix first')
prefix_arch = next((line.strip()[6:] for line in (args.prefix / 'system.reg').read_text().splitlines()
                    if line.startswith('#arch=')), None)
if prefix_arch != ('win32' if args.wine_arch == 'win32' else 'win64'):
    parser.error('prefix architecture does not match the requested Wine mode')
if not (args.game / args.replay).is_file():
    parser.error('replay does not exist under the game directory')
shared_lock = Path(__file__).resolve().parents[1] / 'reports/local/.retail.lock'
shared_lock.parent.mkdir(parents=True, exist_ok=True)
runtime_ownership = shared_lock.open('a')
ownership = (args.prefix.parent / '.retail.lock').open('a')
try:
    fcntl.flock(runtime_ownership, fcntl.LOCK_EX | fcntl.LOCK_NB)
    fcntl.flock(ownership, fcntl.LOCK_EX | fcntl.LOCK_NB)
except BlockingIOError:
    parser.error('another owned retail replay is already running')
frozen = args.output / 'bin'
frozen.mkdir()
binary_hashes = {}
sources = {name: args.build / 'wine-replay' / name for name in ['launch.exe', 'observer.dll']}
sources.update({'run_wine_replay.py': Path(__file__).resolve(),
                'replay_pacing.py': Path(__file__).with_name('replay_pacing.py'),
                'replay_display.py': Path(__file__).with_name('replay_display.py')})
for name, source in sources.items():
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
           'WINEPREFIX=' + str(args.prefix), 'WINEARCH=' + args.wine_arch, 'WINESERVER=' + str(wine_server),
           'WINEDEBUG=-all,+winediag',
           'WINEDLLOVERRIDES=' + args.dll_overrides, 'LANG=ja_JP.UTF-8', 'LC_ALL=ja_JP.UTF-8',
           'LP_NUM_THREADS=' + str(args.renderer_threads), 'LIBGL_ALWAYS_SOFTWARE=1',
           'MESA_SHADER_CACHE_DISABLE=' + ('false' if args.shader_cache else 'true'),
           'TH06_RETAIL_REPLAY=' + args.replay, 'TH06_RETAIL_UPDATES=' + str(args.updates),
           'TH06_REPLAY_START_STAGE=' + str(args.start_stage),
           'TH06_REPLAY_SINGLE_STAGE=' + str(int(args.single_stage)),
           'TH06_REPLAY_RENDER_TIMING=' + str(int(args.render_timing)),
           'TH06_REPLAY_PRECISE_LIMITER=' + str(int(args.precise_limiter)),
           'TH06_REPLAY_SLEEP_LIMITER=' + str(int(args.sleep_limiter)),
           'TH06_REPLAY_SOFTWARE_BULLETS=' + str(int(args.software_bullets)),
           'TH06_REPLAY_BATCH_BULLETS=' + str(int(args.batch_bullets)),
           '/usr/bin/xvfb-run', '-a', sys.executable, str(frozen / 'run_wine_replay.py'),
           '--worker', str(args.output)]
render_names = ['WINE_D3D_CONFIG', 'mesa_glthread', 'GALLIUM_THREAD', 'LP_NATIVE_VECTOR_WIDTH', 'VK_ICD_FILENAMES',
                'LD_LIBRARY_PATH', 'LIBGL_DRIVERS_PATH']
normalized = render_names + ['vblank_mode', 'MESA_SHADER_CACHE_DIR', 'MESA_SHADER_CACHE_MAX_SIZE']
env_start = command.index('/usr/bin/env') + 1
command[env_start:env_start] = [item for name in normalized for item in ['-u', name]]
wine_only = ['LD_LIBRARY_PATH', 'LIBGL_DRIVERS_PATH']
extra = [name + '=' + os.environ[name] for name in render_names if name in os.environ and name not in wine_only]
vblank = '0' if args.vblank == 'off' else os.environ.get('vblank_mode')
if vblank is not None:
    extra.append('vblank_mode=' + vblank)
if args.shader_cache:
    extra.extend(['MESA_SHADER_CACHE_DIR=' + str(args.shader_cache), 'MESA_SHADER_CACHE_MAX_SIZE=64M'])
index = command.index('/usr/bin/xvfb-run')
command[index:index] = extra
if args.display_cpus:
    index = command.index('/usr/bin/xvfb-run')
    command[index:index] = ['/usr/bin/taskset', '--cpu-list', ','.join(map(str, args.display_cpus))]
expected_environment = {**{name: os.environ.get(name) for name in render_names},
                        'WINEPREFIX': str(args.prefix), 'WINEARCH': args.wine_arch, 'WINESERVER': str(wine_server),
                        'WINEDLLOVERRIDES': args.dll_overrides,
                        'vblank_mode': vblank, 'LP_NUM_THREADS': str(args.renderer_threads),
                        'LIBGL_ALWAYS_SOFTWARE': '1',
                        'MESA_SHADER_CACHE_DIR': str(args.shader_cache) if args.shader_cache else None,
                        'MESA_SHADER_CACHE_MAX_SIZE': '64M' if args.shader_cache else None,
                        'MESA_SHADER_CACHE_DISABLE': 'false' if args.shader_cache else 'true',
                        'TH06_REPLAY_RENDER_TIMING': str(int(args.render_timing)),
                        'TH06_REPLAY_PRECISE_LIMITER': str(int(args.precise_limiter)),
                        'TH06_REPLAY_SLEEP_LIMITER': str(int(args.sleep_limiter)),
                        'TH06_REPLAY_SOFTWARE_BULLETS': str(int(args.software_bullets)),
                        'TH06_REPLAY_BATCH_BULLETS': str(int(args.batch_bullets))}
worker_environment = dict(expected_environment, **{name: None for name in wine_only})
config = {'expected_environment': worker_environment, 'expected_wine_environment': expected_environment,
          'wine_nice': args.wine_nice,
          'wine_cpus': args.wine_cpus, 'display_cpus': args.display_cpus,
          'wine_environment': {name: expected_environment[name] for name in wine_only},
          'snapshot': args.snapshot,
          'display_timing': args.display_timing,
          'wineserver': str(wine_server),
          'wine': [str(wine_loader), str(frozen / 'launch.exe'), 'th06.exe',
                   windows(frozen / 'observer.dll'), windows(args.output)]}
(args.output / 'worker.json').write_text(json.dumps(config, indent=2) + '\n')
receipt = {'unit': unit, 'memory_max_bytes': 2 * 1024**3, 'swap_max_bytes': 0,
           'peak_cgroup_bytes': 0, 'oom': 0, 'oom_kill': 0,
           'cgroup_cpu_usage_usec': 0,
           'cgroup_limits_verified': False,
           'retail_sha256': pinned, 'replay_sha256': hashlib.sha256((args.game / args.replay).read_bytes()).hexdigest(),
           'binaries': binary_hashes, 'mode': 'retail-rpy-playback', 'start_stage': args.start_stage,
           'wine_arch': args.wine_arch, 'wine_loader': str(wine_loader), 'wineserver': str(wine_server),
           'wine_nice': args.wine_nice,
           'wine_cpus': args.wine_cpus, 'display_cpus': args.display_cpus,
           'single_stage': args.single_stage, 'precise_limiter': args.precise_limiter,
           'sleep_limiter': args.sleep_limiter,
           'software_bullets': args.software_bullets, 'batch_bullets': args.batch_bullets,
           'render_environment': expected_environment, 'native_controller_running': False,
           'shader_cache_before': cache_state(args.shader_cache),
           'config_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in args.game.iterdir() if p.suffix.lower() == '.cfg'},
           'game_runtime_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in args.game.iterdir() if p.suffix.lower() in ('.dll', '.conf')},
           'dat_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in args.game.iterdir() if p.suffix.lower() == '.dat'},
           'host_load_average_start': os.getloadavg(),
           'host_cpu_pressure_start': Path('/proc/pressure/cpu').read_text()}
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
                try:
                    usage = dict(line.split() for line in (cgroup / 'cpu.stat').read_text().splitlines())
                    receipt['cgroup_cpu_usage_usec'] = max(receipt['cgroup_cpu_usage_usec'], int(usage['usage_usec']))
                except FileNotFoundError:
                    pass
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
receipt['shader_cache_after'] = cache_state(args.shader_cache)
receipt['host_load_average_end'] = os.getloadavg()
receipt['host_cpu_pressure_end'] = Path('/proc/pressure/cpu').read_text()
state = subprocess.run(['systemctl', '--user', 'show', unit, '-p', 'Result', '-p', 'ExecMainStatus'],
                       capture_output=True, text=True, check=True)
receipt['systemd'] = dict(line.split('=', 1) for line in state.stdout.splitlines())
receipt['synchronous_command_stream_confirmed'] = ('Setting multithreaded command stream to 0.'
                                                  in (args.output / 'process.log').read_text(errors='replace'))
if (args.output / 'outcome.json').exists():
    receipt['outcome'] = json.loads((args.output / 'outcome.json').read_text())
(args.output / 'resource-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
if args.render_timing:
    try:
        pacing = analyze(args.output)
        (args.output / 'pacing.json').write_text(json.dumps(pacing, indent=2) + '\n')
        receipt['pacing'] = pacing
    except (ValueError, OSError) as error:
        receipt['pacing_error'] = str(error)
if args.native_trace and (args.output / 'retail.csv').exists():
    comparison = compare_native(args.native_trace, args.output / 'retail.csv',
                                args.start_stage, args.single_stage)
    receipt['native_comparison'] = comparison
    (args.output / 'native-comparison.json').write_text(json.dumps(comparison, indent=2) + '\n')
(args.output / 'resource-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt, indent=2), flush=True)
raise SystemExit(child.returncode or (1 if receipt['oom_kill'] or not receipt['cgroup_limits_verified']
                                      or not receipt.get('outcome', {}).get('retail_nmnb_complete')
                                      or receipt.get('native_comparison', {}).get('fields_match') is False
                                      or (args.native_trace and not receipt.get('native_comparison', {}).get('complete_route'))
                                      or (args.require_pacing and not receipt.get('pacing', {}).get('acceptance_gate'))
                                      or (args.require_pacing and args.display_timing and not receipt.get('pacing', {}).get('display_gate')) else 0))
