/*
 * SION! Server Face (recognition) plugin.
 *
 * Copyright (C) Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Gilles Fabre <gilles.fabre@intel.com>
 */

#include "qopencvimage.h"

/**
  * Builds a new widget embedding the given openCV image. The openCV
  * image is converted into a QImage here.
  */
QOpenCvImageWidget::QOpenCvImageWidget(cv::Mat image, QWidget *parentP) : QWidget(parentP), m_image(image) {
    setFixedSize(m_image.getQImage()->width(), m_image.getQImage()->height());
}

/**
  * Initiate a drag & drop process when the left mouse button is pressed on the
  * widget.
  */
void QOpenCvImageWidget::mousePressEvent(QMouseEvent *eventP) {
    const QImage *imageP = m_image.getQImage();

    if (!imageP || !(eventP->buttons() & Qt::LeftButton))
        return;

    QMimeData *mimeDataP = new QMimeData();
    mimeDataP->setImageData(*imageP);

    QDrag *dragP = new QDrag(this);
    dragP->setMimeData(mimeDataP);
    dragP->setPixmap(QPixmap::fromImage(*imageP));
    dragP->setHotSpot(eventP->pos());
    dragP->exec(Qt::CopyAction);
}

/**
  * Paint the embedded QImage on paint event receipt.
  */
void QOpenCvImageWidget::paintEvent(QPaintEvent *eventP) {
    Q_UNUSED(eventP);

    const QImage *imageP = m_image.getQImage();
    if (!imageP)
        return;

    // display the image
    QPainter painter(this);
    painter.drawImage(QPoint(0,0), *imageP);
    painter.end();
}

/**
  * Converts an openCV image into a new instance of QImage and return the latter.
  */
QImage *QOpenCvImage::mat2QImage(const cv::Mat3b &src) {
    QImage *destP = new QImage(src.cols, src.rows, QImage::Format_ARGB32);

    for (int y = 0; y < src.rows; ++y) {
        const cv::Vec3b *srcrow = src[y];
        QRgb *destrow = (QRgb*)destP->scanLine(y);
        for (int x = 0; x < src.cols; ++x)
            destrow[x] = qRgba(srcrow[x][2], srcrow[x][1], srcrow[x][0], 255);
    }

    return destP;
}

/**
  * Rotate this image.
  */
void QOpenCvImage::rotate(double angle) {
    cv::Mat rotMat(2, 3, CV_32FC1);

    // compute a rotation matrix with respect to the center of the image
    cv::Point center = cv::Point(m_matImage.cols / 2, m_matImage.rows / 2);

    // get the rotation matrix with the specifications above
    rotMat = cv::getRotationMatrix2D(center, angle, 1.0);

    // rotate the image
    cv::warpAffine(m_matImage, m_matImage, rotMat, m_matImage.size());

    // update the QImage
    delete m_qImageP;
    m_qImageP = mat2QImage((const cv::Mat3b &)m_matImage);
}

/**
  * Warp (perspective) the image by applying a x,y transformation ratio to the
  * top right & left, and bottom right & left angles of the image. Ratios
  * are relative to the max row/col values (e.g: 0.0 does not move a point,
  * while 1.0 shifts the point by the image size).
  */
void QOpenCvImage::perspective(double topLeftRatioX, double topLeftRatioY,
                               double topRightRatioX, double topRightRatioY,
                               double bottomRightRatioX, double bottomRightRatioY,
                               double bottomLeftRatioX, double bottomLeftRatioY) {
    cv::Point2f srcTri[4];
    cv::Point2f dstTri[4];

    cv::Mat warpMat(2, 3, CV_32FC1);

    // set the coordinates of the topLeft, topRight, bottomRight, bottomLeft points
    srcTri[0] = cv::Point2f(0, 0);
    srcTri[1] = cv::Point2f(m_matImage.cols - 1, 0);
    srcTri[2] = cv::Point2f(m_matImage.cols - 1, m_matImage.rows - 1);
    srcTri[3] = cv::Point2f(0, m_matImage.rows - 1);

    // set the points transformation matrix
    dstTri[0] = cv::Point2f(m_matImage.cols * topLeftRatioX, m_matImage.rows * topLeftRatioY);
    dstTri[1] = cv::Point2f(m_matImage.cols * topRightRatioX, m_matImage.rows * topRightRatioY);
    dstTri[2] = cv::Point2f(m_matImage.cols * bottomRightRatioX, m_matImage.rows * bottomRightRatioY);
    dstTri[3] = cv::Point2f(m_matImage.cols * bottomLeftRatioX, m_matImage.rows * bottomLeftRatioY);

    // compute the image affine transformation matrix
    warpMat = cv::getPerspectiveTransform(srcTri, dstTri);

    // apply the perspective transform matrix
    cv::warpPerspective(m_matImage, m_matImage, warpMat, m_matImage.size());

    // update the QImage
    delete m_qImageP;
    m_qImageP = mat2QImage((const cv::Mat3b &)m_matImage);
}

/**
  * All images are THUMB_SIZE x THUMB_SIZE pixels and grayscaled.
  *
  * Warning: m_imageP can't be null.
  */

void QOpenCvImage::normalize() {
    // normalize the mat image

    // normalize format
    switch(m_matImage.channels()) {
        case 1:
            cv::normalize(m_matImage, m_matImage, 0, 255, cv::NORM_MINMAX, CV_8UC1);
            break;

        case 3:
            cv::normalize(m_matImage, m_matImage, 0, 255, cv::NORM_MINMAX, CV_8UC3);
            break;

        default:
            break;
    }
    // convert to gray, SINGLE channel, to prevent problems in the face recognizer...
    cvtColor(m_matImage, m_matImage, CV_BGR2GRAY, 1);

    // equalize histogram
    equalizeHist(m_matImage, m_matImage);

    // update the QImage
    setMatImage(m_matImage);
}

void QOpenCvImage::changeToThumb() {
    // change the mat image to grayed/thumbsized
    normalize();
    cv::resize(m_matImage, m_matImage, cv::Size(THUMB_WIDTH, THUMB_HEIGHT));

    // update the QImage
    setMatImage(m_matImage);
}

/**
  * Paint the thumb on paint event receipt. Embedded image is normalize
  * when painted for the first time.
  */
void QOpenCvImageThumbWidget::paintEvent(QPaintEvent *eventP) {
    const QImage *imageP = m_image.getQImage();
    if (!imageP)
        return;

    // transform the image into a grayscale thumb if not done yet
    if (!m_thumbed)
        changeToThumb();

    // superclass will display the image
    QOpenCvImageWidget::paintEvent(eventP);
}

/**
  * Normalize the image so that all images are THUMB_SIZE x THUMB_SIZE pixels and
  * grayscaled.
  */
void QOpenCvImageThumbWidget::changeToThumb() {
    m_image.changeToThumb();

    setFixedSize(m_image.getWidth(), m_image.getHeight());

    m_thumbed = true;
}

