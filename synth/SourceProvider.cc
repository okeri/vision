#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <thread>
#include <stdexcept>

#include "SourceProvider.hh"

namespace synth {

SourceProvider::SourceProvider(const std::string &id, FrameFormat format) :
        jpeg_(format), dir_(Direction::Forward)
{
    fd_ = open(id.c_str(), O_RDONLY);
    if (fd_ == -1) {
        throw std::runtime_error("Error: cannot open " + id);
    }
    max_ =  lseek(fd_, 0, SEEK_END);

    uint32_t rawSize;
    lseek(fd_, 0, SEEK_SET);
    if (read(fd_, &info_, sizeof(info_)) != sizeof(info_) ||
        read(fd_, &rawSize, sizeof(rawSize)) != sizeof(rawSize)) {
        throw std::runtime_error("Error: cannot read data header " + id);
    }
    min_ = lseek(fd_, 0, SEEK_CUR);
    raw_.resize(rawSize);
}

SourceProvider::~SourceProvider() {
    close(fd_);
}

// if (synthetic) {
//     std::this_thread::sleep_for(std::chrono::milliseconds(30));
// }


Frame SourceProvider::nextFrame() {
    uint64_t pos;
    //    static time_t access;
    if (dir_ == Direction::Forward) {
        pos = lseek(fd_, 0, SEEK_CUR);
        if (pos == max_) {
            dir_ = Direction::Backward;
        }
    } else {
        pos = lseek(fd_, -2 * raw_.size(), SEEK_CUR);
        if (pos <= min_) {
            dir_ = Direction::Forward;
            lseek(fd_, min_, SEEK_SET);
        }
    }

    read(fd_, raw_.data(), raw_.size());
    return jpeg_.unpack(raw_.data(), raw_.size());
}

void SourceProvider::dumpFrame(int stream) {
    throw std::runtime_error("dumping video stream from synthetic "
                             "data is unimplemented");
}

FrameInfo SourceProvider::info() {
    return info_;
}

}  // namespace
