CXXFLAGS = -std=c++11 $(shell pkg-config --cflags opencv4)
LDFLAGS = $(shell pkg-config --libs opencv4)

all: thermalcam

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

thermalcam: thermalcam.o
	$(CXX) $^ $(LDFLAGS) -o $@
