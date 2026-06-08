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

// ============================================================================
// KHỐI 1: KHỞI TẠO & GIẢI PHÓNG HỆ THỐNG (VÒNG ĐỜI ỨNG DỤNG)
// ============================================================================

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    currentMode = 0;
    hSerial = INVALID_HANDLE_VALUE; 

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

    // Group 2: BẢNG ĐIỀU KHIỂN NÚT BẤM + GA TỔNG
    QGroupBox *controlGroup = new QGroupBox("BẢNG ĐIỀU KHIỂN HỆ THỐNG", this);
    QVBoxLayout *controlGroupLayout = new QVBoxLayout(controlGroup);
    controlGroupLayout->setContentsMargins(15, 15, 15, 15);
    controlGroupLayout->setSpacing(15);

    QHBoxLayout *masterLayout = new QHBoxLayout();
    QLabel *masterTitle = new QLabel("GA TỔNG (SPEED):", this);
    masterTitle->setStyleSheet("font-weight: bold; color: #cbd5e1;");
    
    masterSlider = new QSlider(Qt::Horizontal);
    masterSlider->setRange(0, 255);
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

    // --- GROUP 3: ĐIỀU CHỈNH TỐC ĐỘ ĐỘNG CƠ ĐỘC LẬP QSLIDER ---
    QGroupBox *motorGroupBox = new QGroupBox("ĐIỀU CHỈNH TỐC ĐỘ ĐỘNG CƠ ĐỘC LẬP", this);
    QGridLayout *motorGrid = new QGridLayout(motorGroupBox);
    motorGrid->setSpacing(12);

    struct MotorConfig {
        QString name;
        QSlider** slider;
        QLabel** label;
    } motors[] = {
        {"M1 (Trước Trái):", &m1Slider, &m1ValueLabel},
        {"M2 (Trước Phải):", &m2Slider, &m2ValueLabel},
        {"M3 (Sau Trái):",  &m3Slider, &m3ValueLabel},
        {"M4 (Sau Phải):",  &m4Slider, &m4ValueLabel}
    };

    QString sliderStyle = "QSlider::groove:horizontal { height: 6px; background: #334155; border-radius: 3px; }"
                          "QSlider::handle:horizontal { background: #38bdf8; width: 14px; margin-top: -4px; margin-bottom: -4px; border-radius: 7px; }"
                          "QSlider::sub-page:horizontal { background: #0ea5e9; border-radius: 3px; }";

    for (int i = 0; i < 4; ++i) {
        QLabel *nameLabel = new QLabel(motors[i].name, this);
        nameLabel->setStyleSheet("color: #e2e8f0; font-size: 11px;");
        motorGrid->addWidget(nameLabel, i, 0);

        QSlider* currentSlider = new QSlider(Qt::Horizontal, this);
        currentSlider->setRange(0, 255);
        currentSlider->setValue(0); 
        currentSlider->setStyleSheet(sliderStyle);
        
        *(motors[i].slider) = currentSlider;
        motorGrid->addWidget(currentSlider, i, 1);

        QLabel *currentLabel = new QLabel("0", this);
        currentLabel->setStyleSheet("color: #38bdf8; font-weight: bold; min-width: 25px; font-size: 11px;");
        
        *(motors[i].label) = currentLabel;
        motorGrid->addWidget(currentLabel, i, 2);

        connect(currentSlider, &QSlider::valueChanged, this, &MainWindow::onMotorSliderChanged);
    }
    leftLayout->addWidget(motorGroupBox);

    btnReset = new QPushButton("RESET HỆ THỐNG (EMERGENCY STOP)", this);
    btnReset->setStyleSheet("QPushButton { background-color: #dc2626; color: white; padding: 12px; font-weight: bold; font-size: 14px; border-radius: 5px; border: none; } "
                            "QPushButton:pressed { background-color: #991b1b; }");
    leftLayout->addWidget(btnReset);

    mainLayout->addLayout(leftLayout, 4);
    
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
            
            COMMTIMEOUTS timeouts = { 0 };
            timeouts.ReadIntervalTimeout         = MAXDWORD; 
            timeouts.ReadTotalTimeoutMultiplier  = 0;
            timeouts.ReadTotalTimeoutConstant    = 0;        
            timeouts.WriteTotalTimeoutMultiplier = 0;
            timeouts.WriteTotalTimeoutConstant   = 10;       
            SetCommTimeouts(hSerial, &timeouts);

            currentModeLabel->setText("TRẠNG THÁI: ĐÃ KẾT NỐI CÁP USB THÀNH CÔNG (9600 Bps)");
            currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
        }
    }

    // Kết nối các nút và Slider hệ thống
    connect(masterSlider, &QSlider::valueChanged, this, &MainWindow::onMasterSliderChanged);
    connect(btnUp, &QPushButton::clicked, this, &MainWindow::onMoveUpClicked);
    connect(btnDown, &QPushButton::clicked, this, &MainWindow::onMoveDownClicked);
    connect(btnLeft, &QPushButton::clicked, this, &MainWindow::onMoveLeftClicked);
    connect(btnRight, &QPushButton::clicked, this, &MainWindow::onMoveRightClicked);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopCarClicked);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onResetPressed);

    // --- KHỔI TẠO TIẾN TRÌNH PYTHON CHẠY NGẦM --- //
    pythonProcess = new QProcess(this);
    connect(pythonProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readPythonOutput);
 
    connect(pythonProcess, &QProcess::readyReadStandardError, this, [this]() {
        QByteArray errData = pythonProcess->readAllStandardError();
        qDebug() << "[PYTHON SYSTEM ERROR]:" << errData;
    });

    connect(pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
         this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus);
        qDebug() << "[PROCESS FINISHED]:" << exitCode;
        currentModeLabel->setText("LỖI: TIẾN TRÌNH PYTHON BỊ ĐÓNG ĐỘT NGỘT!");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
    });

    QString scriptPath = QCoreApplication::applicationDirPath() + "/hand_tracker.py";
    QString pythonPath = "C:/Users/duc/AppData/Local/Programs/Python/Python312/python.exe";
    pythonProcess->start(pythonPath, QStringList() << scriptPath);

    // --- ĐỌC DỮ LIỆU CẢM BIẾN TỪ ARDUINO (MỖI 100ms) --- //
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
    if (pythonProcess && pythonProcess->state() == QProcess::Running) {
        pythonProcess->terminate();
        pythonProcess->waitForFinished(1000);
    }
}

