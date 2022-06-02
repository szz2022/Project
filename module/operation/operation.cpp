#include "operation.h"
#include "../logger/logger.h"
#include "../socket/socket.h"

//定义全局变量

// 预处理执行器->业务逻辑 队列
ConcurrentQueue<pt_calibration> *calibrate_queue = new ConcurrentQueue<pt_calibration>();
ConcurrentQueue<pt_offset_table> *offset_table_queue = new ConcurrentQueue<pt_offset_table>();
ConcurrentQueue<pt_auto_detect> *auto_detect_queue = new ConcurrentQueue<pt_auto_detect>();

// 预处理执行器->神经网络 队列
ConcurrentQueue<pt_needle_data> *needle_queue = new ConcurrentQueue<pt_needle_data>();

// 神经网络->业务逻辑 队列
ConcurrentQueue<nn_needle_ans_data> *needle_ans_queue = new ConcurrentQueue<nn_needle_ans_data>();

// 相机->校验层 队列
ConcurrentQueue<cam_img_data> *camera_queue0 = new ConcurrentQueue<cam_img_data>();
ConcurrentQueue<cam_img_data> *camera_queue1 = new ConcurrentQueue<cam_img_data>();
ConcurrentQueue<cam_img_data> *camera_queue2 = new ConcurrentQueue<cam_img_data>();
ConcurrentQueue<cam_img_data> *camera_queue3 = new ConcurrentQueue<cam_img_data>();

// 串口->校验层 队列
ConcurrentQueue<machine_head_xy> *machine_head_xy_queue = new ConcurrentQueue<machine_head_xy>();

// 串口->预处理分配器 队列
ConcurrentQueue<lateral_y_pos> *lateral_y_pos_queue = new ConcurrentQueue<lateral_y_pos>();
ConcurrentQueue<shuttle_pos> *shuttle_pos_queue = new ConcurrentQueue<shuttle_pos>();

// 校验层->预处理执行器 队列
ConcurrentQueue<pt_data_pack> *ptreate_queue0 = new ConcurrentQueue<pt_data_pack>();
ConcurrentQueue<pt_data_pack> *ptreate_queue1 = new ConcurrentQueue<pt_data_pack>();
ConcurrentQueue<pt_data_pack> *ptreate_queue2 = new ConcurrentQueue<pt_data_pack>();

// 预处理队列vector
std::vector<ConcurrentQueue<pt_data_pack> *> pt_queue_set{ptreate_queue0, ptreate_queue1, ptreate_queue2};

ConcurrentQueue<EventNumber> *event_number_queue = new ConcurrentQueue<EventNumber>();

ConcurrentQueue<module_except_code> *except_code_queue = new ConcurrentQueue<module_except_code>();
ConcurrentQueue<pt_auto_detect> *pt_auto_detect_queue = new ConcurrentQueue<pt_auto_detect>();


StateLevel level = IDLE;
mutex mtx;
int sums = 0;
bool auto_dect_a = false;    //自动检测光路a标志
bool auto_dect_b = false;    //自动检测光路b标志
bool auto_dect_c = false;    //自动检测光路c标志
string auto_dect_err_name = ""; //记录自动检测出错光路
int visdect_sum = 0;        //visdect总异常计数  
float CONFIDENCE = 0.8;     //衣物检测置信度
std::string defect_image_path = "../../image/defect_image/";
std::string low_confidence_image_path = "../../image/low_confidence_image/";

StateLevel state_change(EventNumber event)
{
    switch (level)
    {
    case IDLE:
        if(event == E1) return IDLE;
        else if(event == E2) return AUTO_CALIBRATE;
        else if(event == E3) return VISDECT;
        else if(event == E8 || event == E14) return ERR_MODE;
        break;
    case AUTO_CALIBRATE:    
        if(event == E4) return GENTABLE;
        else if(event == E8 || event == E9 || event == E14) return ERR_MODE;
        break;  
    case GENTABLE:
        if(event == E5) return IDLE;
        else if(event == E2) return AUTO_CALIBRATE;
        else if(event == E8 || event == E10 || event == E14) return ERR_MODE;
        break;
    case VISDECT:
        if(event == E2) return AUTO_CALIBRATE;
        else if(event == E3) return VISDECT;
        else if(event == E6) return AUTO_DETECT;
        else if(event == E8 || event == E11 || event == E14) return ERR_MODE;
        break;
    case AUTO_DETECT:
        if(event == E7) return IDLE;
        else if(event == E2) return AUTO_CALIBRATE;
        else if(event == E8 || event == E12 || event == E14) return ERR_MODE;
        break;
    case ERR_MODE:
        if(event == E13) return IDLE; 
    default:
        break;
    }
    return level; // 没有合法跳转返回当前状态
}

