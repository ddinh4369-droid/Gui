// ============================================================================
// --- 1. ĐỊNH NGHĨA CHÂN ĐỘNG CƠ MECANUM ---
// ============================================================================
#define PWM_FL 11
#define DIR_FL 8

#define PWM_RL 10
#define DIR_RL 7

#define PWM_FR 9
#define DIR_FR 4

#define PWM_RR 6
#define DIR_RR 5

// ============================================================================
// --- 2. ĐỊNH NGHĨA CHÂN CẢM BIẾN DÒ LINE & SIÊU ÂM ---
// ============================================================================
#define SENSOR_L2      A1  // Ngoài cùng bên trái (OUT1)
#define SENSOR_L1      A2  // Lệch trái (OUT2)
#define SENSOR_CENTER  A3  // Chính giữa (OUT3)
#define SENSOR_R1      A4  // Lệch phải (OUT4)

#define TRIG_PIN       12  // Chân phát xung siêu âm HC-SR04
#define ECHO_PIN       13  // Chân nhận xung siêu âm HC-SR04

// ============================================================================
// --- 3. BIẾN TRẠNG THÁI HỆ THỐNG ---
// ============================================================================
int speedFL = 0, speedFR = 0, speedRL = 0, speedRR = 0;
int currentMode = -1; // -1: Tự động chạy pin khi vừa khởi động

void executeMotor();
void stopCar();
void handleLineFollowing();
void handleObstacleAvoidance();
void handleAutoMode(); 

// ============================================================================
// --- 4. HÀM KHỞI TẠO CẤU HÌNH (SETUP) ---
// ============================================================================
void setup() {
  Serial.begin(9600);       
  Serial.setTimeout(10); // SỬA LỖI ĐỘ TRỄ: Ép thời gian đợi dữ liệu xuống 10ms
  
  pinMode(PWM_FL, OUTPUT); pinMode(DIR_FL, OUTPUT);
  pinMode(PWM_RL, OUTPUT); pinMode(DIR_RL, OUTPUT);
  pinMode(PWM_FR, OUTPUT); pinMode(DIR_FR, OUTPUT);
  pinMode(PWM_RR, OUTPUT); pinMode(DIR_RR, OUTPUT);
  
  // Khởi tạo chân cảm biến dò line
  pinMode(SENSOR_L2, INPUT);
  pinMode(SENSOR_L1, INPUT);
  pinMode(SENSOR_CENTER, INPUT);
  pinMode(SENSOR_R1, INPUT);
  
  // Khởi tạo chân Cảm biến siêu âm
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  stopCar(); // Khởi động xe ở trạng thái đứng im an toàn
}