// ============================================================================
// KHỐI 2: ĐỌC VÀ PHÂN TÍCH LUỒNG DỮ LIỆU ĐẦU VÀO (INPUTS)
// ============================================================================

void MainWindow::readPythonOutput() {
    while (pythonProcess->canReadLine()) {
        QByteArray line = pythonProcess->readLine().trimmed();
        
        if (line == "ERROR:CamNotFound") {
            currentModeLabel->setText("LỖI CAMERA: KHÔNG TÌM THẤY CAMERA THIẾT BỊ!");
            currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
            continue;
        }

        // ⭐ ĐÓN VÀ PHÂN TÍCH CHUỖI ĐIỀU KHIỂN HỆ THỐNG 2 TAY TỪ PYTHON BẮN LÊN
        if (line.startsWith("MODE:5")) {
            QString cmdStr = QString::fromUtf8(line);
            
            // Cập nhật text đồ họa trên trạm điều khiển để hiển thị đúng hành vi thực tế của xe
            if (cmdStr.contains("DIR:W;")) {
                currentModeLabel->setText("CHẾ ĐỘ [2 TAY]: TAY 2 ĐIỀU HƯỚNG XE TIẾN TỚI ⬆️");
                currentModeLabel->setStyleSheet("color: #00ffcc; font-weight: bold;");
            } else if (cmdStr.contains("DIR:S;")) {
                currentModeLabel->setText("CHẾ ĐỘ [2 TAY]: TAY 2 ĐIỀU HƯỚNG XE LÙI LẠI ⬇️");
                currentModeLabel->setStyleSheet("color: #f59e0b; font-weight: bold;");
            } else if (cmdStr.contains("DIR:A;")) {
                currentModeLabel->setText("CHẾ ĐỘ [2 TAY]: TAY 2 ĐIỀU HƯỚNG XOAY TRÁI ↩️");
                currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
            } else if (cmdStr.contains("DIR:D;")) {
                currentModeLabel->setText("CHẾ ĐỘ [2 TAY]: TAY 2 ĐIỀU HƯỚNG XOAY PHẢI ↪️");
                currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
            } else if (cmdStr.contains("DIR:CIRCLE;")) {
                currentModeLabel->setText("CHẾ ĐỘ [2 TAY]: PHÁT HIỆN XOAY VÒNG - XE QUAY TRÒN TẠI CHỖ REVOLUTION 🔄");
                currentModeLabel->setStyleSheet("color: #a855f7; font-weight: bold;");
            } else {
                currentModeLabel->setText("CHẾ ĐỘ [5 NGÓN]: KHỞI CHẠY AI VISION TỰ ĐỘNG DÒ LINE CAMERA 🤖");
                currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
            }

            // Đẩy lệnh xuống vi điều khiển Arduino qua cổng Serial Win32 API công nghiệp
            if (hSerial != INVALID_HANDLE_VALUE) {
                DWORD bytesWritten;
                std::string data = line.toStdString();
                WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
            }
            continue;
        }

        // Đoạn xử lý ảnh nén Base64 cũ giữ nguyên vẹn bên dưới...
        int colonIdx = line.indexOf(':');
        if (colonIdx != -1) {
            QByteArray fingerPart = line.left(colonIdx);
            QByteArray base64Part = line.mid(colonIdx + 1);

            bool ok;
            int fingers = fingerPart.toInt(&ok);
            if (ok) {
                handleGesture(fingers);
            }

            QByteArray jpegData = QByteArray::fromBase64(base64Part);
            QImage img;
            if (img.loadFromData(jpegData, "JPG")) {
                updateCameraFrame(img);
            }
        }
    }
}

