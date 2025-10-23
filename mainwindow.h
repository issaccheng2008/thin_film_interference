#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QSlider;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void initializePlot();
    void connectControls();
    void updateVisualization();
    double sliderToValue(const QSlider *slider, double scale, double offset = 0.0) const;
    double computeBrightness(double r) const;
    void updateRingImage();
    void updatePlot();

    Ui::MainWindow *ui;
    QImage mRingImage;
    bool mLockingXAxisRange = false;
    bool mLockingYAxisRange = false;
    double mE = 744.0;
    double mN1 = 3.0;
    double mN2 = 5.0;
    double mN3 = 10.0;
    double mLambda = 163.0;
    double mF = 1.0;
};

#endif // MAINWINDOW_H
