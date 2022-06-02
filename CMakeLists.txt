cmake_minimum_required(VERSION 3.16)
project(VIE)

set(CMAKE_CXX_STANDARD 14)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "Current platform: Linux ")
    set(Torch_DIR third_party/libtorch/share/cmake/Torch)
    set(OpenCV_INC third_party/OpenCV/Includes)
    set(OpenCV_LIB third_party/OpenCV/Libraries)
    set(HikCam_INC third_party/libcam/hikcam/Linux/Includes)
    set(HikCam_LIB third_party/libcam/hikcam/Linux/Libraries/64)
    set(DAHENG_INC third_party/galaxy/Linux/Includes)
    set(DAHENG_LIB third_party/galaxy/Linux/Libraries/64)
    set(SpdLog_INC third_party/libspdlog)
    set(RapidJson_INC third_party/librapidjson)
    set(SerialPort_INC third_party/libserial)
    set(ThreadPool_INC third_party/thread-pool/include)

ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    MESSAGE(STATUS "Current platform: Windows")
    set(Torch_DIR ./third_party/libtorch/share/cmake/Torch)
    set(OpenCV_DIR ./third_party/opencv/build/x64/vc16/lib)
    set(MVS_INC ./third_party/hikcam/Win/Includes)
    set(MVS_LIB ./third_party/hikcam/Win/Libraries/win64)
    set(SpdLog_INC ./third_party)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    MESSAGE(STATUS "Current platform: FreeBSD")
ELSE ()
    MESSAGE(STATUS "Other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

find_package(PythonInterp REQUIRED)
find_package(Torch REQUIRED)

message(STATUS "Torch library status:")
message(STATUS "    include path: ${TORCH_INCLUDE_DIRS}")
message(STATUS "    torch lib : ${TORCH_LIBRARIES} ")

include_directories(${HikCam_INC} ${DAHENG_INC} ${OpenCV_INC} ${TORCH_INCLUDE_DIRS} ${SpdLog_INC} ${RapidJson_INC} ${SerialPort_INC} ${ThreadPool_INC}) 

link_directories(${HikCam_LIB} ${DAHENG_LIB} ${OpenCV_LIB} ${TORCH_LIBRARIES} )

add_executable(VIE VIE.cpp module/camera/hikcam.cpp module/camera/hikcam.h
                module/camera/dhcam.cpp module/camera/dhcam.h
                module/camera/camera.cpp module/camera/camera.h module/camera/camerafactory.h
                module/cserial/cserial.cpp module/cserial/cserial.h
                module/common/header.h module/common/utils.cpp module/common/utils.h
                module/json/json.cpp module/json/json.h
                module/logger/logger.cpp module/logger/logger.h
                module/socket/socket.cpp module/socket/socket.h
                module/operation/operation.cpp module/operation/operation.h
                module/optjson/templatejson.cpp module/optjson/templatejson.h
                module/optjson/config.cpp module/optjson/config.h
                module/optjson/offsettable.cpp module/optjson/offsettable.h
                module/validity/validity.cpp module/validity/validity.h

                module/ptreate/pt_allocator.cpp module/ptreate/pt_allocator.h
                module/ptreate/pt_operator.cpp module/ptreate/pt_operator.h
                module/ptreate/pt_logic.cpp module/ptreate/pt_logic.h
                module/predict/nn.cpp module/predict/nn.h
                
                third_party/libserial/serial/Serial.h third_party/libserial/serial/Serial.cpp
                third_party/libserial/serial/Win/WinSerialBase.h third_party/libserial/serial/Win/WinSerialBase.cpp
                third_party/libserial/serial/Linux/LinuxSerialBase.h third_party/libserial/serial/Linux/LinuxSerialBase.cpp
                third_party/libserial/serial/Linux/LinuxListPort.cpp third_party/libserial/serial/Win/WinListPort.cpp)

target_link_libraries(VIE pthread opencv_highgui opencv_core opencv_imgcodecs opencv_imgproc opencv_cudawarping c10 cuda ${TORCH_LIBRARIES} libMvCameraControl.so libgxiapi.so)
