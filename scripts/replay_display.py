#!/usr/bin/env python3
"""Read actual X11 drawable damage events; never sample or drive game input."""
import ctypes as C
import csv
import os
import select
import signal
import struct
import sys
import time
from pathlib import Path


class Rectangle(C.Structure):
    _fields_ = [('x', C.c_short), ('y', C.c_short),
                ('width', C.c_ushort), ('height', C.c_ushort)]


class DamageEvent(C.Structure):
    _fields_ = [('type', C.c_int), ('serial', C.c_ulong), ('send_event', C.c_int),
                ('display', C.c_void_p), ('drawable', C.c_ulong), ('damage', C.c_ulong),
                ('level', C.c_int), ('more', C.c_int), ('timestamp', C.c_ulong),
                ('area', Rectangle), ('geometry', Rectangle)]


class Event(C.Union):
    _fields_ = [('type', C.c_int), ('damage', DamageEvent), ('pad', C.c_long * 24)]


x = C.CDLL('libX11.so.6')
d = C.CDLL('libXdamage.so.1')


def signature(lib, name, args, result):
    function = getattr(lib, name)
    function.argtypes = args
    function.restype = result
    return function


ptr = C.c_void_p
ulong = C.c_ulong
integer = C.c_int
open_display = signature(x, 'XOpenDisplay', [C.c_char_p], ptr)
pending = signature(x, 'XPending', [ptr], integer)
next_event = signature(x, 'XNextEvent', [ptr, C.POINTER(Event)], integer)
connection = signature(x, 'XConnectionNumber', [ptr], integer)
flush = signature(x, 'XFlush', [ptr], integer)
select_input = signature(x, 'XSelectInput', [ptr, ulong, C.c_long], integer)
close = signature(x, 'XCloseDisplay', [ptr], integer)
query = signature(d, 'XDamageQueryExtension', [ptr, C.POINTER(integer), C.POINTER(integer)], integer)
create = signature(d, 'XDamageCreate', [ptr, ulong, integer], ulong)
subtract = signature(d, 'XDamageSubtract', [ptr, ulong, ulong, ulong], None)
display = open_display(None)
if not display:
    raise RuntimeError('cannot open owned display')
base, error = integer(), integer()
if not query(display, C.byref(base), C.byref(error)):
    raise RuntimeError('DAMAGE extension missing')
window = int(sys.argv[1])
root = Path(sys.argv[2])
damage = create(display, window, 3)  # XDamageReportNonEmpty.
select_input(display, window, 1 << 17)  # StructureNotifyMask, including DestroyNotify.
flush(display)
running = True

def stop(*args):
    global running
    running = False


signal.signal(signal.SIGTERM, stop)
with (root / 'display-times.csv').open('x', buffering=1) as file:
    writer = csv.writer(file)
    writer.writerow(['event', 'server_ms', 'received_ns', 'latest_update', 'stage',
                     'x', 'y', 'width', 'height', 'geometry_width', 'geometry_height'])
    count = 0
    while running:
        if not pending(display):
            select.select([connection(display)], [], [], .1)
            continue
        event = Event()
        next_event(display, C.byref(event))
        if event.type == 17:  # DestroyNotify.
            break
        if event.type != base.value:
            continue
        stamp = time.monotonic_ns()
        seq = stage = 0
        path = root / 'update-times.bin'
        if path.exists():
            with path.open('rb') as data:
                size = os.fstat(data.fileno()).st_size
                if size >= 40:
                    data.seek(16 + ((size - 16) // 24 - 1) * 24)
                    seq, stage, _ = struct.unpack('<3Q', data.read(24))
        event_damage = event.damage
        count += 1
        writer.writerow([count, event_damage.timestamp, stamp, seq, stage,
                         event_damage.area.x, event_damage.area.y,
                         event_damage.area.width, event_damage.area.height,
                         event_damage.geometry.width, event_damage.geometry.height])
        subtract(display, damage, 0, 0)
        flush(display)
close(display)
