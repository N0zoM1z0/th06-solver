#!/usr/bin/env python3
"""Extract and natively verify a captured online query's acknowledged queue."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def unsigned(value, maximum=2**64 - 1):
    if type(value) is not int or not 0 <= value <= maximum:
        raise ValueError('invalid unsigned capture value')
    return value


def journal(path):
    text = path.read_text()
    if text and not text.endswith('\n'):
        raise ValueError('incomplete capture journal')
    return [json.loads(line) for line in text.splitlines()]


def extract(directory, origin):
    directory = Path(directory)
    lines = (directory / 'actual.actions').read_text().splitlines()
    if len(lines) < 3 or lines[0] != 'TH06ACTIONS 2 native-i386-v1':
        raise ValueError('unsupported actual tape')
    start = [int(value) for value in lines[1].split()]
    if len(start) != 6:
        raise ValueError('invalid start tuple')
    fingerprint, count, complete = map(int, lines[2].split())
    unsigned(fingerprint)
    unsigned(count, 1000000)
    if complete not in (0, 1) or len(lines) != count + 3:
        raise ValueError('actual tape length/completion mismatch')
    actual = []
    for number, line in enumerate(lines[3:], 1):
        frame, input_value, digest = map(int, line.split())
        if frame != number or unsigned(input_value, 65535) & ~0x1f5:
            raise ValueError('invalid actual tape row')
        actual.append((input_value, unsigned(digest)))
    unsigned(origin, len(actual))
    decisions = [row for row in journal(directory / 'decisions.jsonl')
                 if row['origin'] == origin]
    if len(decisions) != 1:
        raise ValueError('origin must identify exactly one captured query')
    decision = decisions[0]
    generation = unsigned(decision['active_generation'], 2**32 - 1)
    if not generation:
        raise ValueError('query has no acknowledged certificate generation')
    certificates = journal(directory / 'certificates.jsonl')
    for number, row in enumerate(certificates, 1):
        if (unsigned(row['version']) != 1 or
                unsigned(row['generation'], 2**32 - 1) != number * 2):
            raise ValueError('certificate generation/schema mismatch')
    matches = [row for row in certificates if row['generation'] == generation]
    if len(matches) != 1:
        raise ValueError('acknowledged certificate is missing')
    certificate = matches[0]
    for name in ('origin', 'stage', 'first_change', 'count', 'lead', 'horizon'):
        unsigned(certificate[name], 2**32 - 1)
    begin, size = certificate['origin'], certificate['count']
    lead, horizon = certificate['lead'], certificate['horizon']
    if (not 1 <= certificate['stage'] <= 7 or not 1 <= size <= 600 or
            not 1 <= lead <= 64 or not 8 <= horizon <= 600 - lead or
            not lead < size <= lead + horizon or
            certificate['first_change'] != begin + lead + 1 or begin > origin):
        raise ValueError('invalid acknowledged certificate bounds')
    inputs, digests = certificate['inputs'], certificate['digests']
    if len(inputs) != size or len(digests) != size:
        raise ValueError('certificate input/digest length mismatch')
    for value in inputs:
        if unsigned(value, 65535) & ~0x1f5:
            raise ValueError('invalid certificate input')
    for value in digests:
        unsigned(value)
    future = list(zip(inputs, digests))
    for frame in range(begin, min(origin, begin + size)):
        if actual[frame] != future[frame - begin]:
            raise ValueError('acknowledged certificate disagrees with actual prefix')
    remaining = max(0, begin + size - origin)
    if unsigned(decision['queue_before'], 600) != remaining:
        raise ValueError('captured remaining queue disagrees with certificate')
    rows = actual[:origin] + (future[origin - begin:] if remaining else [])
    metadata = {'type': 'captured_native_queue', 'retail_proof': False,
                'origin': origin, 'generation': generation, 'remaining': remaining,
                'certificate_origin': begin, 'certificate_end': begin + size,
                'lead': lead, 'horizon': horizon, 'native_verified': False,
                'actual_tape_sha256': hashlib.sha256(
                    (directory / 'actual.actions').read_bytes()).hexdigest(),
                'certificate_journal_sha256': hashlib.sha256(
                    (directory / 'certificates.jsonl').read_bytes()).hexdigest()}
    return lines[:2], fingerprint, rows, metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--origin', type=int, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--checker', type=Path, default=Path('build/th06_replay_check'))
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = args.output or args.directory / ('queue-' + str(args.origin) + '.actions')
    proof_path = Path(str(output) + '.json')
    if output.exists() or proof_path.exists():
        parser.error('output already exists; preserve earlier evidence')
    header, fingerprint, rows, metadata = extract(args.directory, args.origin)
    checker = args.checker.resolve()
    checker_hash = hashlib.sha256(checker.read_bytes()).hexdigest()
    metadata['checker_sha256'] = checker_hash
    metadata['output'] = str(output.resolve())
    metadata['checks'] = []
    for complete in (False, True):
        candidate = Path(str(output) + ('.candidate-complete' if complete else '.candidate'))
        with candidate.open('x') as file:
            file.write('\n'.join(header) + '\n')
            file.write(f'{fingerprint} {len(rows)} {int(complete)}\n')
            for frame, (input_value, digest) in enumerate(rows, 1):
                file.write(f'{frame} {input_value} {digest}\n')
        command = [str(checker), '--data', str(args.data.resolve()),
                   '--tape', str(candidate.resolve())]
        if complete:
            command.append('--require-complete')
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        if hashlib.sha256(checker.read_bytes()).hexdigest() != checker_hash:
            raise ValueError('native checker changed during verification')
        metadata['checks'].append({'command': command, 'exit': result.returncode,
                                   'stdout': result.stdout, 'stderr': result.stderr})
        if result.returncode == 0:
            metadata['native_verified'] = True
            metadata['native_complete'] = complete
            candidate.rename(output)
            metadata['output_tape_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
            break
        if complete or 'tape completion mismatch' not in result.stderr:
            break
    proof_path.write_text(json.dumps(metadata, indent=2) + '\n')
    print(json.dumps(metadata, indent=2))
    return 0 if metadata['native_verified'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