//暴露给线程池调用
void handle()
{
    while (1)
    {
        // 每次进循环要判断下是否有状态跳转
        EventNumber op_event_number = E0;
        if(!event_number_queue->Empty())
        {
            event_number_queue->Pop(&op_event_number,false); // 非阻塞取事件
            StateLevel nextLevel = state_change(op_event_number);
            if(level != nextLevel) set_level(level, nextLevel);
        }

        if (level == AUTO_CALIBRATE)
        {
            struct pt_calibration cal;
            //取出数据
            calibrate_queue->Pop(&cal);
            std::cout << "auto_calibrate：" << cal.light_path << std::endl;
            //对cal中的数据进行逻辑判断
            if (cal.mid_line != -1 && cal.angle != -1)
            {  //正确条件为mid_line、cal.angel均不为-1

                //存储到offset_table数据结构并设置状态为GENTABLE(json操作待实现)
                // write_offset_table(&cal);
                // set_level(GENTABLE);
            }
            else
            { //统计失败次数，超过100设置状态为ERR_MODE
                // if (sums >= 100)
                //     set_level(ERR_MODE);
                // else
                //     sums++;
            }
        }
        else if (level == GENTABLE)
        {
            // TODO:
        }
        else if (level== VISDECT){
            //std::cout<<"进入VISDECT!"<<endl;
            if(!needle_ans_queue->Empty())
            {
                nn_needle_ans_data predict_data;
                //从队列取出预测结果
                needle_ans_queue->Pop(&predict_data, false);

                // std::cout<<"衣物当前行数："+std::to_string(predict_data.row)<<endl;
                bool compare_flag = true;   //比对模板标识
                int row_cnt = 0;            //一行内连续异常计数
                EventNumber event_number;
                    
                //取模板中的数组
                const rapidjson::Value& predict_val = templatejson::get_member("row"+std::to_string(predict_data.row), LIGHT_PATH_MAPPING[predict_data.light_path]);
                if(!predict_val.HasMember(NEEDLE_POS[predict_data.needle_position].c_str())){
                    continue;
                }

                const rapidjson::Value& predict_array = templatejson::get_member("row"+std::to_string(predict_data.row), \
                                    LIGHT_PATH_MAPPING[predict_data.light_path],NEEDLE_POS[predict_data.needle_position]);
                
                //std:array<int, NEEDLE_GRID_COUNT> predict_array = {0};
                //比较预测结果和模板结果
                for(unsigned i = predict_data.start;i<=predict_data.end;i++){
                    if(predict_array[i]!=predict_data.category[i]){
                        // std::cout<<"比对异常"+std::to_string(predict_data.row)<<endl;
                        if(compare_flag){
                            compare_flag = false;
                        }
                        
                        //一件织物总异常计数
                        visdect_sum++;

                        //一行内针格连续异常计数
                        row_cnt++;

                        LOG_PLUS("针格"+std::to_string(i)+"处比对失败", LOGGER_ERROR);
                        LOG_PLUS("当前织片异常数:"+std::to_string(visdect_sum), LOGGER_ERROR);

                        //保存缺陷图片
                        // int ftyp1 = access(defect_image_path.c_str(),0);
                        // if(ftyp1 != 0){ //路径不存在
                        //     util::create_dir(defect_image_path);
                        // }
                        // std::string defect_image = defect_image_path + std::to_string(predict_data.row)\
                        //     +"_"+LIGHT_PATH_MAPPING[predict_data.light_path]+"_"+NEEDLE_POS[predict_data.needle_position]+"_"+\
                        //     std::to_string(i)+".jpg";
                        // cv::imwrite(defect_image, predict_data.cvimage[i]);

                        //衣物质检异常 停止比对
                        if(row_cnt>=3 || visdect_sum>=5){
                            break;
                        }
                    }
                    else{
                        row_cnt = 0;
                    }
                    
                    //保存低置信度图片
                    // if(predict_data.confidence[i]<CONFIDENCE){
                    //     int ftyp2 = access(low_confidence_image_path.c_str(),0);
                    //     if(ftyp2 != 0){ //路径不存在
                    //         util::create_dir(low_confidence_image_path);
                    //     }
                    //     std::string low_confidence_image = low_confidence_image_path + std::to_string(predict_data.row)\
                    //         +"_"+LIGHT_PATH_MAPPING[predict_data.light_path]+"_"+NEEDLE_POS[predict_data.needle_position]+"_"+\
                    //         std::to_string(i)+".jpg";
                    //     cv::imwrite(low_confidence_image, predict_data.cvimage[i]);
                    // }
                    
                }

                //与模板比对失败
                if(!compare_flag){ 
                    if(row_cnt>=3 || visdect_sum>=5){
                        //衣物质检异常
                        std::cout<<"衣物质检异常"<<endl;
                        //异常计数清零
                        visdect_sum = 0;
                    
                        //推送异常情况
                        event_number = E11;
                        event_number_queue->Push(event_number);
                        module_except_code ex_code = VISDECT_EXCEPTION;
                        except_code_queue->Push(ex_code);
                    }
                }        
            }
        }
        else if(level == ERR_MODE) //能否跳转到ERR_MODE
        {
            ///ERR_MODE如何知道自己已经修复好异常呢？
            if(!except_code_queue->Empty()){
                std::cout << "ERR_MODE" << std::endl;
                module_except_code ex_code = NORMAL_CODE;
                //非阻塞读取异常状态
                except_code_queue->Pop(&ex_code,false);
                if(ex_code == CAM_RETRY_FAILURE || ex_code == CAM_IMAGE_CALL_FAILURE || ex_code == CAM_LOST_FRAME)
                {
                    //下发指令到告警灯TODO
                }
                else if(ex_code == AUTO_DECT_ERROR)
                {
                    // 自动检测错误
                    string msg = "自动检测错误，错误的光路为"+auto_dect_err_name+"光路,转人工处理";
                    LOG_PLUS(msg,LOGGER_ERROR);
                }
                else if(ex_code == VISDECT_EXCEPTION){
                    //衣物检测异常 通知MCU重启织片

                }
            }
            
        }
        else if(level == AUTO_DETECT)//状态为自动检测并且衣物质检完成
        {
            // std::cout<<"进入AUTO_DETECT!"<<endl;
            if(!pt_auto_detect_queue->Empty())
                {
                    // 从队列中取数据
                    pt_auto_detect auto_dect;
                    pt_auto_detect_queue->Pop(&auto_dect,false); // 非阻塞取事件
                    switch (auto_dect.light_path)
                    {
                    case 'a':
                        if(auto_dect.pass)
                            auto_dect_a = true;
                        else
                        {
                            // 推送异常，人工处理
                            except_code_queue->Push(AUTO_DECT_ERROR);
                            event_number_queue->Push(E12);
                            auto_dect_err_name = "a";
                            // 重置标志位，等待下一次进入代码块
                            auto_dect_a = false;
                            auto_dect_b = false;
                            auto_dect_c = false;
                        }
                        break;
                    case 'b':
                        if(auto_dect.pass)
                            auto_dect_b = true;
                        else
                        {
                            // 推送异常，人工处理
                            except_code_queue->Push(AUTO_DECT_ERROR);
                            event_number_queue->Push(E12);
                            auto_dect_err_name = "b";
                            // 重置标志位，等待下一次进入代码块
                            auto_dect_a = false;
                            auto_dect_b = false;
                            auto_dect_c = false;
                        }
                        break; 
                    case 'c':
                        if(auto_dect.pass)
                            auto_dect_c = true;
                        else
                        {
                            // 推送异常，人工处理
                            except_code_queue->Push(AUTO_DECT_ERROR);
                            event_number_queue->Push(E12);
                            auto_dect_err_name = "c";
                            // 重置标志位，等待下一次进入代码块
                            auto_dect_a = false;
                            auto_dect_b = false;
                            auto_dect_c = false;
                        }
                        break;
                    default:
                        break;
                    }
                }

            // 一组光路abc均ture,质检完成 
            if(auto_dect_a && auto_dect_b && auto_dect_c)
            {
                LOG_PLUS("成功",LOGGER_INFO);
                // 自动监测成功
                event_number_queue->Push(E7);
                // 重置标志位
                auto_dect_a = false;
                auto_dect_b = false;
                auto_dect_c = false;
            }
            else
            {
                // 数据未齐，等待
                LOG_PLUS("等待",LOGGER_INFO);
            }
        }
    }
}

//设置状态时需要上锁，防止并发修改出现问题
void set_level(StateLevel prev_level, StateLevel next_level)
{
    mtx.lock();
    level = next_level;
    mtx.unlock();
    // write your code here
    //VISDECT清空队列
    if(prev_level == VISDECT && next_level == AUTO_DETECT){
        //重置总异常计数器
        visdect_sum = 0;

        //清空队列
        needle_ans_queue->Clear();
    }
    // autodect清空队列
    if(prev_level == AUTO_DETECT && next_level == IDLE)
    {
        pt_auto_detect pad;
        while (!pt_auto_detect_queue->Empty())
        {
            pt_auto_detect_queue->Pop(&pad,false);
        }
    }
}
