#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>
#include <QCoreApplication>
#include <QTimer>
#include <windows.h> 

// Biến Handle quản lý cổng kết nối Serial của Windows
HANDLE hSerial = INVALID_HANDLE_VALUE;

// ============================================================================
// KHỐI 1: KHỞI TẠO & GIẢI PHÓNG HỆ THỐNG (VÒNG ĐỜI ỨNG DỤNG)
// ============================================================================

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    currentMode = 0;

    setWindowTitle("HUST Autonomous Control Station Pro");
    resize(1200, 650);
    
    setStyleSheet("QMainWindow { background-color: #11121a; } "
                  "QLabel { color: #e2e8f0; font-family: 'Segoe UI'; font-size: 13px; } "
                  "QGroupBox { color: #00ffcc; font-weight: bold; font-size: 13px; border: 1px solid #334155; margin-top: 12px; border-radius: 6px; } "
                  "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 5px; } "
                  "QPushButton { background-color: #1e293b; color: #00ffcc; border: 1px solid #475569; border-radius: 4px; font-weight: bold; padding: 8px; } "
                  "QPushButton:pressed { background-color: #334155; }");

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // CỘT TRÁI
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(12);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/HUST.jpg"); 
    logoLabel->setPixmap(logoPixmap.scaledToHeight(100, Qt::SmoothTransformation));
    
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setStyleSheet("line-height: 1.4; font-family: 'Segoe UI';");
    titleLabel->setText("<span style='font-size: 15px; font-weight: bold; color: #f59e0b;'>ĐẠI HỌC BÁCH KHOA HÀ NỘI<br>"
                        "TRUNG TÂM ĐIỀU KHIỂN XE TỰ HÀNH</span><br>"
                        "<span style='font-size: 11px; font-weight: normal; color: #94a3b8;'>Đinh Duy Đức - 20236111</span><br>"
                        "<span style='font-size: 11px; font-weight: normal; color: #94a3b8;'>Chu Minh Dương - 20236132</span><br>"
                        "<span style='font-size: 11px; font-weight: normal; color: #94a3b8;'>Nguyễn Tiến Đạt - 20236104</span>");
    
    headerLayout->addWidget(logoLabel);
    headerLayout->addSpacing(10); 
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    leftLayout->addLayout(headerLayout);

    // Group 1: Giám sát telemetry
    QGroupBox *telemetryGroup = new QGroupBox("GIÁM SÁT THỜI GIAN THỰC", this);
    QFormLayout *teleLayout = new QFormLayout(telemetryGroup);
    teleLayout->setContentsMargins(15, 15, 15, 15);
    teleLayout->setSpacing(10);
    
    speedDisplay = new QLCDNumber();
    speedDisplay->setSegmentStyle(QLCDNumber::Flat);
    speedDisplay->setFixedSize(140, 55);
    speedDisplay->setStyleSheet("color: #00ffcc; background: #1e293b; border: 1px solid #475569; border-radius: 4px;");
    
    batteryBar = new QProgressBar();
    batteryBar->setRange(0, 100);
    batteryBar->setValue(100);
    batteryBar->setFixedSize(220, 22);
    batteryBar->setStyleSheet("QProgressBar { border: 1px solid #475569; border-radius: 4px; text-align: center; color: white; font-weight: bold; } "
                              "QProgressBar::chunk { background-color: #10b981; border-radius: 3px; }");

    currentModeLabel = new QLabel("CHẾ ĐỘ: ĐANG KIỂM TRA CỔNG KẾT NỐI CÁP USB...", this);
    currentModeLabel->setStyleSheet("font-size: 13px; color: #eab308; font-weight: bold;");

    lineStatus = new QLabel("Dò line: CHƯA CÓ DỮ LIỆU", this);
    obstacleWarning = new QLabel("Khoảng cách cản: -- cm", this);

    teleLayout->addRow("VẬN TỐC MASTER:", speedDisplay);
    teleLayout->addRow("MỨC NĂNG LƯỢNG PIN XE:", batteryBar);
    teleLayout->addRow("CẢM BIẾN HỒNG NGOẠI:", lineStatus);
    teleLayout->addRow("CẢM BIẾN SIÊU ÂM:", obstacleWarning);
    teleLayout->addRow("TRẠNG THÁI HỆ THỐNG:", currentModeLabel);
    leftLayout->addWidget(telemetryGroup);

    // Group 2: BẢNG ĐIỀU KHIỂN NÚT BẤM
    QGroupBox *controlGroup = new QGroupBox("BẢNG ĐIỀU KHIỂN HỆ THỐNG", this);
    QVBoxLayout *controlGroupLayout = new QVBoxLayout(controlGroup);
    controlGroupLayout->setContentsMargins(15, 15, 15, 15);
    controlGroupLayout->setSpacing(15);

    QHBoxLayout *masterLayout = new QHBoxLayout();
    QLabel *masterTitle = new QLabel("GA TỔNG (SPEED):", this);
    masterTitle->setStyleSheet("font-weight: bold; color: #cbd5e1;");
    
    masterSlider = new QSlider(Qt::Horizontal);
    masterSlider->setRange(-255, 255);
    masterSlider->setValue(0);
    masterSlider->setStyleSheet("QSlider::groove:horizontal { background: #334155; height: 6px; border-radius: 3px; } "
                                "QSlider::handle:horizontal { background: #f59e0b; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px; }");
    
    masterValueLabel = new QLabel("0", this);
    masterValueLabel->setFixedWidth(30);
    masterValueLabel->setStyleSheet("font-weight: bold; color: #f59e0b; font-size: 14px;");

    masterLayout->addWidget(masterTitle);
    masterLayout->addWidget(masterSlider);
    masterLayout->addWidget(masterValueLabel);
    controlGroupLayout->addLayout(masterLayout);

    btnUp = new QPushButton("TIẾN (W)", this);
    btnDown = new QPushButton("LÙI (S)", this);
    btnLeft = new QPushButton("TRÁI (A)", this);
    btnRight = new QPushButton("PHẢI (D)", this);
    btnStop = new QPushButton("STOP", this);
    btnStop->setStyleSheet("QPushButton { background-color: #b91c1c; color: white; border: none; } QPushButton:pressed { background-color: #7f1d1d; }");

    QGridLayout *dPadLayout = new QGridLayout();
    dPadLayout->setSpacing(8);
    dPadLayout->addWidget(btnUp,    0, 1);
    dPadLayout->addWidget(btnLeft,  1, 0);
    dPadLayout->addWidget(btnStop,  1, 1); 
    dPadLayout->addWidget(btnRight, 1, 2);
    dPadLayout->addWidget(btnDown,  2, 1);
    
    controlGroupLayout->addLayout(dPadLayout);
    leftLayout->addWidget(controlGroup);

    btnReset = new QPushButton("RESET HỆ THỐNG (EMERGENCY STOP)", this);
    btnReset->setStyleSheet("QPushButton { background-color: #dc2626; color: white; padding: 12px; font-weight: bold; font-size: 14px; border-radius: 5px; border: none; } "
                            "QPushButton:pressed { background-color: #991b1b; }");
    leftLayout->addWidget(btnReset);

    mainLayout->addLayout(leftLayout, 4);
    
    // --- THÊM: BẢNG ĐIỀU KHIỂN ĐỘNG CƠ ĐỘC LẬP CHẾ ĐỘ 4 ---
    QLabel *motorTitle = new QLabel("TEST ĐỘNG CƠ ĐỘC LẬP:", this);
    motorTitle->setStyleSheet("font-weight: bold; color: #cbd5e1; margin-top: 5px;");
    controlGroupLayout->addWidget(motorTitle);

    btnFL = new QPushButton("TRƯỚC TRÁI (FL)", this);
    btnFR = new QPushButton("TRƯỚC PHẢI (FR)", this);
    btnRL = new QPushButton("SAU TRÁI (RL)", this);
    btnRR = new QPushButton("SAU PHẢI (RR)", this);

    // Cấu hình Style riêng cho các nút test động cơ (màu xanh dương đậm)
    QString motorBtnStyle = "QPushButton { background-color: #1e3a8a; color: #00ffcc; border: 1px solid #3b82f6; border-radius: 4px; padding: 6px; font-size: 11px; }"
                            "QPushButton:pressed { background-color: #2563eb; }";
    btnFL->setStyleSheet(motorBtnStyle);
    btnFR->setStyleSheet(motorBtnStyle);
    btnRL->setStyleSheet(motorBtnStyle);
    btnRR->setStyleSheet(motorBtnStyle);

    // Bố trí 4 nút thành dạng lưới 2x2
    QGridLayout *motorGrid = new QGridLayout();
    motorGrid->setSpacing(6);
    motorGrid->addWidget(btnFL, 0, 0);
    motorGrid->addWidget(btnFR, 0, 1);
    motorGrid->addWidget(btnRL, 1, 0);
    motorGrid->addWidget(btnRR, 1, 1);
    controlGroupLayout->addLayout(motorGrid);

    // CỘT PHẢI
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(8);
    
    QLabel *camTitle = new QLabel(" LUỒNG VIDEO CAMERA STREAM (MEDIAPIPE DEEP LEARNING) ", this);
    camTitle->setStyleSheet("font-size: 13px; font-weight: bold; color: #00ffcc;");
    rightLayout->addWidget(camTitle);

    cameraDisplay = new QLabel("ĐANG KẾT NỐI CAMERA...", this);
    cameraDisplay->setFixedSize(800, 600); 
    cameraDisplay->setAlignment(Qt::AlignCenter);
    cameraDisplay->setStyleSheet("background-color: #090a0f; border: 2px dashed #00ffcc; border-radius: 8px;");
    rightLayout->addWidget(cameraDisplay);

    mainLayout->addLayout(rightLayout, 6); 

    // --- KẾT NỐI CỔNG SERIAL USB BẰNG WIN32 API CHO ARDUINO UNO ---
    LPCSTR portName = "\\\\.\\COM4"; 
    hSerial = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        currentModeLabel->setText("LỖI: KHÔNG MỞ ĐƯỢC CỔNG USB CỦA XE! KIỂM TRA LẠI DÂY CẮM.");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
    } else {
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (GetCommState(hSerial, &dcbSerialParams)) {
            dcbSerialParams.BaudRate = CBR_9600; 
            dcbSerialParams.ByteSize = 8;
            dcbSerialParams.StopBits = ONESTOPBIT;
            dcbSerialParams.Parity = NOPARITY;
            SetCommState(hSerial, &dcbSerialParams);
            
            // --- CẤU HÌNH TIMEOUTS CHUẨN PHI CHẶN (NON-BLOCKING) ---
            COMMTIMEOUTS timeouts = { 0 };
            timeouts.ReadIntervalTimeout         = MAXDWORD; // Đọc ngay lập tức bộ đệm hiện tại
            timeouts.ReadTotalTimeoutMultiplier  = 0;
            timeouts.ReadTotalTimeoutConstant    = 0;        // Ép ReadFile không được đợi dữ liệu
            timeouts.WriteTotalTimeoutMultiplier = 0;
            timeouts.WriteTotalTimeoutConstant   = 10;       // Giới hạn ghi lệnh lái trong 10ms
            SetCommTimeouts(hSerial, &timeouts);
            // -------------------------------------------------------

            currentModeLabel->setText("TRẠNG THÁI: ĐÃ KẾT NỐI CÁP USB THÀNH CÔNG (9600 Bps)");
            currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
        }
    }

    // Kết nối các nút tiến lùi
    connect(masterSlider, &QSlider::valueChanged, this, &MainWindow::onMasterSpeedChanged);
    connect(btnUp, &QPushButton::clicked, this, &MainWindow::onMoveUpClicked);
    connect(btnDown, &QPushButton::clicked, this, &MainWindow::onMoveDownClicked);
    connect(btnLeft, &QPushButton::clicked, this, &MainWindow::onMoveLeftClicked);
    connect(btnRight, &QPushButton::clicked, this, &MainWindow::onMoveRightClicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopCarClicked);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onResetPressed);
    // Kết nối các nút bấm độc lập
    connect(btnFL, &QPushButton::clicked, this, [this]() { processSingleMotorMovement("FL"); });
    connect(btnFR, &QPushButton::clicked, this, [this]() { processSingleMotorMovement("FR"); });
    connect(btnRL, &QPushButton::clicked, this, [this]() { processSingleMotorMovement("RL"); });
    connect(btnRR, &QPushButton::clicked, this, [this]() { processSingleMotorMovement("RR"); });

    // --- KHỞI TẠO TIẾN TRÌNH PYTHON CHẠY NGẦM VỚI BỘ CHẨN ĐOÁN LỖI --- //
   
    pythonProcess = new QProcess(this);
 
    // Đọc luồng dữ liệu chuẩn (stdout)
    connect(pythonProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readPythonOutput);
 
    // Lắng nghe lỗi hệ thống của Python (stderr) và in ra cửa sổ Application Output của Qt
    connect(pythonProcess, &QProcess::readyReadStandardError, this, [this]() {
        QByteArray errData = pythonProcess->readAllStandardError();
        qDebug() << "[PYTHON SYSTEM ERROR]:" << errData;
 });

    // BỔ SUNG: Theo dõi sự kiện tiến trình Python bị đóng đột ngột
    connect(pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
         this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "[PROCESS FINISHED]: Tiến trình Python đã dừng. Exit Code:" << exitCode;
        currentModeLabel->setText("LỖI: TIẾN TRÌNH PYTHON BỊ ĐÓNG ĐỘT NGỘT!");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
 });

    // Tự động lấy đường dẫn tuyệt đối của file hand_tracker.py nằm cùng thư mục file .exe
    QString scriptPath = QCoreApplication::applicationDirPath() + "/hand_tracker.py";
    qDebug() << "Đang tìm file Python tại đường dẫn:" << scriptPath;

    QString pythonPath =
    "C:/Users/duc/AppData/Local/Programs/Python/Python312/python.exe";

    pythonProcess->start(pythonPath, QStringList() << scriptPath);

 // --- TỰ ĐỘNG ĐỌC DỮ LIỆU CẢM BIẾN TỪ ARDUINO (MỖI 100ms) --- //
    if (hSerial != INVALID_HANDLE_VALUE) {
        serialTimer = new QTimer(this);
        connect(serialTimer, &QTimer::timeout, this, &MainWindow::readSerialData);
        serialTimer->start(100); 
    }
}

