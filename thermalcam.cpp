#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdint>
#include <algorithm>

inline double round2(double d)
{
  return round(d*100)/100;
}

inline double get_temp(int16_t pixel)
{
  return round2((pixel / 64) - 273.15);
}

int main(int, char**)
{
  int w, h;
  int scale = 3;
  cv::ColormapTypes colorMap = cv::COLORMAP_JET;
  double min, max, center;
  double minVal, maxVal;
  cv::Point minPoint;
  cv::Point maxPoint;
  cv::Mat fullFrame;
  cv::Mat imageData;
  cv::Mat thermalData;

  cv::VideoCapture cap("/dev/video0", cv::CAP_V4L);

  if (!cap.isOpened()) {
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

    std::cout << "minMaxIdx: " << "min: " << min << " max: " << max << " center: " << center << std::endl;
    std::cout << "min pos: " << minPoint.x << "x" << minPoint.y
              << " max pos: " << maxPoint.x << "x" << maxPoint.y << std::endl;

    cv::resize(imageData, imageData, {w * scale, h * scale});

    cv::cvtColor(imageData, imageData, cv::COLOR_YUV2BGR_YUYV);
    cv::applyColorMap(imageData, imageData, colorMap);

    cv::imwrite("out.png", imageData);
    break;
  }

  return 0;
}
