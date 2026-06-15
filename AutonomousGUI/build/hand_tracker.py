import cv2
import mediapipe as mp
import sys
import base64
import math
import numpy as np
import collections
from PIL import Image, ImageDraw, ImageFont # Thêm thư viện vẽ font chuyên nghiệp

print("PYTHON PATH:", sys.executable)
sys.stdout.flush()

mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
cap.set(cv2.CAP_PROP_FPS, 30)
if not cap.isOpened():
    print("ERROR:CamNotFound")
    sys.stdout.flush()
    sys.exit()

def calculate_distance(p1, p2):
    return math.sqrt((p1.x - p2.x)**2 + (p1.y - p2.y)**2)

circle_buffer = collections.deque(maxlen=15)

# --- KHỞI TẠO CẤU HÌNH FONT UNICODE (CHỐNG LỖI Ô VUÔNG) ---
# Thử tải Font Arial (mặc định Windows). Nếu không có, Pillow sẽ tự dùng font mặc định (nhưng có thể lỗi mũi tên)
try:
    # Có thể thay 'arial.ttf' bằng đường dẫn đến font VNI hoặc font khác nếu muốn tiếng Việt đẹp hơn
    FONT_UNICODE = ImageFont.truetype("arial.ttf", 18) 
    FONT_UNICODE_BIG = ImageFont.truetype("arial.ttf", 24)
except IOError:
    FONT_UNICODE = ImageFont.load_default()
    FONT_UNICODE_BIG = ImageFont.load_default()

def draw_unicode_text(img, text, position, color=(255, 255, 0), font=FONT_UNICODE):
    """ Hàm phụ hỗ trợ vẽ chữ Unicode (mũi tên, tiếng Việt) lên ảnh OpenCV """
    img_pil = Image.fromarray(img) # Chuyển ảnh OpenCV sang PIL
    draw = ImageDraw.Draw(img_pil)
    draw.text(position, text, font=font, fill=color[::-1]) # Vẽ chữ (lật ngược màu do PIL dùng RGB)
    return np.array(img_pil) # Chuyển ngược về ảnh OpenCV

def detect_circle(points):
    """ Giữ nguyên thuật toán quét vòng tròn cũ """
    if len(points) < 10: return False
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    cx, cy = np.mean(xs), np.mean(ys)
    radii = [math.sqrt((x - cx)**2 + (y - cy)**2) for x, y in zip(xs, ys)]
    if not radii: return False
    mean_radius = np.mean(radii)
    if mean_radius < 0.03: return False
    std_radius = np.std(radii)
    return (std_radius / mean_radius) < 0.35

