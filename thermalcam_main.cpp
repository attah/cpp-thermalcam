#include "thermalcam.h"
#include <opencv2/highgui.hpp>

int main(int, char**)
{
  cv::VideoCapture captureDevice = find_camera();

  if(!captureDevice.isOpened())
  {
    std::cerr << "ERROR: Failed to open camera." << std::endl;
    return 1;
  }

  cv::namedWindow("ThermalCam");

  ImageCallback imageCallback = [](const cv::Mat& imageData)
                                {
                                  cv::imshow("ThermalCam", imageData);
                                  cv::waitKey(1);
                                  return true;
                                };

  return capture_loop(captureDevice, imageCallback);
}
