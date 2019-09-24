"""
Layer a tree and red sun with noise background.
When triggered the sun position changes.

1) Noise size
2) Noise position
3) Tree alpha
4) Noise and sun color
"""
import pygame
import random


NOISE = {'size': 1, 'size_range': 20, 'count': 100, 'count_range': 1000, 'color': (115, 115, 115, 100)}

SUN = {'x': 750, 'x_min': 400, 'x_range': 500,
       'y': 350, 'y_min': 200, 'y_range': 300,
       'size': 30, 'size_min': 20, 'size_range': 300,
       'color': (161, 19, 19, 100)}

trigger = False

def setup(screen, etc):
    global tree_img
    filepath = etc.mode_root + '../images/tree1.png'
    tree_img = pygame.image.load(filepath)

def draw(screen, etc):
    global SUN, NOISE, trigger, tree_img

    color = etc.color_picker()

    if etc.audio_trig or etc.midi_note_new:
        trigger = True

    if trigger == True:
        SUN['x'] = int(SUN['x_min'] + random.random() * SUN['x_range'])
        SUN['y'] = int(SUN['y_min'] + random.random() * SUN['y_range'])

    # Add noise ------------------------------------------------------------------------------------
    NOISE['size'] = int(etc.knob1 * NOISE['size_range'])           # Knob 1 | Noise size
    NOISE['count'] = int(etc.knob2 * NOISE['count_range'])         # Knob 2 | Noise count
    NOISE['color'] = tuple(int(i * 0.8) for i in color)
    for i in range(NOISE['count']):
        x, y = int(random.random() * 1280), int(random.random() * 720)
        pygame.draw.circle(screen, NOISE['color'], (x, y), NOISE['size'], 0)

    # Add sun --------------------------------------------------------------------------------------
    SUN['color'] = color
    SUN['size'] = int(SUN['size_min'] + etc.knob3 * SUN['size_range'])  # Knob 3 | Sun size
    pygame.draw.circle(screen, SUN['color'], (SUN['x'], SUN['y']), SUN['size'], 0)

    # Add image ------------------------------------------------------------------------------------
    # tree_img.fill((0, 0, 0, 255), None, pygame.BLEND_RGBA_MULT)
    screen.blit(tree_img, (200, 0))

    trigger = False