def process_ai_lane_detection(img):
    """ Giữ nguyên bộ lọc Canny + Hough Lines cũ """
    height, width = img.shape[:2]
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    edges = cv2.Canny(blur, 50, 150)
    
    mask = np.zeros_like(edges)
    polygon = np.array([[(0, height), (width, height), (int(width * 0.85), int(height * 0.5)), (int(width * 0.15), int(height * 0.5))]], np.int32)
    cv2.fillPoly(mask, polygon, 255)
    cropped_edges = cv2.bitwise_and(edges, mask)
    
    lines = cv2.HoughLinesP(cropped_edges, 1, np.pi/180, 20, minLineLength=30, maxLineGap=10)
    left_lines, right_lines = [], []
    
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            if x2 == x1: continue
            slope = (y2 - y1) / (x2 - x1)
            if slope < -0.3: left_lines.append((x1, y1, x2, y2))
            elif slope > 0.3: right_lines.append((x1, y1, x2, y2))
            cv2.line(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
            
    if len(left_lines) > 0 and len(right_lines) == 0: return img, "A"
    elif len(right_lines) > 0 and len(left_lines) == 0: return img, "D"
    elif len(left_lines) == 0 and len(right_lines) == 0: return img, "S"
    else: return img, "W"

# Khởi tạo MediaPipeHands
with mp_hands.Hands(
    static_image_mode=False, max_num_hands=2,
    min_detection_confidence=0.65, min_tracking_confidence=0.65
) as hands:
    
    last_sent_mode = -1
    
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret: break
            
        frame = cv2.flip(frame, 1)
        frame_resized = cv2.resize(frame, (800, 600))
        kernel = np.array([
            [0,-1,0],
            [-1,5,-1],
            [0,-1,0]
        ])

        frame_resized = cv2.filter2D(frame_resized, -1, kernel)
        rgb_frame = cv2.cvtColor(frame_resized, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb_frame)
        
        hand1_fingers = -1
        hand2_direction = "HOLD" 
        
        if results.multi_hand_landmarks:
            paired_hands = []
            for res in results.multi_hand_landmarks: paired_hands.append(res)
            
            computed_fingers = []
            hand_sign_vectors = [] 
            all_hand_landmarks = []
            
            for hl in paired_hands:
                mp_drawing.draw_landmarks(frame_resized, hl, mp_hands.HAND_CONNECTIONS)
                landmarks = hl.landmark
                all_hand_landmarks.append(landmarks)
                
                if calculate_distance(landmarks[4], landmarks[9]) > calculate_distance(landmarks[3], landmarks[9]): thumb = 1
                else: thumb = 0
                
                fingers_list = [thumb]; tips = [8, 12, 16, 20]; pips = [6, 10, 14, 18]
                for tip, pip in zip(tips, pips):
                    if landmarks[tip].y < landmarks[pip].y: fingers_list.append(1)
                    else: fingers_list.append(0)
                
                computed_fingers.append(sum(fingers_list))
                hand_sign_vectors.append((landmarks[0], landmarks[5], landmarks[8]))

            h1_idx = -1; h2_idx = -1
            for i, f_cnt in enumerate(computed_fingers):
                if f_cnt == 5: h1_idx = i; break
            
            if h1_idx != -1:
                hand1_fingers = 5
                for j in range(len(computed_fingers)):
                    if j != h1_idx: h2_idx = j; break
            else:
                if len(computed_fingers) > 0: hand1_fingers = computed_fingers[0]

            if h1_idx != -1 and h2_idx != -1:
                wrist, f_base, f_tip = hand_sign_vectors[h2_idx]
                h2_lms = all_hand_landmarks[h2_idx]
                
                if calculate_distance(f_tip, wrist) > calculate_distance(f_base, wrist) * 1.25:
                    circle_buffer.append((f_tip.x, f_tip.y))
                    
                    if detect_circle(circle_buffer):
                        hand2_direction = "CIRCLE"
                    else:
                        dx = f_tip.x - f_base.x; dy = f_tip.y - f_base.y
                        angle_deg = math.degrees(math.atan2(-dy, dx))
                        if angle_deg < 0: angle_deg += 360

                        if 45 <= angle_deg < 135: hand2_direction = "W" 
                        elif 135 <= angle_deg < 225: hand2_direction = "A" 
                        elif 225 <= angle_deg < 315: hand2_direction = "S" 
                        else: hand2_direction = "D"
                else: circle_buffer.clear()
            else: circle_buffer.clear()

        if hand1_fingers == 5:
            text_color = (0, 255, 255) # Màu vàng cyan
            
            if hand2_direction == "HOLD":
                frame_resized, ai_direction = process_ai_lane_detection(frame_resized)
                status_text = f"AI LÁI TỰ ĐỘNG: {ai_direction}"
            else:
                ai_direction = hand2_direction
                # Đổi tên hiển thị cho trực quan
                direction_map = {"W": "TIẾN", "S": "LÙI", "A": "XOAY TRÁI", "D": "XOAY PHẢI", "CIRCLE": "QUAY VÒNG"}
                text_color = (255, 255, 0) # Màu vàng gold
                status_text = f"TAY 2 CHIẾM QUYỀN: {direction_map.get(ai_direction, ai_direction)}"
            
            # --- SỬA LỖI FONT: SỬ DỤNG HÀM PHỤ VẼ FONT PIL UNICODE ---
            frame_resized = draw_unicode_text(frame_resized, status_text, (15, 50), color=text_color, font=FONT_UNICODE)
            frame_resized = draw_unicode_text(frame_resized, "HUST Autonomous Station | AI VISION ACTIVE", (15, 270), color=(0, 255, 0), font=FONT_UNICODE)
            
            cmd = f"MODE:5;FL:145;FR:145;RL:145;RR:145;DIR:{ai_direction};\n"
            sys.stdout.write(cmd)
            sys.stdout.flush()
            last_sent_mode = 5
        else:
            if last_sent_mode == 5:
                cmd = "MODE:3;FL:0;FR:0;RL:0;RR:0;DIR:W;\n"
                sys.stdout.write(cmd)
                sys.stdout.flush()
                last_sent_mode = -1

        _, jpeg = cv2.imencode('.jpg', frame_resized, [cv2.IMWRITE_JPEG_QUALITY, 85])
        b64_data = base64.b64encode(jpeg).decode('utf-8')
        print(f"{hand1_fingers}:{b64_data}")
        sys.stdout.flush()
        
cap.release()