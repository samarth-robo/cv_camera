#ifndef BOSON_CAPTURE_H
#define BOSON_CAPTURE_H

#include <stdio.h>
#include <fcntl.h>               // open, O_RDWR
#include <opencv2/opencv.hpp>
#include <unistd.h>              // close
#include <sys/ioctl.h>           // ioctl
#include <asm/types.h>           // videodev2.h
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/videodev2.h>
#include <string>

#define YUV   0
#define RAW16 1

// Define COLOR CODES
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

// Types of sensors supported
enum sensor_types {
  Boson320, Boson640
};


class BosonCapture {
 private:
  int ret;
  int fd;
  int i;
  struct v4l2_capability cap;
  char video[20];                // To store Video Port Device
  char thermal_sensor_name[20];  // To store the sensor name
  int  video_mode;
  sensor_types my_thermal;
  int type;

  bool is_open;
  int width, height;
  int luma_height, luma_width;
  int color_space;
  cv::Mat thermal16, thermal_luma, thermal_rgb;
  struct v4l2_buffer bufferinfo;

public:
  BosonCapture();
  ~BosonCapture();
  void open(int32_t id);
  void open() { open(0); }
  bool isOpened() { return is_open; }
  bool read(cv::Mat &im);
  bool set(int, double) { return false; }
};

#endif  // BOSON_CAPTURE_H