#!/usr/bin/env python3
"""Read actual retail update/presentation clocks; capture timestamps are separate."""
import argparse
import bisect
import json
from pathlib import Path
import statistics
import struct
import csv
import math
import mmap
import os
from itertools import zip_longest

UPDATE = struct.Struct('<3Q')
RENDER_HEADER = struct.Struct('<4I')
RENDER = struct.Struct('<8Q')


def journal_count(data, magic, stride):
    if len(data) < 16:
        raise ValueError('truncated journal header')
    found, version, width, count = RENDER_HEADER.unpack_from(data)
    if found != magic or width != stride or (len(data) - 16) % stride:
        raise ValueError('invalid journal layout')
    capacity = (len(data) - 16) // stride
    if version == 1 and count == 0:
        return capacity
    if version == 2 and count <= capacity:
        return count
    raise ValueError('invalid journal version or committed count')


def journal_rows(path, magic, record):
    # A mapped v2 file can retain an unused tail after a crash. Only a published
    # record is evidence; normal close truncates the tail. Old v1 remains strict.
    with Path(path).open('rb') as file:
        if not os.fstat(file.fileno()).st_size:
            raise ValueError('empty journal')
        with mmap.mmap(file.fileno(), 0, access=mmap.ACCESS_READ) as data:
            count = journal_count(data, magic, record.size)
            for index in range(count):
                yield record.unpack_from(data, 16 + index * record.size)


