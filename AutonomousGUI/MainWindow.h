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
#include <QtSerialPort/QSerialPort>      // Sử dụng thư viện Serial chính thức của Qt
#include <QtSerialPort/QSerialPortInfo>
#include <QTcpSocket>                   // Thêm Socket để kết nối Python
#include <QByteArray>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void refreshComPorts();

    // Các slots tối ưu hóa bằng Signal/Slot bất đồng bộ
    void readSerialData();
    void readSocketData();
    void readVideoData();
    void readPythonOutput();

    void onMasterSliderChanged();
    void onMotorSliderChanged();
    void onStopCarClicked();
    void onResetPressed();

private:
    void handleGesture(int fingers);
    void updateUI(int battery, double dist, int lineState);
    void updateCameraFrame(const QImage &frame);
    void processManualMovement(QString direction);
    void sendCommand(QString cmd); // Hàm bổ trợ gửi dữ liệu qua QSerialPort

    // --- BIẾN ĐIỀU KHIỂN HỆ THỐNG VÀ PHẦN CỨNG (ĐÃ TỐI ƯU) ---
    QSerialPort *serial;        // Thay thế HANDLE bằng QSerialPort chuẩn Qt
    QTcpSocket *socket;         // Cổng nhận lệnh điều khiển từ Python (Port 65432)
    QTcpSocket *video_socket;   // Cổng nhận Livestream Video từ Python (Port 65433)
    QByteArray video_buffer;    // Bộ đệm gom dữ liệu ảnh JPEG
    int expected_image_size;    // Biến lưu kích thước ảnh đang đợi

    int currentMode;            
    QProcess *pythonProcess;    

    // --- CÁC THÀNH PHẦN GIAO DIỆN (UI GIỮ NGUYÊN) ---
    QLCDNumber *speedDisplay;
    QProgressBar *batteryBar;
    QLabel *currentModeLabel;
    QLabel *lineStatus;
    QLabel *obstacleWarning;

    QComboBox *comPortComboBox;
    QPushButton *connectButton;
    QPushButton *disconnectButton;

    QPushButton *btnUp; QPushButton *btnDown;
    QPushButton *btnLeft; QPushButton *btnRight;
    QPushButton *btnStop; QPushButton *btnReset;

    QSlider *masterSlider;
    QLabel *masterValueLabel;

    QSlider *m1Slider; QSlider *m2Slider;
    QSlider *m3Slider; QSlider *m4Slider;

    QLabel *m1ValueLabel; QLabel *m2ValueLabel;
    QLabel *m3ValueLabel; QLabel *m4ValueLabel;

    QLabel *cameraDisplay;
};

#endif // MAINWINDOW_H