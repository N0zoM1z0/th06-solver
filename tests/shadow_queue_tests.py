import copy
import importlib.util
import json
from pathlib import Path
import tempfile

path = Path(__file__).resolve().parents[1] / 'scripts/shadow_queue.py'
spec = importlib.util.spec_from_file_location('shadow_queue', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

with tempfile.TemporaryDirectory() as temp:
    root = Path(temp)
    # Deliberately synthetic scalars test capture contracts, not native safety.
    rows = [(261, 2**64 - 1), (261, 7), (325, 8), (325, 9)]
    (root / 'actual.actions').write_text(
        'TH06ACTIONS 2 native-i386-v1\n7 4 1 4902 0 0\n42 4 0\n' +
        ''.join(f'{i} {value} {digest}\n' for i, (value, digest) in enumerate(rows, 1)))
    certificate = {'version': 1, 'generation': 2, 'origin': 0, 'stage': 7,
                   'first_change': 3, 'lead': 2, 'horizon': 8, 'count': 4,
                   'inputs': [row[0] for row in rows], 'digests': [row[1] for row in rows]}
    decision = {'origin': 2, 'active_generation': 2, 'queue_before': 2}

    def write(cert=certificate, query=decision):
        (root / 'certificates.jsonl').write_text(json.dumps(cert) + '\n')
        (root / 'decisions.jsonl').write_text(json.dumps(query) + '\n')

    def reject(cert=certificate, query=decision):
        write(cert, query)
        try:
            module.extract(root, query['origin'])
        except ValueError:
            return
        raise AssertionError('corrupt capture accepted')

    write()
    _, fingerprint, actual, metadata = module.extract(root, 2)
    assert fingerprint == 42 and actual == rows and metadata['remaining'] == 2
    assert not metadata['native_verified'] and not metadata['retail_proof']
    assert metadata['query_state'] == {}
    state = {'horizon': 64, 'normal_horizon': 256, 'motion_cursor_before': 39,
             'pulse_recovery': True, 'previous_replacement_failed': True, 'shortened': True}
    write(query={**decision, **state})
    metadata = module.extract(root, 2)[3]
    assert metadata['query_state'] == state and metadata['horizon'] == 8
    reject(query={**decision, **state, 'pulse_recovery': 1})
    reject(query={**decision, **state, 'motion_cursor_before': -1})
    changed = copy.deepcopy(certificate)
    changed['requested_horizon'] = 32
    write(changed)
    assert module.extract(root, 2)[3]['horizon'] == 8
    changed['requested_horizon'] = 7
    reject(changed)
    write(query={'origin': 4, 'active_generation': 2, 'queue_before': 0})
    assert module.extract(root, 4)[3]['remaining'] == 0
    for field, value in [('count', 5), ('generation', 4), ('first_change', 4), ('version', True)]:
        changed = copy.deepcopy(certificate)
        changed[field] = value
        reject(changed)
    changed = copy.deepcopy(certificate)
    changed['digests'][1] = 6
    reject(changed)
    changed['digests'][1] = 2**64
    reject(changed)
    reject(query={'origin': 2, 'active_generation': 0, 'queue_before': 2})
    reject(query={'origin': 2, 'active_generation': 2, 'queue_before': 1})
    write()
    (root / 'certificates.jsonl').write_text(json.dumps(certificate))
    try:
        module.extract(root, 2)
        raise AssertionError('truncated journal accepted')
    except ValueError:
        pass
print('captured queue parsing contracts passed')
