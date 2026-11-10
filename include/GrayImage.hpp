#ifndef GRAYIMAGEWIDGET_H
#define GRAYIMAGEWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMutex>
#include <QPainter>
#include <QPaintEvent>
#include <cstring>

class GrayImageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GrayImageWidget(QWidget *parent = nullptr)
        : QWidget(parent), m_width(0), m_height(0)
    {
    }

    // 初始化图像大小
    void init(int width, int height)
    {
        QMutexLocker locker(&m_mutex);
        m_width = width;
        m_height = height;

        // 创建灰度图
        m_image = QImage(width, height, QImage::Format_Grayscale8);
        m_image.fill(0); // 初始化为黑色

        setFixedSize(width, height);
    }

    // 获取图像指针，直接操作内存
    uchar* getImagePointer()
    {
        return m_image.bits();
    }

    // 返回每行字节数（考虑对齐）
    int getBytesPerLine() const
    {
        return m_image.bytesPerLine();
    }

    // 高效更新灰度数据（考虑对齐）
    void updateGrayData()
    {
        // QMutexLocker locker(&m_mutex);
        // if (!m_image.isNull() && data) {
        //     uchar* dst = m_image.bits();
        //     int bytesPerLine = m_image.bytesPerLine();
        //     for (int y = 0; y < m_height; ++y) {
        //         uchar* line = dst + y * bytesPerLine;
        //         const uchar* src = data + y * m_width;
        //         memcpy(line, src, m_width); // 每行只拷贝实际像素宽度
        //     }
        // }
        update(); // 通知 QWidget 重绘
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        QMutexLocker locker(&m_mutex);
        if (!m_image.isNull()) {
            painter.drawImage(rect(), m_image); // 自动缩放到窗口大小
        }
    }

private:
    QImage m_image;
    int m_width;
    int m_height;
public:
    QMutex m_mutex; // 线程安全
};

#endif // GRAYIMAGEWIDGET_H