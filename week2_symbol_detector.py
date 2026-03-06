#some shapes weren't stable
import cv2
import numpy as np
import os
import time
from picamera2 import Picamera2
 
FRAME_WIDTH  = 640
FRAME_HEIGHT = 480
TEMPLATE_DIR = "templates"
COOLDOWN     = 2.0
MIN_AREA     = FRAME_WIDTH * FRAME_HEIGHT * 0.002
 
# ── NO ARROWS HERE - arrows detected separately by direction ──────────────────
COLOUR_GROUPS = {
    "red"   : ["quarter_circle"],
    "orange": ["cross"],
    "yellow": ["star", "warning"],
    "green" : ["recycle", "button_press"],
    "teal"  : ["octagon"],
    "cyan"  : ["qr_code"],
    "blue"  : ["pacman"],
    "purple": ["diamond", "trapezoid", "fingerprint"],
}
 
COLOUR_RANGES = {
    "red"   : [(0,   80,  40), (8,   255, 255)],
    "orange": [(8,   80,  40), (22,  255, 255)],
    "yellow": [(18,  60,  40), (38,  255, 255)],
    "green" : [(38,  40,  30), (82,  255, 255)],
    "teal"  : [(78,  80,  30), (102, 255, 255)],
    "cyan"  : [(82,  10,  30), (100, 140, 255)],
    "blue"  : [(100, 100, 40), (122, 255, 255)],
    "purple": [(115, 30,  20), (165, 255, 255)],
}
 
RED_RANGE_2 = [(170, 100, 80), (180, 255, 255)]
 
DRAW_COLOURS = {
    "arrow_up"      : (0,   220, 0),
    "arrow_right"   : (255, 100, 0),
    "arrow_down"    : (0,   0,   220),
    "arrow_left"    : (0,   165, 255),
    "qr_code"       : (200, 100, 0),
    "fingerprint"   : (180, 0,   180),
    "recycle"       : (0,   180, 80),
    "warning"       : (0,   200, 200),
    "button_press"  : (0,   150, 100),
    "star"          : (0,   215, 255),
    "diamond"       : (255, 0,   200),
    "cross"         : (0,   140, 255),
    "trapezoid"     : (150, 0,   200),
    "pacman"        : (255, 180, 0),
    "quarter_circle": (0,   0,   200),
    "octagon"       : (180, 150, 0),
}
 
 
def load_templates():
    templates = {}
    if not os.path.isdir(TEMPLATE_DIR):
        print("[WARNING] templates folder not found.")
        return templates
    for fname in sorted(os.listdir(TEMPLATE_DIR)):
        if fname.lower().endswith(".png"):
            img = cv2.imread(os.path.join(TEMPLATE_DIR, fname))
            if img is None:
                continue
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            name = os.path.splitext(fname)[0]
            templates[name] = gray
            print("[INFO] Loaded:", name)
    return templates
 
 
def get_colour_mask(hsv, colour):
    lo, hi = COLOUR_RANGES[colour]
    mask = cv2.inRange(hsv, np.array(lo), np.array(hi))
    if colour == "red":
        m2   = cv2.inRange(hsv, np.array(RED_RANGE_2[0]), np.array(RED_RANGE_2[1]))
        mask = cv2.bitwise_or(mask, m2)
    k    = np.ones((5, 5), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN,  k)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, k)
    return mask
 
 
def get_features(cnt, mask):
    area = cv2.contourArea(cnt)
    if area < MIN_AREA:
        return None
    peri = cv2.arcLength(cnt, True)
    if peri == 0:
        return None
    circ = 4 * np.pi * area / (peri * peri)
    x, y, w, h = cv2.boundingRect(cnt)
    ar  = w / float(h) if h > 0 else 0
    ext = area / float(w * h) if w * h > 0 else 0
    apx = cv2.approxPolyDP(cnt, 0.02 * peri, True)
    hull      = cv2.convexHull(cnt)
    hull_area = cv2.contourArea(hull)
    sol       = area / hull_area if hull_area > 0 else 0
    return {
        "area": area, "circ": circ, "ar": ar,
        "ext":  ext,  "vert": len(apx), "sol": sol,
        "bbox": (x, y, w, h),
    }
 
 
