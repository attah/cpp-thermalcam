#pragma once

#include <opencv2/opencv.hpp>
#include <cstdint>
#include <functional>

typedef std::function<bool(const cv::Mat&)> ImageCallback;

cv::VideoCapture find_camera();

int capture_loop(cv::VideoCapture captureDevice, ImageCallback imageCallback);
