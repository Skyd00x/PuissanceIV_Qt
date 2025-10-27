# detect_loader.py
import os
import sys
import cv2
from ultralytics import YOLO

# =====================================================
# Chargement du modèle une seule fois au démarrage
# =====================================================
MODEL_PATH = os.path.join(os.path.dirname(__file__), "best.pt")

try:
    model = YOLO(MODEL_PATH)
    print(f"[Python] Modèle chargé : {MODEL_PATH}")
except Exception as e:
    print(f"[Python] Erreur lors du chargement du modèle : {e}")
    sys.exit(1)


def main():
    """
    Usage : python detect_loader.py input.jpg output.jpg
    """
    if len(sys.argv) < 3:
        print("Usage: python detect_loader.py input.jpg output.jpg")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    try:
        # Lecture de l'image
        img = cv2.imread(input_path)
        if img is None:
            print(f"Erreur : impossible de lire {input_path}")
            sys.exit(1)

        # Inférence
        results = model(img)
        annotated = results[0].plot()

        # Sauvegarde du résultat
        cv2.imwrite(output_path, annotated)
        print("OK")

    except Exception as e:
        print(f"Erreur: {e}")


if __name__ == "__main__":
    main()
