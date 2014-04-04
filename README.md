Vision code for team 3324, running on a Raspberry Pi using a Raspberry Pi camera and OpenCV. Building is pretty primitive -- run the commands

    g++ -c gpio.cpp
    g++ -lopencv_core -lopencv_highgui -lopencv_imgproc -lraspicam -lraspicam_cv rapsicam_cvwindow.cpp gpio.o

