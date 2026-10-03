#include "thermalcam.h"
#include <fstream>
#include <sstream>
#include <set>
#include <cstdint>
#include <libudev.h>

#define WHITE {0xff, 0xff, 0xff}
#define BLACK {0x00, 0x00, 0x00}
#define B1(B) (uint8_t)(B & 0xff)
#define B2(B) (uint8_t)((B >> 8) & 0xff)
#define B3(B) (uint8_t)((B >> 16) & 0xff)
#define B4(B) (uint8_t)((B >> 24) & 0xff)

enum LabelMarker
{
  NoMarker,
  Dot,
  Crosshair
};

#define TPD_PARAMS 0x8514

#define GET 0x0000
#define SET 0x4000

enum TpdParams
{
  Distance = 0, // 1/163.835 m, 0-32767, Distance
  TU,           // 1 K, 0-1024, Reflection temperature
  TA,           // 1 K, 0-1024, Atmospheric temperature
  EMS,          // 1/127, 0-127, Emissivity
  TAU,          // 1/127, 0-127, Atmospheric transmittance
  GainSel       // binary, 0-1, Gain select (0=low, 1=high)
};

inline double get_temp(int16_t pixel)
{
  return (pixel / 64.0) - 273.15;
}

inline std::string fmt2(double d)
{
  std::stringstream ss;
  ss.imbue(std::locale("C"));
  ss.setf(std::ios::fixed);
  ss.precision(2);
  ss << d;
  return ss.str();
}

inline cv::Point scale_point(cv::Point point, double scale)
{
  point.x *= scale;
  point.y *= scale;
  return point;
}

void putLabel(cv::InputOutputArray img, const std::string& text, cv::Point point0, double scale, LabelMarker type)
{
  int xMid = img.cols()/2;
  int yMid = img.rows()/2;
  int xSpacing = 2;
  int ySpacing = 4;
  int baseLine;
  cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, scale/4, 2, &baseLine);
  cv::Point point = {point0.x - ((point0.x > xMid) ? textSize.width + xSpacing : - xSpacing),
                     point0.y + ((point0.y < yMid) ? textSize.height + ySpacing : - ySpacing)};

  if(type == Dot)
  {
    cv::circle(img, point0, std::trunc(scale)/2, BLACK, 2, cv::LINE_AA);
    cv::circle(img, point0, std::trunc(scale)/2, WHITE, 1, cv::LINE_AA);
  }
  else if(type == Crosshair)
  {
    int iscale = std::trunc(scale) * 2;
    cv::line(img, {point0.x, point0.y - iscale}, {point0.x, point0.y + iscale}, BLACK, 2, cv::LINE_AA);
    cv::line(img, {point0.x - iscale, point0.y}, {point0.x + iscale, point0.y}, BLACK, 2, cv::LINE_AA);
    cv::line(img, {point0.x, point0.y - iscale}, {point0.x, point0.y + iscale}, WHITE, 1);
    cv::line(img, {point0.x - iscale, point0.y}, {point0.x + iscale, point0.y}, WHITE, 1);
  }

  cv::putText(img, text, point, cv::FONT_HERSHEY_SIMPLEX, scale/4, BLACK, 2, cv::LINE_AA);
  cv::putText(img, text, point, cv::FONT_HERSHEY_SIMPLEX, scale/4, WHITE, 1, cv::LINE_AA);
}

void ThermalCam::findCamera()
{
  static std::set<std::string> supportedCameras({"5830", "5840"});
  struct udev_enumerate* e = udev_enumerate_new(_udev);
  udev_enumerate_add_match_subsystem(e, "video4linux");
  udev_enumerate_scan_devices(e);

  struct udev_list_entry* entry;
  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
  {
    struct udev_device* dev = udev_device_new_from_syspath(_udev, udev_list_entry_get_name(entry));
    struct udev_device* usb = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if(!usb)
    {
      udev_device_unref(dev);
      continue;
    }
    std::string idVendor = udev_device_get_sysattr_value(usb, "idVendor");
    std::string idProduct = udev_device_get_sysattr_value(usb, "idProduct");
    std::string devnode = udev_device_get_devnode(dev);

    udev_device_unref(dev);

    if((idVendor == "0bda") && supportedCameras.find(idProduct) != supportedCameras.end())
    {
      _captureDevice = cv::VideoCapture(devnode, cv::CAP_V4L2);
      if(_captureDevice.isOpened())
      {
        _captureDevice.set(cv::CAP_PROP_CONVERT_RGB, false);
        _usb_handle = libusb_open_device_with_vid_pid(_usb_context, 0x0bda, stoi(idProduct, 0, 16));
        break;
      }
    }
  }

  udev_enumerate_unref(e);
}

