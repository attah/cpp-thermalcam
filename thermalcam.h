#pragma once

#include <opencv2/opencv.hpp>

cv::VideoCapture find_camera();

bool do_capture(cv::VideoCapture captureDevice, cv::Mat& imageData, int wTarget, int hTarget);
