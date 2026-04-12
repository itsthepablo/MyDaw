import json
import os
import sys
import math
import random

# --- CONFIGURACIÓN ---
# Puedes cambiar esta progresión por la que quieras probar:
DEFAULT_PROGRESSION = "Cmaj7 Am7 Fmaj7 G7"
DEFAULT_OCTAVE = 4
# ---------------------

NOTES = {
    "C":0,"C#":1,"Db":1,"D":2,"D#":3,"Eb":3,"E":4,"F":5,"F#":6,"Gb":6,
    "G":7,"G#":8,"Ab":8,"A":9,"A#":10,"Bb":10,"B":11
}

CHORDS = {
    "": [0,4,7], "maj":[0,4,7], "m":[0,3,7], "min":[0,3,7],
    "7":[0,4,7,10], "maj7":[0,4,7,11], "m7":[0,3,7,10],
    "dim":[0,3,6], "m7b5":[0,3,6,10], "sus2":[0,2,7], "sus4":[0,5,7],
    "9":[0,4,7,10,14], "m9":[0,3,7,10,14], "maj9":[0,4,7,11,14]
}

def parse_chord(txt):
    txt = txt.strip().replace(" ", "")
    if not txt: return None, None
    
    root = ""
    qual = ""
    for n in sorted(NOTES.keys(), key=len, reverse=True):
        if txt.upper().startswith(n.upper()):
            root = n.upper()
            qual = txt[len(n):].lower()
            break
            
    if root == "": return None, None
    
    formula = CHORDS.get(qual, CHORDS.get("", [0,4,7]))
    return NOTES[root], formula

def process(input_file):
    with open(input_file, 'r') as f:
        data = json.load(f)

    notes = data.get('notes', [])
    
    # En nuestro DAW, 320 píxeles = 1 compás (aprox)
    ticks_per_bar = 320
    base_midi = DEFAULT_OCTAVE * 12
    
    # Determinar dónde empezar a insertar
    start_time = 0
    if notes:
        start_time = max([n['x'] + n['width'] for n in notes])
        # Alinear al siguiente compás
        start_time = int(math.ceil(start_time / ticks_per_bar) * ticks_per_bar)

    chords = DEFAULT_PROGRESSION.split()
    for chord_text in chords:
        root, formula = parse_chord(chord_text)
        if root is None: continue

        for interval in formula:
            pitch = base_midi + root + interval
            new_note = {
                "pitch": pitch,
                "x": start_time,
                "width": ticks_per_bar,
                "frequency": 440.0 * (2 ** ((pitch - 69) / 12.0)),
                "velocity": 100
            }
            notes.append(new_note)
        
        start_time += ticks_per_bar

    data['notes'] = notes
    with open(input_file, 'w') as f:
        json.dump(data, f, indent=4)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        process(sys.argv[1])
