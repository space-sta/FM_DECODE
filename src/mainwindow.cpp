#include "mainwindow.hpp"
#include <QVBoxLayout>
#include "SysParams.hpp"
// #include <QDateTime>
// #include <QRandomGenerator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QList<QPointF> points;
    points.reserve(DATALEN);
    int i;
    for ( i = 0; i < DATALEN; ++i) {
        points.append(QPointF(i, 4.1e6));
    }
    // 1. 创建数据系列
    series1 = new QLineSeries();
    series1->append(points);
    series1->setName("FM解调并去加重后的数据");

    series2 = new QLineSeries();
    series2->setName("第二条线");          // 图例中显示的名称
    series2->setColor(Qt::red);            // 设置颜色为红色（可选）    
    series2->append(0,iretohz(-20));
    series2->append(DATALEN-1,iretohz(-20));
    series2->append(DATALEN-1,iretohz(10));
    series2->append(0,iretohz(10));

    // 添加数据点
    QList<QPointF> points1;
    points1.reserve(20); // QList 预分配内存

    // 用默认值填充（例如 (0,4.1e6)），占住位置
    for (i = 0; i < 20; ++i) {
        points1.append(QPointF(2.387+100*i, 4.1e6)); 
    }
    // 创建散点系列
    scatterSeries = new QScatterSeries();
    scatterSeries->append(points1);
    scatterSeries->setName("过渡点");          // 图例名称
    scatterSeries->setColor(Qt::green);         // 点颜色
    // scatterSeries->setMarkerSize(10);          // 点的大小（像素）
    // scatterSeries->setBorderColor(Qt::black);  // 点的边框颜色（可选）
    
    // 2. 创建图表
    chart = new QChart();
    chart->addSeries(series1);
    chart->addSeries(series2);
    chart->addSeries(scatterSeries);
    chart->legend()->show();
    chart->setTitle("可缩放的图表");

    // 3. 创建坐标轴
    axisX = new QValueAxis();
    axisX->setTitleText("坐标");
    axisX->setLabelFormat("%.2f");
    axisX->setRange(0, DATALEN); // 初始显示最近

    axisY = new QValueAxis();
    axisY->setTitleText("幅值/hz");
    axisY->setRange(3.75e6,4.8e6);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series1->attachAxis(axisX);
    series1->attachAxis(axisY);
    series2->attachAxis(axisX);
    series2->attachAxis(axisY);
    scatterSeries->attachAxis(axisX);
    scatterSeries->attachAxis(axisY);

    // 4. 图表视图
    chartView = new QChartView(chart);
    // chartView->setRenderHint(QPainter::Antialiasing);

    // 假设 chartView 是您的 QChartView 对象
    chartView->setRubberBand(QChartView::RectangleRubberBand);

    // 创建按钮
    resetBtn = new QPushButton("重置缩放");
    connect(resetBtn, &QPushButton::clicked, chart, &QChart::zoomReset);
    m_continueBtn = new QPushButton("继续", this);
    setCentralWidget(m_continueBtn);
    connect(m_continueBtn, &QPushButton::clicked, this, &MainWindow::onContinueClicked);

    // 5. 布局
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->addWidget(chartView);
    layout->addWidget(resetBtn);
    layout->addWidget(m_continueBtn);
    setCentralWidget(central);

}

void MainWindow::updateSeries1Points(QList<QPointF>& points)
{
    // 添加新点
    series1->replace(points);
}
void MainWindow::updateSeries2Points(float hsyn,float ire0)
{
    series2->replace(0,0,hsyn);
    series2->replace(1,DATALEN-1,hsyn);
    series2->replace(2,DATALEN-1,ire0);
    series2->replace(3,0,ire0);
    series2->setColor(Qt::darkRed);            // 设置颜色为
}

// void MainWindow::addSelect(QList<int>& li)
// {
//     series1->selectPoints(li);
//     series1->setSelectedColor(Qt::green);
// }


void MainWindow::addScatterPoints(QList<QPointF> &points)
{
    scatterSeries->replace(points);
}

void MainWindow::waitForContinue()
{
    if (m_paused) {
            qDebug() << "暂停中，等待继续按钮...";
            QEventLoop loop;
            m_loop = &loop;
            loop.exec();          // 阻塞在这里，但界面不会卡死
            m_loop = nullptr;
            qDebug() << "收到继续信号，恢复运行";
    }

}

void MainWindow::onContinueClicked()
{
    if (m_loop) {
        // m_paused = false;   // 清除暂停标志
        m_loop->quit();     // 退出事件循环，恢复执行
    }
}