// ============================================================================
// --- 5. VÒNG LẶP CHÍNH (LOOP) ---
// ============================================================================
void loop() {
  // ------------------------------------------------------------------------
  // LUỒNG 1: NHẬN LỆNH ĐIỀU KHIỂN TỪ GUI C++ (HOẶC AI PYTHON) GỬI XUỐNG
  // ------------------------------------------------------------------------
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    int modeIdx = cmd.indexOf("MODE:");
    int flIdx   = cmd.indexOf(";FL:");
    int frIdx   = cmd.indexOf(";FR:");
    int rlIdx   = cmd.indexOf(";RL:");
    int rrIdx   = cmd.indexOf(";RR:");
    int dirIdx  = cmd.indexOf(";DIR:");
    
    if (modeIdx != -1 && flIdx != -1 && frIdx != -1 && rlIdx != -1 && rrIdx != -1) {
      currentMode = cmd.substring(modeIdx + 5, flIdx).toInt();
      
      // Khẩn cấp: Nếu Mode = 0, lập tức phanh xe và thoát vòng lặp
      if (currentMode == 0) {
        stopCar(); 
        return; 
      }
      
      // Bóc tách hướng di chuyển (Dùng chung cho cả phím bấm Manual và lệnh rẽ tự động từ AI)
      String direction = "";
      if (dirIdx != -1) {
        int endDirIdx = cmd.indexOf(";", dirIdx + 5);
        direction = (endDirIdx != -1) ? cmd.substring(dirIdx + 5, endDirIdx) : cmd.substring(dirIdx + 5);
      }
      
      // Bóc tách vận tốc nền từ thanh Slider của GUI C++ đổ xuống
      speedFL = cmd.substring(flIdx + 4, frIdx).toInt();
      speedFR = cmd.substring(frIdx + 4, rlIdx).toInt();
      speedRL = cmd.substring(rlIdx + 4, rrIdx).toInt();
      
      int endRrIdx = cmd.indexOf(";", rrIdx + 4);
      speedRR = (endRrIdx != -1) ? cmd.substring(rrIdx + 4, endRrIdx).toInt() : cmd.substring(rrIdx + 4).toInt();
      
      // ------------------------------------------------------------------------
      // CHẾ ĐỘ 3 (TAY) HOẶC CHẾ ĐỘ 5 (AI CAMERA): ĐIỀU HƯỚNG MECANUM ĐỒNG BỘ
      // ------------------------------------------------------------------------
      if (currentMode == 3 || currentMode == 5) {
        int maxSpd = max(max(abs(speedFL), abs(speedFR)), max(abs(speedRL), abs(speedRR)));
        if (maxSpd == 0) maxSpd = 140; // Tốc độ nền an toàn chống kẹt động cơ nếu Slider bằng 0

        if (direction == "W") { // TIẾN THẲNG
          digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
          digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
          speedFL = speedFR = speedRL = speedRR = maxSpd;
        } 
        else if (direction == "S") { // LÙI XE / PHANH AI
          digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
          digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
          speedFL = speedFR = speedRL = speedRR = maxSpd;
        } 
        else if (direction == "A") { // XOAY LÁI SANG TRÁI (Ép xe sửa sai lệch làn phải)
          digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
          digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
          speedFL = speedFR = speedRL = speedRR = maxSpd;
        } 
        else if (direction == "D") { // XOAY LÁI SANG PHẢI (Ép xe sửa sai lệch làn trái)
          digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
          digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
          speedFL = speedFR = speedRL = speedRR = maxSpd;
        }
        else { // Trường hợp đổi chế độ chưa rõ hướng, triệt tiêu xung âm để bảo vệ driver
          speedFL = abs(speedFL); speedFR = abs(speedFR);
          speedRL = abs(speedRL); speedRR = abs(speedRR);
        }
        executeMotor(); 
      }
      
      // ------------------------------------------------------------------------
      // CHẾ ĐỘ 4: LIVE-TEST RIÊNG BIỆT TỪNG ĐỘNG CƠ (HỖ TRỢ ĐẢO CHIỀU XUNG ÂM)
      // ------------------------------------------------------------------------
      else if (currentMode == 4) {
        if (speedFL >= 0) { digitalWrite(DIR_FL, LOW); } else { digitalWrite(DIR_FL, HIGH); speedFL = abs(speedFL); }
        if (speedFR >= 0) { digitalWrite(DIR_FR, HIGH); } else { digitalWrite(DIR_FR, LOW);  speedFR = abs(speedFR); }
        if (speedRL >= 0) { digitalWrite(DIR_RL, LOW); } else { digitalWrite(DIR_RL, HIGH); speedRL = abs(speedRL); }
        if (speedRR >= 0) { digitalWrite(DIR_RR, HIGH); } else { digitalWrite(DIR_RR, LOW);  speedRR = abs(speedRR); }
        executeMotor();
      }
    }
  }
  
  // ------------------------------------------------------------------------
  // LUỒNG 2: KHI KHÔNG CÓ LỆNH SERIAL MỚI -> DUY TRÌ CHẠY TỰ ĐỘNG OFFLINE
  // ------------------------------------------------------------------------
  else {
    if (currentMode == 1) {
      handleLineFollowing(); 
    } 
    else if (currentMode == 2) {
      handleObstacleAvoidance();
    }
    else if (currentMode == 0) {
      stopCar(); 
    }
  }

  // ------------------------------------------------------------------------
  // LUỒNG 3: TỰ ĐỘNG GỬI TELEMETRY (CẢM BIẾN) LÊN MÁY TÍNH THEO CHU KỲ (NON-BLOCKING)
  // ------------------------------------------------------------------------
  static unsigned long lastSendTime = 0;
  if (millis() - lastSendTime > 80) { // Quét dữ liệu định kỳ mỗi 80ms để chống nghẽn cáp USB
    lastSendTime = millis();
    
    // 1. Kích hoạt và tính toán khoảng cách từ cảm biến siêu âm HC-SR04
    digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // Giới hạn Timeout 30ms tránh treo mạch
    float distance = (duration == 0) ? 99.0 : (duration * 0.034 / 2);
    
    // 2. Đọc trạng thái logic của 4 mắt hồng ngoại dò line
    int L2 = digitalRead(SENSOR_L2);     
    int L1 = digitalRead(SENSOR_L1);     
    int C  = digitalRead(SENSOR_CENTER); 
    int R1 = digitalRead(SENSOR_R1);     
    
    int lState = 0; 
    if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) { lState = 9; }     // Vạch ngang / Dừng bài
    else if (L2 == HIGH) { lState = -2; }                                        // Lệch trái nặng
    else if (L1 == HIGH) { lState = -1; }                                        // Lệch trái nhẹ
    else if (R1 == HIGH) { lState = 1; }                                         // Lệch phải nhẹ
    else if (C == HIGH)  { lState = 0; }                                         // Tâm đường thẳng
    else                 { lState = 404; }                                       // Mất line hoàn toàn
    
    // 3. Gửi gói tin chuẩn hóa lên Qt GUI (Cấm xóa hoặc thay đổi cấu trúc định dạng này)
    Serial.print("BAT:100;DIST:");
    Serial.print(distance, 1);
    Serial.print(";LINE:");
    Serial.print(lState);
    Serial.println(";"); 
  }
}

