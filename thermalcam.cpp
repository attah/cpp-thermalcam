#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <filesystem>

#define WHITE {0xff, 0xff, 0xff}
#define BLACK {0x00, 0x00, 0x00}

enum LabelMarker
{
  NoMarker,
  Dot,
  Crosshair
};

inline double round2(double d)
{
  return round(d*100)/100;
}

inline double get_temp(int16_t pixel)
{
  return round2((pixel / 64) - 273.15);
}

inline std::string fmt2(double d)
{
  std::string tmp = std::to_string(d);
  return tmp.substr(0, tmp.find('.') + 3);
}

inline cv::Point scale_point(cv::Point point, int scale)
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

int main(int, char**)
{
  int w, h;
  int scale = 3;
  double scale2 = scale*0.25;
  cv::ColormapTypes colorMap = cv::COLORMAP_JET;
  double min, max, center;
  double minVal, maxVal;
  cv::Point minPoint;
  cv::Point maxPoint;
  cv::Mat fullFrame;
  cv::Mat imageData;
  cv::Mat thermalData;
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
      break;
    }
  }

  if(!cap.isOpened())
  {
    std::cerr << "ERROR: Failed to open camera." << std::endl;
    return 1;
  }

  cap.set(cv::CAP_PROP_CONVERT_RGB, false);

  while(cap.read(fullFrame))
  {
    w = fullFrame.cols;
    h = fullFrame.rows / 2;

    imageData = fullFrame.rowRange(0, h);
    thermalData = cv::Mat(h, w, CV_16SC1, fullFrame.row(h).data);

    cv::minMaxLoc(thermalData, &minVal, &maxVal, &minPoint, &maxPoint);
    min = get_temp(minVal);
    max = get_temp(maxVal);
    center = get_temp(thermalData.at<int16_t>(w/2, h/2));

    std::cout << "min: " << min << " max: " << max << " center: " << center << std::endl;
    std::cout << "min pos: " << minPoint.x << "x" << minPoint.y
              << " max pos: " << maxPoint.x << "x" << maxPoint.y << std::endl;

    cv::resize(imageData, imageData, {w * scale, h * scale});
    cv::cvtColor(imageData, imageData, cv::COLOR_YUV2BGR_YUYV);
    cv::applyColorMap(imageData, imageData, colorMap);

    putLabel(imageData, fmt2(center), {imageData.cols/2, imageData.rows/2}, scale2, Crosshair);
    putLabel(imageData, fmt2(min), scale_point(minPoint, scale), scale2, Dot);
    putLabel(imageData, fmt2(max), scale_point(maxPoint, scale), scale2, Dot);

    cv::imwrite("out.png", imageData);
    break;
  }

  return 0;
}