void MainWindow::handleGesture(int fingers) {
    if (fingers == -1) return;
    if (currentMode == 0 && masterSlider->value() == 0 && fingers == 0) return;
    
    int previousMode = currentMode;
    switch (fingers) {
        case 0:
            currentMode = 0;
            currentModeLabel->setText("CHẾ ĐỘ [NẮM ĐẤM]: DỪNG XE KHẨN CẤP (STOP 🛑)");
            currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
            onStopCarClicked();
            break;
        case 1:
            currentMode = 1;
            currentModeLabel->setText("CHẾ ĐỘ [1 NGÓN]: TỰ ĐỘNG DÒ LINE (ACTIVE 🟢)");
            currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
            break;
        case 2:
            currentMode = 2;
            currentModeLabel->setText("CHẾ ĐỘ [2 NGÓN]: TỰ ĐỘNG TRÁNH VẬT CẢN 🔵");
            currentModeLabel->setStyleSheet("color: #3b82f6; font-weight: bold;");
            break;
        case 3:
            currentMode = 3;
            currentModeLabel->setText("CHẾ ĐỘ [3 NGÓN]: ĐIỀU KHIỂN TỰ DO (MANUAL DRIVE 🟡)");
            currentModeLabel->setStyleSheet("color: #eab308; font-weight: bold;");
            break;
        case 4:
            // ⭐ ĐÃ KHỬ CHẾ ĐỘ 4 NGÓN: Bỏ qua không đổi Mode rác
            break;
        case 5:
            currentMode = 5;
            currentModeLabel->setText("CHẾ ĐỘ [5 NGÓN]: KHỞI CHẠY AI VISION TRÊN CAMERA (AUTO 🤖)");
            currentModeLabel->setStyleSheet("color: #00ffcc; font-weight: bold;"); 
            break;
        default:
            break;
    }
    
    if (currentMode != previousMode && currentMode != 4) {
        sendControlPacket();
    }
}

