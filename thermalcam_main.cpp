#include "thermalcam.h"
#include <opencv2/highgui.hpp>

int main(int, char**)
{
  ThermalCam thermalCam;

  if(!thermalCam.isOk())
  {
    std::cerr << "ERROR: Failed to open camera." << std::endl;
    return 1;
  }

  cv::namedWindow("ThermalCam");
  cv::Mat imageData;

  while(thermalCam.doCapture(imageData, 640, 480))
  {
    cv::imshow("ThermalCam", imageData);
    cv::waitKey(1);
  }

}
