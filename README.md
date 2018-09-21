# MultiPersonCameraTrackingPOC
A POC for a Student-Project. Goal is to track football players with multiple cameras.  
Tested on Ubuntu with Opencv 3.4.1. How to install Opencv? Look down below ("Install Opencv with contrib")    
Weights for yolo are not pushed. Just download them here: https://pjreddie.com/media/files/yolov3.weights and save them to the folder "resources"

Build with  
  cd build  
  cmake ..  
  make  

Run with  
  ./MultiTracker -i=SOMEFILE -c=../resources/yolov3.cfg -m=../resources/yolov3.weights -classes=../resources/coco.names --scale=0.00392 --width=416 --height=416  

# Install Opencv with contrib
(No guarantees)  
git clone https://github.com/opencv/opencv.git  
git clone https://github.com/opencv/opencv_contrib.git  
cd opencv_contrib/  
git checkout 3.4  
cd ..  
cd opencv  
git checkout 3.4  
sudo apt-get install build-essential  
sudo apt-get install cmake git libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev  
sudo apt-get install python-dev python-numpy libtbb2 libtbb-dev libjpeg-dev libpng-dev libtiff-dev libjasper-dev libdc1394-22-dev  
mkdir build    
cd build/  
cmake -D CMAKE_BUILD_TYPE=Release -D ENABLE_CXX11=1 -D CMAKE_INSTALL_PREFIX=/usr/local -D OPENCV_EXTRA_MODULES_PATH=**PATHTOCONTRIB**/opencv_contrib/module ..  
make -j4  
sudo make install  
