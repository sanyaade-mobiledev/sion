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

#include <QDebug>
#include <QtCore/qplugin.h>
#include <QFile>
#include <QDir>
#include <QPluginLoader>
#include <QApplication>

#include "faceplugin.h"
#include "scriptrunner.h"

const QString                   FacePlugin::m_recognizerNames[] = {"eigen", "fisher", "lbph"};
cv::Ptr<cv::FaceRecognizer>     FacePlugin::m_faceRecognizer;
cv::CascadeClassifier           FacePlugin::m_faceDetector;
bool                            FacePlugin::m_cascadeFilesLoaded = false;
bool                            FacePlugin::m_recognizerLoaded = false;
QStringList                     FacePlugin::m_labels;

QMap<QString, AttributeCacheEntry *>   FacePlugin::m_attributesCache;

PluginInterface *FacePlugin::newInstance(QString virtualDirectoryPath) {
    FacePlugin *newInstanceP = new FacePlugin();
    newInstanceP->initialize(virtualDirectoryPath);
    return newInstanceP;
}

void FacePlugin::initialize(QString virtualDirectoryPath) {
    FilePlugin::initialize(virtualDirectoryPath);

    // this one keeps only image files containing a face (default rule).
    m_scriptP = new Script("{\n\tplugin.setResult(plugin.getAttributeValue(\"Faces\") > 0);\n}");

    m_attributes.insert(NAMES_ATTR, new Attribute(NAMES_ATTR, tr("Names of people found in the picture (ie: \"Paul;Jhon;Theresa\".)"), "String"));
    m_attributes.insert(RECOGNIZED_ATTR,  new Attribute(RECOGNIZED_ATTR, tr("Number of recognized faces"), "Numeric"));
    m_attributes.insert(FACES_ATTR, new Attribute(FACES_ATTR, tr("Number of faces found in the picture"), "Numeric"));

    // load the cascade classifier file (front face)
    if (!m_cascadeFilesLoaded && !m_faceDetector.load(CASCADE_FACE_XML_FILE))
        return;

    m_cascadeFilesLoaded = true;

    // load the recognizer.
    if (!m_recognizerLoaded) {
        setRecognizer(LBPH); // faster yet less accurate
        loadRecognizer();

        // load the names if the recognizer could be loaded
        if (m_recognizerLoaded)
            loadLabels();
    }
}

void FacePlugin::loadAttributes(QString filepath) {
    int                     recognized = 0;
    QString                 names;
    uint                    numFaces = 0;
    std::vector<cv::Rect>   faces;
    cv::Mat                 originalImage;
    cv::Mat                 testImage;

#ifdef _VERBOSE_FACE_PLUGIN
    qDebug() << "loading attributes for file: " << filepath;
#endif

    // loads the base attributes
    FilePlugin::loadAttributes(filepath);

    // are the attributes in the cache?
    if (retrieveAttributesFromCache(filepath, FacePlugin::m_attributesCache))
        return;

    setAttributeValue(NAMES_ATTR, QVariant(""));
    setAttributeValue(RECOGNIZED_ATTR, QVariant(0));
    setAttributeValue(FACES_ATTR, QVariant(0));
    
    // can the plugin actually do the job?
    if (!m_cascadeFilesLoaded || !m_recognizerLoaded)
        return;

    // loads the "face" attributes
    originalImage = cv::imread(filepath.toAscii().data()); //, CV_LOAD_IMAGE_COLOR);
    if (originalImage.empty())
        return;

    // convert it
    QOpenCvImage qcvImage(originalImage);
    qcvImage.normalize();
    testImage = cv::Mat(*qcvImage.getMatImage());

    // search for faces
    findFaces(testImage, faces);
    numFaces = (uint)faces.size();
    setAttributeValue(FACES_ATTR, QVariant(numFaces));
    if (numFaces == 0)
        goto endLoadAttributes;

    for(uint i = 0; i < numFaces; i++ ) {
        cv::Mat face = cv::Mat(testImage(faces[i]));
        cv::resize(face, face, cv::Size(THUMB_WIDTH, THUMB_HEIGHT));

        // check the face against the recognizer training
        int     predictedLabelIndex = -1;
        double  distance = 0.0;

        // convert to gray, SINGLE channel, to prevent problems in the face recognizer...
        cvtColor(face, face, CV_BGR2GRAY, 1);

        // equalize histogram
        equalizeHist(face, face);

        m_faceRecognizer->predict(face, predictedLabelIndex, distance);
        // #### for some reason, the returned label index can be a large negative value...
        // so let's be paranoid.
        if (predictedLabelIndex >= 0 && predictedLabelIndex < m_labels.size()) {
            ++recognized;

            // label
            names += m_labels[predictedLabelIndex];
            names += ";";
        }
    }

#ifdef _VERBOSE_FACE_PLUGIN
    qDebug() << "found " << numFaces << " faces, and recognized " << recognized << " faces: " << names;
#endif

    setAttributeValue(RECOGNIZED_ATTR, QVariant(recognized));
    setAttributeValue(NAMES_ATTR, names);

endLoadAttributes:
    // save attributes in the cache
    saveAttributesInCache(filepath, FacePlugin::m_attributesCache);
}

