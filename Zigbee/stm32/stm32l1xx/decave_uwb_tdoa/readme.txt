1.此版本是D:\P4\main\Zigbee\stm32\stm32l1xx\decave_uwb_tdoa1_modify_v3.0版本的改进版本
2.此版本解决多设备多帧类型发送时引起基站无法接收问题，原因为非RTLS_DEMO_MSG_TAG_POLL类型帧没有开启基站的接收功能导致基站收到异常帧后持续无法正常接收，异常帧时开启基站接收即可解决问题
3.此版本代码已经完成了工程的分库
4.此版本使用了快发帧间隔一帧或者间隔两帧进行消息的封装，有效的提高了收包率，但是在一定程度上会由于时间周期时间拉长导致测试的精度降低

基站、待测卡、学习卡的编译方式：

在上一个目录Application下有个instance.h文件，打开后开始部分有如下：
#define   TDOA_INST_MODE_ANCHOR         //基站
#define   TDOA_INST_MODE_TAG_TEST       //待测卡
#define   TDOA_INST_MODE_TAG_STANDARD   //学习卡

若编译基站代码，则修改如下
#define   TDOA_INST_MODE_ANCHOR           //基站
//#define   TDOA_INST_MODE_TAG_TEST       //待测卡
//#define   TDOA_INST_MODE_TAG_STANDARD   //学习卡

若编译待测卡代码，则修改如下
//#define   TDOA_INST_MODE_ANCHOR         //基站
#define   TDOA_INST_MODE_TAG_TEST         //待测卡
//#define   TDOA_INST_MODE_TAG_STANDARD   //学习卡

若编译学习卡代码，则修改如下
//#define   TDOA_INST_MODE_ANCHOR         //基站
//#define   TDOA_INST_MODE_TAG_TEST       //待测卡
#define   TDOA_INST_MODE_TAG_STANDARD     //学习卡


/*********************设备参数设置*****************************/
#define DEV_TAG_ANCHOR_ADDRESS          30003    //基站ID 可以指定也可以不指定直接写到内存中
#define DEV_TAG_TEST_ADDRESS            10009    //待测卡ID   10009
#define DEV_TAG_STANDARD_ADDRESS        10007    //快发卡ID
#define SLOW_SPEED_TAG_SEND_TIME  	1000     //待测卡触发周期即待测卡信息发送周期 400
#define QUICK_SPEED_TAG_SEND_TIME   	150      //待测卡触发周期即待测卡信息发送周期 45
#define DEVICE_UART_SEND_TIME   	1000     //串口缓冲区信息发送周期 1000 不能大于卡的轮询周期

