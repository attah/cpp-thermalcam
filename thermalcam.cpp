#include "thermalcam.h"
#include <fstream>
#include <sstream>
#include <set>
#include <cstdint>
#include <libudev.h>

#define WHITE {0xff, 0xff, 0xff}
#define BLACK {0x00, 0x00, 0x00}

enum LabelMarker
{
  NoMarker,
  Dot,
  Crosshair
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

cv::VideoCapture find_camera()
{
  cv::VideoCapture cap;
  static std::set<std::string> supportedCameras({"5830", "5840"});
  struct udev* udev = udev_new();
  struct udev_enumerate* e = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(e, "video4linux");
  udev_enumerate_scan_devices(e);

  struct udev_list_entry* entry;
  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
  {
    struct udev_device* dev = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
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
      cap = cv::VideoCapture(devnode, cv::CAP_V4L2);
      if(cap.isOpened())
      {
        cap.set(cv::CAP_PROP_CONVERT_RGB, false);
        break;
      }
    }
  }

  udev_enumerate_unref(e);
  udev_unref(udev);

  return cap;
}

bool do_capture(cv::VideoCapture captureDevice, cv::Mat& imageData, int wTarget, int hTarget)
{
  cv::Mat fullFrame;

  if(!captureDevice.read(fullFrame))
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
