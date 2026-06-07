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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
private slots:
    void onMasterSpeedChanged(int value);
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onMoveLeftClicked();
    void onMoveRightClicked();
    void onStopCarClicked();
    void onResetPressed();
    
    void updateCameraFrame(const QImage &frame);
    void handleGesture(int fingers);
    void readPythonOutput(); // Slot để đọc dữ liệu từ tiến trình Python gửi lên
    
    void readSerialData();

private:
    void processSingleMotorMovement(QString motorName);
    void processManualMovement(QString direction);
    void sendControlPacket();
    void updateUI(int battery, double dist, int lineState);

    int currentMode;

    QLCDNumber *speedDisplay;
    QProgressBar *batteryBar;
    QLabel *currentModeLabel;
    QLabel *lineStatus;
    QLabel *obstacleWarning;
    QLabel *cameraDisplay;

    QSlider *masterSlider;
    QLabel *masterValueLabel;

    QString currentTestingMotor;
    
    // Chung động cơ 
    QPushButton *btnUp;
    QPushButton *btnDown;
    QPushButton *btnLeft;
    QPushButton *btnRight;
    QPushButton *btnStop;
    QPushButton *btnReset;

    // Riêng từng động cơ
    QPushButton *btnFL; // Front Left - Trước Trái
    QPushButton *btnFR; // Front Right - Trước Phải
    QPushButton *btnRL; // Rear Left - Sau Trái
    QPushButton *btnRR; // Rear Right - Sau Phải

    // Thay thế QThread và CameraWorker cũ bằng QProcess
    QProcess *pythonProcess;
    QTimer *serialTimer;
};

#endif // MAINWINDOW_H