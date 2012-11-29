#ifndef QOPENCVIMAGE_H
#define QOPENCVIMAGE_H

#include <QImage>
#include <QPainter>
#include <QWidget>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QDataStream>

#include <opencv2/opencv.hpp>

#define THUMB_WIDTH  90
#define THUMB_HEIGHT 120

/**
  * This class handles an image representation which is based on an openCV
  * image, copied into an embedded QImage.
  */
class QOpenCvImage : public QObject {
    Q_OBJECT

public:
    explicit QOpenCvImage(cv::Mat image) {
        m_qImageP = NULL;

        setMatImage(image);
    }

    void setMatImage(cv::Mat image) {
        if (m_qImageP)
            delete m_qImageP;

        m_qImageP = NULL;
        m_width = 0;
        m_height = 0;

        // convert the image to the RGB888 format
        switch (image.type()) {
            case CV_8UC1:
                cvtColor(image, m_matImage, CV_GRAY2RGB);
                break;

            case CV_8UC3:
                cvtColor(image, m_matImage, CV_BGR2RGB);
                break;

            default:
                qDebug() << "unsupported openCV image type";
                return;
        }

        // QImage needs the data to be stored continuously in memory
        if (!m_matImage.isContinuous()) {
            qDebug() << "unsupported openCV image format (discontinuous pixel data)";
            return;
        }

        m_qImageP = mat2QImage((const cv::Mat3b &)m_matImage);
        m_width = m_qImageP->width();
        m_height = m_qImageP->height();
    }

    virtual ~QOpenCvImage() {
        if (m_qImageP)
            delete m_qImageP;
    }

    static QImage *mat2QImage(const cv::Mat3b &src);

    const cv::Mat* getMatImage() {
        return &m_matImage;
    }

    const QImage *getQImage() {
        return m_qImageP;
    }

    void changeToThumb();
    void normalize();
    void rotate(double angle);
    void perspective(double topLeftRatioX, double topLeftRatioY,
                     double topRightRatioX, double topRightRatioY,
                     double bottomRightRatioX, double bottomRightRatioY,
                     double bottomLeftRatioX, double bottomLeftRatioY);

    int getWidth() {
        return m_width;
    }

    int getHeight() {
        return m_height;
    }

private:
    QImage  *m_qImageP;
    cv::Mat m_matImage;
    int     m_width;        // always reflect the embedded QImage size (might be temporarilly inconsistent with the m_matImage size)
    int     m_height;
};

/**
  * This class specifies a widget capable of displaying an openCV
  * (cv::Mat) image.
  */
class QOpenCvImageWidget : public QWidget {
    Q_OBJECT

public:
    explicit QOpenCvImageWidget(cv::Mat image, QWidget *parentP = 0);

    const cv::Mat* getMatImage() {
        return m_image.getMatImage();
    }

    const QImage *getQImage() {
        return m_image.getQImage();
    }

protected:
    QOpenCvImage m_image;

    void mousePressEvent(QMouseEvent *eventP);
    void paintEvent(QPaintEvent *eventP);
};


/**
  * This class specifies a widget capable of displaying an openCV
  * (cv::Mat) image, resized and grayscaled.
  */
class QOpenCvImageThumbWidget : public QOpenCvImageWidget {
    Q_OBJECT

public:
    explicit QOpenCvImageThumbWidget(cv::Mat image, QWidget *parentP = 0) : QOpenCvImageWidget(image, parentP) {
        m_thumbed = false;
    }

protected:
    void paintEvent(QPaintEvent *eventP);

private:
    bool m_thumbed;

    virtual void changeToThumb();
};

#endif // QOPENCVIMAGE_H
