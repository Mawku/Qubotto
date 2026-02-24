import cv2
import numpy as np
import sys
import signal

CAMERA_INDEX = 1

COLOR_VALUES = {"Black": -2, "Red": -1, "Yellow": 0, "Green": 1, "Blue": 2}

THRESHOLDS = {
    "Black": {"L": [0, 50],   "A": [115, 140], "B": [115, 140]},
    "Red":   {"L": [20, 160], "A": [150, 255], "B": [120, 185]},
    "Yellow":{"L": [90, 255], "A": [110, 145], "B": [155, 255]},
    "Green": {"L": [20, 160], "A": [0, 110],   "B": [130, 185]},
    "Blue":  {"L": [20, 160], "A": [110, 150], "B": [0, 115]},
}


def get_color_name(lab_pixel):
    l, a, b = lab_pixel.astype(np.int16)
    for name, t in THRESHOLDS.items():
        if t["L"][0] <= l <= t["L"][1] and t["A"][0] <= a <= t["A"][1] and t["B"][0] <= b <= t["B"][1]:
            return name
    return "Unknown"


def shutdown(cap):
    cap.release()
    sys.exit(0)


cap = cv2.VideoCapture(CAMERA_INDEX)
signal.signal(signal.SIGINT, lambda s, f: shutdown(cap))

try:
    while True:
        ret, frame = cap.read()
        if not ret or frame is None:
            break

        h, w = frame.shape[:2]
        # crop centrato 640x640 (se il frame e' piu' piccolo, si usa com'e')
        if h >= 640 and w >= 640:
            y0 = (h - 640) // 2
            x0 = (w - 640) // 2
            frame = frame[y0:y0+640, x0:x0+640]
            h, w = 640, 640

        lab_img = cv2.cvtColor(frame, cv2.COLOR_BGR2LAB)
        gray    = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        blurred = cv2.GaussianBlur(gray, (7, 7), 0)
        _, mask  = cv2.threshold(blurred, 65, 255, cv2.THRESH_BINARY_INV)
        kernel  = np.ones((3, 3), np.uint8)
        mask    = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        valid_targets = []
        for cnt in contours:
            area = cv2.contourArea(cnt)
            if 1200 < area < (h * w * 0.4):
                if len(cnt) >= 5:
                    ellipse = cv2.fitEllipse(cnt)
                    (cx, cy), (d1, d2), angle = ellipse
                    ratio = max(d1, d2) / max(0.01, min(d1, d2))
                    if ratio < 2.1:
                        valid_targets.append((cnt, ellipse, area))

        if valid_targets:
            cnt, ellipse, area = max(valid_targets, key=lambda x: x[2])
            (cx, cy), (d1, d2), angle = ellipse
            cx, cy = int(cx), int(cy)
            rx, ry = d1 / 2, d2 / 2

            rings_colors = []
            total_sum    = 0
            factors      = [0.1, 0.3, 0.5, 0.67, 0.8]

            rad = np.deg2rad(angle)
            cos_a, sin_a = np.cos(rad), np.sin(rad)

            for f in factors:
                test_points = [
                    (int(cx + f * rx * cos_a), int(cy + f * ry * sin_a)),
                    (int(cx - f * rx * cos_a), int(cy - f * ry * sin_a)),
                    (int(cx - f * rx * sin_a), int(cy + f * ry * cos_a)),
                    (int(cx + f * rx * sin_a), int(cy - f * ry * cos_a)),
                ]
                votes = {}
                for px, py in test_points:
                    px, py = np.clip(px, 0, w - 1), np.clip(py, 0, h - 1)
                    c_name = get_color_name(lab_img[py, px])
                    if c_name != "Unknown":
                        votes[c_name] = votes.get(c_name, 0) + 1

                if votes:
                    best_color = max(votes, key=votes.get)
                    rings_colors.append(best_color)
                    total_sum += COLOR_VALUES[best_color]
                else:
                    rings_colors.append("?")

            if not all(color in ["Black", "?"] for color in rings_colors):
                if total_sum == 0:
                    status = "UNHARMED"
                elif total_sum == 1:
                    status = "STABLE"
                elif total_sum == 2:
                    status = "HARMED"
                else:
                    status = "FAKE TARGET"

                print(f"{status},{total_sum},{'->'.join(rings_colors)}", flush=True)

finally:
    cap.release()
    sys.exit(0)