MainWindow::~MainWindow() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
    }
    // Đóng tiến trình Python an toàn
    if (pythonProcess && pythonProcess->state() == QProcess::Running) {
        pythonProcess->terminate();
        pythonProcess->waitForFinished(1000);
    }
}

// ============================================================================
// KHỐI 2: ĐỌC VÀ PHÂN TÍCH LUỒNG DỮ LIỆU ĐẦU VÀO (INPUTS)
// ============================================================================

    // --- Đọc Python --- //
void MainWindow::readPythonOutput() {
    while (pythonProcess->canReadLine()) {
        QByteArray line = pythonProcess->readLine().trimmed();
        
        if (line == "ERROR:CamNotFound") {
            currentModeLabel->setText("LỖI CAMERA: KHÔNG TÌM THẤY CAMERA THIẾT BỊ!");
            currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
            continue;
        }

        int colonIdx = line.indexOf(':');
        if (colonIdx != -1) {
            QByteArray fingerPart = line.left(colonIdx);
            QByteArray base64Part = line.mid(colonIdx + 1);

            bool ok;
            int fingers = fingerPart.toInt(&ok);
            if (ok) {
                handleGesture(fingers);
            }

            // Giải mã chuỗi ảnh Base64 thu được
            QByteArray jpegData = QByteArray::fromBase64(base64Part);
            QImage img;
            if (img.loadFromData(jpegData, "JPG")) {
                updateCameraFrame(img);
            }
        }
    }
}
    // --- Đếm ngón --- //
