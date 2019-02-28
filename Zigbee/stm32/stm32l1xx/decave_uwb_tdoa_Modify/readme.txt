1.此版本是D:\P4\main\Zigbee\stm32\stm32l1xx\decave_uwb_tdoa1_modify_v3.0版本的改进版本
2.此版本解决多设备多帧类型发送时引起基站无法接收问题，原因为非RTLS_DEMO_MSG_TAG_POLL类型帧没有开启基站的接收功能导致基站收到异常帧后持续无法正常接收，异常帧时开启基站接收即可解决问题
3.根据设备进行分工程编译将在v4.0实现
4.此版本使用了快发帧间隔一帧或者间隔两帧进行消息的封装，有效的提高了收包率，但是在一定程度上会由于时间周期时间拉长导致测试的精度降低

注：主要宏在instance.h中定义
    作为基站时 DEC_UWB_ANCHOR 在insatance.h中定义
    作为人卡时 DEC_UWB_ANCHOR 在config.h中定义
    基站板模块作为学习卡时 DEC_UWB_ANCHOR 可以不定义
    人卡模块作为学习卡时 DEC_UWB_ANCHOR 在config.h中定义


卡，基站，辅站的编译方式：

在上一个目录common下有个config.h文件，打开后开始部分有如下：
#define   DEC_UWB_TAG
#define   DEC_UWB_SUB

#define   DEC_UWB_ANCHOR 

若编译主站代码，则修改如下
#define   DEC_UWB_TAG
//#define   DEC_UWB_SUB
//#define   DEC_UWB_ANCHOR 


若编译辅站代码，则修改如下
//#define   DEC_UWB_TAG
#define   DEC_UWB_SUB
//#define   DEC_UWB_ANCHOR 


若编译定位卡代码，则修改如下
//#define   DEC_UWB_TAG
//#define   DEC_UWB_SUB
#define   DEC_UWB_ANCHOR 