#include "cserial.h"
#include "../operation/operation.h"

CSerial::CSerial()
{
    h_serial = new serial::Serial(SERIAL_PORT_NAME, 115200, serial::Timeout::simpleTimeout(1000));
    h_serial->flush();
    h_serial->flushInput();
    h_serial->flushOutput();
}

bool CSerial::resetStatus()
{
    runFlag = false;
    sleep(2);
    h_serial->close();
    delete h_serial;
    cur_frame_type = 0;
    sn_set.clear();
    read_index = 0;
    write_index = 0;
    last_hb_sn = -1;
    last_data_sn = -1;
    last_frame_time = 0;
    temp_sn = -1;
    interval_queue.clear();
    h_serial = new serial::Serial(SERIAL_PORT_NAME, 115200, serial::Timeout::simpleTimeout(1000));
    sleep(2);
    runFlag = true;
    return h_serial->available();
}

bool CSerial::addAndJdugeInterval(time_t cur)
{
    while (!interval_queue.empty() && cur - interval_queue.front() > INTERVAL)
    {
        interval_queue.pop_front();
    }
    interval_queue.push_back(cur);
    return interval_queue.size() > MAX_LOSS_CNT;
}

void CSerial::checkHeart()
{
    sleep(5);
    if (last_frame_time == 0)
        last_frame_time = std::time(0);
    while (1)
    {
        if (time(0) - last_frame_time > MAX_AGE * 2) // 超过两个心跳包的时间没有收到数据包
        {
            if (!h_serial->available()) // 判断串口是否可用，不可用则删除再连接
            {
                // 重连三次
                bool flag = false; // 标志是否重连成功
                for (int i = 0; i < MAX_RECONNECT_CNT; i++)
                {
                    //如果不可用,则删除这个,再创建一个,此时更新last时间，再判断能否正常收到
                    if (resetStatus())
                    {
                        flag = true;
                        h_serial->flush();
                        h_serial->flushInput();
                        h_serial->flushOutput();
                        break;
                    }
                }
                if (!flag)
                {
                    // 告警 重连三次失败
                    LOG_SOCKET("串口重连三次失败", LOGGER_ALARM);
                    // 这时候应该推送个错误状态
                    except_code_queue->Push(SERIAL_CONNECT_FAIL);
                }
            }
            else
            {
                // 串口可用,但是还是收不到心跳包，那么应该报警
                LOG_SOCKET("串口可用，但无法收到心跳包", LOGGER_ALARM);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 休眠2s
        if (addAndJdugeInterval(time(0)))
        {
            LOG_SOCKET("串口" + std::to_string(INTERVAL) + "内丢帧大于" + std::to_string(MAX_LOSS_CNT) + "!", LOGGER_ALARM);
            if(resetStatus()) 
            {
                h_serial->flush();
                h_serial->flushInput();
                h_serial->flushOutput();
            } else 
            {
                LOG_SOCKET("serial loss frame to many, reconnect failed", LOGGER_ALARM);
            }
        }
    }
}

void CSerial::run()
{
    while (1)
    {
        if(!runFlag || level == ERR_MODE) continue;
        switch (cur_bytes_status)
        {
        case HEAD:
        {
            for(int i = 0; i < 2; i++)
            {
                if(i == 0) {
                    if (!(h_serial->read(read_buffer, 1) == 1 && read_buffer[0] == 0x7E)) 
                    {
                        i--;
                    } 
                } else 
                {
                    if (!(h_serial->read(read_buffer, 1) == 1 && read_buffer[0] == 0xA5)) 
                    {
                        i = -1;
                    } 
                }
            }
            read_index = 0;
            cur_bytes_status = SN;
            // LOG_PLUS("HEAD ---> 7E A5", LOGGER_INFO); // TODO: DELETE
            break;
        }

        case SN:
        {
            if (h_serial->read(read_buffer, 1) == 1)
            {
                CRC_buffer[read_index++] = read_buffer[0];
                temp_sn = read_buffer[0];
                cur_bytes_status = FT;
                // LOG_PLUS("SN ---> " + dec2hex((int)read_buffer[0]), LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case FT:
        {
            if (h_serial->read(read_buffer, 1) == 1)
            {
                // LOG_PLUS("FT ---> " + dec2hex((int)read_buffer[0]), LOGGER_INFO); // TODO: DELETE
                CRC_buffer[read_index++] = read_buffer[0];
                cur_frame_type = read_buffer[0];
                last_frame_time = std::time(0); // 收到消息，更新收到消息时间
                if (read_buffer[0] == H_FRAME || read_buffer[0] == HA_FRAME)
                { // 心跳帧或心跳回复帧

                    cur_bytes_status = CRC16;
                }
                else if (read_buffer[0] == D_FRAME)
                {
                    cur_bytes_status = DST;
                }
                else if (read_buffer[0] == DA_FRAME)
                {
                    //判断是否是数据确认帧
                    cur_bytes_status = CRC16;
                }
                else
                    cur_bytes_status = HEAD;
            }
            break;
        }

        case DST:
        {
            if (h_serial->read(read_buffer, 1) == 1)
            {
                CRC_buffer[read_index++] = read_buffer[0];
                cur_bytes_status = SRC;
                // LOG_PLUS("DST ---> " + dec2hex((int)read_buffer[0]), LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case SRC:
        {
            if (h_serial->read(read_buffer, 1) == 1)
            {
                CRC_buffer[read_index++] = read_buffer[0];
                cur_bytes_status = LEN;
                // LOG_PLUS("SRC ---> " + dec2hex((int)read_buffer[0]), LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case LEN:
        {
            if (h_serial->read(read_buffer, 1) == 1)
            {
                data_length = (int)read_buffer[0] - 3;
                CRC_buffer[read_index++] = read_buffer[0];
                cur_bytes_status = DATA;
                // LOG_PLUS("LEN ---> " + dec2hex((int)read_buffer[0]), LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case DATA:
        {
            if (h_serial->read(read_buffer, data_length) == data_length)
            {
                // string tempmsg; // TODO: DELETE
                for (int i = 0; i < data_length; i++)
                {
                    CRC_buffer[read_index++] = read_buffer[i];
                    // tempmsg = tempmsg + dec2hex((int)read_buffer[i]) + " "; // TODO: DELETE
                }
                cur_bytes_status = CRC16;
                // LOG_PLUS("DATA " + tempmsg, LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case CRC16:
        {
            if (h_serial->read(read_buffer, 2) == 2)
            {
                auto cal_crc = usMBCRC16(CRC_buffer, read_index);
                if ((static_cast<unsigned short>(read_buffer[0]) << 8 | static_cast<unsigned short>(read_buffer[1])) == cal_crc)
                    cur_bytes_status = TAIL;
                else
                    cur_bytes_status = HEAD;
                // LOG_PLUS("CRC ---> " + dec2hex((int)read_buffer[0]) + " " + dec2hex((int)read_buffer[1]), LOGGER_INFO); // TODO: DELETE
            }
            break;
        }

        case TAIL:
        {
            for(int i = 0; i < 2; i++)
            {
                if(i == 0) {
                    if (!(h_serial->read(read_buffer, 1) == 1 && read_buffer[0] == FRAME_TAIL_0)) 
                    {
                        i--;
                    } 
                } else 
                {
                    if (!(h_serial->read(read_buffer, 1) == 1 && read_buffer[0] == FRAME_TAIL_1)) 
                    {
                        i = -1;
                    } 
                }
            }
            if(cur_frame_type == H_FRAME)
            {
                // 判断是否心跳帧存在丢帧现象
                if (last_hb_sn != -1 && (last_hb_sn + 1) % MAX_SN_CNT != temp_sn && last_hb_sn != temp_sn)
                {
                    // 丢帧了, 丢的帧为[last_hb_sn + 1, read_buffer[0])
                    string ids;
                    time_t loss_time;
                    for (int fid = (last_hb_sn + 1) % MAX_SN_CNT; fid < temp_sn; fid++)
                    {
                        interval_queue.push_back(loss_time);
                        if (fid == last_hb_sn + 1)
                            ids += " " + std::to_string(fid);
                        else
                            ids += ", " + std::to_string(fid);
                    }
                    if (ids != "")
                        LOG_SOCKET("串口心跳帧 " + ids + " 丢失", LOGGER_ALARM);
                }
                last_hb_sn = temp_sn;
            } else if(cur_frame_type == D_FRAME)
            {
                if (last_data_sn != -1 && (last_data_sn + 1) % MAX_SN_CNT != temp_sn && last_data_sn != temp_sn) // 逻辑同心跳帧丢失
                {
                    string ids;
                    time_t loss_time;
                    for (int fid = (last_data_sn + 1) % MAX_SN_CNT; fid < temp_sn; fid++)
                    {
                        interval_queue.push_back(loss_time);
                        if (fid == last_data_sn + 1)
                            ids += " " + std::to_string(fid);
                        else
                            ids += ", " + std::to_string(fid);
                    }
                    if (ids != "")
                        LOG_SOCKET("串口数据帧 " + ids + " 丢失", LOGGER_ALARM);
                }
                last_data_sn = temp_sn;
            }
            // LOG_PLUS("TAIL ---> 7E 0D", LOGGER_INFO); // TODO: DELETE
                if (cur_frame_type == H_FRAME)
                { // 心跳帧直接响应
                    // ACK
                    heartFrameAck(CRC_buffer[0]);
                    LOG_PLUS("响应心跳帧" + std::to_string(CRC_buffer[0]), LOGGER_INFO); // TODO: 临时注释
                }
                else if (cur_frame_type == D_FRAME)
                { // 数据帧
                    // printf("COM[4] Get One Data, Frame Num[%d]\n", ++test_cnt);
                    LOG_PLUS("接收到数据帧" + std::to_string(CRC_buffer[0]) + " " + std::to_string(CRC_buffer[5]), LOGGER_INFO); // TODO: 临时注释
                    // 判断帧类型
                    // TODO: 接收到需要做哪些具体处理
                    switch (CRC_buffer[5])
                    {
                    case M2O_UD_STATUS: // 3.1 MCU状态汇报，
                    {
                        LOG_PLUS("数据帧status", LOGGER_INFO); // TODO: 临时注释
                        // CRC_buffer的组成参考接口规范-R1-20220427文档，分别是MDSN/PDSN(1)、FT(1)、DST(1)、SRC(1)、LEN(1)、DATA(字节不确定)
                        unsigned char cmd = CRC_buffer[6];
                        unsigned char type0 = CRC_buffer[7];
                        unsigned char type1 = CRC_buffer[8];
                        char b0b1 = type0 & 3; // 织片状态
                        char b2 = (type0 >> 2 & 1);
                        char b3 = (type0 >> 3 & 1);
                        if (b0b1 == 0) // 未有织片
                        {
                        }
                        else if (b0b1 == 0x01) // 织片开始
                        {
                            if (status_type_b0b1 == 0x00)
                                event_number_queue->Push(E2); // 触发E2
                            // cwrite_OP10_Frame();
                        }
                        else if (b0b1 == 0x02) // TODO: 织片完成
                        {
                        }
                        if (b2 == 0x01 && status_type_b2 == 0x00)
                        {
                            event_number_queue->Push(E3); // 触发E3
                        }
                        status_type_b0b1 = b0b1;
                        status_type_b2 = b2;
                        status_type_b3 = b3;
                        LOG_SOCKET("收到status数据帧", LOGGER_INFO);
                    }
                    break;
                    case M2O_UD_XY: // 3.2 MCU坐标信息汇报
                    {
                        // LOG_PLUS("数据帧xy", LOGGER_INFO); // TODO: 临时注释
                        //  CRC_buffer从第五个字节开始是CMD，对应文档中的DATA，第六个字节是接口规范-R1文档-需求3.2中的Flag

                        // for(int i = 0; i < read_index; i++)
                        // {
                        //     std::cout << hex << int(CRC_buffer[i]) << " ";
                        // }
                        // std::cout << std::endl;

                        unsigned char flag = CRC_buffer[6];
                        //帧号

                        int frameNo = (int)(CRC_buffer[7] << 24) | (int)(CRC_buffer[8] << 16) | (int)(CRC_buffer[9] << 8) | (int)CRC_buffer[10];
                        //方向1 or 0
                        short D = CRC_buffer[11];
                        //行号
                        short X = ((short)CRC_buffer[12] << 8) | (short)CRC_buffer[13];
                        //列号
                        short Y = ((short)CRC_buffer[14] << 8) | (short)CRC_buffer[15];
                        short cur_timestamp = (short)(CRC_buffer[16] << 8) | (short)CRC_buffer[17];

                        struct machine_head_xy msg(flag, frameNo, D, X, Y, cur_timestamp);
                        //往串口帧队列插入串口帧数据
                        // std::cout << "接收到xy数据帧" << " " << std::to_string(frameNo) << std::endl;
                        // unsigned char flag, unsigned int frame_num, unsigned char direction, short x, short y, short timestamp
                        std::cout << "接收到xy数据帧" << " " << std::to_string(flag) << " "
                            << D << " " << X << " " << Y << " " << std::to_string(frameNo) << " " << std::to_string(cur_timestamp) << std::endl;
                        machine_head_xy_queue->Push(msg);
                    }
                    break;
                    case UD_LATERAL_Y_POS: // 3.3 横移坐标汇报
                    {
                        LOG_PLUS("数据帧lateral_y_pos", LOGGER_INFO); // TODO: 临时注释
                        unsigned char flag = CRC_buffer[6];
                        if (flag == 0xBB)
                        {
                            // 错误
                            LOG_SOCKET("横移坐标帧出错", LOGGER_ALARM);
                        }
                        else
                        {
                            unsigned char D = CRC_buffer[7];                            // 方向
                            short Y = (short)CRC_buffer[8] << 8 | (short)CRC_buffer[9]; // 列号
                            short cur_timestamp = (short)CRC_buffer[10] | (short)CRC_buffer[11];
                            lateral_y_pos msg(CRC_buffer[5], flag, D, Y, cur_timestamp);
                            lateral_y_pos_queue->Push(msg);
                        }
                    }
                    break;
                    case UD_SHUTTLE_POS:
                    {
                        // TODO 沙嘴数据需要更新
                        LOG_PLUS("数据帧shuttle_pos", LOGGER_INFO); // TODO: 临时注释
                        // 两位一列，共八列
                        unsigned short colNo = (unsigned short)CRC_buffer[6] << 8 | (unsigned char)CRC_buffer[7];
                        unsigned char D = CRC_buffer[8];
                        unsigned short Y = (short)CRC_buffer[9] << 8 | (short)CRC_buffer[10];         // 列坐标
                        unsigned short cur_timestamp = (short)CRC_buffer[11] | (short)CRC_buffer[12]; // 时间戳
                        shuttle_pos msg(CRC_buffer[5], colNo, D, Y, Y, cur_timestamp);
                        shuttle_pos_queue->Push(msg);
                    }
                    break;
                    case M2O_UD_Fault:
                    {
                        LOG_PLUS("数据帧fault", LOGGER_INFO); // TODO: 临时注释
                        /* code */
                        //如果是3.5 Fault_REP 故障
                        unsigned char tempType = CRC_buffer[6];
                        unsigned char tempValue = CRC_buffer[7]; //有两种值，0表示告警，1表示恢复
                        if (tempValue)
                        {
                            //如果是恢复
                        }
                        else
                        {
                            //如果是告警，
                            if (tempType == 0x00)
                            {
                                //如果是手柄故障
                                LOG_SOCKET("手柄故障", LOGGER_ERROR);
                            }
                            else if (tempType == 0x01)
                            {
                                //如果是横移机构故障
                                LOG_SOCKET("横移机构故障", LOGGER_ERROR);
                            }
                            else if (tempType == 0x02 || tempType == 0x03)
                            {
                                //如果是牵拉梳上下传感器故障
                                LOG_SOCKET("牵拉梳上下传感器故障", LOGGER_ERROR);
                            }
                            else if (tempType == 0x04 || tempType == 0x05)
                            {
                                //如果是横移机构故障
                                LOG_SOCKET("横移机构故障", LOGGER_ERROR);
                            }
                            else if (tempType == 0x06 || tempType == 0x07)
                            {
                                //如果是前/后位置传感器故障
                                LOG_SOCKET("前/后位置传感器故障", LOGGER_ERROR);
                            }
                            else if (tempType >= 0x10 && tempType <= 0x17)
                            {
                                //如果是光源1~8接线故障
                                int idx = tempType % 0x10 + 1;
                                LOG_SOCKET("光源" + to_string(idx) + "接线故障", LOGGER_ERROR);
                            }
                            else if (tempType >= 0x20 && tempType <= 0x27)
                            {
                                //如果是光源1~8过流错误
                                int idx = tempType % 0x20 + 1;
                                LOG_SOCKET("光源" + to_string(idx) + "过流", LOGGER_ERROR);
                            }
                            else if (tempType >= 0x30 && tempType <= 0x37)
                            {
                                //如果是光源1~8过温错误
                                int idx = tempType % 0x30 + 1;
                                LOG_SOCKET("光源" + to_string(idx) + "过温", LOGGER_ERROR);
                            }
                            else if (tempType >= 0x50 && tempType <= 0x57)
                            {
                                //如果是梭子0-7故障
                                int idx = tempType % 0x50;
                                LOG_SOCKET("梭子" + to_string(idx) + "故障", LOGGER_ERROR);
                            }
                            else if (tempType == 0x60)
                            {
                                //马达故障
                                int idx = tempType % 0x60 + 1;
                                LOG_SOCKET("马达" + to_string(idx) + "故障", LOGGER_ERROR);
                            }
                            else if (tempType >= 0x70 && tempType <= 0x73)
                            {
                                //相机1~4故障
                                int idx = tempType % 0x10 + 1;
                                LOG_SOCKET("相机" + to_string(idx) + "故障", LOGGER_ERROR);
                            }
                            else
                            {
                                // Fault_REP 未知的故障
                                LOG_SOCKET("未知的故障", LOGGER_ERROR);
                            }
                            // TODO:切换预处理线程状态,更改为事件
                            //  set_level(ERR_MODE);
                        }
                    }
                    break;
                    case M2O_UD_ALM_REP:
                    {
                        LOG_PLUS("数据帧alm_rep", LOGGER_INFO); // TODO: 临时注释
                        // 如果是3.6 告警消息,可以考虑用set
                        alm_data cur_arm_data{CRC_buffer[6], CRC_buffer[7]}; //一个字节存告警类型，一个直接存值
                        //逻辑同3.5
                        if (cur_arm_data.value)
                        {
                        }
                        else
                        {
                            if (cur_arm_data.type == 0x00)
                            {
                                // 0x00：主板+12V电源电压范围告警
                                LOG_SOCKET("主板+12V电源电压范围告警", LOGGER_ALARM);
                            }
                            else if (cur_arm_data.type == 0x01)
                            {
                                // 0x01：相机电源电压范围告警
                                LOG_SOCKET("相机电源电压范围告警", LOGGER_ALARM);
                            }
                            else if (cur_arm_data.type == 0x02)
                            {
                                // 0x02：光源输入电源电压范围告警
                                LOG_SOCKET("光源输入电源电压范围告警", LOGGER_ALARM);
                            }
                            else if (cur_arm_data.type >= 0x10 && cur_arm_data.type <= 0x17)
                            {
                                // 0x10~0x17：光源1~8电流平坦度失衡告警
                                int idx = cur_arm_data.type % 0x10 + 1; //获取光源编号
                                LOG_SOCKET("光源" + to_string(idx) + "电流平坦度失衡告警", LOGGER_ALARM);
                            }
                            else if (cur_arm_data.type >= 0x20 && cur_arm_data.type <= 0x27)
                            {
                                // 0x20~0x27：光源1~8电压平坦度失衡告警
                                int idx = cur_arm_data.type % 0x10 + 1; //获取光源编号
                                LOG_SOCKET("光源1" + to_string(idx) + "电压平坦度失衡告警", LOGGER_ALARM);
                            }
                            else if (cur_arm_data.type == 0x30)
                            {
                                // 0x30：针织线断线告警
                                LOG_SOCKET("针织线断线告警", LOGGER_ALARM);
                            }
                            else
                            {
                                // Alarm_REP 未知类型的告警报告
                                LOG_SOCKET("未知类型的告警报告", LOGGER_ALARM);
                            }
                            // TODO: 切换预处理线程状态 更改为事件
                            //  set_level(ERR_MODE);
                        }
                    }
                    break;
                    case M2O_DU_SET1_New:
                    {
                        LOG_PLUS("数据帧set1", LOGGER_INFO); // TODO: 临时注释
                        //如果是3.7 机台设置信息
                        unsigned char tempType = CRC_buffer[6];
                        if (tempType == 0x00)
                        {
                            //设置BUZ开关
                        }
                        else if (tempType == 0x01)
                        {
                            //设置LED开关
                        }
                        else if (tempType == 0x10)
                        {
                            //设置光源亮度
                        }
                        else if (tempType == 0x11)
                        {
                            //设置光源点亮时长
                        }
                        else if (tempType == 0x20)
                        {
                            //设置拍照步长（正弦波1/4周期数）
                        }
                        else if (tempType == 0x30)
                        {
                            //设置免检开关
                        }
                        else
                        {
                            //未知设置信息
                        }
                    }
                    break;
                    case M2O_DU_CHECK_SET1:
                    {
                        LOG_PLUS("数据帧checkset1", LOGGER_INFO); // TODO: 临时注释
                        //如果是3.8 PC查询机台设置信息
                        unsigned char tempType = CRC_buffer[6];
                        if (tempType == 0x00)
                        {
                            //查找BUZ开关
                        }
                        else if (tempType == 0x01)
                        {
                            //查找LED开关
                        }
                        else if (tempType == 0x10)
                        {
                            //查找光源亮度
                        }
                        else if (tempType == 0x11)
                        {
                            //查找光源点亮时长
                        }
                        else if (tempType == 0x20)
                        {
                            //查找拍照步长（正弦波1/4周期数）
                        }
                        else if (tempType == 0x30)
                        {
                            //查找免检开关
                        }
                        else
                        {
                            //未知查找信息
                        }
                    }
                    break;
                    case M2O_DU_SET2_New:
                        LOG_PLUS("数据帧set2", LOGGER_INFO); // TODO: 临时注释
                        //如果是3.9 扩展指令
                        break;
                    case M2O_DU_CHECK_SET2:
                        LOG_PLUS("数据帧check_set2", LOGGER_INFO); // TODO: 临时注释
                        //如果是3.10 设置扩展指令
                        break;
                    case M2O_UD_OP4:
                    {
                        LOG_PLUS("数据帧op4", LOGGER_INFO); // TODO: 临时注释
                        /* code */
                        //如果是3.11 MCU 丢包信息
                        unsigned char tempType = CRC_buffer[6];
                        int frameCount = (CRC_buffer[7] << 24) | (CRC_buffer[8] << 16) | (CRC_buffer[9] << 8) | CRC_buffer[10];
                        // 返回结果写入运行日志
                        if (tempType == 0x01)
                        {
                            //如果是 MCU 未收到的 DA 帧数
                            LOG_SOCKET("MCU 未收到的 DA 帧数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else if (tempType == 0x02)
                        {
                            //如果是 MCU 的 CRC16 失败次数
                            LOG_SOCKET("MCU 的 CRC16 失败次数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else if (tempType == 0x03)
                        {
                            //如果是 MCU 包头包尾校验失败次数
                            LOG_SOCKET("MCU 包头包尾校验失败次数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else if (tempType == 0x04)
                        {
                            //如果是 MCU 未收到的 HA 帧数
                            LOG_SOCKET("MCU 未收到的 HA 帧数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else if (tempType == 0x05)
                        {
                            //如果是 MCU 重发的 D 帧数
                            LOG_SOCKET("MCU 重发的 D 帧数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else if (tempType == 0x06)
                        {
                            //如果是 MCU 拍照总数
                            LOG_SOCKET("MCU 拍照总数", LOGGER_INFO);
                        }
                        else if (tempType >= 0x07)
                        {
                            // 如果是 MCU 发 XY 坐标总数
                            LOG_SOCKET("MCU 发 XY 坐标总数：" + to_string(frameCount), LOGGER_INFO);
                        }
                        else
                        {
                            // 未知的故障
                            LOG_SOCKET("未知的故障", LOGGER_ERROR);
                        }
                    }
                    break;
                    case M2O_UD_OP6:
                    {
                        LOG_PLUS("数据帧op6", LOGGER_INFO); // TODO: 临时注释
                        /* code */
                        //如果是3.12 PC 下发织片分类信息
                        unsigned char value = CRC_buffer[6];
                        // MCU 返回结果写入运行日志
                        if (value == 0)
                        {
                            LOG_SOCKET("PC 下发良品织片", LOGGER_INFO);
                        }
                        else if (value == 1)
                        {
                            LOG_SOCKET("PC 下发疵品织片", LOGGER_INFO);
                        }
                        else
                        {
                            // 未知的故障
                            LOG_SOCKET("未知的故障", LOGGER_ERROR);
                        }
                        // TODO: 分类失败则进入 Errmode 并告警，等待人工干预处理
                    }
                    break;
                    case M2O_UD_OP8:
                    {
                        LOG_PLUS("数据帧op8", LOGGER_INFO); // TODO: 临时注释
                        /* code */
                        //如果是3.13 PC 下发除尘信息
                        unsigned char value = CRC_buffer[6];
                        LOG_SOCKET("PC 下发除尘时常：" + to_string(value) + "s", LOGGER_INFO);
                    }
                    break;
                    default:
                        break;
                    }
                    // if (CRC_buffer[5] == M2O_UD_XY) {
                    //     uart_group_data src_uart{};
                    //     src_uart.flag = CRC_buffer[6];
                    //     src_uart.direction = CRC_buffer[11];
                    //     src_uart.row = (unsigned short)CRC_buffer[12] << 8 | (unsigned short)CRC_buffer[13];
                    //     src_uart.col = (unsigned short)CRC_buffer[14] << 8 | (unsigned short)CRC_buffer[15];
                    //     src_uart.frame_num = (unsigned int)CRC_buffer[7] << 24 | (unsigned int)CRC_buffer[8] << 16 | (unsigned int)CRC_buffer[9] << 8 | (unsigned int)CRC_buffer[10];
                    // }
                    // else if (CRC_buffer[5] == M2O_UD_STATUS) {
                    //     if (CRC_buffer[6] == M2O_UD_STATUS_NEW_PIECE) {
                    //         //printf("牵拉梳上升！\n");
                    //     }
                    //     else if (CRC_buffer[6] == M2O_UD_STATUS_MCU_ALIGN_OK){
                    //         //h_ptreate.gpm_cur_status = calibrate;
                    //     }
                    // }
                    // ACK
                    // cwrite(CRC_buffer[0], CRC_buffer[1], CRC_buffer[2], CRC_buffer[3], CRC_buffer + 4, read_index - 4);
                    // cwrite(CRC_buffer[0], CRC_buffer[1]); //这个地方发的是数据帧的ACK，但是上一行调用的write发送的是数据帧，所以做了修改。
                    dataFrameAck(CRC_buffer[0]);
                }
                else if (cur_frame_type == DA_FRAME)
                {
                    std::cout << "接收到da sn" << std::to_string(temp_sn) << std::endl;
                    //从集合中删掉帧号
                    sn_set.erase(temp_sn);
                }
            }
            cur_bytes_status = HEAD;
            // LOG_PLUS("TAIL ---> " + dec2hex((int)read_buffer[0]) + " " + dec2hex((int)read_buffer[1]), LOGGER_INFO); // TODO: DELETE
        break;
        default:
            break;
        }
    }
}

void CSerial::dataFrameAck(unsigned char sn)
{
    cwrite(sn, 0x03);
}

void CSerial::heartFrameAck(unsigned char sn)
{
    cwrite(sn, 0x02);
}

//写确认帧
void CSerial::cwrite(unsigned char sn, unsigned char ft)
{
    write_index = 2;

    write_buffer[write_index++] = sn;
    write_buffer[write_index++] = ft;
    auto crc_ans = usMBCRC16(write_buffer + 2, 2);
    write_buffer[write_index++] = crc_ans >> 8;
    write_buffer[write_index++] = crc_ans;
    write_buffer[write_index++] = FRAME_TAIL_0;
    write_buffer[write_index++] = FRAME_TAIL_1;

    h_serial->write(write_buffer, write_index);
}

//写数据帧
void CSerial::cwrite(unsigned char sn, unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
    write_index = 2;

    write_buffer[write_index++] = sn;
    write_buffer[write_index++] = ft;
    write_buffer[write_index++] = dst;
    write_buffer[write_index++] = src;

    unsigned char cur_len = length + 3;
    write_buffer[write_index++] = cur_len;

    for (int i = 0; i < length; i++)
        write_buffer[write_index++] = data[i];
    auto crc_ans = usMBCRC16(write_buffer + 2, write_index - 2);

    write_buffer[write_index++] = crc_ans >> 8;
    write_buffer[write_index++] = crc_ans;
    write_buffer[write_index++] = FRAME_TAIL_0;
    write_buffer[write_index++] = FRAME_TAIL_1;

    // for(int i = 0; i < write_index; i++)
    // {
    //     std::cout << hex << int(write_buffer[i]) << " ";
    // }
    // std::cout << std::endl;

    h_serial->write(write_buffer, write_index);
}

/**
 * sn：
 * ft：帧格式，0->表示H帧，心跳帧，MCU->PC；1->HA，心跳确认帧，PC->MCU；2->D，数据帧，双向，MCU和PC都可以发起；3->DA，数据确认帧
 * dst：PC -> 0x10  MCU -> 0x12
 * src和dst的值表示的含义相同
 *
 * @description：PC往MCU汇报机台开关传感器状态信息
 * **/

// *****************************************************************************************************************************************
void CSerial::cwrite_mcu_state(unsigned short type) //*
{
    unsigned char data[3]; // state的data长度为3字节
    data[0] = M2O_UD_STATUS;
    data[1] = type;
    data[2] = type >> 8;
    write_with_ack(MCU, PC, data, 3);
}
//*

void CSerial::cwrite_op14_frame()
{
    unsigned char data[5];
    data[0] = M20_UD_OP_14;
    write_with_ack(MCU, PC, data, 5);
}

void CSerial::write_with_ack(unsigned char dst, unsigned char src, unsigned char *data, unsigned char length) // 公共发送函数，之后全部调用这个
{
    unsigned char sn = cur_frame_sn.fetch_add(1, memory_order_seq_cst);
    sn_set.insert(sn);
    std::cout << "发送帧号为" << std::to_string(sn) << endl;
    bool flag = false;
    for (int i = 0; i < MAX_RESEND_CNT; i++)
    {
        if (flag)
            break;
        cwrite(sn, 0x01, dst, src, data, length);
        sleep(1);
        flag = sn_set.find(sn) == sn_set.end();
    }
    if (!flag)
    {
        //重传3次都没有收到确认帧
        sn_set.erase(temp_sn);
        LOG_SOCKET("发送数据帧" + std::to_string(sn) + "失败", LOGGER_ALARM);
    }
}
//*
// *****************************************************************************************************************************************

void CSerial::cwrite_Fault_REP_Frame(unsigned char type, unsigned char value)
{
    unsigned char data[3]; // fault的data长度为3字节
    data[0] = M2O_UD_Fault;
    data[1] = type;
    data[2] = value;
    write_with_ack(MCU, PC, data, 3);
}

void CSerial::cwrite_Alarm_REP_Frame(unsigned char type, unsigned char value)
{
    unsigned char data[3]; // alarm的data长度为3字节
    data[0] = M2O_UD_ALM_REP;
    data[1] = type;
    data[2] = value;
    write_with_ack(MCU, PC, data, 3);
}

void CSerial::cwrite_Check_SET1_Frame(unsigned char type, unsigned char value)
{
    unsigned char data[3]; // alarm的data长度为3字节
    data[0] = M2O_DU_CHECK_SET1;
    data[1] = type;
    data[2] = value;
    write_with_ack(MCU, PC, data, 3);
}

unsigned short CSerial::usMBCRC16(unsigned char *pucFrame, unsigned short usLen)
{
    unsigned char ucCRCHi = 0xFF;
    unsigned char ucCRCLo = 0xFF;
    unsigned short iread_index;
    while (usLen--)
    {
        iread_index = ucCRCLo ^ *(pucFrame++);
        ucCRCLo = (unsigned char)(ucCRCHi ^ aucCRCHi[iread_index]);
        ucCRCHi = aucCRCLo[iread_index];
    }
    return (unsigned short)(ucCRCHi << 8 | ucCRCLo);
}

void CSerial::enumerate_ports()
{
    std::vector<serial::PortInfo> devices_found = serial::list_ports();
    std::vector<serial::PortInfo>::iterator iter = devices_found.begin();
    while (iter != devices_found.end())
    {
        serial::PortInfo device = *iter++;
        printf("(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(), device.hardware_id.c_str());
    }
}

void CSerial::cwrite_OP4_Frame(unsigned char type, unsigned char *value)
{
    unsigned char data[6];
    data[0] = M2O_UD_OP4;
    data[1] = type;
    for (int i = 0; i < 4; i++)
    {
        data[i + 2] = value[i];
    }
    write_with_ack(MCU, PC, data, 6);
}

void CSerial::cwrite_OP6_Frame(unsigned char value)
{
    unsigned char data[2];
    data[0] = M2O_UD_OP6;
    data[1] = value;
    write_with_ack(MCU, PC, data, 2);
}

void CSerial::cwrite_OP8_Frame(unsigned char value)
{
    unsigned char data[2];
    data[0] = M2O_UD_OP8;
    data[1] = value;
    write_with_ack(MCU, PC, data, 2);
}

void CSerial::cwrite_OP13_Frame(unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
    int sn = cur_frame_sn.fetch_add(1, memory_order_seq_cst);
    sn_set.insert(sn);
    bool flag = false;
    for (int i = 0; i < 3; i++)
    {
        if (flag)
            break;
        cwrite(sn, 0x01, dst, src, data, length);
        usleep(1000);
        flag = sn_set.find(sn) == sn_set.end();
    }
    if (flag)
    {
        //重传3次都没有收到确认帧
        sn_set.erase(temp_sn);
        LOG_SOCKET("发送数据帧" + std::to_string(sn) + "失败", LOGGER_ALARM);
    }
}

void CSerial::cwrite_OP10_Frame(unsigned char sn, unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
}

void CSerial::cwrite_OP11_Frame(unsigned char sn, unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
}

void CSerial::cwrite_OP12_Frame(unsigned char sn, unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
}
int CSerial::get_cur_frame_sn()
{
    return cur_frame_sn;
}

void CSerial::cwrite_OP10_Frame(unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
    // 取sn=SN
    _mutex.lock();
    int cur_sn_local = cur_frame_sn++;
    _mutex.unlock();
    cwrite(cur_sn_local, ft, dst, src, data, length);
}
void CSerial::cwrite_OP11_Frame(unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
    // 取sn=SN
    _mutex.lock();
    int cur_sn_local = cur_frame_sn++;
    _mutex.unlock();
    cwrite(cur_sn_local, ft, dst, src, data, length);
}
void CSerial::cwrite_OP12_Frame(unsigned char ft, unsigned char dst, unsigned char src, unsigned char *data, unsigned char length)
{
    // 取sn=SN
    _mutex.lock();
    int cur_sn_local = cur_frame_sn++;
    _mutex.unlock();
    cwrite(cur_sn_local, ft, dst, src, data, length);
}