// ============================================================================
// --- HÀM TỰ ĐỘNG: DÒ LINE BẰNG HỒNG NGOẠI (ĂN KHỚP HOÀN TOÀN MODE 1) ---
// ============================================================================
void handleLineFollowing() {
  int L2 = digitalRead(SENSOR_L2); int L1 = digitalRead(SENSOR_L1);
  int C  = digitalRead(SENSOR_CENTER); int R1 = digitalRead(SENSOR_R1);
  
  if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) { stopCar(); return; }
  else if (L2 == HIGH) { 
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 120;
  }
  else if (L1 == HIGH) { 
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 70; speedFR = speedRR = 140;
  }
  else if (R1 == HIGH) { 
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 140; speedFR = speedRR = 70;
  }
  else if (C == HIGH) { 
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 130;
  }
  else { 
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 90;
  }
  executeMotor();
}

// ============================================================================
// --- HÀM TỰ ĐỘNG: NÉ VẬT CẢN BẰNG SIÊU ÂM (ĂN KHỚP HOÀN TOÀN MODE 2) ---
// ============================================================================
void handleObstacleAvoidance() {
  digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  int distance = (duration == 0) ? 999 : (duration * 0.034 / 2);
  
  if (distance > 0 && distance < 25) { // Phát hiện vật cản dưới 25cm
    stopCar(); delay(300);
    // Bước 1: Lùi xe lại một chút
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 130; executeMotor(); delay(250); 
    // Bước 2: Đánh lái xoay ngang dịch chuyển sang phải để tránh
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 140; executeMotor(); delay(300); 
    stopCar(); delay(100);
  } 
  else { // Đường trống -> Tiếp tục tiến thẳng tuần tra
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 130; executeMotor();
  }
}

// ============================================================================
// --- 6. HÀM CHẠY TỰ ĐỘNG KHÉP KÍN (TỰ CHẠY OFFLINE KHÔNG CẦN MÁY TÍNH) ---
// ============================================================================
void handleAutoMode() {
  // --- BƯỚC 1: QUÉT SIÊU ÂM KHẨN CẤP ĐỂ TÌM VẬT CẢN TRƯỚC ---
  digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  int distance = (duration == 0) ? 999 : (duration * 0.034 / 2);
  
  // Nếu có vật cản nguy hiểm phía trước (< 25cm), ưu tiên né vật cản trước
  if (distance > 0 && distance < 25) { 
    stopCar(); 
    delay(300);
    
    // Lùi xe an toàn
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 130;
    executeMotor();
    delay(250); 
    
    // Dạt ngang sang phải theo đặc tính bánh Mecanum để tránh vật cản
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 140;
    executeMotor();
    delay(300); 
    
    stopCar();
    delay(100);
    return; // Thoát hàm để vòng lặp sau kiểm tra lại đường đi
  }

  // --- BƯỚC 2: NẾU ĐƯỜNG TRỐNG, TỰ ĐỘNG ĐỌC CẢM BIẾN HỒNG NGOẠI ĐỂ DÒ LINE ---
  int L2 = digitalRead(SENSOR_L2);
  int L1 = digitalRead(SENSOR_L1);
  int C  = digitalRead(SENSOR_CENTER);
  int R1 = digitalRead(SENSOR_R1);

  if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) {
    stopCar(); // Gặp vạch ngang (vạch đích) -> Dừng xe
    return;
  }
  else if (L2 == HIGH) { // Lệch trái nặng -> Xoay cua gấp sang trái để bám lại vạch
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 130;
    speedFR = speedRR = 130;
  }
  else if (L1 == HIGH) { // Lệch trái nhẹ -> Chỉnh nhẹ sang trái
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 80;
    speedFR = speedRR = 140;
  }
  else if (R1 == HIGH) { // Lệch phải nhẹ -> Chỉnh nhẹ sang phải
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 140;
    speedFR = speedRR = 80;
  }
  else if (C == HIGH) { // Xe đang đi đúng tâm line -> Tiến thẳng đều
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 130;
    speedFR = speedRR = 130;
  }
  else {
    // Mất line hoàn toàn -> Dừng xe bảo vệ hệ thống không lao ra ngoài
    stopCar();
    return;
  }
  executeMotor(); // Thực thi xuất xung PWM ra chân Motor
}

// ============================================================================
// --- 6. CÁC HÀM CƠ SỞ XUẤT XUNG ĐIỀU KHIỂN PHẦN CỨNG ---
// ============================================================================
void executeMotor() {
  analogWrite(PWM_FL, speedFL);
  analogWrite(PWM_RL, speedRL);
  analogWrite(PWM_FR, speedFR);
  analogWrite(PWM_RR, speedRR);
}

void stopCar() {
  analogWrite(PWM_FL, 0); digitalWrite(DIR_FL, LOW);
  analogWrite(PWM_RL, 0); digitalWrite(DIR_RL, LOW);
  analogWrite(PWM_FR, 0); digitalWrite(DIR_FR, LOW);
  analogWrite(PWM_RR, 0); digitalWrite(DIR_RR, LOW);
  speedFL = speedRL = speedFR = speedRR = 0;
}