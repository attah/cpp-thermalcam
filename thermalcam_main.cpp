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
  cv::Mat imageData;

  while(do_capture(captureDevice, imageData, 640, 480))
  {
    cv::imshow("ThermalCam", imageData);
    cv::waitKey(1);
  }

}
