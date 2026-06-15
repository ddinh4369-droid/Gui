#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLCDNumber>
#include <QProgressBar>
#include <QLabel>
#include <QSlider>
#include <QProcess>
#include <QTimer>
#include <QList>
#include <QtSerialPort/QSerialPortInfo>

// Nhúng Win32 API thuần để giao tiếp Serial cứng
#include <windows.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // ⭐ Bắt buộc phải có để xử lý sự kiện click vào ComboBox tự động làm mới cổng COM
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // Xử lý sự kiện nhấn thả phím từ bàn phím (W, A, S, D)
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    // Các Slot xử lý cấu hình kết nối
    void onConnectClicked();
    void onDisconnectClicked();
    void refreshComPorts();

    // Các Slot xử lý luồng nhận dữ liệu
    void readPythonOutput();
    void readSerialData();

    // Các Slot điều khiển tốc độ và động cơ
    void onMasterSliderChanged();
    void onMotorSliderChanged();
    void onStopCarClicked();
    void onResetPressed();

private:
    // Hàm bổ trợ nội bộ để phân tích cử chỉ tay và điều hướng xe
    void handleGesture(int fingers);
    void updateUI(int battery, double dist, int lineState);
    void updateCameraFrame(const QImage &frame);
    void processManualMovement(QString direction);

    // --- BIẾN ĐIỀU KHIỂN HỆ THỐNG VÀ PHẦN CỨNG ---
    HANDLE hSerial;             // Handle quản lý cổng COM theo cấu trúc Win32 API
    int currentMode;            // Chế độ hoạt động hiện tại (0: Stop, 1: Line, 2: Tránh vật cản, 3: Manual, 5: AI)
    QProcess *pythonProcess;    // Tiến trình chạy ngầm script Mediapipe (hand_tracker.py)
    QTimer *serialTimer;        // Timer quét chu kỳ đọc dữ liệu từ mạch

    // --- CÁC THÀNH PHẦN GIAO DIỆN (UI COMPONENTS) ---
    // Khối Telemetry
    QLCDNumber *speedDisplay;
    QProgressBar *batteryBar;
    QLabel *currentModeLabel;
    QLabel *lineStatus;
    QLabel *obstacleWarning;

    // Khối Kết nối
    QComboBox *comPortComboBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;

    // Khối điều hướng hướng đi
    QPushButton *btnUp;
    QPushButton *btnDown;
    QPushButton *btnLeft;
    QPushButton *btnRight;
    QPushButton *btnStop;
    QPushButton *btnReset;

    // Khối Slider Ga tổng và Vi chỉnh 4 động cơ
    QSlider *masterSlider;
    QLabel *masterValueLabel;

    QSlider *m1Slider; // Trước Trái
    QSlider *m2Slider; // Trước Phải
    QSlider *m3Slider; // Sau Trái
    QSlider *m4Slider; // Sau Phải

    QLabel *m1ValueLabel;
    QLabel *m2ValueLabel;
    QLabel *m3ValueLabel;
    QLabel *m4ValueLabel;

    // Khối hiển thị Camera stream từ AI
    QLabel *cameraDisplay;
};

#endif // MAINWINDOW_H