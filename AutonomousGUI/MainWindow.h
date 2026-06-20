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
#include <QList>
#include <QSerialPort>
#include <QSerialPortInfo>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // Xử lý sự kiện click vào ComboBox để tự động làm mới danh sách cổng COM
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    // Xử lý sự kiện nhấn/thả phím điều hướng (W, A, S, D) từ bàn phím
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    // Các Slot xử lý cấu hình kết nối cổng nối tiếp
    void onConnectClicked();
    void onDisconnectClicked();
    void refreshComPorts();

    // Các Slot xử lý luồng nhận dữ liệu đầu vào
    void readPythonOutput();
    void readSerialData();

    // Các Slot điều khiển tốc độ và trạng thái động cơ
    void onMasterSliderChanged();
    void onMotorSliderChanged();
    void onStopCarClicked();
    void onResetPressed();

private:
    // --- CÁC HÀM TRỢ GIÚP TỐI ƯU HÓA (ĐÓNG GÓI & TÁI SỬ DỤNG MÃ NGUỒN) ---
    void sendCommand(int mode, int fl, int fr, int rl, int rr, const QString &dir, bool reset = false);
    void updateMotorSliders(int value);

    // Phân tích cử chỉ ngón tay từ AI để điều phối hệ thống
    void handleGesture(int fingers);
    void updateUI(int battery, double dist, int lineState);
    void updateCameraFrame(const QImage &frame);
    void processManualMovement(QString direction);

    // --- BIẾN ĐIỀU KHIỂN HỆ THỐNG VÀ PHẦN CỨNG ---
    QSerialPort *serialPort;    // Đối tượng quản lý kết nối nối tiếp (thay thế cho Win32 HANDLE)
    QString serialBuffer;       // Bộ đệm xử lý dòng dữ liệu nhận từ mạch Serial
    int currentMode;            // Chế độ hoạt động hiện tại (0: Stop, 1: Line, 2: Tránh vật cản, 3: Manual, 5: AI)
    QProcess *pythonProcess;    // Tiến trình chạy ngầm script Mediapipe (hand_tracker.py)
    QTimer *serialTimer;
    
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