#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QPushButton>
// #include <QTimer>

// QT_CHARTS_USE_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

// private slots:
public:
    void updateSeries1Points(QList<QPointF>& points);
    void updateSeries2Points(float hsyn,float ire0);
    void addScatterPoints(QList<QPointF>& points);
    void onContinueClicked();
    void waitForContinue();
    // void addSelect(QList<int>& li);

private:
    QChartView *chartView;
    QChart *chart;
    QLineSeries *series1;
    QLineSeries *series2;
    QScatterSeries *scatterSeries;
    QValueAxis *axisX;
    QValueAxis *axisY;

    QPushButton *resetBtn;
    QPushButton *m_continueBtn;
    QEventLoop *m_loop = nullptr;
    bool m_paused = true;    // 初始为暂停状态
    // QTimer *timer;
    // qreal timeCounter;  // 用于模拟 X 轴时间
    // static const int MAX_POINTS = 500; // 最多显示的点数
};

#endif