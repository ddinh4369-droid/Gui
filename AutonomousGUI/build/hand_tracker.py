import cv2
import mediapipe as mp
import sys
import base64
import math
import numpy as np

print("PYTHON PATH:", sys.executable)
sys.stdout.flush()

# Khởi tạo MediaPipe Hands
mp_hands = mp.solutions.hands
mp_drawing = mp.solutions.drawing_utils

cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("ERROR:CamNotFound")
    sys.stdout.flush()
    sys.exit()

def calculate_distance(p1, p2):
    return math.sqrt((p1.x - p2.x)**2 + (p1.y - p2.y)**2)

# --- THUẬT TOÁN AI: TỰ ĐỘNG PHÂN TÍCH VẠCH ĐƯỜNG (LANE DETECTION) ---
def process_ai_lane_detection(img):
    """
    Thuật toán AI xử lý ảnh để tìm làn đường và đưa ra quyết định bẻ lái tự động
    Trả về: ảnh đã vẽ đường làn, và chuỗi lệnh điều hướng tương ứng (W, A, D, S)
    """
    height, width = img.shape[:2]
    
    # 1. Chuyển ảnh sang màu xám và làm mịn để khử nhiễu
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    
    # 2. Phát hiện cạnh bằng bộ lọc Canny
    edges = cv2.Canny(blur, 50, 150)
    
    # 3. Tạo mặt nạ vùng quan tâm (Region of Interest - ROI): Chỉ lấy nửa dưới của ảnh camera (mặt đường)
    mask = np.zeros_like(edges)
    polygon = np.array([[
        (0, height),
        (width, height),
        (int(width * 0.85), int(height * 0.5)),
        (int(width * 0.15), int(height * 0.5))
    ]], np.int32)
    cv2.fillPoly(mask, polygon, 255)
    cropped_edges = cv2.bitwise_and(edges, mask)
    
    # 4. Tìm các đường thẳng bằng thuật toán Hough Lines Transform
    lines = cv2.HoughLinesP(cropped_edges, 1, np.pi/180, 20, minLineLength=30, maxLineGap=10)
    
    left_lines = []
    right_lines = []
    
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            if x2 == x1:
                continue
            slope = (y2 - y1) / (x2 - x1)
            # Dựa vào độ dốc (slope) để phân loại vạch trái / vạch phải
            if slope < -0.3: # Vạch bên trái
                left_lines.append((x1, y1, x2, y2))
            elif slope > 0.3: # Vạch bên phải
                right_lines.append((x1, y1, x2, y2))
                
    # Vẽ các đường vạch phát hiện được lên màn hình để hiển thị lên Qt GUI
    if lines is not None:
        for line in lines:
            x1, y1, x2, y2 = line[0]
            cv2.line(img, (x1, y1), (x2, y2), (0, 255, 0), 2) # Vẽ đường màu xanh lá

    # 5. RA QUYẾT ĐỊNH ĐIỀU KHIỂN (AI LOGIC)
    # So sánh mật độ vạch đường để biết đường đang rẽ hay đi thẳng
    if len(left_lines) > 0 and len(right_lines) == 0:
        cv2.putText(img, "AI: TURN LEFT (<-)", (15, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        return img, "A" # Lệch phải -> Ép xe rẽ trái để sửa sai
    elif len(right_lines) > 0 and len(left_lines) == 0:
        cv2.putText(img, "AI: TURN RIGHT (->)", (15, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        return img, "D" # Lệch trái -> Ép xe rẽ phải để sửa sai
    elif len(left_lines) == 0 and len(right_lines) == 0:
        cv2.putText(img, "AI: NO LINEFOUND - STOP", (15, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)
        return img, "S" # Mất hoàn toàn vạch -> Dừng xe cho an toàn
    else:
        cv2.putText(img, "AI: FORWARD (FORWARD)", (15, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        return img, "W" # Thấy cả 2 vạch cân đối -> Tiến thẳng

# --- CẤU HÌNH BỘ NHẬN DIỆN BÀN TAY MEDIAPIPE ---
with mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.65,
    min_tracking_confidence=0.65
) as hands:
    
    # Biến nhớ trạng thái Mode để tối ưu hóa, tránh bắn lệnh trùng lặp liên tục
    last_sent_mode = -1
    
    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break
            
        frame = cv2.flip(frame, 1)
        frame_resized = cv2.resize(frame, (400, 300))
        rgb_frame = cv2.cvtColor(frame_resized, cv2.COLOR_BGR2RGB)
        results = hands.process(rgb_frame)
        
        finger_count = -1  # -1 tức là không thấy bàn tay
        
        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                # Chỉ vẽ khung xương tay nếu đang ở các Mode 0, 1, 2, 3, 4
                mp_drawing.draw_landmarks(frame_resized, hand_landmarks, mp_hands.HAND_CONNECTIONS)
                
                landmarks = hand_landmarks.landmark
                
                # Kiểm tra Ngón Cái
                if calculate_distance(landmarks[4], landmarks[9]) > calculate_distance(landmarks[3], landmarks[9]):
                    thumb = 1
                else:
                    thumb = 0
                
                # 4 ngón còn lại
                fingers = [thumb]
                tips = [8, 12, 16, 20]
                pips = [6, 10, 14, 18]
                
                for tip, pip in zip(tips, pips):
                    if landmarks[tip].y < landmarks[pip].y:
                        fingers.append(1)
                    else:
                        fingers.append(0)
                
                finger_count = sum(fingers)

        # ⭐ XỬ LÝ ĐẶC BIỆT CHO CHẾ ĐỘ 5 NGÓN (AI VISION)
        if finger_count == 5:
            # Khi kích hoạt Mode 5, ảnh camera thu được sẽ được nạp vào bộ lọc AI Lane Detection
            frame_resized, ai_direction = process_ai_lane_detection(frame_resized)
            
            # Ghi chú nhỏ đè lên ảnh thể hiện AI đang chiếm quyền điều khiển xe
            cv2.putText(frame_resized, "AI AUTONOMOUS MODE ACTIVE", (15, 280), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1, cv2.LINE_AA)

        # Nén ảnh kết quả thành Base64 để bắn lên giao diện Qt hiển thị
        _, jpeg = cv2.imencode('.jpg', frame_resized, [cv2.IMWRITE_JPEG_QUALITY, 55])
        b64_data = base64.b64encode(jpeg).decode('utf-8')
        
        # Đẩy dữ liệu qua stdout dạng "FINGERS:BASE64_IMAGE"
        print(f"{finger_count}:{b64_data}")
        sys.stdout.flush()
        
cap.release()