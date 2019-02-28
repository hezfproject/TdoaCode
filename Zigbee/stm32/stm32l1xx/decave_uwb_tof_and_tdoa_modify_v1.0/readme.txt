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
