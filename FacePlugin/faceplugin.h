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

#ifndef FACEPLUGIN_H
#define FACEPLUGIN_H

#include "FacePlugin_global.h"

#include <QMap>
#include <QString>

#include "fileplugin.h"
#include <opencv2/opencv.hpp>
#include "qopencvimage.h"

#define  FACE_PLUGIN_NAME  "Face Recognition Plugin"
#define  FACE_PLUGIN_TIP   "Handles Face recognition in pictures"

#define NAMES_ATTR      	"Names"
#define RECOGNIZED_ATTR     "Recognized"
#define FACES_ATTR     		"Faces"

#define CASCADE_FACE_XML_FILE "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"

#define LABELS_FILE_SUFFIX      "_labels.txt"
#define RECOGNIZER_FILE_SUFFIX  "_face_recognizer.xml"

#define MIN_FACE_WIDTH  60
#define MIN_FACE_HEIGHT 90


#define _VERBOSE_FACE_PLUGIN 1

class FACEPLUGINSHARED_EXPORT FacePlugin : public FilePlugin {
//    Q_OBJECT
    Q_INTERFACES(PluginInterface)

public:
    enum RecognizerType {EIGEN, FISHER, LBPH};

    explicit FacePlugin() : FilePlugin() {}

    // there's no way to specify a constructor in a plugin interface (nor a static factory)
    // so we call pluginP = pluginP->newInstance(<vPath>); then unload the plugin.
    PluginInterface *newInstance(QString virtualDirectoryPath);
    void            initialize(QString virtualDirectoryPath);

    /**
     * plugin information
     */
    QString getName() {
        return FACE_PLUGIN_NAME;
    }

    QString getTip() {
        return FACE_PLUGIN_TIP;
    }

    void loadAttributes(QString filepath);

private:
    static cv::Ptr<cv::FaceRecognizer>          m_faceRecognizer;     // face recognizer
    static cv::CascadeClassifier                m_faceDetector;       // face detector
    static bool                                 m_cascadeFilesLoaded; // haar cascade files loaded
    static bool                                 m_recognizerLoaded;   // training loaded
    static QStringList                          m_labels;             // the persons names
    static const QString                        m_recognizerNames[];
    RecognizerType                              m_recognizerType;
    static  QMap<QString, AttributeCacheEntry *> m_attributesCache;    // the attributes cache

    cv::CascadeClassifier *getFaceDetector() {
        return &m_faceDetector;
    }

    cv::Ptr<cv::FaceRecognizer> getFaceRecognizer() {
        return m_faceRecognizer;
    }

    bool canDetectFaces() {
        return m_cascadeFilesLoaded;
    }

    bool canRecognizeFaces() {
        return m_recognizerLoaded;
    }

    void setRecognizer(RecognizerType type) {
        switch (m_recognizerType = type) {
            case EIGEN:
                m_faceRecognizer = cv::createEigenFaceRecognizer();
                break;

            case FISHER:
                m_faceRecognizer = cv::createFisherFaceRecognizer();
                break;

            case LBPH:
            default:
                m_faceRecognizer = cv::createLBPHFaceRecognizer();
                break;
        }

        m_recognizerLoaded = false;
    }

    RecognizerType getRecognizerType() {
        return m_recognizerType;
    }

    const QString getRecognizerName() {
        return m_recognizerNames[m_recognizerType];
    }

    QString getLabelsFileName() {
        return m_recognizerNames[m_recognizerType] + QString(LABELS_FILE_SUFFIX);
    }

    QString getFileRecognizerName() {
        return m_recognizerNames[m_recognizerType] + QString(RECOGNIZER_FILE_SUFFIX);
    }

    void loadLabels();
    void loadRecognizer();

    void computeMinFaceSize(cv::Mat &image, double *widthP, double *heightP);
    void findFaces(cv::Mat& image, std::vector<cv::Rect>& objects);
};

#endif // FACEPLUGIN_H

