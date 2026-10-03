#pragma once

#include <opencv2/opencv.hpp>
#include <libusb-1.0/libusb.h>

class ThermalCam
{
public:
  ThermalCam();
  ~ThermalCam();

  ThermalCam(const ThermalCam&) = delete;
  ThermalCam& operator=(const ThermalCam&) = delete;

  bool isOk();
  bool doCapture(cv::Mat& imageData, int wTarget, int hTarget);
  void setGain(uint16_t gain);
  uint32_t getGain();

private:
  void findCamera();
  void longUsbCmdWrite(uint16_t cmd, uint16_t prop, uint32_t v1, uint32_t v2=0, uint32_t v3=0);
  bool longUsbCmdRead(uint16_t cmd, uint16_t prop, uint8_t* data, int len);
  void waitUsbReadyOrfailed();

  struct udev* _udev;
  cv::VideoCapture _captureDevice;
  libusb_device_handle* _usb_handle = 0;
};
