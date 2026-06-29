#include "imagecacher.hpp"

#include <qcryptographichash.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qimage.h>
#include <qloggingcategory.h>
#include <qmutex.h>
#include <qpainter.h>
#include <qsavefile.h>
#include <qthreadpool.h>

Q_LOGGING_CATEGORY(lcCacher, "caelestia.images.cacher", QtInfoMsg)

namespace caelestia::images {

namespace {

QString sourceContentHash(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcCacher).noquote() << "sourceContentHash: failed to open" << path;
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(&file);
    file.close();

    return hash.result().toHex();
}

QString fillSuffix(ImageCacher::FillMode fillMode) {
    switch (fillMode) {
    case ImageCacher::FillMode::Crop:
        return QStringLiteral("crop");
    case ImageCacher::FillMode::Fit:
        return QStringLiteral("fit");
    default:
        return QStringLiteral("stretch");
    }
}

} // namespace

const QString& ImageCacher::cacheDir() {
    static const QString s_dir = [] {
        QString cache = qEnvironmentVariable("XDG_CACHE_HOME");
        if (cache.isEmpty())
            cache = QDir::homePath() + QStringLiteral("/.cache");
        return cache + QStringLiteral("/caelestia/imagecache");
    }();
    return s_dir;
}

QString ImageCacher::cachePathFor(const QString& sourcePath, const QSize& size, FillMode fillMode) {
    const QString sha = sourceContentHash(sourcePath);
    if (sha.isEmpty())
        return {};

    const QString filename =
        QStringLiteral("%1@%2x%3-%4.png")
            .arg(sha, QString::number(size.width()), QString::number(size.height()), fillSuffix(fillMode));

    return cacheDir() + QLatin1Char('/') + filename;
}

QImage ImageCacher::render(const QImage& source, const QSize& size, FillMode fillMode) {
    if (source.isNull() || !size.isValid() || size.isEmpty()) {
        return {};
    }

    Qt::AspectRatioMode scaleMode;
    switch (fillMode) {
    case FillMode::Crop:
        scaleMode = Qt::KeepAspectRatioByExpanding;
        break;
    case FillMode::Fit:
        scaleMode = Qt::KeepAspectRatio;
        break;
    case FillMode::Stretch:
        scaleMode = Qt::IgnoreAspectRatio;
        break;
    }

    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    image = image.scaled(size, scaleMode, Qt::SmoothTransformation);

    if (image.isNull()) {
        return {};
    }

    if (fillMode == FillMode::Stretch) {
        return image;
    }

    QImage canvas(size, QImage::Format_ARGB32);
    canvas.fill(Qt::transparent);

    QPainter painter(&canvas);
    painter.drawImage((size.width() - image.width()) / 2, (size.height() - image.height()) / 2, image);
    painter.end();
    return canvas;
}

ImageCacher* ImageCacher::instance() {
    static ImageCacher s_instance;
    return &s_instance;
}

ImageCacher::ImageCacher(QObject* parent)
    : QObject(parent) {}

void ImageCacher::scheduleSave(const QString& cachePath, const QImage& image) {
    if (cachePath.isEmpty() || image.isNull())
        return;

    {
        QMutexLocker locker(&m_mutex);
        if (m_inflight.contains(cachePath))
            return;
        m_inflight.insert(cachePath);
    }

    QThreadPool::globalInstance()->start([this, cachePath, image]() {
        saveImage(cachePath, image);
        QMutexLocker locker(&m_mutex);
        m_inflight.remove(cachePath);
    });
}

void ImageCacher::saveImage(const QString& cachePath, const QImage& image) {
    if (QFile::exists(cachePath)) {
        return;
    }

    const QString parent = QFileInfo(cachePath).absolutePath();
    if (!QDir().mkpath(parent)) {
        qCWarning(lcCacher).noquote() << "Failed to create cache dir" << parent;
        return;
    }

    QSaveFile saveFile(cachePath);
    if (!saveFile.open(QIODevice::WriteOnly) || !image.save(&saveFile, "PNG") || !saveFile.commit()) {
        qCWarning(
            lcCacher, "Failed to save to %s: %s", qUtf8Printable(cachePath), qUtf8Printable(saveFile.errorString()));
        return;
    }

    qCDebug(lcCacher).noquote() << "Saved to" << cachePath;
}

} // namespace caelestia::images
