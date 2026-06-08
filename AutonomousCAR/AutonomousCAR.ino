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
int currentMode = -1; // -1: Tự động chạy kết hợp (Line + Siêu âm) khi vừa khởi động

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
  Serial.setTimeout(10); // Ép thời gian đợi dữ liệu xuống 10ms chống trễ mẫu
  
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
  // LUỒNG 1: NHẬN LỆNH ĐIỀU KHIỂN TỪ GUI C++ (ĐÃ SỬA ĐỔI BÓC TÁCH CHUẨN)
  // ------------------------------------------------------------------------
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    // Khởi tạo các biến tạm để lưu dữ liệu bóc tách
    int p_mode = -1;
    int p_fl = -1, p_fr = -1, p_rl = -1, p_rr = -1;
    String p_dir = "";
    
    // Tách chuỗi theo từng cặp key:value dựa trên dấu ';'
    int startIdx = 0;
    int endIdx = cmd.indexOf(';');
    
    while (endIdx != -1) {
      String token = cmd.substring(startIdx, endIdx);
      token.trim();
      
      int colonIdx = token.indexOf(':');
      if (colonIdx != -1) {
        String key = token.substring(0, colonIdx);
        String val = token.substring(colonIdx + 1);
        
        if (key == "MODE") p_mode = val.toInt();
        else if (key == "FL")  p_fl = val.toInt();
        else if (key == "FR")  p_fr = val.toInt();
        else if (key == "RL")  p_rl = val.toInt();
        else if (key == "RR")  p_rr = val.toInt();
        else if (key == "DIR") p_dir = val;
      }
      
      startIdx = endIdx + 1;
      endIdx = cmd.indexOf(';', startIdx);
    }
    
    // Nếu bóc tách thành công đầy đủ các tham số kiểm tra
    if (p_mode != -1) {
      currentMode = p_mode;
      
      if (currentMode == 0) {
        stopCar();
        return;
      }
      
      // Gán giá trị an toàn vào biến toàn cục
      speedFL = (p_fl != -1) ? p_fl : 0;
      speedFR = (p_fr != -1) ? p_fr : 0;
      speedRL = (p_rl != -1) ? p_rl : 0;
      speedRR = (p_rr != -1) ? p_rr : 0;
      
      // ------------------------------------------------------------------------
      // CHẾ ĐỘ 3 (TAY) HOẶC CHẾ ĐỘ 5: ĐIỀU HƯỚNG THEO ĐÚNG Ý ĐỨC (TRẢ VỀ PHẦN CỨNG GỐC)
      // ------------------------------------------------------------------------
      if (currentMode == 3 || currentMode == 5) {
        // Trả lại nguyên vẹn cấu hình chân DIR ban đầu đang chạy đúng của bạn
        if (p_dir == "W") { // TIẾN THẲNG
          digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
          digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
        } 
        else if (p_dir == "S") { // LÙI XE
          digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
          digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
        } 
        else if (p_dir == "A") { // XOAY LÁI TRÁI
          digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
          digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
        } 
        else if (p_dir == "D") { // XOAY LÁI PHẢI
          digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
          digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
        }

        // Cấp xung PWM chuẩn, bánh nào = 0 thì ngắt hẳn chân đó
        if (speedFL > 0)  { analogWrite(PWM_FL, speedFL); }   else { analogWrite(PWM_FL, 0); }
        if (speedFR > 0)  { analogWrite(PWM_FR, speedFR); }   else { analogWrite(PWM_FR, 0); }
        if (speedRL > 0)  { analogWrite(PWM_RL, speedRL); }   else { analogWrite(PWM_RL, 0); }
        if (speedRR > 0)  { analogWrite(PWM_RR, speedRR); }   else { analogWrite(PWM_RR, 0); }
      }
      
      // Chế độ 4: Live test độc lập
      else if (currentMode == 4) {
        digitalWrite(DIR_FL, (speedFL >= 0) ? LOW : HIGH);   analogWrite(PWM_FL, abs(speedFL));
        digitalWrite(DIR_FR, (speedFR >= 0) ? HIGH : LOW);   analogWrite(PWM_FR, abs(speedFR));
        digitalWrite(DIR_RL, (speedRL >= 0) ? LOW : HIGH);   analogWrite(PWM_RL, abs(speedRL));
        digitalWrite(DIR_RR, (speedRR >= 0) ? HIGH : LOW);   analogWrite(PWM_RR, abs(speedRR));
      }
    }
  }
  
  // ------------------------------------------------------------------------
  // LUỒNG 2: KHI KHÔNG CÓ LỆNH SERIAL MỚI -> DUY TRÌ CHẠY TỰ ĐỘNG OFFLINE
  // ------------------------------------------------------------------------
  else {
    if (currentMode == -1) {   // Mặc định ban đầu hoặc chế độ kết hợp hoàn toàn
      handleAutoMode();
    }
    else if (currentMode == 1) {
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
  if (millis() - lastSendTime > 80) { 
    lastSendTime = millis();
    
    // Đọc cảm biến siêu âm
    digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); 
    float distance = (duration == 0) ? 99.0 : (duration * 0.034 / 2);
    
    // Đọc cảm biến dò line
    int L2 = digitalRead(SENSOR_L2);     
    int L1 = digitalRead(SENSOR_L1);     
    int C  = digitalRead(SENSOR_CENTER); 
    int R1 = digitalRead(SENSOR_R1);     
    
    int lState = 0; 
    if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) { lState = 9; }    
    else if (L2 == HIGH) { lState = -2; }                                       
    else if (L1 == HIGH) { lState = -1; }                                       
    else if (R1 == HIGH) { lState = 1; }                                        
    else if (C == HIGH)  { lState = 0; }                                        
    else                 { lState = 404; }                                      
    
    // Gửi gói tin lên Qt GUI
    Serial.print("BAT:100;DIST:");
    Serial.print(distance, 1);
    Serial.print(";LINE:");
    Serial.print(lState);
    Serial.println(";"); 
  }
}

