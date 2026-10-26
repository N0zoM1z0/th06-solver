import importlib.util
from pathlib import Path
import struct
import tempfile
import subprocess
import sys
import json
import csv

spec = importlib.util.spec_from_file_location('pacing', Path(__file__).resolve().parents[1] / 'scripts/replay_pacing.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)



def rejects(fn):
    try:
        fn()
    except ValueError:
        return
    raise AssertionError('invalid timing/reference data accepted')


with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    updates = bytearray(module.RENDER_HEADER.pack(0x36545055, 1, 24, 0))
    renders = bytearray(module.RENDER_HEADER.pack(0x36544d52, 1, 64, 0))
    for i in range(1, 2401):
        ns = round((i - 1) * 1e9 / 60)
        updates.extend(module.UPDATE.pack(i, 5, ns))
        renders.extend(module.RENDER.pack(i, 5, ns + 1000000, 16666667, 2000000, 50000, 1, 0))
    (root / 'update-times.bin').write_bytes(updates)
    (root / 'render-times.bin').write_bytes(renders)
    report = module.analyze(root)
    assert report['pacing_gate'] and abs(report['present_steady']['mean_hz'] - 60) < .001
    assert report['present_call']['mean_ms'] == .05
    assert not report['acceptance_gate']  # Async API receipts alone prove no completed presentation.
    (root / 'process.log').write_text('Setting multithreaded command stream to 0.\n')
    assert module.analyze(root)['acceptance_gate']
    with (root / 'display-times.csv').open('w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['event', 'server_ms', 'received_ns', 'latest_update', 'stage', 'x', 'y',
                         'width', 'height', 'geometry_width', 'geometry_height'])
        for i in range(1, 2401):
            ns = round((i - 1) * 1e9 / 60)
            writer.writerow([i, round(ns / 1e6), ns, i, 5, 0, 0, 640, 480, 640, 480])
    report = module.analyze(root)
    assert report['display_gate'] and report['acceptance_gate']
    (root / 'process.log').unlink()
    assert module.analyze(root)['acceptance_gate']  # Independent output validates async presentation.
    (root / 'display-times.csv').unlink()
    # A slow actual game cannot pass because capture/search timing looks fast.
    rejects(lambda: module.cadence([0, 1, 1]))
    for i in range(2400):
        offset = 16 + i * 64
        row = list(module.RENDER.unpack(renders[offset:offset + 64]))
        row[2] = round(i * 1e9 / 30)
        renders[offset:offset + 64] = module.RENDER.pack(*row)
    (root / 'render-times.bin').write_bytes(renders)
    assert not module.analyze(root)['pacing_gate']
    (root / 'render-times.bin').write_bytes(renders[:-1])
    rejects(lambda: module.analyze(root))
    (root / 'update-times.bin').write_bytes(updates[:-1])
    rejects(lambda: module.update_rows(root / 'update-times.bin'))
    fields = ['frame', 'stage', 'local_frame', 'input', 'x', 'y', 'timeline', 'rng',
              'rng_calls', 'power', 'deaths', 'bombs', 'score', 'rank', 'player_state']
    native = [dict.fromkeys(fields, '0') for _ in range(3)]
    for i, row in enumerate(native, 1):
        row.update(frame=str(i + 100), stage='5', local_frame=str(i-1), input='261', x='192', y='384')
    def write(path, rows):
        with path.open('w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            writer.writerows(rows)
    n, r = root / 'native.csv', root / 'retail.csv'
    write(n, native)
    actual = [dict(row, frame=str(i)) for i, row in enumerate(native, 1)]
    write(r, actual)
    assert module.compare_native(n, r, 5, True)['complete_route']
    write(r, actual[:2])
    assert module.compare_native(n, r, 5, True)['fields_match']
    assert not module.compare_native(n, r, 5, True)['complete_route']
    actual[1]['rng'] = '1'
    write(r, actual)
    assert module.compare_native(n, r, 5, True)['first_difference'] == {'frame': 2, 'field': 'rng', 'native': '0', 'retail': '1'}
    assert module.compare_native(n, r, 5, True)['expected_rows'] == 3
    actual[1]['rng'] = '0'
    write(r, actual + [actual[-1]])
    extra = module.compare_native(n, r, 5, True)
    assert not extra['fields_match'] and extra['first_difference'] == {'frame': 4, 'field': 'extra_retail_row'}
    for position in ('nan', 'inf', '192.026'):
        actual[1]['x'] = position
        write(r, actual)
        assert module.compare_native(n, r, 5, True)['first_difference']['field'] == 'x'
    actual[1]['x'] = '192.024'
    write(r, actual)
    assert module.compare_native(n, r, 5, True)['complete_route']
    write(r, [])
    assert module.compare_native(n, r, 5, True)['error'] == 'empty trace'
    # A declared setting missing at the service boundary must fail before
    # even attempting a controller or Wine launch (no launch keys supplied).
    (root / 'worker.json').write_text(json.dumps({'expected_environment': {'TH06_PACING_TEST_MISSING': 'required'}}))
    result = subprocess.run([sys.executable, str(Path(__file__).resolve().parents[1] / 'scripts/run_wine_replay.py'),
                             '--worker', str(root)], capture_output=True, text=True)
    assert result.returncode and 'did not receive the declared rendering environment' in result.stderr
    assert json.loads((root / 'effective-environment.json').read_text())['TH06_PACING_TEST_MISSING'] is None
print('retail replay pacing/comparison contracts passed')
