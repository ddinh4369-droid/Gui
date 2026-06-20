#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPainter>
#include <QDebug>
#include <QKeyEvent>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QFile>
#include <QTimer>

// ============================================================================
// KHỐI 1: KHỞI TẠO & GIẢI PHÓNG HỆ THỐNG
// ============================================================================

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    currentMode = 0;
    serialPort = nullptr; 

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
    speedDisplay->setFixedSize(60, 25);
    speedDisplay->setStyleSheet("color: #00ffcc; background: #1e293b; border: 1px solid #475569; border-radius: 4px;");
    
    batteryBar = new QProgressBar();
    batteryBar->setRange(0, 100);
    batteryBar->setValue(100);
    batteryBar->setFixedSize(220, 22);
    batteryBar->setStyleSheet("QProgressBar { border: 1px solid #475569; border-radius: 4px; text-align: center; color: white; font-weight: bold; } "
                              "QProgressBar::chunk { background-color: #10b981; border-radius: 3px; }");

    currentModeLabel = new QLabel("CHẾ ĐỘ: CHƯA KẾT NỐI CỔNG SERIAL USB", this);
    currentModeLabel->setStyleSheet("font-size: 13px; color: #eab308; font-weight: bold;");

    lineStatus = new QLabel("Dò line: CHƯA CÓ DỮ LIỆU", this);
    obstacleWarning = new QLabel("Khoảng cách cản: -- cm", this);

    teleLayout->addRow("VẬN TỐC MASTER:", speedDisplay);
    teleLayout->addRow("MỨC NĂNG LƯỢNG PIN XE:", batteryBar);
    teleLayout->addRow("CẢM BIẾN HỒNG NGOẠI:", lineStatus);
    teleLayout->addRow("CẢM BIẾN SIÊU ÂM:", obstacleWarning);
    teleLayout->addRow("TRẠNG THÁI HỆ THỐNG:", currentModeLabel);
    leftLayout->addWidget(telemetryGroup);

    // Khối cấu hình kết nối cổng COM động
    QGroupBox *connectionGroup = new QGroupBox("CẤU HÌNH KẾT NỐI CỔNG COM", this);
    QHBoxLayout *connLayout = new QHBoxLayout(connectionGroup);
    comPortComboBox = new QComboBox(this);
    comPortComboBox->setFixedWidth(120);
    
    comPortComboBox->installEventFilter(this);

    connectButton = new QPushButton("Kết nối", this);
    disconnectButton = new QPushButton("Ngắt kết nối", this);
    connectButton->setStyleSheet("QPushButton { background-color: #2ECC71; color: white; border-radius: 4px; font-weight: bold; padding: 5px; }"
                                 "QPushButton:hover { background-color: #27AE60; }"
                                 "QPushButton:pressed { background-color: #1E8449; }");
    disconnectButton->setStyleSheet("QPushButton { background-color: #E74C3C; color: white; border-radius: 4px; font-weight: bold; padding: 5px; }"
                                    "QPushButton:hover { background-color: #C0392B; }"
                                    "QPushButton:pressed { background-color: #962D22; }");
    disconnectButton->setEnabled(false);
    
    connLayout->addWidget(new QLabel("Chọn cổng:", this));
    connLayout->addWidget(comPortComboBox);
    connLayout->addWidget(connectButton);
    connLayout->addWidget(disconnectButton);
    leftLayout->addWidget(connectionGroup);
    
    refreshComPorts(); 

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

    // Kết nối các nút và Slider hệ thống
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);

    connect(masterSlider, &QSlider::valueChanged, this, &MainWindow::onMasterSliderChanged);
    connect(btnUp, &QPushButton::clicked, this, [this](){ processManualMovement("W"); });
    connect(btnDown, &QPushButton::clicked, this, [this](){ processManualMovement("S"); });
    connect(btnLeft, &QPushButton::clicked, this, [this](){ processManualMovement("A"); });
    connect(btnRight, &QPushButton::clicked, this, [this](){ processManualMovement("D"); });
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStopCarClicked);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onResetPressed);

    // --- KHỞI TẠO TIẾN TRÌNH PYTHON CHẠY NGẦM --- //
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

    // ========================================================================
    // BƯỚC 1: QUÉT ĐỘNG FILE SCRIPT PYTHON (ĐA MÁY)
    // ========================================================================
    QString scriptName = "hand_tracker.py";
    QString scriptPath = "";

    QStringList potentialPaths = {
        QCoreApplication::applicationDirPath() + "/" + scriptName,
        QCoreApplication::applicationDirPath() + "/../" + scriptName,
        QCoreApplication::applicationDirPath() + "/../../" + scriptName,
        QDir::currentPath() + "/" + scriptName
    };

    for (const QString &path : potentialPaths) {
        if (QFile::exists(path)) {
            scriptPath = QDir::cleanPath(path);
            break;
        }
    }

    if (scriptPath.isEmpty()) {
        scriptPath = QCoreApplication::applicationDirPath() + "/" + scriptName; // Fallback cũ
    }

    // ========================================================================
    // BƯỚC 2: QUÉT ĐỘNG PYTHON (ƯU TIÊN TUYỆT ĐỐI APPDATA PYTHON 3.12)
    // ========================================================================
    QString pythonPath = ""; // Fallback mặc định gọi lệnh toàn cục
    bool hasValidPython = false;

