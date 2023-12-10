#include "thermalcam.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdint>

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
  return (pixel / 64) - 273.15;
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
  cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, scale, 2, &baseLine);
  cv::Point point = {point0.x - ((point0.x > xMid) ? textSize.width + xSpacing : - xSpacing),
                     point0.y + ((point0.y < yMid) ? textSize.height + ySpacing : - ySpacing)};

  if(type == Dot)
  {
    cv::circle(img, point0, 1, BLACK, 2, cv::LINE_AA);
    cv::circle(img, point0, 1, WHITE, 1, cv::LINE_AA);
  }
  else if(type == Crosshair)
  {

    cv::line(img, {xMid, yMid - 10}, {xMid, yMid + 10}, BLACK, 2, cv::LINE_AA);
    cv::line(img, {xMid - 10, yMid}, {xMid + 10, yMid}, BLACK, 2, cv::LINE_AA);
    cv::line(img, {xMid, yMid - 10}, {xMid, yMid + 10}, WHITE, 1);
    cv::line(img, {xMid - 10, yMid}, {xMid + 10, yMid}, WHITE, 1);
  }

  cv::putText(img, text, point, cv::FONT_HERSHEY_SIMPLEX, scale, BLACK, 2, cv::LINE_AA);
  cv::putText(img, text, point, cv::FONT_HERSHEY_SIMPLEX, scale, WHITE, 1, cv::LINE_AA);
}

cv::VideoCapture find_camera()
{
  cv::VideoCapture cap;

  for(std::filesystem::directory_entry const& dir :
      std::filesystem::directory_iterator("/sys/class/video4linux"))
  {
    std::string dirname = dir.path().filename();
    if(dirname.substr(0, 5) != "video")
    {
      continue;
    }

    std::ifstream file(dir.path() / "device" / "uevent");
    if(file.is_open())
    {
      std::string line;
      while(std::getline(file, line))
      {
        if(line.substr(0, 16) == "PRODUCT=bda/5830")
        {
          // std::cout << "Trying " << "/dev/" + dirname << std::endl;
          cap = cv::VideoCapture("/dev/" + dirname, cv::CAP_V4L2);
          break;
        }
      }
    }

    if(cap.isOpened())
    {
      cap.set(cv::CAP_PROP_CONVERT_RGB, false);
      break;
    }
  }

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
  double center = get_temp(thermalData.at<int16_t>(w/2, h/2));

  cv::resize(imageData, imageData, {(int)std::round(w * scale), (int)std::round(h * scale)});
  cv::cvtColor(imageData, imageData, cv::COLOR_YUV2BGR_YUYV);
  cv::applyColorMap(imageData, imageData, cv::COLORMAP_JET);

  putLabel(imageData, fmt2(center), {imageData.cols/2, imageData.rows/2}, scale/4, Crosshair);
  putLabel(imageData, fmt2(min), scale_point(minPoint, scale), scale/4, Dot);
  putLabel(imageData, fmt2(max), scale_point(maxPoint, scale), scale/4, Dot);
  return true;
}
