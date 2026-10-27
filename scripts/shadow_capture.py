"""Capture the owned Xvfb game and private PulseAudio output independently.

Follows th08-solver 969ae466 docs/PLANNER.md's separate-capture experiment.
No gameplay speed changes, frame interpolation, or unrelated host audio.
"""
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import tempfile
import time


class Capture:
    def __init__(self, output):
        self.output = Path(output)
        self.processes = []
        self.logs = []
        self.pulse = None
        self.video = None
        self.private = Path(tempfile.mkdtemp(prefix='th06-pulse-'))
        self.server = 'unix:' + str(self.private / 'native')
        self.metadata = {'display': os.environ['DISPLAY'], 'fps_requested': 60,
                         'separate_audio_video': True, 'private_audio': True,
                         'audio_environment': {'PULSE_SERVER': self.server, 'PULSE_SINK': 'th06_game'}}
        config = self.private / 'pulse.pa'
        config.write_text('load-module module-native-protocol-unix socket=' +
                          str(self.private / 'native') + ' auth-anonymous=1\n'
                          'load-module module-null-sink sink_name=th06_game rate=48000 channels=2\n'
                          'set-default-sink th06_game\n')
        self.pulse = subprocess.Popen(['pulseaudio', '-n', '--daemonize=no', '--use-pid-file=no',
                                      '--exit-idle-time=-1', '--disable-shm=yes',
                                      '--log-target=file:' + str(self.output / 'pulse.log'),
                                      '-F', str(config)])
        try:
            deadline = time.monotonic() + 5
            while not (self.private / 'native').exists():
                if self.pulse.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError('private PulseAudio failed to initialize')
                time.sleep(.02)
            os.environ['PULSE_SERVER'] = self.server
            os.environ['PULSE_SINK'] = 'th06_game'
            self.start('audio', ['-f', 'pulse', '-server', self.server, '-sample_rate', '48000',
                                '-channels', '2', '-i', 'th06_game.monitor',
                                '-c:a', 'pcm_s16le', str(self.output / 'audio.wav')])
        except BaseException:
            self.close()
            raise

    def start(self, name, arguments):
        log = (self.output / (name + '-capture.log')).open('wb')
        self.logs.append(log)
        process = subprocess.Popen(['ffmpeg', '-nostdin', '-hide_banner', '-nostats', '-y'] + arguments,
                                   stdout=log, stderr=subprocess.STDOUT)
        self.processes.append(process)
        return process

    def attach(self, game):
        deadline = time.monotonic() + 15
        while game.poll() is None and time.monotonic() < deadline:
            windows = subprocess.run(['xdotool', 'search', '--onlyvisible', '--class', '^th06.exe$'],
                                     capture_output=True, text=True)
            for window in windows.stdout.split():
                geometry = subprocess.run(['xdotool', 'getwindowgeometry', '--shell', window],
                                          capture_output=True, text=True)
                fields = dict(line.split('=', 1) for line in geometry.stdout.splitlines() if '=' in line)
                if fields.get('WIDTH') == '640' and fields.get('HEIGHT') == '480':
                    x, y = int(fields['X']), int(fields['Y'])
                    self.metadata['rectangle'] = [x, y, 640, 480]
                    self.video = self.start('video', ['-f', 'x11grab', '-framerate', '60',
                        '-video_size', '640x480', '-i', os.environ['DISPLAY'] + '+' + str(x) + ',' + str(y),
                        '-an', '-c:v', 'libx264', '-preset', 'ultrafast', '-crf', '21',
                        '-threads:v', '1', '-pix_fmt', 'yuv420p', '-fps_mode', 'passthrough',
                        '-enc_time_base', '1:1000000',
                        '-f', 'matroska', str(self.output / 'video.mkv')])
                    return
            time.sleep(.03)
        raise RuntimeError('owned game rectangle unavailable for recording')

    def close(self):
        for process in self.processes:
            if process.poll() is None:
                process.send_signal(signal.SIGINT)
        for process in self.processes:
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        for log in self.logs:
            log.close()
        if self.pulse is not None:
            self.pulse.terminate()
            try:
                self.pulse.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.pulse.kill()
                self.pulse.wait()
        shutil.rmtree(self.private, ignore_errors=True)

    def mux(self, compact=False):
        origins = {}
        for name in ['audio', 'video']:
            log = (self.output / (name + '-capture.log')).read_text(errors='replace')
            match = re.search(r'start: ([0-9.]+)', log)
            if not match:
                raise RuntimeError('missing original capture clock: ' + name)
            origins[name] = float(match[1])
        offset = origins['video'] - origins['audio']
        samples = round(abs(offset) * 48000)
        alignment = ('atrim=start_sample=' + str(samples) + ',asetpts=PTS-STARTPTS' if offset >= 0
                     else 'adelay=' + str(samples) + 'S:all=1')
        self.metadata.update({'clock_origins_seconds': origins, 'audio_trim_seconds': offset,
                              'compact': compact, 'compact_profile': '640x480 fast CRF26 / AAC128k' if compact else None,
                              'capture_process_exits': [p.returncode for p in self.processes]})
        (self.output / 'capture.json').write_text(json.dumps(self.metadata, indent=2) + '\n')
        # Keep real capture timestamps; compact changes coding quality, not time.
        video_args = (['-c:v', 'libx264', '-preset', 'fast', '-crf', '26',
                       '-threads:v', '1', '-pix_fmt', 'yuv420p', '-fps_mode', 'passthrough',
                       '-enc_time_base', '1:1000000'] if compact else ['-c:v', 'copy'])
        destination = self.output / ('gameplay-compact.mp4' if compact else 'gameplay.mp4')
        with (self.output / 'mux.log').open('wb') as log:
            subprocess.run(['ffmpeg', '-nostdin', '-hide_banner', '-y', '-i', str(self.output / 'video.mkv'),
                            '-i', str(self.output / 'audio.wav'), '-map', '0:v', '-map', '1:a',
                            '-af', alignment + ',apad', *video_args, '-c:a', 'aac', '-b:a', '128k',
                            '-shortest', '-movflags', '+faststart', str(destination)],
                           stdout=log, stderr=subprocess.STDOUT, check=True)
        self.metadata['delivery'] = destination.name
        self.metadata['delivery_bytes'] = destination.stat().st_size
        (self.output / 'capture.json').write_text(json.dumps(self.metadata, indent=2) + '\n')