#if defined(Q_OS_WIN)
    // Tự động trích xuất thư mục AppData ẩn của user hiện tại trên máy
    QString localAppData = QProcessEnvironment::systemEnvironment().value("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        QDir appDataDir(localAppData + "/Programs/Python");
        if (appDataDir.exists()) {
            // Quét các thư mục con (ví dụ: Python312, Python311...)
            QStringList subDirs = appDataDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const QString &subDir : subDirs) {
                // CHỈ lọc chọn thư mục chứa chuỗi "Python312" để đảm bảo đúng bản hợp lệ
                if (subDir.contains("Python312", Qt::CaseInsensitive)) {
                    QString exeCheck = QDir::toNativeSeparators(appDataDir.absolutePath() + "/" + subDir + "/python.exe");
                    if (QFile::exists(exeCheck)) {
                        pythonPath = exeCheck;
                        hasValidPython = true;
                        qDebug() << "[SYSTEM MATCH]: Da tu dong nhan diện Python 3.12 tai AppData:" << pythonPath;
                        break; // Đã tìm thấy bản chuẩn nhất, dừng quét ngay lập tức
                    }
                }
            }
        }
    }

    // Nếu không tìm thấy Python 3.12 trong AppData, quét các vị trí dự phòng khác
    if (!hasValidPython) {
        QStringList backupPaths = {
            "C:/msys64/mingw64/bin/python.exe",
            "C:/msys64/ucrt64/bin/python.exe",
            "C:/Python312/python.exe"
        };
        for (const QString &path : backupPaths) {
            if (QFile::exists(path)) {
                pythonPath = QDir::toNativeSeparators(path);
                hasValidPython = true;
                qDebug() << "[SYSTEM MATCH]: Chon Python du phong hop le tai:" << pythonPath;
                break;
            }
        }
    }
#endif

    // Hiển thị thông báo lỗi lên giao diện nếu toàn bộ các phương án quét động đều thất bại
    if (!hasValidPython) { //
        qDebug() << "[SYSTEM ERROR]: Khong tim thay Python 3.12 tren may tinh!"; //
        currentModeLabel->setText("LỖI: THIẾU COMPILER PYTHON 3.12 TRÊN MÁY!"); //
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;"); //
    } else {
    // ========================================================================
    // BƯỚC 3: CHỈ KÍCH HOẠT TIẾN TRÌNH KHI ĐÃ ĐẢM BẢO ĐƯỜNG DẪN HỢP LỆ
    // ========================================================================
    pythonProcess->start(pythonPath, QStringList() << scriptPath);
    }
    // Khởi tạo Timer nhận dữ liệu Serial
    serialTimer = new QTimer(this);
    connect(serialTimer, &QTimer::timeout, this, &MainWindow::readSerialData);
}

MainWindow::~MainWindow() {
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
    if (pythonProcess && pythonProcess->state() == QProcess::Running) {
        pythonProcess->terminate();
        if (!pythonProcess->waitForFinished(1000)) {
            pythonProcess->kill(); // Force kill nếu không thể tắt bình thường
        }
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == comPortComboBox && event->type() == QEvent::MouseButtonPress) {
        refreshComPorts(); 
    }
    return QMainWindow::eventFilter(obj, event);
}

// ============================================================================
// KHỐI 2: ĐỌC VÀ PHÂN TÍCH LUỒNG DỮ LIỆU ĐẦU VÀO (INPUTS)
// ============================================================================

void MainWindow::refreshComPorts() {
    comPortComboBox->clear(); 
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    
    for (const QSerialPortInfo &port : ports) {
        comPortComboBox->addItem(port.portName());
    }
    
    if (comPortComboBox->count() == 0) {
        comPortComboBox->addItem("Không tìm thấy COM");
        connectButton->setEnabled(false);
    } else {
        connectButton->setEnabled(true);
    }
}