/**
 * Load the people names.
 */
void FacePlugin::loadLabels() {
    QString labelsFullFilename = QCoreApplication::applicationDirPath() + QDir::separator() + getLabelsFileName();

    m_labels.clear();

    // load the labels
    QFile labelsFile(labelsFullFilename);
    if (!labelsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString label;
    for(;;) {
        label = labelsFile.readLine();
        label.chop(QString('\n').length());
        if (!label.isEmpty())
            m_labels.append(label);
        else
            break;
    }

    labelsFile.close();

#ifdef _VERBOSE_FACE_PLUGIN
    qDebug() << "loaded labels (from file: " << labelsFullFilename << "):\n" << m_labels;
#endif
}

/**
  * Load the face recognizer state.
  */
void FacePlugin::loadRecognizer() {
    QString faceRecognizerFullFilename = QCoreApplication::applicationDirPath() + QDir::separator() + getFileRecognizerName();
    if (QFileInfo(faceRecognizerFullFilename).exists()) {
        m_faceRecognizer->load(faceRecognizerFullFilename.toAscii().data());
        m_recognizerLoaded = true;
    }

#ifdef _VERBOSE_FACE_PLUGIN
    qDebug() << "loaded recognizer from file: " << faceRecognizerFullFilename;
#endif
}

/**
 * Return in objects the rectangles of the faces found in image.
 */
void FacePlugin::findFaces(cv::Mat& image, std::vector<cv::Rect>& objects) {
    double width, height;

    // get the min face size depending on the image size
    computeMinFaceSize(image, &width, &height);

    // search for faces
    m_faceDetector.detectMultiScale(image, objects, 1.1, 3, CV_HAAR_SCALE_IMAGE, cv::Size((int)width, (int)height), cv::Size((int)image.cols, (int)image.rows));
}

/**
 * Compute the minimum faces size depending on the image size and orientation
 */
void FacePlugin::computeMinFaceSize(cv::Mat &image, double *widthP, double *heightP) {
    if (!widthP || !heightP)
        return;

    // edge case, small picture, could be a portrait
    // the face detection cost will be ok anyway
    if (image.rows <= THUMB_HEIGHT * 2 &&
        image.cols <= THUMB_WIDTH * 2) {
        *widthP = image.cols / 3;
        *heightP = image.rows / 3;
        return;
    }

    // face height occupies at least 1/8th * height/width of the image
    // height, hence more space on a portrait image, than on a landscape
    // image. I'm just assuming that, when people are photographied,
    // portrait pictures are focused on a small number of persons, at a close
    // distance, while landscape pictures would show more people, or nature
    // oriented stuff in the background, with people standing further from
    // the camera.
    //
    // the min face width is 2/3 of the face min height
    *heightP = image.rows;
    *widthP = image.cols;
    *heightP /= (8.0 * *heightP / *widthP);

    // face aspect ratio applied to width
    *widthP = (*heightP * 2.0) / 3.0;

    // check we've not messed up min sizes
    if (*widthP < MIN_FACE_WIDTH)
        *widthP = MIN_FACE_WIDTH;
    if (*heightP < MIN_FACE_HEIGHT)
        *heightP = MIN_FACE_HEIGHT;

#ifdef _VERBOSE_FACE_PLUGIN
    qDebug() << "min face width: " << *widthP << ", min face height: " << *heightP;
#endif
}

Q_EXPORT_PLUGIN2(FacePlugin, FacePlugin)

