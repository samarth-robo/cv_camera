#ifndef CAPTURE_WRAPPER_H
#define CAPTURE_WRAPPER_H

#include "cv_camera/BosonCapture.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>

class CaptureWrapper {
 private:
  cv::VideoCapture ccap_;
  BosonCapture bcap_;
  bool is_boson;

 public:
  CaptureWrapper(bool boson = true) : is_boson(boson) {}
  void open(int32_t id);
  void open(const std::string &device_path, int api = cv::CAP_V4L);
  void open() { open(0); }
  bool isOpened() { return is_boson ? bcap_.isOpened() : ccap_.isOpened(); }
  bool set(int propId, double value);
  bool read(cv::Mat &im);
  void set_boson(bool boson) { is_boson = boson; }
};

#endif  // CAPTURE_WRAPPER_H