void MainWindow::handleGesture(int fingers) {
    if (fingers == -1) return;

    if (currentMode == 0 && masterSlider->value() == 0 && fingers == 0) return;
    
    int previousMode = currentMode;
    switch (fingers) {
        case 0:
            currentMode = 0;
            currentTestingMotor = ""; // Reset bộ nhớ test động cơ
            currentModeLabel->setText("CHẾ ĐỘ [NẮM ĐẤM]: DỪNG XE KHẨN CẤP (STOP 🛑)");
            currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
            onStopCarClicked();
            break;
        case 1:
            currentMode = 1;
            currentTestingMotor = "";
            currentModeLabel->setText("CHẾ ĐỘ [1 NGÓN]: TỰ ĐỘNG DÒ LINE (ACTIVE 🟢)");
            currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
            break;
        case 2:
            currentMode = 2;
            currentTestingMotor = "";
            currentModeLabel->setText("CHẾ ĐỘ [2 NGÓN]: TỰ ĐỘNG TRÁNH VẬT CẢN 🔵");
            currentModeLabel->setStyleSheet("color: #3b82f6; font-weight: bold;");
            break;
        case 3:
            currentMode = 3;
            currentTestingMotor = "";
            currentModeLabel->setText("CHẾ ĐỘ [3 NGÓN]: ĐIỀU KHIỂN TỰ DO (MANUAL DRIVE 🟡)");
            currentModeLabel->setStyleSheet("color: #eab308; font-weight: bold;");
            break;
        case 4:
            currentMode = 4;
            currentTestingMotor = ""; // Vừa giơ 4 ngón lên thì chưa chọn bánh nào, đợi ấn nút
            currentModeLabel->setText("CHẾ ĐỘ [4 NGÓN]: TEST RIÊNG BIỆT ĐỘNG CƠ (AN TOÀN 🟣)");
            currentModeLabel->setStyleSheet("color: #a855f7; font-weight: bold;"); 
            
            // Lệnh phanh đứng im an toàn ban đầu
            {
                QString stopPayload = "MODE:4;FL:0;FR:0;RL:0;RR:0;DIR:MOTOR_TEST;\n";
                std::string data = stopPayload.toStdString();
                DWORD bytesWritten;
                if (hSerial != INVALID_HANDLE_VALUE) {
                    WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
                }
            }
            break;
        case 5:
            currentMode = 5;
            currentTestingMotor = ""; // Reset bộ nhớ test động cơ
            currentModeLabel->setText("CHẾ ĐỘ [5 NGÓN]: KHỞI CHẠY AI VISION TRÊN CAMERA (AUTO 🤖)");
            currentModeLabel->setStyleSheet("color: #00ffcc; font-weight: bold;"); // Màu xanh neon AI
            break;

        default:
            break;
    }
    
    if (currentMode != previousMode && currentMode != 4) {
        sendControlPacket();
    }
}
    // --- Đọc Serial --- //
