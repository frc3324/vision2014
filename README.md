Vision code for team 3324, running on a Raspberry Pi using a Raspberry Pi camera and OpenCV. Building is pretty primitive -- run the commands

    g++ -c gpio.cpp
    g++ -lopencv_core -lopencv_highgui -lopencv_imgproc -lraspicam -lraspicam_cv rapsicam_cvwindow.cpp gpio.o

#Todos

1. Set up a makefile for building.
2. Split off the unsafe queue code into a separate file (see `gpio.cpp` and `gpio.h` as a guide). Refactor it to use templates so it can work with any type (not just the custom `pt` struct) and take a length in the constructor. Also consider switching it out for `std::queue`, since it's not clear that speed matters enough to warrant the unsafety.
3. Add more command line options for setting threshold values and other things.
