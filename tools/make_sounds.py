#!/usr/bin/env python3
"""Generates the UI sounds of the demo theme into demo/sounds.

The sounds are synthesized, so they can be tweaked here and regenerated:
    python3 demo/tools/make_sounds.py
"""

import math
import random
import struct
import wave
from pathlib import Path

RATE = 44100
DEMO = Path(__file__).resolve().parent.parent
OUT = DEMO / "sounds"


def tone(freq, length, decay, harmonics=((1, 1.0),), glide=0.0, attack=0.002):
    """A sine tone with harmonics, an exponential decay and an optional pitch glide."""
    samples = []
    phase = [0.0] * len(harmonics)
    count = int(length * RATE)
    for i in range(count):
        t = i / RATE
        f = freq * (1.0 + glide * t / length)
        env = min(1.0, t / attack) * math.exp(-t / decay)
        value = 0.0
        for h, (mult, amp) in enumerate(harmonics):
            phase[h] += 2.0 * math.pi * f * mult / RATE
            value += amp * math.sin(phase[h])
        samples.append(value * env)
    return samples


def noise(length, decay, cutoff, sweep=0.0, seed=1):
    """Low-passed noise; sweep moves the cutoff over time (positive rises)."""
    rng = random.Random(seed)
    samples = []
    low = 0.0
    count = int(length * RATE)
    for i in range(count):
        t = i / RATE
        c = max(50.0, cutoff * (1.0 + sweep * t / length))
        a = 1.0 - math.exp(-2.0 * math.pi * c / RATE)
        low += a * (rng.uniform(-1.0, 1.0) - low)
        env = min(1.0, t / 0.004) * math.exp(-t / decay)
        samples.append(low * env)
    return samples


def silence(length):
    return [0.0] * int(length * RATE)


def mix(*parts):
    length = max(len(p) for p in parts)
    return [sum(p[i] for p in parts if i < len(p)) for i in range(length)]


def save(name, samples, peak):
    top = max(abs(s) for s in samples) or 1.0
    # A short fade out avoids clicks at the end.
    fade = min(len(samples), int(0.005 * RATE))
    for i in range(fade):
        samples[-1 - i] *= i / fade
    OUT.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT / f"{name}.wav"), "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(b"".join(struct.pack("<h", int(s / top * peak * 32767)) for s in samples))


def main():
    bell = ((1, 1.0), (2, 0.3), (3, 0.12))
    save("hover", tone(1900, 0.04, 0.008), 0.12)
    save("pressed", mix(tone(820, 0.09, 0.025, bell, glide=-0.25), noise(0.02, 0.004, 4000)), 0.45)
    save("toggled_on", tone(660, 0.05, 0.02, bell) + tone(990, 0.09, 0.03, bell), 0.4)
    save("toggled_off", tone(990, 0.05, 0.02, bell) + tone(660, 0.09, 0.03, bell), 0.4)
    buzz = ((1, 1.0), (3, 0.35), (5, 0.15))
    save("denied", tone(170, 0.07, 0.05, buzz) + silence(0.035) + tone(150, 0.1, 0.05, buzz), 0.45)
    save("typed", noise(0.025, 0.006, 2500, seed=7), 0.25)
    save("submitted", tone(880, 0.12, 0.05, bell) + tone(1320, 0.3, 0.12, bell), 0.4)
    save("value_changed", tone(1250, 0.03, 0.007), 0.18)
    save("opened", noise(0.1, 0.05, 600, sweep=5.0, seed=3), 0.3)
    save("closed", noise(0.1, 0.04, 3000, sweep=-0.85, seed=5), 0.25)
    save("selected", tone(1046, 0.14, 0.04, bell), 0.32)
    save("activated", tone(784, 0.07, 0.03, bell) + tone(1175, 0.16, 0.06, bell), 0.38)


if __name__ == "__main__":
    main()