void MainWindow::readSerialData() {
    if (hSerial == INVALID_HANDLE_VALUE) return;

    DWORD bytesAvailable;
    COMSTAT comStat; 
    
    // Sử dụng ClearCommError để lấy chính xác số lượng bytes đang chờ trong bộ đệm nhận (cbInQue)
    if (ClearCommError(hSerial, NULL, &comStat)) {
        bytesAvailable = comStat.cbInQue; 
        
        if (bytesAvailable > 0) {
            char* buffer = new char[bytesAvailable + 1];
            DWORD bytesRead;
            
            if (ReadFile(hSerial, buffer, bytesAvailable, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                QString rawData = QString::fromLocal8Bit(buffer);
                
                // Ghi log thẳng ra Terminal/Debug Console của VS Code để kiểm tra mạch
                qDebug() << "[RAW DATA TỪ ARDUINO]:" << rawData;
                
                static QString incompleteLine = "";
                incompleteLine += rawData;
                
                while (incompleteLine.contains('\n')) {
                    int newlinePos = incompleteLine.indexOf('\n');
                    QString validLine = incompleteLine.left(newlinePos).trimmed();
                    incompleteLine = incompleteLine.mid(newlinePos + 1);
                    
                    if (validLine.isEmpty()) continue;
                    
                    int battery = 100;
                    double distance = -1.0;
                    int lState = 0;
                    bool parseSuccess = false;

                    QStringList tokens = validLine.split(';');
                    foreach (QString token, tokens) {
                        token = token.trimmed();
                        if (token.startsWith("BAT:")) {
                            battery = token.mid(4).toInt();
                            parseSuccess = true;
                        }
                        else if (token.startsWith("DIST:")) {
                            distance = token.mid(5).toDouble();
                            parseSuccess = true;
                        }
                        else if (token.startsWith("LINE:")) {
                            lState = token.mid(5).toInt();
                            parseSuccess = true;
                        }
                    }
                    
                   if (parseSuccess) {
                        // Gọi hàm cập nhật giao diện đồ họa từ dữ liệu đã phân tích
                        updateUI(battery, distance, lState);
                    } 
                }
            }
            delete[] buffer;
        }
    }
}
    // --- Thiết lập phím --- //
void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    // Khi người dùng chủ động can thiệp bằng tay, ép chặt chế độ MANUAL
    currentMode = 3;

    switch (event->key()) {
        case Qt::Key_W: processManualMovement("W"); break;
        case Qt::Key_S: processManualMovement("S"); break;
        case Qt::Key_A: processManualMovement("A"); break;
        case Qt::Key_D: processManualMovement("D"); break;
        default: QMainWindow::keyPressEvent(event); break;
    }
}
void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;

    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_S:
        case Qt::Key_A:
        case Qt::Key_D:
            onStopCarClicked(); 
            break;
        default: QMainWindow::keyReleaseEvent(event); break;
    }
}

