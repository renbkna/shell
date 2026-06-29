#pragma once

#include <qmutex.h>
#include <qobject.h>
#include <qimage.h>
#include <qset.h>
#include <qsize.h>
#include <qstring.h>

namespace caelestia::images {

class ImageCacher : public QObject {
    Q_OBJECT

public:
    enum class FillMode {
        Crop,
        Fit,
        Stretch,
    };

    static ImageCacher* instance();

    static const QString& cacheDir();
    static QString cachePathFor(const QString& sourcePath, const QSize& size, FillMode fillMode);
    static QImage render(const QImage& source, const QSize& size, FillMode fillMode);

    void scheduleSave(const QString& cachePath, const QImage& image);

private:
    explicit ImageCacher(QObject* parent = nullptr);

    static void saveImage(const QString& cachePath, const QImage& image);

    QMutex m_mutex;
    QSet<QString> m_inflight;
};

} // namespace caelestia::images
