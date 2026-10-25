#!/usr/bin/env python3
"""Compare the declared scalar projection; this is not a hidden-state oracle."""
import argparse
import csv
import itertools
import json
from pathlib import Path

FIELDS = 'frame,stage,local_frame,input,x,y,timeline,rng,rng_calls,power,deaths,bombs,score,rank,player_state'.split(',')


def compare(native, retail, outcome=None, allow_prefix=False):
    result = {'native_frames': 0, 'retail_frames': 0, 'compared_frames': 0,
              'first_difference': None, 'fields_match': True, 'complete': False}
    last_stage = 0
    with Path(native).open(newline='') as nf, Path(retail).open(newline='') as rf:
        n, r = csv.DictReader(nf), csv.DictReader(rf)
        if n.fieldnames != FIELDS or r.fieldnames != FIELDS:
            raise ValueError('unsupported scalar trace header')
        retail_ended = False
        for a, b in itertools.zip_longest(n, r):
            if a is not None:
                result['native_frames'] += 1
                if None in a or any(v is None for v in a.values()):
                    raise ValueError('malformed native trace row')
            if b is not None and not retail_ended:
                if None in b or any(v is None for v in b.values()):
                    if not allow_prefix:
                        raise ValueError('malformed retail trace row')
                    retail_ended = True
                else:
                    result['retail_frames'] += 1
            if retail_ended or b is None:
                continue
            if a is None:
                result['first_difference'] = result['first_difference'] or {'reason': 'retail exceeds native trace'}
                continue
            result['compared_frames'] += 1
            last_stage = int(b['stage'])
            differences = {k: {'native': a[k], 'retail': b[k]} for k in FIELDS if a[k] != b[k]}
            if differences and result['first_difference'] is None:
                result['first_difference'] = {'frame': int(a['frame']), 'fields': differences}
    result['fields_match'] = result['first_difference'] is None
    if outcome is not None:
        event = json.loads(Path(outcome).read_text())
        result['outcome'] = event
        result['complete'] = bool(
            result['fields_match'] and not retail_ended and event.get('reason') == 'complete' and
            event.get('retail_nmnb_complete') is True and event.get('deaths') == 0 and event.get('bombs') == 0 and
            last_stage in (5, 6) and event.get('cleared_stages') == last_stage and event.get('stage') == last_stage and
            event.get('supervisor_state') == 8 and
            event.get('frames') == result['native_frames'] == result['retail_frames'] > 0)
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--retail', type=Path, required=True)
    p.add_argument('--outcome', type=Path)
    p.add_argument('--output', type=Path)
    p.add_argument('--allow-prefix', action='store_true')
    p.add_argument('--require-complete', action='store_true')
    a = p.parse_args()
    try:
        result = compare(a.native, a.retail, a.outcome, a.allow_prefix)
    except (OSError, ValueError) as error:
        p.exit(1, str(error) + '\n')
    text = json.dumps(result, indent=2) + '\n'
    if a.output:
        a.output.write_text(text)
    print(text, end='')
    return 1 if not result['fields_match'] else 2 if a.require_complete and not result['complete'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