// ============================================================================
// KHỐI 3: CẬP NHẬT ĐỒ HỌA & HIỂN THỊ GIAO DIỆN (GUI OUTPUTS)
// ============================================================================
    // --- Pin, line, khoảng cách --- //
void MainWindow::updateUI(int battery, double dist, int lineState) {
    batteryBar->setValue(battery);
    
    if (battery < 20) {
        batteryBar->setStyleSheet("QProgressBar { border: 1px solid #dc2626; text-align: center; color: white; } "
                                  "QProgressBar::chunk { background-color: #dc2626; }");
    } else {
        batteryBar->setStyleSheet("QProgressBar { border: 1px solid #475569; text-align: center; color: white; } "
                                  "QProgressBar::chunk { background-color: #10b981; }");
    }

    // Hiển thị khoảng cách siêu âm
    if (dist >= 99.0) {
        obstacleWarning->setText("Đường thoáng (Safe)");
        obstacleWarning->setStyleSheet("color: #10b981; font-weight: bold;"); 
    } else {
        obstacleWarning->setText(QString::number(dist, 'f', 1) + " cm");
        if (dist < 25.0) {
            obstacleWarning->setStyleSheet("color: #dc2626; font-weight: bold;"); 
        } else {
            obstacleWarning->setStyleSheet("color: #e2e8f0;");
        }
    }

    // Hiển thị trạng thái mắt đọc hồng ngoại
    if (lineState == -2) {
        lineStatus->setText("Dò line: LỆCH TRÁI QUÁ NHIỀU (<<-)");
        lineStatus->setStyleSheet("color: #f59e0b; font-weight: bold;");
    }
    else if (lineState == -1) {
        lineStatus->setText("Dò line: LỆCH TRÁI NHẸ (<-)");
        lineStatus->setStyleSheet("color: #cbd5e1;");
    }
    else if (lineState == 1) {
        lineStatus->setText("Dò line: LỆCH PHẢI NHẸ (->)");
        lineStatus->setStyleSheet("color: #cbd5e1;");
    }
    else if (lineState == 9) {
        lineStatus->setText("Dò line: PHÁT HIỆN NGÃ TƯ (🛑)");
        lineStatus->setStyleSheet("color: #ef4444; font-weight: bold;");
    }
    else if (lineState == 404) {
        lineStatus->setText("Dò line: MẤT VẠCH ĐƯỜNG (!)");
        lineStatus->setStyleSheet("color: #94a3b8; font-style: italic;");
    }
    else {
        lineStatus->setText("Dò line: Ở GIỮA CHUẨN (OK)");
        lineStatus->setStyleSheet("color: #10b981; font-weight: bold;"); 
    }

    // Phanh khẩn cấp từ xa bằng phần mềm bảo vệ phần cứng
    if (currentMode == 2 && dist < 25.0 && dist > 0.1) {
        masterSlider->setValue(0); 
        currentModeLabel->setText("CẢNH BÁO NGUY HIỂM: TỰ ĐỘNG PHANH TRÁNH VẬT CẢN!");
        currentModeLabel->setStyleSheet("color: #dc2626; font-weight: bold;");
    }
}
    // --- Camera --- //
