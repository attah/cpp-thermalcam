CXXFLAGS = -std=c++17 -O3 -pedantic -Wall -Wextra -Werror \
		$(shell pkg-config --cflags opencv4)
LDFLAGS = $(shell pkg-config --libs opencv4) \
	  $(shell pkg-config --libs libudev) \
	  $(shell pkg-config --libs libusb-1.0)

all: thermalcam

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

thermalcam: thermalcam.o thermalcam_main.o
	$(CXX) $^ $(LDFLAGS) -o $@
