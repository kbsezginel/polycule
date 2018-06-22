"""
Convert given audio files to MP3.
 >>> python organize_samples.py AUDIO_SRC
"""
import os
import sys
import subprocess


AUDIO_SRC = os.path.abspath(sys.argv[1])
# AUDIO_DST = os.path.abspath(sys.argv[2])
ASK_ORDER = False


def convert_mp3(audio_in, audio_out, bit_rate=320000, sample_rate=44100):
    """ Convert audio file to mp3. """
    avconv = ['avconv', '-i', audio_in, '-f', 'mp3', '-acodec', 'libmp3lame', '-ab', str(bit_rate), '-ar', str(sample_rate), audio_out]
    subprocess.run(avconv)


def save_sample_list(sample_dict, sample_list_file):
    """ Save sample list into a file. """
    with open(sample_list_file, 'w') as f:
        for i in range(1, len(sample_dict) + 1):
            f.write('%2i - %s\n' % (i, sample_dict[i]))
    print('Saved sample list -> %s' % sample_list_file)


samples = {}
AUDIO_DST = os.path.abspath(input('Where do you wanna save the samples?: '))
SAMPLE_LIST_FILE = os.path.join(AUDIO_DST, 'samples.txt')
os.makedirs(AUDIO_DST, exist_ok=True)
for idx, audio in enumerate(os.listdir(AUDIO_SRC), start=1):
    audio_file = os.path.join(AUDIO_SRC, audio)
    if ASK_ORDER:
        audio_idx = int(input('Enter sample no -> %s: ' % audio))
    else:
        audio_idx = idx
    mp3_file = os.path.join(AUDIO_DST, '%03i.mp3' % audio_idx)
    convert_mp3(audio_file, mp3_file)
    samples[audio_idx] = os.path.splitext(audio)[0]

save_sample_list(samples, SAMPLE_LIST_FILE)