void MainWindow::updateCameraFrame(const QImage &frame) {
    QImage processedFrame = frame.copy();
    QPainter painter(&processedFrame);
    
    painter.setPen(Qt::cyan);
    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    
    QString modeStr = "STOPPED";
    if (currentMode == 1) modeStr = "LINE FOLLOWING";
    else if (currentMode == 2) modeStr = "OBSTACLE AVOIDANCE";
    else if (currentMode == 3) modeStr = "MANUAL CONTROL";
    else if (currentMode == 4) modeStr = "IMAGE PROCESSING (Q)";
    else if (currentMode == 5) modeStr = "AI VISION ACTIVE 🧠";

    painter.drawText(15, 25, QString("HUST AI VISION | CHẾ ĐỘ: %1").arg(modeStr));
    painter.end();
    
    cameraDisplay->setPixmap(QPixmap::fromImage(processedFrame).scaled(
        QSize(640, 480), 
        Qt::KeepAspectRatio,          
        Qt::SmoothTransformation
    ));
}

// ============================================================================
// KHỐI 4: ĐIỀU KHIỂN & ĐÓNG GÓI LỆNH ĐẨY XUỐNG CỔNG XUẤT (CONTROL OUTPUTS)
// ============================================================================
    // --- Set tốc độ tức thời --- //