void MainWindow::readPythonOutput() {
    while (pythonProcess->canReadLine()) {
        QByteArray line = pythonProcess->readLine().trimmed();
        
        if (line == "ERROR:CamNotFound") {
            currentModeLabel->setText("LỖI CAMERA: KHÔNG TÌM THẤY CAMERA THIẾT BỊ!");
            currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
            continue;
        }

        if (line.startsWith("MODE:5")) {
            QString cmdStr = QString::fromUtf8(line);
            
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

            if (serialPort && serialPort->isOpen()) {
                serialPort->write(line + "\n");
            }
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
        int value = masterSlider->value();
        sendCommand(currentMode, value, value, value, value, "W");
    }
}

void MainWindow::readSerialData() {
    if (!serialPort || !serialPort->isOpen()) return;

    serialBuffer += serialPort->readAll();
    
    while (serialBuffer.contains('\n')) {
        int newlinePos = serialBuffer.indexOf('\n');
        QString validLine = serialBuffer.left(newlinePos).trimmed();
        serialBuffer = serialBuffer.mid(newlinePos + 1);
        
        if (validLine.isEmpty()) continue; 
        
        int battery = 100;
        double distance = -1.0;
        int lState = 0;
        bool parseSuccess = false;

        QStringList tokens = validLine.split(';');
        for (QString token : tokens) {
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

    if (dist >= 99.0 || dist <= 0.0) {
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
        masterSlider->blockSignals(true);
        masterSlider->setValue(0); 
        masterSlider->blockSignals(false);
        if (speedDisplay) speedDisplay->display(0);
        masterValueLabel->setText("0");
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
    else if (currentMode == 5) modeStr = "AI VISION ACTIVE";

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

void MainWindow::onConnectClicked() {
    if (!comPortComboBox || comPortComboBox->count() == 0) {
        currentModeLabel->setText("LỖI: KHÔNG CÓ CỔNG COM NÀO ĐỂ KẾT NỐI! ❌");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
        return;
    }

    QString selectedPort = comPortComboBox->currentText().trimmed();
    if (selectedPort.isEmpty() || selectedPort == "Không tìm thấy COM" || selectedPort == "No COM Port found") {
        currentModeLabel->setText("LỖI: CỔNG COM KHÔNG HỢP LỆ! ❌");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
        return;
    }

    serialPort = new QSerialPort(this);
    serialPort->setPortName(selectedPort);
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        currentModeLabel->setText("LỖI: KHÔNG THỂ MỞ CỔNG " + selectedPort + " ❌");
        currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");
        delete serialPort;
        serialPort = nullptr;
        return;
    }

    currentModeLabel->setText("ĐÃ KẾT NỐI THÀNH CÔNG VỚI " + selectedPort + " 🟢");
    currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");

    connectButton->setEnabled(false);     
    comPortComboBox->setEnabled(false);   
    disconnectButton->setEnabled(true);   

    connectButton->setStyleSheet("QPushButton { background-color: #10b981; color: white; border-radius: 4px; font-weight: bold; }");
    disconnectButton->setStyleSheet(""); 

    // Kết nối tín hiệu đọc tự động (Event-driven, thay thế cho QTimer polling)
    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::readSerialData);
}

void MainWindow::onDisconnectClicked() {
    if (serialPort && serialPort->isOpen()) {
        onStopCarClicked(); 
        serialPort->close();
    }
    delete serialPort;
    serialPort = nullptr;

    currentModeLabel->setText("ĐÃ NGẮT KẾT NỐI CỔNG COM 🛑");
    currentModeLabel->setStyleSheet("color: #ef4444; font-weight: bold;");

    connectButton->setEnabled(true);      
    comPortComboBox->setEnabled(true);    
    disconnectButton->setEnabled(false);  
    
    disconnectButton->setStyleSheet("QPushButton { background-color: #ef4444; color: white; border-radius: 4px; font-weight: bold; }");
    connectButton->setStyleSheet(""); 

    refreshComPorts(); 
}

// Hàm trợ giúp đóng gói & định dạng gói lệnh gửi xuống Arduino
void MainWindow::sendCommand(int mode, int fl, int fr, int rl, int rr, const QString &dir, bool reset) {
    if (!serialPort || !serialPort->isOpen()) return;

    QString command;
    if (reset) {
        command = "MODE:0;RESET:1;\n";
    } else {
        command = QString("MODE:%1;FL:%2;FR:%3;RL:%4;RR:%5;DIR:%6;\n")
                    .arg(mode).arg(fl).arg(fr).arg(rl).arg(rr).arg(dir);
    }
    serialPort->write(command.toUtf8());
}

// Hàm trợ giúp tối ưu cập nhật trạng thái Slider động cơ (áp dụng DRY)
void MainWindow::updateMotorSliders(int value) {
    QSlider* sliders[] = {m1Slider, m2Slider, m3Slider, m4Slider};
    QLabel* labels[] = {m1ValueLabel, m2ValueLabel, m3ValueLabel, m4ValueLabel};

    for (int i = 0; i < 4; ++i) {
        if (sliders[i]) {
            sliders[i]->blockSignals(true);
            sliders[i]->setValue(value);
            sliders[i]->blockSignals(false);
        }
        if (labels[i]) {
            labels[i]->setText(QString::number(value));
        }
    }
}

void MainWindow::onMasterSliderChanged() {
    int value = masterSlider->value(); 
    masterValueLabel->setText(QString::number(value));
    if (speedDisplay) speedDisplay->display(value); 

    updateMotorSliders(value);

    QString direction = "W"; 
    if (currentMode == 5) {
        direction = "AI_CONTROL"; 
    } else {
        QString modeText = currentModeLabel->text();
        if (modeText.contains("LÙI")) direction = "S";
        else if (modeText.contains("TRÁI")) direction = "A";
        else if (modeText.contains("PHẢI")) direction = "D";
    }

    int modeToSend = (currentMode == 0) ? 3 : currentMode; 
    sendCommand(modeToSend, value, value, value, value, direction);
}

void MainWindow::onMotorSliderChanged() {
    m1ValueLabel->setText(QString::number(m1Slider->value()));
    m2ValueLabel->setText(QString::number(m2Slider->value()));
    m3ValueLabel->setText(QString::number(m3Slider->value()));
    m4ValueLabel->setText(QString::number(m4Slider->value()));

    masterValueLabel->setText("--"); 

    QString direction = "W";
    QString modeText = currentModeLabel->text();
    if (modeText.contains("LÙI")) direction = "S";
    else if (modeText.contains("TRÁI")) direction = "A";
    else if (modeText.contains("PHẢI")) direction = "D";

    currentMode = 3;
    currentModeLabel->setText("CHẾ ĐỘ: VI CHỈNH ĐỘNG CƠ (MANUAL)");
    currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");

    sendCommand(3, m1Slider->value(), m2Slider->value(), m3Slider->value(), m4Slider->value(), direction);
}

void MainWindow::processManualMovement(QString direction) {
    currentMode = 3; 
    int value = masterSlider->value();
    
    if (value == 0) {
        value = 150; 
        masterSlider->blockSignals(true);
        masterSlider->setValue(value);
        masterSlider->blockSignals(false);
        masterValueLabel->setText(QString::number(value));
        if (speedDisplay) speedDisplay->display(value);
        updateMotorSliders(value);
    }

    if (direction == "W") {
        currentModeLabel->setText("CHẾ ĐỘ: ĐIỀU KHIỂN THỦ CÔNG - TIẾN TỚI ⬆️");
        currentModeLabel->setStyleSheet("color: #10b981; font-weight: bold;");
    } else if (direction == "S") {
        currentModeLabel->setText("CHẾ ĐỘ: ĐIỀU KHIỂN THỦ CÔNG - LÙI LẠI ⬇️");
        currentModeLabel->setStyleSheet("color: #f59e0b; font-weight: bold;");
    } else if (direction == "A") {
        currentModeLabel->setText("CHẾ ĐỘ: ĐIỀU KHIỂN THỦ CÔNG - XOAY TRÁI ↩️");
        currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
    } else if (direction == "D") {
        currentModeLabel->setText("CHẾ ĐỘ: ĐIỀU KHIỂN THỦ CÔNG - XOAY PHẢI ↪️");
        currentModeLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
    }

    sendCommand(3, value, value, value, value, direction);
}

void MainWindow::onStopCarClicked() {
    masterSlider->blockSignals(true);
    masterSlider->setValue(0);
    masterSlider->blockSignals(false);

    if (speedDisplay) speedDisplay->display(0);
    masterValueLabel->setText("0");

    updateMotorSliders(0);
    sendCommand(3, 0, 0, 0, 0, "STOP");
}

void MainWindow::onResetPressed() {
    onStopCarClicked();
    currentMode = 0;
    currentModeLabel->setText("DỪNG KHẨN CẤP: EMERGENCY SYSTEM RESET! 🚨");
    currentModeLabel->setStyleSheet("color: #dc2626; font-weight: bold;");
    
    sendCommand(0, 0, 0, 0, 0, "STOP", true);
}