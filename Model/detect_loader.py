# detect_loader.py
import os
import sys
import cv2
import json
import numpy as np
from ultralytics import YOLO

MODEL_PATH = os.path.join(os.path.dirname(__file__), "model2.pt")
model = YOLO(MODEL_PATH)

def build_grid(detections, names, grid_cols=7, grid_rows=6):
    """Construit la grille du jeu (0=vide, 1=rouge, 2=jaune) à partir des bounding boxes."""
    if not detections:
        return np.zeros((grid_rows, grid_cols), dtype=int).tolist()

    centers = []
    for b in detections:
        cls = int(b.cls)
        x1, y1, x2, y2 = map(float, b.xyxy[0])
        centers.append(((x1 + x2) / 2, (y1 + y2) / 2, cls))

    # Tri spatial
    centers.sort(key=lambda c: (c[1], c[0]))  # d'abord Y, puis X

    xs = [c[0] for c in centers]
    ys = [c[1] for c in centers]
    if len(xs) < 4 or len(ys) < 4:
        # fallback si peu de détections
        return np.zeros((grid_rows, grid_cols), dtype=int).tolist()

    # Divisions pour les colonnes et lignes
    xs_split = np.linspace(min(xs), max(xs), grid_cols + 1)
    ys_split = np.linspace(min(ys), max(ys), grid_rows + 1)

    grid = np.zeros((grid_rows, grid_cols), dtype=int)

    for x, y, cls in centers:
        col = np.searchsorted(xs_split, x) - 1
        row = np.searchsorted(ys_split, y) - 1
        if not (0 <= col < grid_cols and 0 <= row < grid_rows):
            continue

        label = names[cls].lower()
        if "red" in label:
            grid[row, col] = 1
        elif "yellow" in label:
            grid[row, col] = 2
        elif "empty" in label:
            grid[row, col] = 0  # explicite

    return grid.tolist()


def main():
    if len(sys.argv) < 4:
        print("Usage: python detect_loader.py input.jpg output.jpg grid.json")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    json_path = sys.argv[3]

    img = cv2.imread(input_path)
    results = model(img)
    annotated = results[0].plot()
    cv2.imwrite(output_path, annotated)

    detections = results[0].boxes
    names = results[0].names
    grid = build_grid(detections, names)

    with open(json_path, "w") as f:
        json.dump({"grid": grid}, f, indent=2)

    print("OK")

if __name__ == "__main__":
    main()
