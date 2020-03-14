#include "cv_camera/CaptureWrapper.h"

using namespace cv;

void CaptureWrapper::open(int32_t id) {
    if (is_boson) bcap_.open(id);
    else ccap_.open(id);
}

void CaptureWrapper::open(const std::string &device_path, int api) {
    if (!is_boson) {
        ccap_.open(device_path, api);
    }
}

bool CaptureWrapper::set(int propId, double value) {
    return is_boson ? bcap_.set(propId, value) : ccap_.set(propId, value);
}

bool CaptureWrapper::read(cv::Mat &im) {
    return is_boson ? bcap_.read(im) : ccap_.read(im);
}