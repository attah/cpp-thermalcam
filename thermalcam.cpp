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

int main(int argc, char** argv)
{
  int w, h;
  int scale = 3;
  cv::ColormapTypes colorMap = cv::COLORMAP_JET;
  float center, min, max;
  ptrdiff_t minoffs, maxoffs;
  ptrdiff_t minx, miny, maxx, maxy;
  std::pair<cv::MatIterator_<int16_t>, cv::MatIterator_<int16_t>> minmax;
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
    thermalData = fullFrame.rowRange(h, h*2);

    center = get_temp(thermalData.at<int16_t>(w/2, h/2));

    minmax = std::minmax_element(thermalData.begin<int16_t>(), thermalData.end<int16_t>());
    min = get_temp(*minmax.first);
    max = get_temp(*minmax.second);

    minoffs = std::distance(thermalData.begin<int16_t>(), minmax.first);
    maxoffs = std::distance(thermalData.begin<int16_t>(), minmax.second);
    minx = minoffs % w;
    miny = minoffs / w;
    maxx = maxoffs % w;
    maxy = maxoffs / w;

    std::cout << "center: " << center << " min: " << min << " max: " << max << std::endl;
    std::cout << "min pos: " << minx << "x" << miny << " max pos: " << maxx << "x" << maxy << std::endl;

    double minval, maxval;
    cv::minMaxLoc(thermalData, &minval, &maxval);
    min = get_temp(minval);
    max = get_temp(maxval);

    std::cout << "minMaxIdx: " << "min: " << min << " max: " << max << std::endl;

    // cv::resize(imageData, imageData, {w * scale, h * scale});

    cv::cvtColor(imageData, imageData, cv::COLOR_YUV2BGR_YUYV);
    cv::applyColorMap(imageData, imageData, colorMap);

    cv::imwrite("out.png", imageData);
    break;
  }

  return 0;
}
