#pragma once

#include "tracking/TrackingTypes.hpp"

#include <QFile>
#include <QString>
#include <array>

namespace vc {

// Reads M5 labelled landmark sessions incrementally (no camera, image or
// SendInput). This class only parses data; it never touches an output backend.
class GestureReplayReader final {
public:
    bool open(const QString &filePath);
    bool next(TrackingFrame *frame, std::array<QString, kHandCount> *labels);
    void close();
    bool active() const { return file_.isOpen(); }
    bool finished() const { return finished_; }
    QString error() const { return error_; }
    QString path() const { return file_.fileName(); }

private:
    QFile file_;
    QString error_;
    bool finished_{true};
    int framesRead_{};
    std::int64_t lastTimestamp_{};
};

} // namespace vc
