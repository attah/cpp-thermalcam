CXXFLAGS = -std=c++17 -O3 -pedantic -Wall -Wextra -Werror \
		$(shell pkg-config --cflags opencv4)
LDFLAGS = $(shell pkg-config --libs opencv4)

all: thermalcam

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

thermalcam: thermalcam.o
	$(CXX) $^ $(LDFLAGS) -o $@
