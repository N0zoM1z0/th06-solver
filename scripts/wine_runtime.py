"""Resolve an owned Wine runtime and isolate its Unix graphics libraries."""
from pathlib import Path
import os
import shutil


GRAPHICS_NAMES = ('WINE_D3D_CONFIG', 'mesa_glthread', 'GALLIUM_THREAD',
                  'LP_NATIVE_VECTOR_WIDTH', 'VK_ICD_FILENAMES',
                  'LD_LIBRARY_PATH', 'LIBGL_DRIVERS_PATH')
WINE_ONLY = ('LD_LIBRARY_PATH', 'LIBGL_DRIVERS_PATH')


def resolve_runtime(loader, server, prefix, architecture):
    selected = shutil.which(str(loader))
    if selected is None:
        raise ValueError('Wine loader is unavailable')
    selected = Path(selected).resolve()
    server = Path(server) if server else selected.parent / 'wineserver'
    if not server.is_file() or not os.access(server, os.X_OK):
        raise ValueError('provide --wineserver matching the selected Wine loader')
    registry = Path(prefix) / 'system.reg'
    if not registry.is_file():
        raise ValueError('initialize the dedicated Wine prefix first')
    prefix_arch = next((line[6:] for line in registry.read_text().splitlines()
                        if line.startswith('#arch=')), None)
    if architecture not in ('win32', 'win64', 'wow64'):
        raise ValueError('unsupported Wine architecture')
    if prefix_arch != ('win32' if architecture == 'win32' else 'win64'):
        raise ValueError('prefix architecture does not match the requested Wine mode')
    return selected, server.resolve()


def render_environment(inherited, prefix, architecture, server, overrides,
                       threads, vblank, cache):
    expected = {name: inherited.get(name) for name in GRAPHICS_NAMES}
    expected.update({
        'WINEPREFIX': str(prefix), 'WINEARCH': architecture,
        'WINESERVER': str(server), 'WINEDLLOVERRIDES': overrides,
        'LP_NUM_THREADS': str(threads), 'LIBGL_ALWAYS_SOFTWARE': '1',
        'vblank_mode': '0' if vblank == 'off' else inherited.get('vblank_mode'),
        'MESA_SHADER_CACHE_DISABLE': 'false' if cache else 'true',
        'MESA_SHADER_CACHE_DIR': str(cache) if cache else None,
        'MESA_SHADER_CACHE_MAX_SIZE': '64M' if cache else None,
    })
    worker = dict(expected, **{name: None for name in WINE_ONLY})
    return expected, worker


def apply_environment(inherited, declared):
    result = dict(inherited)
    for name, value in declared.items():
        if value is None:
            result.pop(name, None)
        else:
            result[name] = value
    return result