// ============================================================================
// --- HÀM TỰ ĐỘNG: DÒ LINE BẰNG HỒNG NGOẠI (MODE 1) ---
// ============================================================================
void handleLineFollowing() {
  int L2 = digitalRead(SENSOR_L2); int L1 = digitalRead(SENSOR_L1);
  int C  = digitalRead(SENSOR_CENTER); int R1 = digitalRead(SENSOR_R1);
  
  if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) { stopCar(); return; }
  else if (L2 == HIGH) { // Lệch trái nặng -> Xoay cua trái gấp
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 120;
  }
  else if (L1 == HIGH) { // Lệch trái nhẹ -> Chỉnh lái hướng sang trái
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 70; speedFR = speedRR = 140;
  }
  else if (R1 == HIGH) { // Lệch phải nhẹ -> Chỉnh lái hướng sang phải
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 140; speedFR = speedRR = 70;
  }
  else if (C == HIGH) { // Đi đúng tâm
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
// --- HÀM TỰ ĐỘNG: NÉ VẬT CẢN BẰNG SIÊU ÂM (MODE 2) ---
// ============================================================================
void handleObstacleAvoidance() {
  digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  int distance = (duration == 0) ? 999 : (duration * 0.034 / 2);
  
  if (distance > 0 && distance < 25) { 
    stopCar(); delay(300);
    // Bước 1: Lùi xe an toàn
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 130; executeMotor(); delay(300); 
    
    // Bước 2: DỊCH CHUYỂN NGANG SANG PHẢI (Chuẩn đặc tính bánh Mecanum)
    // FL: Tiến, RL: Lùi, FR: Lùi, RR: Tiến
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 150; executeMotor(); delay(500); 
    stopCar(); delay(100);
  } 
  else { 
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 130; executeMotor();
  }
}

// ============================================================================
// --- 6. HÀM CHẠY TỰ ĐỘNG KHÉP KÍN (KẾT HỢP DÒ LINE + NÉ VẬT CẢN) ---
// ============================================================================
void handleAutoMode() {
  // --- BƯỚC 1: QUÉT SIÊU ÂM KHẨN CẤP ĐỂ TÌM VẬT CẢN TRƯỚC ---
  digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  int distance = (duration == 0) ? 999 : (duration * 0.034 / 2);
  
  if (distance > 0 && distance < 25) { 
    stopCar(); delay(300);
    
    // Lùi xe an toàn
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, LOW);
    speedFL = speedRL = speedFR = speedRR = 130; executeMotor(); delay(300); 
    
    // Dạt ngang sang phải để tránh vật cản (Đúng chuẩn Mecanum)
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, LOW);  digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 150; executeMotor(); delay(600); 
    
    stopCar(); delay(100);
    return; 
  }

  // --- BƯỚC 2: NẾU ĐƯỜNG TRỐNG, TỰ ĐỘNG DÒ LINE ---
  int L2 = digitalRead(SENSOR_L2); int L1 = digitalRead(SENSOR_L1);
  int C  = digitalRead(SENSOR_CENTER); int R1 = digitalRead(SENSOR_R1);

  if (L2 == HIGH && L1 == HIGH && C == HIGH && R1 == HIGH) {
    stopCar(); 
    return;
  }
  else if (L2 == HIGH) { // Lệch trái nặng -> Cua trái gấp
    digitalWrite(DIR_FL, HIGH); digitalWrite(DIR_RL, HIGH);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 130;
  }
  else if (L1 == HIGH) { // Lệch trái nhẹ -> Chỉnh lái nhẹ sang trái
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 80; speedFR = speedRR = 140;
  }
  else if (R1 == HIGH) { // Lệch phải nhẹ -> Chỉnh lái nhẹ sang phải
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = 140; speedFR = speedRR = 80;
  }
  else if (C == HIGH) { // Đúng tâm -> Tiến đều
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 130;
  }
  else {
    // Mất line hoàn toàn -> Giảm tốc tìm lại line hoặc dừng an toàn (Tùy cấu hình, ở đây giữ tốc độ thấp để bò tìm vạch)
    digitalWrite(DIR_FL, LOW);  digitalWrite(DIR_RL, LOW);
    digitalWrite(DIR_FR, HIGH); digitalWrite(DIR_RR, HIGH);
    speedFL = speedRL = speedFR = speedRR = 90;
  }
  executeMotor(); 
}

// ============================================================================
// --- 7. CÁC HÀM CƠ SỞ XUẤT XUNG ĐIỀU KHIỂN PHẦN CỨNG ---
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