void MainWindow::readSerialData() {
    if (hSerial != INVALID_HANDLE_VALUE) return;

    DWORD bytesAvailable;
    COMSTAT comStat; 
    
    if (ClearCommError(hSerial, NULL, &comStat)) {
        bytesAvailable = comStat.cbInQue; 
        
        if (bytesAvailable > 0) {
            QByteArray buffer;
            buffer.resize(bytesAvailable);
            DWORD bytesRead;
            
            if (ReadFile(hSerial, buffer.data(), bytesAvailable, &bytesRead, NULL) && bytesRead > 0) {
                QString rawData = QString::fromLocal8Bit(buffer.constData(), bytesRead);
                
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
                        updateUI(battery, distance, lState);
                    } 
                }
            }
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
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

void MainWindow::updateUI(int battery, double dist, int lineState) {
    batteryBar->setValue(battery);
    
    if (battery < 20) {
        batteryBar->setStyleSheet("QProgressBar { border: 1px solid #dc2626; text-align: center; color: white; } "
                                  "QProgressBar::chunk { background-color: #dc2626; }");
    } else {
        batteryBar->setStyleSheet("QProgressBar { border: 1px solid #475569; text-align: center; color: white; } "
                                  "QProgressBar::chunk { background-color: #10b981; }");
    }

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

    if (currentMode == 2 && dist < 25.0 && dist > 0.1) {
        masterSlider->setValue(0); 
        currentModeLabel->setText("CẢNH BÁO NGUY HIỂM: TỰ ĐỘNG PHANH TRÁNH VẬT CẢN!");
        currentModeLabel->setStyleSheet("color: #dc2626; font-weight: bold;");
    }
}

void MainWindow::updateCameraFrame(const QImage &frame) {
    QImage processedFrame = frame.copy();
    QPainter painter(&processedFrame);
    
    painter.setPen(Qt::cyan);
    painter.setFont(QFont("Segoe UI", 11, QFont::Bold));
    
    QString modeStr = "STOPPED";
    if (currentMode == 1) modeStr = "LINE FOLLOWING";
    else if (currentMode == 2) modeStr = "OBSTACLE AVOIDANCE";
    else if (currentMode == 3) modeStr = "MANUAL CONTROL";
    else if (currentMode == 5) modeStr = "AI VISION ACTIVE 🧠";

    painter.drawText(15, 25, QString("HUST AI VISION | CHẾ ĐỘ: %1").arg(modeStr));
    painter.end();
    
    cameraDisplay->setPixmap(QPixmap::fromImage(processedFrame).scaled(
        QSize(800, 600), 
        Qt::KeepAspectRatio,          
        Qt::SmoothTransformation
    ));
}

// ============================================================================
// KHỐI 4: ĐIỀU KHIỂN & ĐÓNG GÓI LỆNH ĐẨY XUỐNG CỔNG XUẤT (CONTROL OUTPUTS)
// ============================================================================

void MainWindow::onMasterSliderChanged() {
    int value = masterSlider->value(); 
    
    masterValueLabel->setText(QString::number(value));
    if (speedDisplay) speedDisplay->display(value); 

    m1Slider->blockSignals(true);
    m2Slider->blockSignals(true);
    m3Slider->blockSignals(true);
    m4Slider->blockSignals(true);

    m1Slider->setValue(value);
    m2Slider->setValue(value);
    m3Slider->setValue(value);
    m4Slider->setValue(value);

    m1ValueLabel->setText(QString::number(value));
    m2ValueLabel->setText(QString::number(value));
    m3ValueLabel->setText(QString::number(value));
    m4ValueLabel->setText(QString::number(value));

    m1Slider->blockSignals(false);
    m2Slider->blockSignals(false);
    m3Slider->blockSignals(false);
    m4Slider->blockSignals(false);

    QString direction = "W"; 
    if (currentMode == 5) {
        direction = "AI_CONTROL"; 
    } else {
        if (currentModeLabel->text().contains("LÙI")) direction = "S";
        else if (currentModeLabel->text().contains("TRÁI")) direction = "A";
        else if (currentModeLabel->text().contains("PHẢI")) direction = "D";
    }

    int modeToSend = (currentMode == 0) ? 3 : currentMode; 
    QString command = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:%6;\n")
                        .arg(modeToSend).arg(value).arg(value).arg(value).arg(value).arg(direction);
                      
    if (hSerial != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        std::string data = command.toStdString();
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}

void MainWindow::onMotorSliderChanged() {
    int v1 = m1Slider->value(); 
    int v2 = m2Slider->value(); 
    int v3 = m3Slider->value(); 
    int v4 = m4Slider->value(); 

    m1ValueLabel->setText(QString::number(v1));
    m2ValueLabel->setText(QString::number(v2));
    m3ValueLabel->setText(QString::number(v3));
    m4ValueLabel->setText(QString::number(v4));

    masterValueLabel->setText("--"); 

    QString direction = "W";
    if (currentModeLabel->text().contains("LÙI")) direction = "S";
    else if (currentModeLabel->text().contains("TRÁI")) direction = "A";
    else if (currentModeLabel->text().contains("PHẢI")) direction = "D";

    // ⭐ ĐÃ SỬA: Khi kéo thanh đơn lẻ, ép luồng về MODE 3 (Chạy tay vi chỉnh), bỏ MODE 4
    currentMode = 3;
    currentModeLabel->setText("CHẾ ĐỘ: VI CHỈNH ĐỘNG CƠ (MANUAL)");
    currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");

    QString command = QString("MODE:3;FL:%1;FR:%2;RL:%3;RR:%4;DIR:%6;\n")
                        .arg(v1).arg(v2).arg(v3).arg(v4).arg(direction);
                      
    if (hSerial != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        std::string data = command.toStdString();
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}

void MainWindow::processManualMovement(QString direction) {
    currentMode = 3; 

    if (direction == "W") currentModeLabel->setText("TRẠNG THÁI: XE ĐANG TIẾN ⬆️");
    else if (direction == "S") currentModeLabel->setText("TRẠNG THÁI: XE ĐANG LÙI ⬇️");
    else if (direction == "A") currentModeLabel->setText("TRẠNG THÁI: XOAY TRÁI ↩️");
    else if (direction == "D") currentModeLabel->setText("TRẠNG THÁI: XOAY PHẢI ↪️");
    currentModeLabel->setStyleSheet("color: #eab308; font-weight: bold;");

    int defaultSpeed = 150; 

    masterSlider->blockSignals(true); 
    masterSlider->setValue(defaultSpeed);
    masterValueLabel->setText(QString::number(defaultSpeed));
    if (speedDisplay) speedDisplay->display(defaultSpeed); 
    masterSlider->blockSignals(false);

    m1Slider->blockSignals(true);
    m2Slider->blockSignals(true);
    m3Slider->blockSignals(true);
    m4Slider->blockSignals(true);

    m1Slider->setValue(defaultSpeed);
    m2Slider->setValue(defaultSpeed);
    m3Slider->setValue(defaultSpeed);
    m4Slider->setValue(defaultSpeed);

    m1ValueLabel->setText(QString::number(defaultSpeed));
    m2ValueLabel->setText(QString::number(defaultSpeed));
    m3ValueLabel->setText(QString::number(defaultSpeed));
    m4ValueLabel->setText(QString::number(defaultSpeed));

    m1Slider->blockSignals(false);
    m2Slider->blockSignals(false);
    m3Slider->blockSignals(false);
    m4Slider->blockSignals(false);

    QString payload = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:%6;\n")
                        .arg(currentMode).arg(defaultSpeed).arg(defaultSpeed).arg(defaultSpeed).arg(defaultSpeed).arg(direction);
                        
    if (hSerial != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        std::string data = payload.toStdString();
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
    qDebug() << "Lệnh phím bấm đồng bộ Ga tổng:" << payload.trimmed();
}
    
void MainWindow::sendControlPacket() {
    int currentSpeed = masterSlider->value();
    
    QString payload = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:;\n").arg(currentMode).arg(currentSpeed).arg(currentSpeed).arg(currentSpeed).arg(currentSpeed);
                        
    std::string data = payload.toStdString();
    DWORD bytesWritten;
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL);
    }
}

void MainWindow::onMoveUpClicked()    { processManualMovement("W"); }
void MainWindow::onMoveDownClicked()  { processManualMovement("S"); }
void MainWindow::onMoveLeftClicked()  { processManualMovement("A"); }
void MainWindow::onMoveRightClicked() { processManualMovement("D"); }

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

void MainWindow::onResetPressed() {
    masterSlider->setValue(0); 
    onStopCarClicked(); 
    currentModeLabel->setText("CHẾ ĐỘ: EMERGENCY RESET COMPLETED");
}