class LiveUpdates:
    """Read the last immutable, committed record without per-event file opens."""
    def __init__(self, path):
        self.path, self.file, self.mapping = Path(path), None, None

    def latest(self):
        if self.file is None:
            try:
                self.file = self.path.open('rb', buffering=0)
            except FileNotFoundError:
                return (0, 0, 0)
        if self.mapping is None:
            self.file.seek(0)
            header = self.file.read(16)
            if len(header) < 16 or not any(header):
                return (0, 0, 0)
            magic, version, width, count = RENDER_HEADER.unpack(header)
            if (magic, width) != (0x36545055, 24) or version not in (1, 2):
                raise ValueError('invalid live update journal')
            if version == 2:
                self.mapping = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
            else:
                size = os.fstat(self.file.fileno()).st_size
                if size < 40:
                    return (0, 0, 0)
                self.file.seek(16 + ((size - 16) // 24 - 1) * 24)
                return UPDATE.unpack(self.file.read(24))
        count = struct.unpack_from('<I', self.mapping, 12)[0]
        if count > (len(self.mapping) - 16) // 24:
            raise ValueError('live count exceeds mapped journal capacity')
        if not count:
            return (0, 0, 0)
        row = UPDATE.unpack_from(self.mapping, 16 + (count - 1) * 24)
        if row[0] != count or not 1 <= row[1] <= 6:
            raise ValueError('invalid committed live record')
        return row

    def close(self):
        if self.mapping:
            self.mapping.close()
        if self.file:
            self.file.close()


def export_state_csv(directory):
    directory = Path(directory)
    path = directory / 'retail-states.bin'
    if not path.exists():
        return
    record = struct.Struct('<4I2fi3I2iI2i')
    with (directory / 'retail.csv').open('x', newline='') as file:
        writer = csv.writer(file)
        writer.writerow('frame stage local_frame input x y timeline rng rng_calls power deaths bombs score rank player_state'.split())
        for row in journal_rows(path, 0x36545354, record):
            writer.writerow([*row[:4], format(row[4], '.9g'), format(row[5], '.9g'), *row[6:]])


def update_rows(path):
    rows = list(journal_rows(path, 0x36545055, UPDATE))
    if not rows:
        raise ValueError('empty update journal')
    for i, row in enumerate(rows, 1):
        if row[0] != i or not 1 <= row[1] <= 6:
            raise ValueError('invalid retail journal sequence/version')
    return rows


def distribution(values):
    values = sorted(values)
    if not values:
        return None
    def percentile(fraction):
        return values[int((len(values) - 1) * fraction)]
    return {'count': len(values), 'mean_ms': statistics.fmean(values),
            'p50_ms': percentile(.5), 'p95_ms': percentile(.95),
            'p99_ms': percentile(.99), 'max_ms': values[-1],
            'above_20_ms': sum(x > 20 for x in values),
            'above_33_3_ms': sum(x > 100 / 3 for x in values),
            'above_50_ms': sum(x > 50 for x in values)}


def cadence(times):
    if len(times) < 2 or any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError('insufficient or nonmonotonic retail timestamps')
    gaps = [(b - a) / 1e6 for a, b in zip(times, times[1:])]
    # Complete five-second windows, shifted one second at a time. Keep every
    # outlier in the full distribution; this is not an outlier-trimmed rate.
    windows = []
    start = times[0]
    while start + 5000000000 <= times[-1]:
        count = bisect.bisect_right(times, start + 5000000000) - bisect.bisect_right(times, start)
        windows.append(count / 5)
        start += 1000000000
    return {'samples': len(times), 'seconds': (times[-1] - times[0]) / 1e9,
            'mean_hz': (len(times) - 1) * 1e9 / (times[-1] - times[0]),
            'slowest_5s_hz': min(windows) if windows else None,
            'five_second_windows': len(windows), 'intervals': distribution(gaps)}


def analyze(directory, warmup=120):
    directory = Path(directory)
    rows = update_rows(directory / 'update-times.bin')
    update_times = [r[2] for r in rows]
    report = {'frames': len(rows), 'update_full': cadence(update_times),
              'warmup_updates': warmup, 'clock': 'retail QueryPerformanceCounter',
              'exclusions': 'steady omits initial warmup updates only; full retains all gaps'}
    report['pacing_gate_definition'] = {
        'version': 4, 'minimum_seconds': 30, 'mean_hz_range': [59.5, 60.5],
        'slowest_5s_min_hz': 58, 'present_p99_max_ms': 20,
        'required': 'no failed/repeated/absent draws; original replay callback counts reported separately',
        'update_phase': 'post-gameplay, before original limiter wait; report jitter separately from presentation'}
    report['pacing_gate_definition']['completion_boundary'] = 'confirmed synchronous Wine command stream or independent window update gate; a requested window clock must pass'
    steady = [t for r, t in zip(rows, update_times) if r[0] > warmup]
    if len(steady) > 1:
        report['update_steady'] = cadence(steady)
    path = directory / 'render-times.bin'
    if path.exists():
        samples = list(journal_rows(path, 0x36544d52, RENDER))
        if any(not 1 <= r[0] <= len(rows) or r[1] != rows[r[0] - 1][1] or r[7] & ~7 for r in samples):
            raise ValueError('render context does not match retail journal')
        report['render_sequence_skips'] = sum(max(0, b[0] - a[0] - 1) for a, b in zip(samples, samples[1:]))
        report['render_flags'] = {str(flag): sum(bool(r[7] & flag) for r in samples) for flag in (1, 2, 4)}
        report['present_full'] = cadence([r[2] for r in samples])
        selected = [r for r in samples if r[0] > warmup]
        if len(selected) > 1:
            rate = cadence([r[2] for r in selected])
            report['present_steady'] = rate
            report['draw'] = distribution([r[4] / 1e6 for r in selected])
            report['present_call'] = distribution([r[5] / 1e6 for r in selected])
            # Local shared-host acceptance target, not an every-frame 60-Hz or
            # all-machine guarantee. Gameplay and independent replay gate separately.
            report['pacing_gate'] = (rate['seconds'] >= 30 and 59.5 <= rate['mean_hz'] <= 60.5
                                     and rate['slowest_5s_hz'] >= 58
                                     and rate['intervals']['p99_ms'] <= 20
                                     and not any(report['render_flags'].values()))
            display_path = directory / 'display-times.csv'
            if display_path.exists():
                with display_path.open() as file:
                    display = [{name: int(value) for name, value in row.items()}
                               for row in csv.DictReader(file)]
                gameplay = [r for r in display if r['latest_update'] > 0]
                observed = [r for r in gameplay if r['latest_update'] > warmup]
                report['display_observation'] = {
                    'source': 'XDamageReportNonEmpty on the owned game window; no periodic sampling',
                    'clock': 'X server milliseconds; receive monotonic nanoseconds reported separately',
                    'raw_events': len(display), 'gameplay_events': len(gameplay),
                    'count_difference_from_present': len(observed) - len(selected),
                    'non_full_window_events': sum(r['x'] != 0 or r['y'] != 0 or r['width'] != 640
                                                  or r['height'] != 480 or r['geometry_width'] != 640
                                                  or r['geometry_height'] != 480 for r in observed)}
                report['display_gate'] = False
                try:
                    report['display_full'] = cadence([r['server_ms'] * 1000000 for r in gameplay])
                    shown = cadence([r['server_ms'] * 1000000 for r in observed])
                    report['display_steady'] = shown
                    report['display_receive_steady'] = cadence([r['received_ns'] for r in observed])
                    report['display_gate'] = (shown['seconds'] >= 30 and 59.5 <= shown['mean_hz'] <= 60.5
                                              and shown['slowest_5s_hz'] >= 58
                                              and shown['intervals']['p99_ms'] <= 20
                                              and abs(len(observed) - len(selected)) <= 3
                                              and not report['display_observation']['non_full_window_events'])
                except ValueError as error:
                    report['display_error'] = str(error)
    log = directory / 'process.log'
    report['synchronous_command_stream_confirmed'] = (log.exists() and
        'Setting multithreaded command stream to 0.' in log.read_text(errors='replace'))
    report['acceptance_gate'] = (report.get('pacing_gate', False)
        and (report.get('display_gate', False) if (directory / 'display-times.csv').exists()
             else report['synchronous_command_stream_confirmed']))
    return report


def compare(directories):
    reference = list(csv.DictReader((Path(directories[0]) / 'retail.csv').open()))
    result = []
    for directory in directories[1:]:
        rows = list(csv.DictReader((Path(directory) / 'retail.csv').open()))
        mismatch = next((i + 1 for i, (a, b) in enumerate(zip(reference, rows)) if a != b), None)
        result.append({'case': str(directory), 'same_projected_route': len(rows) == len(reference) and mismatch is None,
                       'frames': len(rows), 'first_difference': mismatch})
    return {'reference': str(directories[0]), 'comparisons': result}


def compare_native(native_path, retail_path, start_stage=1, single_stage=False):
    difference = None
    expected_rows = actual_rows = 0
    # Keep full-story validation bounded independently of route length. Continue
    # counting after a mismatch so failed/prefix receipts retain exact scope.
    with Path(native_path).open() as native_file, Path(retail_path).open() as retail_file:
        native = csv.DictReader(native_file)
        actual = csv.DictReader(retail_file)
        fields = native.fieldnames
        if fields != actual.fieldnames:
            return {'fields_match': False, 'complete_route': False, 'error': 'trace field mismatch'}
        expected = (row for row in native if int(row['stage']) >= start_stage
                    and (not single_stage or int(row['stage']) == start_stage))
        for i, (target, row) in enumerate(zip_longest(expected, actual), 1):
            expected_rows += target is not None
            actual_rows += row is not None
            if difference is not None or row is None:
                continue
            if target is None:
                difference = {'frame': i, 'field': 'extra_retail_row'}
                continue
            target['frame'] = str(i)
            if set(row) != set(fields):
                difference = {'frame': i, 'field': 'malformed_retail_row'}
                continue
            for name in fields:
                try:
                    a, b = float(row[name]), float(target[name])
                    same = math.isfinite(a) and math.isfinite(b) and (abs(a-b) <= .025 if name in ('x', 'y') else a == b)
                except (ValueError, TypeError):
                    same = False
                if not same:
                    difference = {'frame': i, 'field': name, 'native': target[name], 'retail': row[name]}
                    break
    if not expected_rows or not actual_rows:
        return {'fields_match': False, 'complete_route': False, 'error': 'empty trace'}
    return {'fields_match': difference is None, 'complete_route': difference is None and actual_rows == expected_rows,
            'rows': actual_rows, 'expected_rows': expected_rows, 'first_difference': difference,
            'start_stage': start_stage, 'single_stage': single_stage}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directories', type=Path, nargs='+')
    parser.add_argument('--compare', action='store_true')
    parser.add_argument('--warmup', type=int, default=120)
    args = parser.parse_args()
    if args.warmup < 0:
        parser.error('warmup must be nonnegative')
    result = compare(args.directories) if args.compare else {str(p): analyze(p, args.warmup) for p in args.directories}
    print(json.dumps(result, indent=2))
