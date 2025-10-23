#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QColor>
#include <QLabel>
#include <QPen>
#include <QPixmap>
#include <QResizeEvent>
#include <QSlider>
#include <QVector>
#include <QtGlobal>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initializePlot();
    connectControls();
    updateVisualization();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateRingImage();
}

void MainWindow::initializePlot()
{
    ui->customplot->addGraph();
    ui->customplot->graph()->setLineStyle(QCPGraph::lsLine);
    ui->customplot->graph()->setPen(QPen(QColor(0, 120, 215), 2));
    ui->customplot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    ui->customplot->axisRect()->setRangeDrag(Qt::Horizontal);
    ui->customplot->axisRect()->setRangeZoom(Qt::Horizontal);

    ui->customplot->xAxis->setLabel(QStringLiteral("r (m)"));
    ui->customplot->yAxis->setLabel(QStringLiteral("brightness"));
    ui->customplot->yAxis->setRange(0.0, 1.0);
    ui->customplot->xAxis->setRange(0.0, 1.0);

    connect(ui->customplot->xAxis,
            QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this,
            [this](const QCPRange &range) {
        if (mLockingXAxisRange)
            return;
        mLockingXAxisRange = true;
        double upper = range.upper;
        if (upper <= 0.0) {
            upper = 0.1;
        }
        ui->customplot->xAxis->setRange(0.0, upper);
        mLockingXAxisRange = false;
        updateVisualization();
    });

    connect(ui->customplot->yAxis,
            QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged), this,
            [this](const QCPRange &) {
        if (mLockingYAxisRange)
            return;
        mLockingYAxisRange = true;
        ui->customplot->yAxis->setRange(0.0, 1.0);
        mLockingYAxisRange = false;
    });
}

void MainWindow::connectControls()
{
    auto triggerUpdate = [this]() { updateVisualization(); };

    connect(ui->sliderE, &QSlider::valueChanged, this, triggerUpdate);
    connect(ui->sliderN1, &QSlider::valueChanged, this, triggerUpdate);
    connect(ui->sliderN2, &QSlider::valueChanged, this, triggerUpdate);
    connect(ui->sliderN3, &QSlider::valueChanged, this, triggerUpdate);
    connect(ui->sliderLambda, &QSlider::valueChanged, this, triggerUpdate);
    connect(ui->sliderF, &QSlider::valueChanged, this, triggerUpdate);
}

double MainWindow::sliderToValue(const QSlider *slider, double scale, double offset) const
{
    if (!slider || qFuzzyIsNull(scale))
        return 0.0;
    return slider->value() / scale + offset;
}

void MainWindow::updateVisualization()
{
    mE = sliderToValue(ui->sliderE, 0.1);
    mN1 = sliderToValue(ui->sliderN1, 100.0);
    mN2 = sliderToValue(ui->sliderN2, 100.0);
    mN3 = sliderToValue(ui->sliderN3, 100.0);
    mLambda = sliderToValue(ui->sliderLambda, 1.0);
    mF = sliderToValue(ui->sliderF, 100.0);

    ui->labelEValue->setText(QString::number(mE, 'f', 2));
    ui->labelN1Value->setText(QString::number(mN1, 'f', 2));
    ui->labelN2Value->setText(QString::number(mN2, 'f', 2));
    ui->labelN3Value->setText(QString::number(mN3, 'f', 2));
    ui->labelLambdaValue->setText(QString::number(mLambda, 'f', 2));
    ui->labelFValue->setText(QString::number(mF, 'f', 2));

    updateRingImage();
    updatePlot();
}
double ref(double n1, double n2, double theta_i)
{
    if (n1 > n2 && sin(theta_i) > n2 / n1)
        return 1.0;

    // Snell’s law
    double sin_t = n1 * sin(theta_i) / n2;
    double theta_t = asin(sin_t);

    double cos_i = cos(theta_i);
    double cos_t = cos(theta_t);

    // Fresnel reflection coefficients
    double Rs = pow((n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t), 2);
    double Rp = pow((n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t), 2);

    // Return average for unpolarized light
    return 0.5 * (Rs + Rp);
}

double MainWindow::computeBrightness(double r) const
{
    if (mF <= 0.0)
        return 0.0;
    double i=atan(r/mF);
    if(sin(i)>mN2/mN1)return 1;
    double j=asin(sin(i)*mN1/mN2);
    double dif=2*mE*mN2*cos(j)/mN1;
    double d=dif*2*3.1415926/mLambda;
    if(((mN1<mN2)+(mN2<mN3))==1)d+=3.1415926;
    double I1=ref(mN1,mN2,i);
    double I2=(1-I1)*ref(mN2,mN3,j)*(1-ref(mN2,mN1,j));
    double bright=sqrt(I1+I2+2*sqrt(I1*I2)*cos(d));
    return qBound(0.0, bright, 1.0);
}

void MainWindow::updateRingImage()
{
    if (!ui->ringLabel)
        return;

    const int width = ui->ringLabel->width();
    const int height = ui->ringLabel->height();
    if (width <= 0 || height <= 0)
        return;

    if (mRingImage.size() != QSize(width, height)) {
        mRingImage = QImage(width, height, QImage::Format_RGB32);
    }

    mRingImage.fill(Qt::black);

    const double cx = static_cast<double>(width) / 2.0;
    const double cy = static_cast<double>(height) / 2.0;
    const double maxRadiusPixels = qMax(1.0, qMin(cx, cy));
    const double rMax = qMax(0.1, ui->customplot->xAxis->range().upper);

    for (int y = 0; y < height; ++y) {
        QRgb *scanLine = reinterpret_cast<QRgb *>(mRingImage.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const double dx = static_cast<double>(x) - cx;
            const double dy = static_cast<double>(y) - cy;
            const double radiusPixels = qSqrt(dx * dx + dy * dy);
            const double normalizedRadius = radiusPixels / maxRadiusPixels;
            const double r = normalizedRadius * rMax;
            const double brightness = computeBrightness(r);
            const int colorValue = qBound(0, static_cast<int>(brightness * 255.0 + 0.5), 255);
            scanLine[x] = qRgb(colorValue, colorValue, colorValue);
        }
    }

    ui->ringLabel->setPixmap(QPixmap::fromImage(mRingImage));
}

void MainWindow::updatePlot()
{
    if (!ui->customplot || ui->customplot->graphCount() == 0)
        return;

    const double rMax = qMax(0.1, ui->customplot->xAxis->range().upper);
    const double dr = 0.001;
    const int pointCount = static_cast<int>(rMax / dr) + 1;

    QVector<double> x(pointCount);
    QVector<double> y(pointCount);

    for (int i = 0; i < pointCount; ++i) {
        const double r = i * dr;
        x[i] = r;
        y[i] = computeBrightness(r);
    }

    ui->customplot->graph(0)->setData(x, y);
    ui->customplot->replot();
}