bool ThermalCam::doCapture(cv::Mat& imageData, int wTarget, int hTarget)
{
  cv::Mat fullFrame;

  if(!_captureDevice.read(fullFrame))
  {
    return false;
  }

  int w = fullFrame.cols;
  int h = fullFrame.rows / 2;
  double scale = std::min(wTarget/(double)w, hTarget/(double)h);

  imageData = fullFrame.rowRange(0, h);
  cv::Mat thermalData = cv::Mat(h, w, CV_16SC1, fullFrame.row(h).data);

  double minVal;
  double maxVal;
  cv::Point minPoint;
  cv::Point maxPoint;
  cv::minMaxLoc(thermalData, &minVal, &maxVal, &minPoint, &maxPoint);
  double min = get_temp(minVal);
  double max = get_temp(maxVal);
  double center = get_temp(thermalData.at<int16_t>(h/2, w/2));

  cv::resize(imageData, imageData, {(int)std::round(w * scale), (int)std::round(h * scale)});
  cv::cvtColor(imageData, imageData, cv::COLOR_YUV2BGR_YUYV);
  cv::applyColorMap(imageData, imageData, cv::COLORMAP_JET);

  putLabel(imageData, fmt2(center), {imageData.cols/2, imageData.rows/2}, scale, Crosshair);
  putLabel(imageData, fmt2(min), scale_point(minPoint, scale), scale, Dot);
  putLabel(imageData, fmt2(max), scale_point(maxPoint, scale), scale, Dot);
  return true;
}

ThermalCam::ThermalCam()
{
  _udev = udev_new();
  libusb_init(&_usb_context);
  findCamera();
}

ThermalCam::~ThermalCam()
{
  if(_usb_handle)
  {
    libusb_close(_usb_handle);
  }
  libusb_exit(_usb_context);
  udev_unref(_udev);
}

bool ThermalCam::isOk()
{
  return _captureDevice.isOpened();
}

void ThermalCam::setGain(uint16_t gain)
{
  if(!_usb_handle)
  {
    return;
  }
  longUsbCmdWrite(TPD_PARAMS | SET, GainSel, gain);
}

uint32_t ThermalCam::getGain()
{
  if(!_usb_handle)
  {
    return 1;
  }
  const int len = 2;
  uint8_t data[len];
  longUsbCmdRead(TPD_PARAMS | GET, GainSel, data, len);
  return data[0];
}

void ThermalCam::longUsbCmdWrite(uint16_t cmd, uint16_t prop, uint32_t v1, uint32_t v2, uint32_t v3)
{
  uint8_t data1[8] = {B1(cmd), B2(cmd), B2(prop), B1(prop), B4(v1), B3(v1), B2(v1), B1(v1)};
  uint8_t data2[8] = {B4(v2), B3(v2), B2(v2), B1(v2), B4(v3), B3(v3), B2(v3), B1(v3)};

  libusb_claim_interface(_usb_handle, 0x9d00 & 0xff);
  libusb_control_transfer(_usb_handle, 0x41, 0x45, 0x78, 0x9d00, data1, 8, 0);
  libusb_claim_interface(_usb_handle, 0x1d08 & 0xff);
  libusb_control_transfer(_usb_handle, 0x41, 0x45, 0x78, 0x1d08, data2, 8, 0);
  waitUsbReadyOrfailed();
}

bool ThermalCam::longUsbCmdRead(uint16_t cmd, uint16_t prop, uint8_t* data, int len)
{
  longUsbCmdWrite(cmd, prop, 0, 0, 2);
  libusb_claim_interface(_usb_handle, 0x1d10 & 0xff);
  int read = libusb_control_transfer(_usb_handle, 0xC1, 0x44, 0x78, 0x1d10, data, len, 0);
  return read == len;
}

void ThermalCam::waitUsbReadyOrfailed()
{
  uint8_t ret = 0;
  for(int i=0; i < 5; i++)
  {
    libusb_control_transfer(_usb_handle, 0x31, 0x44, 0x78, 0x200, &ret, 1, 1);
    if((ret & 0x03) == 0 || (ret & 0xfc) != 0)
    {
      break;
    }
  }
}