void MainWindow::onMasterSpeedChanged(int value) {
    masterValueLabel->setText(QString("%1").arg(value));
    speedDisplay->display(value); 
    
    // Nếu đang ở Mode 4 (Test động cơ độc lập)
    if (currentMode == 4) {
        // Nếu chưa bấm chọn nút test bánh nào cụ thể thì không phát lệnh quay bậy
        if (currentTestingMotor.isEmpty()) return; 

        int fl = 0, fr = 0, rl = 0, rr = 0;

        // Lấy giá trị mới (`value`) gán trực tiếp vào bánh đang được chọn test
        if (currentTestingMotor == "FL") fl = value;
        else if (currentTestingMotor == "FR") fr = value;
        else if (currentTestingMotor == "RL") rl = value;
        else if (currentTestingMotor == "RR") rr = value;

        QString payload = QString("MODE:4;FL:%1;FR:%2;RL:%3;RR:%4;DIR:MOTOR_TEST;\n").arg(fl).arg(fr).arg(rl).arg(rr);
                            
        std::string data = payload.toStdString();
        DWORD bytesWritten;
        
        if (hSerial != INVALID_HANDLE_VALUE) {
            WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
        }
        return; 
    }
        if (currentMode == 5) {
        QString payload = QString("MODE:5;FL:%1;FR:%2;RL:%3;RR:%4;DIR:AI_CONTROL;\n").arg(value).arg(value).arg(value).arg(value);
        std::string data = payload.toStdString();
        DWORD bytesWritten;
        if (hSerial != INVALID_HANDLE_VALUE) {
            WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
        }
        return;
    }
    
    // Chế độ lái tự do (Mode 3)
    if (currentMode == 3) {
        sendControlPacket(); 
    }
}
    // --- Chạy bình thường --- //
