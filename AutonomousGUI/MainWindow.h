#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QLCDNumber>
#include <QProgressBar>
#include <QProcess>
#include <QKeyEvent>
#include <QImage>
#include <QTimer>

// Bắt buộc phải thêm thư viện hệ thống Windows để nhận diện kiểu dữ liệu HANDLE
#ifdef _WIN32
#include <windows.h>
#endif

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
private slots:
    void onMasterSliderChanged();
    void onMotorSliderChanged();
    
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onMoveLeftClicked();
    void onMoveRightClicked();
    void onStopCarClicked();
    void onResetPressed();
    
    void updateCameraFrame(const QImage &frame);
    void handleGesture(int fingers);
    
    void readPythonOutput(); // Slot để đọc dữ liệu từ tiến trình Python gửi lên
    void readSerialData();   // Slot để đọc dữ liệu cảm biến từ Arduino qua Win32 API

private:
    void processManualMovement(QString direction);
    void sendControlPacket();
    void updateUI(int battery, double dist, int lineState);

    int currentMode;

    // Thành phần giám sát Telemetry
    QLCDNumber *speedDisplay;
    QProgressBar *batteryBar;
    QLabel *currentModeLabel;
    QLabel *lineStatus;
    QLabel *obstacleWarning;
    QLabel *cameraDisplay;

    // Thanh ga tổng
    QSlider *masterSlider;
    QLabel *masterValueLabel;
    
    // Hệ thống nút điều hướng thủ công
    QPushButton *btnUp;
    QPushButton *btnDown;
    QPushButton *btnLeft;
    QPushButton *btnRight;
    QPushButton *btnStop;
    QPushButton *btnReset;

    // Hệ thống Slider điều chỉnh độc lập 4 động cơ
    QSlider *m1Slider;
    QSlider *m2Slider;
    QSlider *m3Slider;
    QSlider *m4Slider;

    QLabel *m1ValueLabel;
    QLabel *m2ValueLabel;
    QLabel *m3ValueLabel;
    QLabel *m4ValueLabel;

    // Quản lý tiến trình chạy nền và định thời
    QProcess *pythonProcess;
    QTimer *serialTimer;

    // --- KẾT NỐI PHẦN CỨNG QUA WIN32 API ---
#ifdef _WIN32
    HANDLE hSerial; // Biến lưu trữ trạng thái kết nối cổng COM tới Arduino
#else
    void* hSerial;  // Giữ cấu trúc nếu biên dịch trên nền tảng khác
#endif
};

#endif // MAINWINDOW_H