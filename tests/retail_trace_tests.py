import csv
import importlib.util
import json
from pathlib import Path
import tempfile

path = Path(__file__).resolve().parents[1] / 'scripts/compare_retail_trace.py'
spec = importlib.util.spec_from_file_location('retail_compare', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    rows = []
    for i in range(1, 7):
        row = dict.fromkeys(module.FIELDS, '0')
        row.update(frame=str(i), stage=str(i), input='261', x='192', y='384')
        rows.append(row)

    def write(name, values, fields=module.FIELDS):
        with (root / name).open('w', newline='') as file:
            writer = csv.DictWriter(file, fieldnames=fields)
            writer.writeheader()
            writer.writerows(values)

    write('native.csv', rows)
    write('retail.csv', rows)
    outcome = {'reason': 'complete', 'frames': 6, 'stage': 6, 'cleared_stages': 6,
               'deaths': 0, 'bombs': 0, 'supervisor_state': 8, 'retail_nmnb_complete': True}
    (root / 'outcome.json').write_text(json.dumps(outcome))
    n, r, o = root / 'native.csv', root / 'retail.csv', root / 'outcome.json'
    assert module.compare(n, r, o)['complete']
    assert not module.compare(n, r)['complete']
    write('retail.csv', rows[:3])
    prefix = module.compare(n, r, o)
    assert prefix['fields_match'] and not prefix['complete']
    changed = [dict(row) for row in rows]
    changed[2]['rng'] = '42'
    write('retail.csv', changed)
    assert module.compare(n, r, o)['first_difference']['frame'] == 3
    write('retail.csv', rows)
    outcome['cleared_stages'] = 5
    o.write_text(json.dumps(outcome))
    assert not module.compare(n, r, o)['complete']
    with r.open('a') as file:
        file.write('7,6,')
    assert not module.compare(n, r, o, allow_prefix=True)['complete']
    try:
        module.compare(n, r)
        raise AssertionError('truncated retail row accepted')
    except ValueError:
        pass
print('retail scalar comparison contracts passed')
