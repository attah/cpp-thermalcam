#pragma once

#include <opencv2/opencv.hpp>


class ThermalCam
{
public:
  ThermalCam();
  ~ThermalCam();

  ThermalCam(const ThermalCam&) = delete;
  ThermalCam& operator=(const ThermalCam&) = delete;

  bool isOk();
  bool doCapture(cv::Mat& imageData, int wTarget, int hTarget);

private:
  void findCamera();

  struct udev* _udev;
  cv::VideoCapture _captureDevice;
};
