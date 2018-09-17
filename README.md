# MultiPersonCameraTrackingPOC
A POC for a Student-Project. Goal is to track football players with multiple cameras.  
Weights for yolo are not pushed. Just download them here: https://pjreddie.com/media/files/yolov3.weights and save them to the root-folder of the project.

Build with  
  cd build  
  cmake ..  
  make  

Run with  
  ./ObjectDetection -i=SOMEFILE -c=../resources/yolov3.cfg -m=../resources/yolov3.weights -classes=../resources/coco.names --scale=0.00392 --width=416 --height=416  
