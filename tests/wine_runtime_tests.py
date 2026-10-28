#!/usr/bin/env python3
"""Runtime selection and mixed-architecture process environment boundaries."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from wine_runtime import apply_environment, render_environment, resolve_runtime


class RuntimeTests(unittest.TestCase):
    def test_prefix_mismatch_fails_before_launch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('wine', 'wineserver'):
                (root / name).write_text('#!/bin/sh\nexit 0\n')
                (root / name).chmod(0o700)
            (root / 'system.reg').write_text('WINE REGISTRY Version 2\n#arch=win64\n')
            with self.assertRaisesRegex(ValueError, 'prefix architecture'):
                resolve_runtime(root / 'wine', None, root, 'win32')
            loader, server = resolve_runtime(root / 'wine', None, root, 'wow64')
            self.assertEqual((loader, server), (root / 'wine', root / 'wineserver'))
            (root / 'wineserver').unlink()
            with self.assertRaisesRegex(ValueError, 'matching'):
                resolve_runtime(root / 'wine', None, root, 'wow64')

    def test_explicit_server_and_symlinked_loader(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'runtime').mkdir()
            loader = root / 'runtime/wine'
            server = root / 'private-server'
            for path in (loader, server):
                path.write_text('#!/bin/sh\nexit 0\n')
                path.chmod(0o700)
            (root / 'loader').symlink_to(loader)
            (root / 'system.reg').write_text('#arch=win32\n')
            self.assertEqual(resolve_runtime(root / 'loader', server, root, 'win32'),
                             (loader, server))

    def test_graphics_libraries_reach_only_wine(self):
        inherited = {'PATH': '/bin', 'LD_LIBRARY_PATH': '/private/amd64',
                     'LIBGL_DRIVERS_PATH': '/private/dri64',
                     'VK_ICD_FILENAMES': '/private/lavapipe.json',
                     'MESA_SHADER_CACHE_DIR': '/stale-cache',
                     'MESA_SHADER_CACHE_MAX_SIZE': '8G', 'vblank_mode': '1'}
        original = dict(inherited)
        wine, worker = render_environment(inherited, '/prefix', 'wow64', '/server',
                                          'd3d8=n', 8, 'off', None)
        native_env = apply_environment(inherited, worker)
        self.assertNotIn('LD_LIBRARY_PATH', native_env)
        self.assertNotIn('LIBGL_DRIVERS_PATH', native_env)
        self.assertNotIn('MESA_SHADER_CACHE_DIR', native_env)
        self.assertNotIn('MESA_SHADER_CACHE_MAX_SIZE', native_env)
        wine_env = apply_environment(native_env, wine)
        self.assertEqual(wine_env['LD_LIBRARY_PATH'], '/private/amd64')
        self.assertEqual(wine_env['LIBGL_DRIVERS_PATH'], '/private/dri64')
        self.assertEqual(wine_env['VK_ICD_FILENAMES'], '/private/lavapipe.json')
        self.assertEqual(wine_env['vblank_mode'], '0')
        self.assertEqual(wine_env['PATH'], '/bin')
        self.assertEqual(inherited, original)
        self.assertNotIn('LD_LIBRARY_PATH', native_env)

    def test_unset_and_cache_overrides_remove_inherited_settings(self):
        wine, _ = render_environment({}, '/prefix', 'win32', '/server', '',
                                     2, 'inherit', Path('/cache'))
        effective = apply_environment({'vblank_mode': 'stale', 'WINEARCH': 'wow64'}, wine)
        self.assertNotIn('vblank_mode', effective)
        self.assertEqual(effective['WINEARCH'], 'win32')
        self.assertEqual(effective['MESA_SHADER_CACHE_DIR'], '/cache')
        self.assertEqual(effective['MESA_SHADER_CACHE_DISABLE'], 'false')


if __name__ == '__main__':
    unittest.main()
