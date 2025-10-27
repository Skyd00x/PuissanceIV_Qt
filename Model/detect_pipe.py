# detect_pipe.py
import sys
import io
import struct
import cv2
import numpy as np
from ultralytics import YOLO

# Chargement unique du modèle
model = YOLO("best.pt")
model.fuse()

def read_frame_from_stdin():
    # Lecture taille (4 octets)
    raw_len = sys.stdin.buffer.read(4)
    if not raw_len:
        return None
    frame_len = struct.unpack('<I', raw_len)[0]
    # Lecture de l'image compressée
    frame_data = sys.stdin.buffer.read(frame_len)
    if not frame_data:
        return None
    # Décodage JPEG
    img_array = np.frombuffer(frame_data, dtype=np.uint8)
    return cv2.imdecode(img_array, cv2.IMREAD_COLOR)

def write_frame_to_stdout(img):
    # Encodage JPEG
    ret, encoded = cv2.imencode('.jpg', img, [int(cv2.IMWRITE_JPEG_QUALITY), 80])
    if not ret:
        return
    data = encoded.tobytes()
    # Envoi taille + données
    sys.stdout.buffer.write(struct.pack('<I', len(data)))
    sys.stdout.buffer.write(data)
    sys.stdout.buffer.flush()

print("[Python] Détection prête", file=sys.stderr, flush=True)

while True:
    frame = read_frame_from_stdin()
    if frame is None:
        break

    results = model(frame)
    annotated = results[0].plot()
    write_frame_to_stdout(annotated)
