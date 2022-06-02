#include <iostream>
// #include "module/camera/hikcam.h"
#include "module/cserial/cserial.h"
#include "module/logger/logger.h"
#include "module/camera/camerafactory.h"
#include "ThreadPool.h"
#include "module/validity/validity.h"
#include "module/msgqueue/mqtest.h"
#include "module/operation/operation.h"
#include "module/socket/socket.h"
#include "module/camera/dhcam.h"
#include "module/optjson/config.h"
#include "module/optjson/offsettable.h"
#include "module/optjson/templatejson.h"
#include "module/ptreate/pt_allocator.h"
#include <sys/time.h>

using namespace std;


void testfunc(CSerial& cserial)
{
    sleep(10);
    cout << "send state frame" << endl;
    cserial.cwrite_mcu_state(0xffff); // 测试status
}

int main()
{    
    // time_t c1;
    // ThreadPool pool(3);
    // pool.init();
    // // 测试串口代码,没有连接设备不要执行
    // CSerial cserial;
    // pool.submit(testfunc, ref(cserial));
    // pool.submit(mem_fn(&CSerial::checkHeart), &cserial);
    // pool.submit(mem_fn(&CSerial::run), &cserial);
    // time_t c2;
    // cout << c1 << c2 << endl;
    // while(1);
   //启动时调用
    // Cserial cserial();
    // cserial.cwrite_OP10_Frame(2, 0x10, 0x12, 0x0f, 4);
    //初始化配置文件路径
    // config::init();
    // offsettable::init();
    // templatejson::init();

    // LOG("测试 new logger", LOGGER_ALARM);
    //  LOG("测试 new logger", LOGGER_INFO);
    //  LOG("测试 new logger", LOGGER_ERROR);
    //  LOG_SOCKET("test socket.log()", LOGGER_INFO);
    //  struct Calibration cal('h',0.01,1);
    //  level = AUTO_CALIBRATE;
    //  calibrate_queue->Push(cal);

    // std::cout<<"开始测试VISDECT！"<<endl;
    // templatejson::init();
    // struct nn_needle_ans_data needle_ans_d;
    // event_number_queue->Push(E3);
    // for(int i = 1; i<1800;i++){
    //     needle_ans_d.row = i;
    //     needle_ans_d.light_path = 0;
    //     needle_ans_d.needle_position = 0;
    //     needle_ans_d.start = 0;
    //     needle_ans_d.end = 600;
    //     // needle_ans_d.category = {0,1,0,0,1,1,0,0,1,1,0,0,0};//比对异常
    //     needle_ans_d.category = {0}; //比对成功
    //     needle_ans_queue->Push(needle_ans_d);
    // }
    
    // ThreadPool::get_instance(3).init();
    // ThreadPool::get_instance(3).submit(handle);
    // ThreadPool pool(3);
    // pool.init();
    // pool.submit(handle);
    // while (1);
    // std::cout << "except_code_queue address：" << &except_code_queue << std::endl;
    // std::cout << "event_number_queue address：" << &event_number_queue << std::endl;
    
    // 创建相机
    Camera* camera0 = CameraFactory::getCamera(DAHENG_CAMERA,1);
    Camera* camera1 = CameraFactory::getCamera(DAHENG_CAMERA,2);
    Camera* camera2 = CameraFactory::getCamera(DAHENG_CAMERA,3);

    // //一定要先启动相机在开
    ThreadPool::get_instance(10).init();
    ThreadPool::get_instance(10).submit(handle);
    ThreadPool::get_instance(10).submit(validity::check_validity);

    CSerial cserial;
    ThreadPool::get_instance(10).submit(testfunc, ref(cserial));
    ThreadPool::get_instance(10).submit(mem_fn(&CSerial::checkHeart), &cserial);
    ThreadPool::get_instance(10).submit(mem_fn(&CSerial::run), &cserial);

    // 加载衣物模板
    templatejson::init();
    // 加载偏移表
    offsettable::init();
    
    // 启动预处理
    std::shared_ptr<PtAllocator> al = std::make_shared<PtAllocator>();
    std::thread th(&PtAllocator::run, al);
    
    // LOG("test hello", LOGGER_ALARM);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(true);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // result_of_autodect_queue->Push(false);
    // // StateLevel op =VISDECT;在operation中修改
    // event_number_queue->Push(E6);

    // ThreadPool pool(3);
    // pool.init();
    // pool.submit(handle);
    while (1);

    // Camera* camera1 = CameraFactory::getCamera(HAIKANG_CAMERA,1);
    // camera0->g_bIsGetImage = true;
    // camera1->g_bIsGetImage = true;

    // pool.submit(std::mem_fn(&Camera::work_thread),camera0,camera0);
    // pool.submit(std::mem_fn(&Camera::work_thread),camera1,camera1);

    // sleep(10);
    // std::cout << "----------------------------" << std::endl;
    // int num0,num1;
    // camera_queue0->Pop(&num0);
    // camera_queue1->Pop(&num1);
    // std::cout << "pop:" << num0 << std::endl;
    // std::cout << "pop:" << num1 << std::endl;

    // pool.shutdown();

    // struct timespec t;

    // clock_gettime(CLOCK_REALTIME, &t);

    // cout << t.tv_sec * 1000000000 + t.tv_nsec;
    return 0;
}
