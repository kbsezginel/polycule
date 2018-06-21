"""
Rename given audio samples.
 >>> python rename_samples.py AUDIO_SRC
"""
import os
import sys


AUDIO_SRC = os.path.abspath(sys.argv[1])
audio_list = sorted(os.listdir(AUDIO_SRC))
for idx, audio in enumerate(audio_list, start=1):
    audio_file = os.path.join(AUDIO_SRC, audio)
    mp3_file = os.path.join(AUDIO_SRC, '%03i.mp3' % idx)
    print('%s -> %s' % (audio_file, mp3_file))
    os.rename(audio_file, mp3_file)
