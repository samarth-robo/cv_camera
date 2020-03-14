// taken from https://github.com/FLIR/BosonUSB/blob/master/BosonUSB.cpp
#include "cv_camera/BosonCapture.h"
using namespace cv;
using namespace std;

BosonCapture::BosonCapture()
    : is_open(false), my_thermal(Boson640), video_mode(RAW16), fd(-1) {
  sprintf(thermal_sensor_name, "Boson640");
}

BosonCapture::~BosonCapture() {
  if (fd < 0) return;
  if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
    perror(RED "VIDIOC_STREAMOFF" WHT);
  };
  close(fd);
}

void BosonCapture::open(int32_t id) {
  is_open = false;
  printf(WHT ">>> " YEL "%s" WHT " selected\n", thermal_sensor_name);

  // We open the Video Device
  sprintf(video, "/dev/video%d", id);
  printf(WHT ">>> " YEL "%s" WHT " selected\n", video);
  if ((fd = ::open(video, O_RDWR)) < 0) {
    perror(RED "Error : OPEN. Invalid Video Device" WHT "\n");
    return;
  }

  // Check VideoCapture mode is available
  if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
    perror(RED "ERROR : VIDIOC_QUERYCAP. Video Capture is not available" WHT
               "\n");
    return;
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, RED
            "The device does not handle single-planar video capture." WHT "\n");
    return;
  }

  struct v4l2_format format;
  if (video_mode == RAW16) {
    printf(WHT ">>> " YEL "16 bits " WHT "capture selected\n");

    // I am requiring thermal 16 bits mode
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_Y16;

    // Select the frame SIZE (will depend on the type of sensor)
    switch (my_thermal) {
      case Boson320:  // Boson320
        width = 320;
        height = 256;
        break;
      case Boson640:  // Boson640
        width = 640;
        height = 512;
        break;
      default:  // Boson320
        width = 320;
        height = 256;
        break;
    }
  } else {  // 8- bits is always 640x512 (even for a Boson 320)
    printf(WHT ">>> " YEL "8 bits " WHT "YUV selected\n");
    format.fmt.pix.pixelformat =
        V4L2_PIX_FMT_YVU420;  // thermal, works   LUMA, full Cr, full Cb
    width = 640;
    height = 512;
  }

  // Common varibles
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  format.fmt.pix.width = width;
  format.fmt.pix.height = height;

  // request desired FORMAT
  if (ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
    perror(RED "VIDIOC_S_FMT" WHT);
    return;
  }

  // we need to inform the device about buffers to use.
  // and we need to allocate them.
  // we’ll use a single buffer, and map our memory using mmap.
  // All this information is sent using the VIDIOC_REQBUFS call and a
  // v4l2_requestbuffers structure:
  struct v4l2_requestbuffers bufrequest;
  bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufrequest.memory = V4L2_MEMORY_MMAP;
  bufrequest.count = 1;  // we are asking for one buffer

  if (ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0) {
    perror(RED "VIDIOC_REQBUFS" WHT);
    return;
  }

  // Now that the device knows how to provide its data,
  // we need to ask it about the amount of memory it needs,
  // and allocate it. This information is retrieved using the VIDIOC_QUERYBUF
  // call, and its v4l2_buffer structure.

  memset(&bufferinfo, 0, sizeof(bufferinfo));

  bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufferinfo.memory = V4L2_MEMORY_MMAP;
  bufferinfo.index = 0;

  if (ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0) {
    perror(RED "VIDIOC_QUERYBUF" WHT);
    return;
  }

  // map fd+offset into a process location (kernel will decide due to our
  // NULL). lenght and properties are also passed
  printf(WHT ">>> Image width  =" YEL "%i" WHT "\n", width);
  printf(WHT ">>> Image height =" YEL "%i" WHT "\n", height);
  printf(WHT ">>> Buffer lenght=" YEL "%i" WHT "\n", bufferinfo.length);

  void *buffer_start = mmap(NULL, bufferinfo.length, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, bufferinfo.m.offset);

  if (buffer_start == MAP_FAILED) {
    perror(RED "mmap" WHT);
    return;
  }

  // Fill this buffer with ceros. Initialization. Optional but nice to do
  memset(buffer_start, 0, bufferinfo.length);

  // Activate streaming
  type = bufferinfo.type;
  if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
    perror(RED "VIDIOC_STREAMON" WHT);
    return;
  }

  // Declarations for RAW16 representation
  // Will be used in case we are reading RAW16 format
  // Boson320 , Boson 640
  // OpenCV input buffer  : Asking for all info: two bytes per pixel (RAW16)
  // RAW16 mode
  thermal16 = cv::Mat(height, width, CV_16U, buffer_start);
  luma_height = height + height / 2;
  luma_width = width;
  color_space = CV_8UC1;
  thermal_luma = cv::Mat(luma_height, luma_width, color_space,
                         buffer_start);  // OpenCV input buffer
  thermal_rgb = cv::Mat(height, width, CV_8UC3, 1);
  is_open = true;
}

bool BosonCapture::read(cv::Mat &im) {
  // Put the buffer in the incoming queue.
  if (ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0) {
    perror(RED "VIDIOC_QBUF" WHT);
    return false;
  }

  // The buffer's waiting in the outgoing queue.
  if (ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0) {
    perror(RED "VIDIOC_QBUF" WHT);
    return false;
  }

  // -----------------------------
  // RAW16 DATA
  if (video_mode == RAW16) {
    im = thermal16;
  }
  // ---------------------------------
  // DATA in YUV
  else {  // Video is in 8 bits YUV
    cvtColor(thermal_luma, thermal_rgb, COLOR_YUV2RGB_I420,
             0);  // 4:2:0 family instead of 4:2:2 ...
    cvtColor(thermal_rgb, im, COLOR_RGB2GRAY, 0);
  }
  return true;
}