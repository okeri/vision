#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <thread>

#include "SourceProvider.hh"

namespace synth {

SourceProvider::SourceProvider(const std::string &id, FrameFormat format) :
        dir_(Direction::Forward)
{
    fd_ = open(id.c_str(), O_RDONLY);
    if (fd_ == -1) {
        throw std::runtime_error("Error: cannot open " + id);
    }
    max_ =  lseek(fd_, 0, SEEK_END);

    uint32_t rawSize;
    lseek(fd_, 0, SEEK_SET);
    if (read(fd_, &info_, sizeof(info_)) != sizeof(info_)) {
        throw std::runtime_error("Error: cannot read data header " + id);
    }
    max_ -= info_.size();
    min_ = lseek(fd_, 0, SEEK_CUR);
}

SourceProvider::~SourceProvider() {
    close(fd_);
}

Frame SourceProvider::nextFrame() {
    int64_t pos;
    if (dir_ == Direction::Forward) {
        pos = lseek(fd_, 0, SEEK_CUR);
        if (pos >= max_) {
            dir_ = Direction::Backward;
        }
    } else {
        pos = lseek(fd_, (min_ == max_  ? -1 :-2) * info_.size(), SEEK_CUR);
        if (pos <= min_) {
            dir_ = Direction::Forward;
            lseek(fd_, min_, SEEK_SET);
        }

    }
    static Frame frame(info_.size());
    read(fd_, frame.data(), frame.size());
    return frame;
}

FrameInfo SourceProvider::info() {
    return info_;
}

}  // namespace