def check_recycle(mask):
    k_small = np.ones((3, 3), np.uint8)
    m       = cv2.morphologyEx(mask, cv2.MORPH_OPEN, k_small)
    cnts, _ = cv2.findContours(m, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    blobs   = sorted([c for c in cnts if cv2.contourArea(c) > MIN_AREA * 0.3],
                     key=cv2.contourArea, reverse=True)
    if len(blobs) < 3:
        return False, None
    areas = [cv2.contourArea(c) for c in blobs[:3]]
    if areas[2] / areas[0] < 0.25:
        return False, None
    xs = [cv2.boundingRect(c)[0] for c in blobs[:3]]
    ys = [cv2.boundingRect(c)[1] for c in blobs[:3]]
    ws = [cv2.boundingRect(c)[2] for c in blobs[:3]]
    hs = [cv2.boundingRect(c)[3] for c in blobs[:3]]
    x1 = min(xs);  y1 = min(ys)
    x2 = max(x + w for x, w in zip(xs, ws))
    y2 = max(y + h for y, h in zip(ys, hs))
    return True, (x1, y1, x2 - x1, y2 - y1)
 
 
def classify(colour, f):
    if f is None:
        return None, 0
    c   = f["circ"]
    ar  = f["ar"]
    ex  = f["ext"]
    sol = f["sol"]
    v   = f["vert"]
 
    if colour == "red":
        if c >= 0.50:
            return "quarter_circle", 0.85
        return None, 0
 
    if colour == "orange":
        if ex >= 0.65 and v >= 10:
            return "cross", 0.85
        return None, 0
 
    if colour == "yellow":
        if ex < 0.50:
            return "star", 0.85
        return "warning", 0.85
 
    if colour == "green":
        # pacman can bleed into green - guard with circularity
        # button_press: circ=0.245, ext=0.817, ar=1.212
        # pacman:       circ=0.609  - much more circular, reject it here
        if c >= 0.50:
            return None, 0
        if ex >= 0.60 and ar >= 1.10:
            return "button_press", 0.85
        return None, 0
 
    if colour == "teal":
        if v == 8 and sol > 0.75:
            return "octagon", 0.90
        return None, 0
 
    if colour == "cyan":
        if ex >= 0.70:
            return "qr_code", 0.90
        return None, 0
 
    if colour == "blue":
        # pacman: circ=0.609
        if 0.50 <= c <= 0.85:
            return "pacman", 0.85
        return None, 0
 
    if colour == "purple":
        # fingerprint: very sparse, low solidity
        if sol < 0.55 and ex < 0.35:
            return "fingerprint", 0.92
        if sol >= 0.55:
            if ex < 0.58:
                return "diamond", 0.88
            return "trapezoid", 0.88
        return None, 0
 
    return None, 0
 
 
def match_template(gray, tmpl, x, y, w, h):
    pad = 25
    x1  = max(0, x - pad);  y1 = max(0, y - pad)
    x2  = min(gray.shape[1], x + w + pad)
    y2  = min(gray.shape[0], y + h + pad)
    roi = gray[y1:y2, x1:x2]
    rh, rw = roi.shape[:2]
    best = 0.0
    for sc in [0.4, 0.6, 0.8, 1.0, 1.2]:
        tw = max(20, int(tmpl.shape[1] * sc))
        th = max(20, int(tmpl.shape[0] * sc))
        if tw >= rw or th >= rh:
            continue
        res = cv2.matchTemplate(roi, cv2.resize(tmpl, (tw, th)), cv2.TM_CCOEFF_NORMED)
        _, mv, _, _ = cv2.minMaxLoc(res)
        if mv > best:
            best = mv
    return best
 
 
# ── ARROW DETECTION - direction only, colour irrelevant ───────────────────────
# Arrow HSV: broad range to catch any printed arrow colour
ARROW_HSV_LO = np.array([0,  40, 40])
ARROW_HSV_HI = np.array([175, 255, 255])
ARROW_NAMES  = ["arrow_up", "arrow_down", "arrow_left", "arrow_right"]
 
def detect_arrow(frame, hsv, gray, templates, found, debug):
    """
    Find any arrow-shaped contour regardless of colour,
    then use template matching to determine direction.
    """
    mask = cv2.inRange(hsv, ARROW_HSV_LO, ARROW_HSV_HI)
 
    # Subtract blue (pacman H=108-122) and teal (octagon H=78-102)
    # so those shapes never get picked up as arrow candidates
    exclude_blue = cv2.inRange(hsv, np.array([98,  80, 30]), np.array([125, 255, 255]))
    exclude_teal = cv2.inRange(hsv, np.array([75,  60, 30]), np.array([105, 255, 255]))
    exclude_purple = cv2.inRange(hsv, np.array([110, 20, 20]), np.array([170, 255, 255]))
    mask = cv2.bitwise_and(mask, cv2.bitwise_not(exclude_blue))
    mask = cv2.bitwise_and(mask, cv2.bitwise_not(exclude_teal))
    mask = cv2.bitwise_and(mask, cv2.bitwise_not(exclude_purple))
 
    k    = np.ones((5, 5), np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN,  k)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, k)
 
    cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    if not cnts:
        return
 
    # Filter to arrow-like shapes: sparse (ext 0.20-0.65), low solidity
    candidates = []
    for cnt in cnts:
        area = cv2.contourArea(cnt)
        if area < MIN_AREA:
            continue
        peri = cv2.arcLength(cnt, True)
        if peri == 0:
            continue
        x, y, w, h = cv2.boundingRect(cnt)
        ext  = area / float(w * h) if w * h > 0 else 0
        hull = cv2.convexHull(cnt)
        sol  = area / cv2.contourArea(hull) if cv2.contourArea(hull) > 0 else 0
        # Arrow shape: not too filled, not too sparse
        if 0.20 <= ext <= 0.65 and sol <= 0.85:
            candidates.append((area, x, y, w, h))
 
    if not candidates:
        return
 
    # Take largest candidate
    candidates.sort(reverse=True)
    _, x, y, w, h = candidates[0]
 
    # Template match all 4 directions, pick best
    best_score = 0.0
    best_name  = None
    for name in ARROW_NAMES:
        if name not in templates:
            continue
        score = match_template(gray, templates[name], x, y, w, h)
        if score > best_score:
            best_score = score
            best_name  = name
 
    if best_name is None or best_score < 0.30:
        return
 
    found.append({"label": best_name, "score": round(best_score, 2), "bbox": (x, y, w, h)})
    col = DRAW_COLOURS.get(best_name, (255, 255, 255))
    cv2.rectangle(debug, (x, y), (x + w, y + h), col, 3)
    cv2.putText(debug, best_name + " " + str(round(best_score, 2)),
                (x, max(y - 8, 14)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, col, 2)
 
 
# ── SUPPRESS OVERLAPS ─────────────────────────────────────────────────────────
def suppress_overlaps(detections):
    if len(detections) <= 1:
        return detections
    kept = []
    for det in detections:
        x, y, w, h = det["bbox"]
        keep = True
        for k in kept:
            x2, y2, w2, h2 = k["bbox"]
            ix1 = max(x, x2);  iy1 = max(y, y2)
            ix2 = min(x+w, x2+w2); iy2 = min(y+h, y2+h2)
            inter = max(0, ix2-ix1) * max(0, iy2-iy1)
            union = w*h + w2*h2 - inter
            if union > 0 and inter/union > 0.35:
                if det["score"] > k["score"]:
                    kept.remove(k)
                else:
                    keep = False
                break
        if keep:
            kept.append(det)
    return kept
 
 
def detect_symbols(frame, templates):
    debug = frame.copy()
    hsv   = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
    gray  = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    found = []
 
    # ── Non-arrow symbols ─────────────────────────────────────────────────────
    for colour in COLOUR_GROUPS:
        mask = get_colour_mask(hsv, colour)
 
        if colour == "green" and "recycle" in COLOUR_GROUPS["green"]:
            is_rec, rbbox = check_recycle(mask)
            if is_rec:
                x, y, w, h = rbbox
                found.append({"label": "recycle", "score": 0.88, "bbox": (x, y, w, h)})
                col = DRAW_COLOURS["recycle"]
                cv2.rectangle(debug, (x, y), (x+w, y+h), col, 3)
                cv2.putText(debug, "recycle 0.88", (x, max(y-8, 14)),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.6, col, 2)
                continue
 
        cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        if not cnts:
            continue
        largest = max(cnts, key=cv2.contourArea)
        feat = get_features(largest, mask)
        if feat is None:
            continue
        x, y, w, h = feat["bbox"]
        name, conf = classify(colour, feat)
        if name is None:
            continue
        found.append({"label": name, "score": conf, "bbox": (x, y, w, h)})
        col = DRAW_COLOURS.get(name, (255, 255, 255))
        cv2.rectangle(debug, (x, y), (x+w, y+h), col, 3)
        cv2.putText(debug, name + " " + str(round(conf, 2)),
                    (x, max(y-8, 14)), cv2.FONT_HERSHEY_SIMPLEX, 0.6, col, 2)
 
    # ── Arrow detection - direction only ──────────────────────────────────────
    detect_arrow(frame, hsv, gray, templates, found, debug)
 
    found = suppress_overlaps(found)
    return found, debug
 
 
def main():
    print("=" * 55)
    print("Symbol Detection  -  Q to quit")
    print("=" * 55)
 
    picam = Picamera2()
    picam.configure(picam.create_preview_configuration(
        main={"size": (FRAME_WIDTH, FRAME_HEIGHT), "format": "BGR888"}
    ))
    picam.start()
    time.sleep(1)
 
    templates    = load_templates()
    last_time    = 0
    stable_counts = {}
    STABLE_NEEDED = 6
    frame_count  = 0
    last_debug   = None
 
    try:
        while True:
            frame = picam.capture_array()
            frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
 
            if frame_count % 3 == 0:
                detections, last_debug = detect_symbols(frame, templates)
                now = time.time()
                current_labels = set(d["label"] for d in detections)
                for label in list(stable_counts.keys()):
                    if label not in current_labels:
                        stable_counts[label] = 0
                for det in detections:
                    lbl = det["label"]
                    stable_counts[lbl] = stable_counts.get(lbl, 0) + 1
                    if stable_counts[lbl] >= STABLE_NEEDED and now - last_time > COOLDOWN:
                        print("[DETECTED]", lbl, "score", det["score"])
                        last_time = now
                        stable_counts[lbl] = 0
 
            display = last_debug if last_debug is not None else frame
            cv2.imshow("Symbol Detection", display)
            frame_count += 1
 
            if cv2.waitKey(1) & 0xFF == ord("q"):
                break
    finally:
        picam.stop()
        cv2.destroyAllWindows()
 
 
if __name__ == "__main__":
    main()
