1.�˰汾��D:\P4\main\Zigbee\stm32\stm32l1xx\decave_uwb_tdoa1_modify_v3.0�汾�ĸĽ��汾
2.�˰汾������豸��֡���ͷ���ʱ�����վ�޷��������⣬ԭ��Ϊ��RTLS_DEMO_MSG_TAG_POLL����֡û�п�����վ�Ľ��չ��ܵ��»�վ�յ��쳣֡������޷��������գ��쳣֡ʱ������վ���ռ��ɽ������
3.�˰汾�����Ѿ�����˹��̵ķֿ�
4.�˰汾ʹ���˿췢֡���һ֡���߼����֡������Ϣ�ķ�װ����Ч��������հ��ʣ�������һ���̶��ϻ�����ʱ������ʱ���������²��Եľ��Ƚ���

��վ�����⿨��ѧϰ���ı��뷽ʽ��

����һ��Ŀ¼Application���и�instance.h�ļ����򿪺�ʼ���������£�
#define   TDOA_INST_MODE_ANCHOR         //��վ
#define   TDOA_INST_MODE_TAG_TEST       //���⿨
#define   TDOA_INST_MODE_TAG_STANDARD   //ѧϰ��

�������վ���룬���޸�����
#define   TDOA_INST_MODE_ANCHOR           //��վ
//#define   TDOA_INST_MODE_TAG_TEST       //���⿨
//#define   TDOA_INST_MODE_TAG_STANDARD   //ѧϰ��

��������⿨���룬���޸�����
//#define   TDOA_INST_MODE_ANCHOR         //��վ
#define   TDOA_INST_MODE_TAG_TEST         //���⿨
//#define   TDOA_INST_MODE_TAG_STANDARD   //ѧϰ��

������ѧϰ�����룬���޸�����
//#define   TDOA_INST_MODE_ANCHOR         //��վ
//#define   TDOA_INST_MODE_TAG_TEST       //���⿨
#define   TDOA_INST_MODE_TAG_STANDARD     //ѧϰ��


/*********************�豸��������*****************************/
#define DEV_TAG_ANCHOR_ADDRESS          30003    //��վID ����ָ��Ҳ���Բ�ָ��ֱ��д���ڴ���
#define DEV_TAG_TEST_ADDRESS            10009    //���⿨ID   10009
#define DEV_TAG_STANDARD_ADDRESS        10007    //�췢��ID
#define SLOW_SPEED_TAG_SEND_TIME  	1000     //���⿨�������ڼ����⿨��Ϣ�������� 400
#define QUICK_SPEED_TAG_SEND_TIME   	150      //���⿨�������ڼ����⿨��Ϣ�������� 45
#define DEVICE_UART_SEND_TIME   	1000     //���ڻ�������Ϣ�������� 1000 ���ܴ��ڿ�����ѯ����

