import json
import sys
import random

def humanize(input_file):
    try:
        with open(input_file, 'r') as f:
            data = json.load(f)
        
        # Asumimos que el JSON es un objeto con una lista llamada "notes"
        # O una lista directamente. JUCE lo enviará como un objeto { "notes": [...] }
        notes = data.get('notes', [])
        
        for note in notes:
            # 1. Jitter de posición (x) +/- 4 píxeles
            note['x'] += random.randint(-4, 4)
            
            # 2. Variación de duración (width) +/- 5%
            width_var = int(note['width'] * 0.05)
            note['width'] += random.randint(-max(1, width_var), max(1, width_var))
            
            # 3. Variación de Velocity (fuerza) +/- 15
            # Nos aseguramos de estar en el rango MIDI 1-127
            note['velocity'] += random.randint(-15, 15)
            note['velocity'] = max(1, min(127, note['velocity']))
            
        # Guardar cambios
        with open(input_file, 'w') as f:
            json.dump(data, f, indent=4)
            
        print(f"Succefully humanized {len(notes)} notes.")
        
    except Exception as e:
        print(f"Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) > 1:
        humanize(sys.argv[1])
    else:
        print("Usage: python humanize.py <path_to_json>")