void MainWindow::processManualMovement(QString direction) {
    int currentSpeed = masterSlider->value();
    if (currentSpeed == 0) {
        currentSpeed = 150; 
    }

    currentMode = 3; 

    if (direction == "W") currentModeLabel->setText("TRẠNG THÁI: XE ĐANG TIẾN ⬆️");
    else if (direction == "S") currentModeLabel->setText("TRẠNG THÁI: XE ĐANG LÙI ⬇️");
    else if (direction == "A") currentModeLabel->setText("TRẠNG THÁI: XOAY TRÁI ↩️");
    else if (direction == "D") currentModeLabel->setText("TRẠNG THÁI: XOAY PHẢI ↪️");
    
    currentModeLabel->setStyleSheet("color: #eab308; font-weight: bold;");

    QString payload = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:%6;\n")
                        .arg(currentMode).arg(currentSpeed).arg(currentSpeed)
                        .arg(currentSpeed).arg(currentSpeed).arg(direction);
                        
    std::string data = payload.toStdString();
    DWORD bytesWritten;
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}
    // --- Từng động cơ --- //
void MainWindow::processSingleMotorMovement(QString motorName) {
    currentMode = 4; 
    currentTestingMotor = motorName; // 💾 Cập nhật biến thành viên của Class để hàm Slider cùng đọc được
    
    currentModeLabel->setText(QString("TRẠNG THÁI: CHẾ ĐỘ 4 - TEST RIÊNG BIỆT [%1] ⚙️").arg(motorName));
    currentModeLabel->setStyleSheet("color: #a855f7; font-weight: bold;");

    int targetSpeed = masterSlider->value(); 
    int fl = 0, fr = 0, rl = 0, rr = 0;

    if (motorName == "FL") fl = targetSpeed;
    else if (motorName == "FR") fr = targetSpeed;
    else if (motorName == "RL") rl = targetSpeed;
    else if (motorName == "RR") rr = targetSpeed;

    QString payload = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:MOTOR_TEST;\n")
                        .arg(currentMode).arg(fl).arg(fr).arg(rl).arg(rr);
                        
    std::string data = payload.toStdString();
    DWORD bytesWritten;
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}
    // --- Gửi tốc độ tức thời --- //
void MainWindow::sendControlPacket() {
    int currentSpeed = masterSlider->value();
    
    QString payload = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:;\n").arg(currentMode).arg(currentSpeed).arg(currentSpeed).arg(currentSpeed).arg(currentSpeed);
                        
    std::string data = payload.toStdString();
    DWORD bytesWritten;
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}
    // --- Set phím --- //
void MainWindow::onMoveUpClicked()    { processManualMovement("W"); }
void MainWindow::onMoveDownClicked()  { processManualMovement("S"); }
void MainWindow::onMoveLeftClicked()  { processManualMovement("A"); }
void MainWindow::onMoveRightClicked() { processManualMovement("D"); }
    // --- Dừng --- //
void MainWindow::onStopCarClicked() {
    currentMode = 0; 
    currentModeLabel->setText("TRẠNG THÁI: ĐÃ DỪNG 🛑");
    currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
    
    QString payload = QString("MODE:0;FL:0;FR:0;RL:0;RR:0;DIR:;\n");
    std::string data = payload.toStdString();
    DWORD bytesWritten;
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}
    // --- reset --- //
void MainWindow::onResetPressed() {
    masterSlider->setValue(0); 
    onStopCarClicked(); 
    currentModeLabel->setText("CHẾ ĐỘ: EMERGENCY RESET COMPLETED");
}