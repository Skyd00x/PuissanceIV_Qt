import os
import sys
import cv2
from ultralytics import YOLO

def main():
    if len(sys.argv) < 3:
        print("Usage: python detect.py input.jpg output.jpg")
        return

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    # 🔧 chemin absolu vers ton modèle
    model_path = os.path.join(os.path.dirname(__file__), "best.pt")

    try:
        model = YOLO(model_path)
        img = cv2.imread(input_path)
        if img is None:
            print("Erreur : impossible de lire", input_path)
            return

        results = model(img)
        annotated = results[0].plot()
        cv2.imwrite(output_path, annotated)
        print("OK")
    except Exception as e:
        print("Erreur:", e)

if __name__ == "__main__":
    main()
