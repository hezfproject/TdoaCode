/********************ƴ�����뷨ģ��*******************
/                      ԭ��:�� ��
/                      ��д:�� ǿ(mail2li@21cn.com)
/                  ���뻷����Keil C 6.14
/                  Porting to IAR (zigbee TI/z-stack environment.) by jifeng.
*/
//#include<string.h>
//#include<stdio.h>
#include "OSAL.h"

typedef struct
{
    uint8 *PY;
    const __code uint8 *PY_mb;
} PY_index;

//"ƴ�����뷨�������б�,���(mb)"
const __code uint8 PY_mb_a[]     ={"����"};
const __code uint8 PY_mb_ai[]    ={"��������������������������"};
const __code uint8 PY_mb_an[]    ={"������������������"};
const __code uint8 PY_mb_ang[]   ={"������"};
const __code uint8 PY_mb_ao[]    ={"�������������°İ�"};
const __code uint8 PY_mb_ba[]    ={"�˰ͰȰǰɰṴ̋ưʰΰϰѰаӰְհ�"};
const __code uint8 PY_mb_bai[]   ={"�װٰ۰ذڰܰݰ�"};
const __code uint8 PY_mb_ban[]   ={"�����߰����������"};
const __code uint8 PY_mb_bang[]  ={"������������������"};
const __code uint8 PY_mb_bao[]   ={"������������������������������������"};
const __code uint8 PY_mb_bei[]   ={"������������������������������"};
const __code uint8 PY_mb_ben[]   ={"����������"};
const __code uint8 PY_mb_beng[]  ={"�����±ñű�"};
const __code uint8 PY_mb_bi[]    ={"�ƱǱȱ˱ʱɱұرϱձӱѱݱбֱԱͱױ̱αڱܱ�"};
const __code uint8 PY_mb_bian[]  ={"�߱�ޱ���������"};
const __code uint8 PY_mb_biao[]  ={"�����"};
const __code uint8 PY_mb_bie[]   ={"�����"};
const __code uint8 PY_mb_bin[]   ={"����������"};
const __code uint8 PY_mb_bing[]  ={"������������������"};
const __code uint8 PY_mb_bo[]    ={"����������������������������������������"};
const __code uint8 PY_mb_bu[]    ={"��������������������"};
const __code uint8 PY_mb_ca[]    ={"��"};
const __code uint8 PY_mb_cai[]   ={"�²ŲĲƲòɲʲǲȲ˲�"};
const __code uint8 PY_mb_can[]   ={"�βͲвϲѲҲ�"};
const __code uint8 PY_mb_cang[]  ={"�ֲײԲղ�"};
const __code uint8 PY_mb_cao[]   ={"�ٲڲܲ۲�"};
const __code uint8 PY_mb_ce[]    ={"���޲��"};
const __code uint8 PY_mb_ceng[]  ={"�����"};
const __code uint8 PY_mb_cha[]   ={"������������ɲ"};
const __code uint8 PY_mb_chai[]  ={"����"};
const __code uint8 PY_mb_chan[]  ={"�������������������"};
const __code uint8 PY_mb_chang[] ={"������������������������"};
const __code uint8 PY_mb_chao[]  ={"��������������������"};
const __code uint8 PY_mb_che[]   ={"������������"};
const __code uint8 PY_mb_chen[]  ={"�������������³��ĳ�"};
const __code uint8 PY_mb_cheng[] ={"�Ƴųɳʳгϳǳ˳ͳ̳γȳѳҳ�"};
const __code uint8 PY_mb_chi[]   ={"�Գճڳس۳ٳֳ߳޳ݳܳ����"};
const __code uint8 PY_mb_chong[] ={"������"};
const __code uint8 PY_mb_chou[]  ={"�������������"};
const __code uint8 PY_mb_chu[]   ={"����������������������������������"};
const __code uint8 PY_mb_chuai[] ={"��"};
const __code uint8 PY_mb_chuan[] ={"��������������"};
const __code uint8 PY_mb_chuang[]={"����������"};
const __code uint8 PY_mb_chui[]  ={"����������"};
const __code uint8 PY_mb_chun[]  ={"��������������"};
const __code uint8 PY_mb_chuo[]  ={"��"};
const __code uint8 PY_mb_ci[]    ={"�ôʴĴɴȴǴŴƴ˴δ̴�"};
const __code uint8 PY_mb_cong[]  ={"�ѴӴҴдϴ�"};
const __code uint8 PY_mb_cou[]   ={"��"};
const __code uint8 PY_mb_cu[]    ={"�ִٴ״�"};
const __code uint8 PY_mb_cuan[]  ={"�ڴܴ�"};
const __code uint8 PY_mb_cui[]   ={"�޴ߴݴ�����"};
const __code uint8 PY_mb_cun[]   ={"����"};
const __code uint8 PY_mb_cuo[]   ={"�������"};
const __code uint8 PY_mb_da[]    ={"�������"};
const __code uint8 PY_mb_dai[]   ={"������������������������"};
const __code uint8 PY_mb_dan[]   ={"������������������������������"};
const __code uint8 PY_mb_dang[]  ={"����������"};
const __code uint8 PY_mb_dao[]   ={"������������������������"};
const __code uint8 PY_mb_de[]    ={"�õµ�"};
const __code uint8 PY_mb_deng[]  ={"�Ƶǵŵȵ˵ʵ�"};
const __code uint8 PY_mb_di[]    ={"�͵̵εҵϵеӵѵյ׵ֵصܵ۵ݵڵ޵�"};
const __code uint8 PY_mb_dian[]  ={"���ߵ�������������"};
const __code uint8 PY_mb_diao[]  ={"�����������"};
const __code uint8 PY_mb_die[]   ={"��������������"};
const __code uint8 PY_mb_ding[]  ={"������������������"};
const __code uint8 PY_mb_diu[]   ={"��"};
const __code uint8 PY_mb_dong[]  ={"��������������������"};
const __code uint8 PY_mb_dou[]   ={"����������������"};
const __code uint8 PY_mb_du[]    ={"�����������¶ĶöʶŶǶȶɶ�"};
const __code uint8 PY_mb_duan[]  ={"�˶̶ζ϶ж�"};
const __code uint8 PY_mb_dui[]   ={"�ѶӶԶ�"};
const __code uint8 PY_mb_dun[]   ={"�ֶضն׶ܶ۶ٶ�"};
const __code uint8 PY_mb_duo[]   ={"��߶�޶��������"};
const __code uint8 PY_mb_e[]     ={"����������������"};
const __code uint8 PY_mb_en[]    ={"��"};
const __code uint8 PY_mb_er[]    ={"����������������"};
const __code uint8 PY_mb_fa[]    ={"����������������"};
const __code uint8 PY_mb_fan[]   ={"����������������������������������"};
const __code uint8 PY_mb_fang[]  ={"���������������·÷ķ�"};
const __code uint8 PY_mb_fei[]   ={"�ɷǷȷƷʷ˷̷ͷϷзη�"};
const __code uint8 PY_mb_fen[]   ={"�ַԷ׷ҷշӷطڷٷ۷ݷܷ޷߷�"};
const __code uint8 PY_mb_feng[]  ={"����������������"};
const __code uint8 PY_mb_fo[]    ={"��"};
const __code uint8 PY_mb_fou[]   ={"��"};
const __code uint8 PY_mb_fu[]    ={"������󸥷�����������������������������������������������������������������������������"};
const __code uint8 PY_mb_ga[]    ={"�¸�"};
const __code uint8 PY_mb_gai[]   ={"�øĸƸǸȸ�"};
const __code uint8 PY_mb_gan[]   ={"�ɸʸ˸θ̸͸ѸϸҸи�"};
const __code uint8 PY_mb_gang[]  ={"�Ըոڸٸظ׸ָ۸�"};
const __code uint8 PY_mb_gao[]   ={"�޸�߸�ݸ�����"};
const __code uint8 PY_mb_ge[]    ={"����������������������"};
const __code uint8 PY_mb_gei[]   ={"��"};
const __code uint8 PY_mb_gen[]   ={"����"};
const __code uint8 PY_mb_geng[]  ={"��������������"};
const __code uint8 PY_mb_gong[]  ={"������������������������������"};
const __code uint8 PY_mb_gou[]   ={"������������������"};
const __code uint8 PY_mb_gu[]    ={"�����ù¹��������ŹȹɹǹƹĹ̹ʹ˹�"};
const __code uint8 PY_mb_gua[]   ={"�Ϲιйѹҹ�"};
const __code uint8 PY_mb_guai[]  ={"�Թչ�"};
const __code uint8 PY_mb_guan[]  ={"�ع۹ٹڹ׹ݹܹ�߹��"};
const __code uint8 PY_mb_guang[] ={"����"};
const __code uint8 PY_mb_gui[]   ={"������������������"};
const __code uint8 PY_mb_gun[]   ={"������"};
const __code uint8 PY_mb_guo[]   ={"������������"};
const __code uint8 PY_mb_ha[]    ={"���"};
const __code uint8 PY_mb_hai[]   ={"��������������"};
const __code uint8 PY_mb_han[]   ={"��������������������������������������"};
const __code uint8 PY_mb_hang[]  ={"������"};
const __code uint8 PY_mb_hao[]   ={"���������úºźƺ�"};
const __code uint8 PY_mb_he[]    ={"�ǺȺ̺ϺκͺӺҺ˺ɺԺкʺغֺպ�"};
const __code uint8 PY_mb_hei[]   ={"�ں�"};
const __code uint8 PY_mb_hen[]   ={"�ۺܺݺ�"};
const __code uint8 PY_mb_heng[]  ={"��ߺ���"};
const __code uint8 PY_mb_hong[]  ={"����������"};
const __code uint8 PY_mb_hou[]   ={"��������"};
const __code uint8 PY_mb_hu[]    ={"������������������������������������"};
const __code uint8 PY_mb_hua[]   ={"������������������"};
const __code uint8 PY_mb_huai[]  ={"����������"};
const __code uint8 PY_mb_huan[]  ={"�����������û»�������������"};
const __code uint8 PY_mb_huang[] ={"�ĻŻʻ˻ƻ̻ͻȻǻɻлλѻ�"};
const __code uint8 PY_mb_hui[]   ={"�һֻӻԻջػ׻ڻܻ������߻޻�ݻٻ�"};
const __code uint8 PY_mb_hun[]   ={"�������"};
const __code uint8 PY_mb_huo[]   ={"�������������"};
const __code uint8 PY_mb_ji[]    ={"���������������������������������������������������������������������ƼǼ��ͼ˼ɼ��ʼ����ȼü̼żļ¼�������"};
const __code uint8 PY_mb_jia[]   ={"�ӼмѼϼҼμԼռ׼ּؼۼݼܼټ޼�Ю"};
const __code uint8 PY_mb_jian[]  ={"����߼����������������������������������������������������"};
const __code uint8 PY_mb_jiang[] ={"��������������������������"};
const __code uint8 PY_mb_jiao[]  ={"���������������������ǽƽʽȽýŽ½��˽ɽнνϽ̽ѽ;���"};
const __code uint8 PY_mb_jie[]   ={"�׽Խӽսҽֽڽٽܽ��ݽ޽ؽ߽����������"};
const __code uint8 PY_mb_jin[]   ={"���������������������������������"};
const __code uint8 PY_mb_jing[]  ={"��������������������������������������������������"};
const __code uint8 PY_mb_jiong[] ={"����"};
const __code uint8 PY_mb_jiu[]   ={"�������žþľ��¾ƾɾʾ̾ξǾȾ;�"};
const __code uint8 PY_mb_ju[]    ={"�ӾоѾԾҾϾֽ۾վ׾ھپؾ�޾ܾ߾����ݾ��۾�"};
const __code uint8 PY_mb_juan[]  ={"��������"};
const __code uint8 PY_mb_jue[]   ={"��������������"};
const __code uint8 PY_mb_jun[]   ={"����������������������"};
const __code uint8 PY_mb_ka[]    ={"������"};
const __code uint8 PY_mb_kai[]   ={"����������"};
const __code uint8 PY_mb_kan[]   ={"��������������"};
const __code uint8 PY_mb_kang[]  ={"��������������"};
const __code uint8 PY_mb_kao[]   ={"��������"};
const __code uint8 PY_mb_ke[]    ={"�����¿ƿÿſĿǿȿɿʿ˿̿Ϳ�"};
const __code uint8 PY_mb_ken[]   ={"�Ͽѿҿ�"};
const __code uint8 PY_mb_keng[]  ={"�Կ�"};
const __code uint8 PY_mb_kong[]  ={"�տ׿ֿ�"};
const __code uint8 PY_mb_kou[]   ={"�ٿڿۿ�"};
const __code uint8 PY_mb_ku[]    ={"�ݿ޿߿����"};
const __code uint8 PY_mb_kua[]   ={"������"};
const __code uint8 PY_mb_kuai[]  ={"�����"};
const __code uint8 PY_mb_kuan[]  ={"���"};
const __code uint8 PY_mb_kuang[] ={"�����������"};
const __code uint8 PY_mb_kui[]   ={"����������������������"};
const __code uint8 PY_mb_kun[]   ={"��������"};
const __code uint8 PY_mb_kuo[]   ={"��������"};
const __code uint8 PY_mb_la[]    ={"��������������"};
const __code uint8 PY_mb_lai[]   ={"������"};
const __code uint8 PY_mb_lan[]   ={"������������������������������"};
const __code uint8 PY_mb_lang[]  ={"��������������"};
const __code uint8 PY_mb_lao[]   ={"������������������"};
const __code uint8 PY_mb_le[]    ={"������"};
const __code uint8 PY_mb_lei[]   ={"����������������������"};
const __code uint8 PY_mb_leng[]  ={"������"};
const __code uint8 PY_mb_li[]    ={"��������������������������������������������������������������������"};
const __code uint8 PY_mb_lian[]  ={"����������������������������"};
const __code uint8 PY_mb_liang[] ={"������������������������"};
const __code uint8 PY_mb_liao[]  ={"������������������������"};
const __code uint8 PY_mb_lie[]   ={"����������"};
const __code uint8 PY_mb_lin[]   ={"������������������������"};
const __code uint8 PY_mb_ling[]  ={"����������������������������"};
const __code uint8 PY_mb_liu[]   ={"����������������������"};
const __code uint8 PY_mb_long[]  ={"��������¡��¤¢£"};
const __code uint8 PY_mb_lou[]   ={"¦¥§¨ª©"};
const __code uint8 PY_mb_lu[]    ={"¶¬®«¯­±²°³½¼¸¹»µ·¾º´"};
const __code uint8 PY_mb_luan[]  ={"������������"};
const __code uint8 PY_mb_lue[]   ={"����"};
const __code uint8 PY_mb_lun[]   ={"��������������"};
const __code uint8 PY_mb_luo[]   ={"������������������������"};
const __code uint8 PY_mb_lv[]    ={"��¿������������������������"};
const __code uint8 PY_mb_ma[]    ={"������������������"};
const __code uint8 PY_mb_mai[]   ={"������������"};
const __code uint8 PY_mb_man[]   ={"����������á������"};
const __code uint8 PY_mb_mang[]  ={"æâäãçå"};
const __code uint8 PY_mb_mao[]   ={"èëìéêîíïðóñò"};
const __code uint8 PY_mb_me[]    ={"ô"};
const __code uint8 PY_mb_mei[]   ={"ûöõü÷ýúøùÿ��þ��������"};
const __code uint8 PY_mb_men[]   ={"������"};
const __code uint8 PY_mb_meng[]  ={"����������������"};
const __code uint8 PY_mb_mi[]    ={"����������������������������"};
const __code uint8 PY_mb_mian[]  ={"������������������"};
const __code uint8 PY_mb_miao[]  ={"����������������"};
const __code uint8 PY_mb_mie[]   ={"����"};
const __code uint8 PY_mb_min[]   ={"������������"};
const __code uint8 PY_mb_ming[]  ={"������������"};
const __code uint8 PY_mb_miu[]   ={"��"};
const __code uint8 PY_mb_mo[]    ={"����ġģĤĦĥĢħĨĩĭİĪįĮīĬ"};
const __code uint8 PY_mb_mou[]   ={"Ĳıĳ"};
const __code uint8 PY_mb_mu[]    ={"ĸĶĵķĴľĿ��ļĹĻ��Ľĺ��"};
const __code uint8 PY_mb_na[]    ={"��������������"};
const __code uint8 PY_mb_nai[]   ={"����������"};
const __code uint8 PY_mb_nan[]   ={"������"};
const __code uint8 PY_mb_nang[]  ={"��"};
const __code uint8 PY_mb_nao[]   ={"����������"};
const __code uint8 PY_mb_ne[]    ={"��"};
const __code uint8 PY_mb_nei[]   ={"����"};
const __code uint8 PY_mb_nen[]   ={"��"};
const __code uint8 PY_mb_neng[]  ={"��"};
const __code uint8 PY_mb_ni[]    ={"����������������������"};
const __code uint8 PY_mb_nian[]  ={"��������������"};
const __code uint8 PY_mb_niang[] ={"����"};
const __code uint8 PY_mb_niao[]  ={"����"};
const __code uint8 PY_mb_nie[]   ={"��������������"};
const __code uint8 PY_mb_nin[]   ={"��"};
const __code uint8 PY_mb_ning[]  ={"��š������Ţ"};
const __code uint8 PY_mb_niu[]   ={"ţŤŦť"};
const __code uint8 PY_mb_nong[]  ={"ũŨŧŪ"};
const __code uint8 PY_mb_nu[]    ={"ūŬŭ"};
const __code uint8 PY_mb_nuan[]  ={"ů"};
const __code uint8 PY_mb_nue[]   ={"űŰ"};
const __code uint8 PY_mb_nuo[]   ={"ŲŵųŴ"};
const __code uint8 PY_mb_nv[]    ={"Ů"};
const __code uint8 PY_mb_o[]     ={"Ŷ"};
const __code uint8 PY_mb_ou[]    ={"ŷŹŸŻżźŽ"};
const __code uint8 PY_mb_pa[]    ={"ſž����������"};
const __code uint8 PY_mb_pai[]   ={"������������"};
const __code uint8 PY_mb_pan[]   ={"����������������"};
const __code uint8 PY_mb_pang[]  ={"����������"};
const __code uint8 PY_mb_pao[]   ={"��������������"};
const __code uint8 PY_mb_pei[]   ={"������������������"};
const __code uint8 PY_mb_pen[]   ={"����"};
const __code uint8 PY_mb_peng[]  ={"����������������������������"};
const __code uint8 PY_mb_pi[]    ={"��������������Ƥ��ƣơ��ƢƥƦƨƧƩ"};
const __code uint8 PY_mb_pian[]  ={"Ƭƫƪƭ"};
const __code uint8 PY_mb_piao[]  ={"ƯƮưƱ"};
const __code uint8 PY_mb_pie[]   ={"ƲƳ"};
const __code uint8 PY_mb_pin[]   ={"ƴƶƵƷƸ"};
const __code uint8 PY_mb_ping[]  ={"ƹƽ��ƾƺƻ��ƿƼ"};
const __code uint8 PY_mb_po[]    ={"����������������"};
const __code uint8 PY_mb_pou[]   ={"��"};
const __code uint8 PY_mb_pu[]    ={"������������������������������"};
const __code uint8 PY_mb_qi[]    ={"������������������������������������������������������������������������"};
const __code uint8 PY_mb_qia[]   ={"��ǡǢ"};
const __code uint8 PY_mb_qian[]  ={"ǧǪǤǨǥǣǦǫǩǰǮǯǬǱǭǳǲǴǷǵǶǸ"};
const __code uint8 PY_mb_qiang[] ={"ǺǼǹǻǿǽǾ��"};
const __code uint8 PY_mb_qiao[]  ={"������������������������������"};
const __code uint8 PY_mb_qie[]   ={"����������"};
const __code uint8 PY_mb_qin[]   ={"����������������������"};
const __code uint8 PY_mb_qing[]  ={"��������������������������"};
const __code uint8 PY_mb_qiong[] ={"����"};
const __code uint8 PY_mb_qiu[]   ={"����������������"};
const __code uint8 PY_mb_qu[]    ={"����������������ȡȢȣȥȤ"};
const __code uint8 PY_mb_quan[]  ={"ȦȫȨȪȭȬȩȧȮȰȯ"};
const __code uint8 PY_mb_que[]   ={"Ȳȱȳȴȸȷȵȶ"};
const __code uint8 PY_mb_qun[]   ={"ȹȺ"};
const __code uint8 PY_mb_ran[]   ={"ȻȼȽȾ"};
const __code uint8 PY_mb_rang[]  ={"ȿ��������"};
const __code uint8 PY_mb_rao[]   ={"������"};
const __code uint8 PY_mb_re[]    ={"����"};
const __code uint8 PY_mb_ren[]   ={"��������������������"};
const __code uint8 PY_mb_reng[]  ={"����"};
const __code uint8 PY_mb_ri[]    ={"��"};
const __code uint8 PY_mb_rong[]  ={"��������������������"};
const __code uint8 PY_mb_rou[]   ={"������"};
const __code uint8 PY_mb_ru[]    ={"��������������������"};
const __code uint8 PY_mb_ruan[]  ={"����"};
const __code uint8 PY_mb_rui[]   ={"������"};
const __code uint8 PY_mb_run[]   ={"����"};
const __code uint8 PY_mb_ruo[]   ={"����"};
const __code uint8 PY_mb_sa[]    ={"������"};
const __code uint8 PY_mb_sai[]   ={"��������"};
const __code uint8 PY_mb_san[]   ={"����ɡɢ"};
const __code uint8 PY_mb_sang[]  ={"ɣɤɥ"};
const __code uint8 PY_mb_sao[]   ={"ɦɧɨɩ"};
const __code uint8 PY_mb_se[]    ={"ɫɬɪ"};
const __code uint8 PY_mb_sen[]   ={"ɭ"};
const __code uint8 PY_mb_seng[]  ={"ɮ"};
const __code uint8 PY_mb_sha[]   ={"ɱɳɴɰɯɵɶɷ��"};
const __code uint8 PY_mb_shai[]  ={"ɸɹ"};
const __code uint8 PY_mb_shan[]  ={"ɽɾɼ��ɺɿ������ɻ������������դ"};
const __code uint8 PY_mb_shang[] ={"����������������"};
const __code uint8 PY_mb_shao[]  ={"����������������������"};
const __code uint8 PY_mb_she[]   ={"������������������������"};
const __code uint8 PY_mb_shen[]  ={"��������������������������������ʲ"};
const __code uint8 PY_mb_sheng[] ={"��������ʤ����ʡʥʢʣ"};
const __code uint8 PY_mb_shi[]   ={"��ʬʧʦʭʫʩʨʪʮʯʱʶʵʰʴʳʷʸʹʼʻʺʿ��������ʾʽ������������������������������������"};
const __code uint8 PY_mb_shou[]  ={"��������������������"};
const __code uint8 PY_mb_shu[]   ={"������������������������������������������������������ˡ����������"};
const __code uint8 PY_mb_shua[]  ={"ˢˣ"};
const __code uint8 PY_mb_shuai[] ={"˥ˤ˦˧"};
const __code uint8 PY_mb_shuan[] ={"˩˨"};
const __code uint8 PY_mb_shuang[]={"˫˪ˬ"};
const __code uint8 PY_mb_shui[]  ={"˭ˮ˰˯"};
const __code uint8 PY_mb_shun[]  ={"˱˳˴˲"};
const __code uint8 PY_mb_shuo[]  ={"˵˸˷˶"};
const __code uint8 PY_mb_si[]    ={"˿˾˽˼˹˻˺����������������"};
const __code uint8 PY_mb_song[]  ={"����������������"};
const __code uint8 PY_mb_sou[]   ={"��������"};
const __code uint8 PY_mb_su[]    ={"����������������������"};
const __code uint8 PY_mb_suan[]  ={"������"};
const __code uint8 PY_mb_sui[]   ={"����������������������"};
const __code uint8 PY_mb_sun[]   ={"������"};
const __code uint8 PY_mb_suo[]   ={"����������������"};
const __code uint8 PY_mb_ta[]    ={"����������̡̢̤̣"};
const __code uint8 PY_mb_tai[]   ={"̨̧̥̦̫̭̬̩̪"};
const __code uint8 PY_mb_tan[]   ={"̸̵̷̶̴̮̰̯̲̱̳̹̻̺̼̾̿̽"};
const __code uint8 PY_mb_tang[]  ={"��������������������������"};
const __code uint8 PY_mb_tao[]   ={"����������������������"};
const __code uint8 PY_mb_te[]    ={"��"};
const __code uint8 PY_mb_teng[]  ={"��������"};
const __code uint8 PY_mb_ti[]    ={"������������������������������"};
const __code uint8 PY_mb_tian[]  ={"����������������"};
const __code uint8 PY_mb_tiao[]  ={"������������"};
const __code uint8 PY_mb_tie[]   ={"������"};
const __code uint8 PY_mb_ting[]  ={"��͡����ͤͥͣͦͧ͢"};
const __code uint8 PY_mb_tong[]  ={"ͨͬͮͩͭͯͪͫͳͱͰͲʹ"};
const __code uint8 PY_mb_tou[]   ={"͵ͷͶ͸"};
const __code uint8 PY_mb_tu[]    ={"͹ͺͻͼͽͿ;��������"};
const __code uint8 PY_mb_tuan[]  ={"����"};
const __code uint8 PY_mb_tui[]   ={"������������"};
const __code uint8 PY_mb_tun[]   ={"��������"};
const __code uint8 PY_mb_tuo[]   ={"����������������������"};
const __code uint8 PY_mb_wa[]    ={"��������������"};
const __code uint8 PY_mb_wai[]   ={"����"};
const __code uint8 PY_mb_wan[]   ={"����������������������������������"};
const __code uint8 PY_mb_wang[]  ={"��������������������"};
const __code uint8 PY_mb_wei[]   ={"Σ��΢ΡΪΤΧΥΦΨΩάΫΰαβγέίή��δλζηθξνιμεοκ"};
const __code uint8 PY_mb_wen[]   ={"��������������������"};
const __code uint8 PY_mb_weng[]  ={"������"};
const __code uint8 PY_mb_wo[]    ={"������������������"};
const __code uint8 PY_mb_wu[]    ={"����������������������������������������������������������"};
const __code uint8 PY_mb_xi[]    ={"Ϧϫ����ϣ������Ϣ��Ϥϧϩ����ϬϡϪ��Ϩ����ϥϰϯϮϱϭϴϲϷϵϸ϶"};
const __code uint8 PY_mb_xia[]   ={"ϺϹϻ��Ͽ��ϾϽϼ������"};
const __code uint8 PY_mb_xian[]  ={"ϳ����������������������������������������������������"};
const __code uint8 PY_mb_xiang[] ={"����������������������������������������"};
const __code uint8 PY_mb_xiao[]  ={"����������������С��ТФ��ЧУЦХ"};
const __code uint8 PY_mb_xie[]   ={"ЩШЪЫЭавбгЯЬдйкжмелиз"};
const __code uint8 PY_mb_xin[]   ={"����о����п��н����"};
const __code uint8 PY_mb_xing[]  ={"����������������������������"};
const __code uint8 PY_mb_xiong[] ={"��������������"};
const __code uint8 PY_mb_xiu[]   ={"��������������������"};
const __code uint8 PY_mb_xu[]    ={"��������������������������������������"};
const __code uint8 PY_mb_xuan[]  ={"������������ѡѢѤѣ"};
const __code uint8 PY_mb_xue[]   ={"��ѥѦѨѧѩѪ"};
const __code uint8 PY_mb_xun[]   ={"ѫѬѰѲѮѱѯѭѵѶѴѸѷѳ"};
const __code uint8 PY_mb_ya[]    ={"ѾѹѽѺѻѼ��ѿ����������������"};
const __code uint8 PY_mb_yan[]   ={"������������������������������������������������������������������"};
const __code uint8 PY_mb_yang[]  ={"����������������������������������"};
const __code uint8 PY_mb_yao[]   ={"��������ҢҦҤҥҡң��ҧҨҩҪҫԿ"};
const __code uint8 PY_mb_ye[]    ={"ҬҭүҮҲұҰҵҶҷҳҹҴҺҸ"};
const __code uint8 PY_mb_yi[]    ={"һ����ҽ��ҿҼҾ������������������������������������������������������������������������������������������"};
const __code uint8 PY_mb_yin[]   ={"������������������������������ӡ"};
const __code uint8 PY_mb_ying[]  ={"ӦӢӤӧӣӥӭӯӫӨөӪӬӮӱӰӳӲ"};
const __code uint8 PY_mb_yo[]    ={"Ӵ"};
const __code uint8 PY_mb_yong[]  ={"ӶӵӸӹӺӷ��ӽӾ��ӿ��Ӽӻ��"};
const __code uint8 PY_mb_you[]   ={"����������������������������������������"};
const __code uint8 PY_mb_yu[]    ={"����������������������������������������������������Ԧ����������ԡԤ������Ԣ��ԣ������ԥ"};
const __code uint8 PY_mb_yuan[]  ={"ԩԧԨԪԱ԰ԫԭԲԬԮԵԴԳԯԶԷԹԺԸ"};
const __code uint8 PY_mb_yue[]   ={"ԻԼ��������Ծ��Խ"};
const __code uint8 PY_mb_yun[]   ={"������������������������"};
const __code uint8 PY_mb_za[]    ={"������զ"};
const __code uint8 PY_mb_zai[]   ={"����������������"};
const __code uint8 PY_mb_zan[]   ={"��������"};
const __code uint8 PY_mb_zang[]  ={"������"};
const __code uint8 PY_mb_zao[]   ={"����������������������������"};
const __code uint8 PY_mb_ze[]    ={"��������"};
const __code uint8 PY_mb_zei[]   ={"��"};
const __code uint8 PY_mb_zen[]   ={"��"};
const __code uint8 PY_mb_zeng[]  ={"������"};
const __code uint8 PY_mb_zha[]   ={"����������բագէթըե��"};
const __code uint8 PY_mb_zhai[]  ={"իժլ��խծկ"};
const __code uint8 PY_mb_zhan[]  ={"մձճղհնչյոշռսջվ��տպ"};
const __code uint8 PY_mb_zhang[] ={"��������������������������������"};
const __code uint8 PY_mb_zhao[]  ={"��������������������צ"};
const __code uint8 PY_mb_zhe[]   ={"����������������������"};
const __code uint8 PY_mb_zhen[]  ={"��������������������������������֡"};
const __code uint8 PY_mb_zheng[] ={"��������������������֤֣��֢"};
const __code uint8 PY_mb_zhi[]   ={"ְֱֲֳִֵֶַָֹֺֻּֽ֧֥֦֪֭֮֨֯֫֬֩��־������������ֿ������������������"};
const __code uint8 PY_mb_zhong[] ={"����������������������"};
const __code uint8 PY_mb_zhou[]  ={"����������������������������"};
const __code uint8 PY_mb_zhu[]   ={"������������������������������ס��ע��פ��ף��������"};
const __code uint8 PY_mb_zhua[]  ={"ץ"};
const __code uint8 PY_mb_zhuai[] ={"ק"};
const __code uint8 PY_mb_zhuan[] ={"רשת׫׭"};
const __code uint8 PY_mb_zhuang[]={"ױׯ׮װ׳״��ײ"};
const __code uint8 PY_mb_zhui[]  ={"׷׵׶׹׺׸"};
const __code uint8 PY_mb_zhun[]  ={"׻׼"};
const __code uint8 PY_mb_zhuo[]  ={"׿׾׽��������������"};
const __code uint8 PY_mb_zi[]    ={"����������������������������"};
const __code uint8 PY_mb_zong[]  ={"��������������"};
const __code uint8 PY_mb_zou[]   ={"��������"};
const __code uint8 PY_mb_zu[]    ={"����������������"};
const __code uint8 PY_mb_zuan[]  ={"׬����"};
const __code uint8 PY_mb_zui[]   ={"��������"};
const __code uint8 PY_mb_zun[]   ={"����"};
const __code uint8 PY_mb_zuo[]   ={"��������������"};
const __code uint8 PY_mb_space[] ={""};

/*"ƴ�����뷨��ѯ���,������ĸ������(index)"*/
PY_index const __code PY_index_a[]={{"",PY_mb_a},
                                    {"i",PY_mb_ai},
                                    {"n",PY_mb_an},
                                    {"ng",PY_mb_ang},
                                    {"o",PY_mb_ao}};
PY_index const __code PY_index_b[]={{"a",PY_mb_ba},
                                    {"ai",PY_mb_bai},
                                    {"an",PY_mb_ban},
                                    {"ang",PY_mb_bang},
                                    {"ao",PY_mb_bao},
                                    {"ei",PY_mb_bei},
                                    {"en",PY_mb_ben},
                                    {"eng",PY_mb_beng},
                                    {"i",PY_mb_bi},
                                    {"ian",PY_mb_bian},
                                    {"iao",PY_mb_biao},
                                    {"ie",PY_mb_bie},
                                    {"in",PY_mb_bin},
                                    {"ing",PY_mb_bing},
                                    {"o",PY_mb_bo},
                                    {"u",PY_mb_bu}};
PY_index const __code PY_index_c[]={{"a",PY_mb_ca},
                                    {"ai",PY_mb_cai},
                                    {"an",PY_mb_can},
                                    {"ang",PY_mb_cang},
                                    {"ao",PY_mb_cao},
                                    {"e",PY_mb_ce},
                                    {"eng",PY_mb_ceng},
                                    {"ha",PY_mb_cha},
                                    {"hai",PY_mb_chai},
                                    {"han",PY_mb_chan},
                                    {"hang",PY_mb_chang},
                                    {"hao",PY_mb_chao},
                                    {"he",PY_mb_che},
                                    {"hen",PY_mb_chen},
                                    {"heng",PY_mb_cheng},
                                    {"hi",PY_mb_chi},
                                    {"hong",PY_mb_chong},
                                    {"hou",PY_mb_chou},
                                    {"hu",PY_mb_chu},
                                    {"huai",PY_mb_chuai},
                                    {"huan",PY_mb_chuan},
                                    {"huang",PY_mb_chuang},
                                    {"hui",PY_mb_chui},
                                    {"hun",PY_mb_chun},
                                    {"huo",PY_mb_chuo},
                                    {"i",PY_mb_ci},
                                    {"ong",PY_mb_cong},
                                    {"ou",PY_mb_cou},
                                    {"u",PY_mb_cu},
                                    {"uan",PY_mb_cuan},
                                    {"ui",PY_mb_cui},
                                    {"un",PY_mb_cun},
                                    {"uo",PY_mb_cuo}};
PY_index const __code PY_index_d[]={{"a",PY_mb_da},
                                    {"ai",PY_mb_dai},
                                    {"an",PY_mb_dan},
                                    {"ang",PY_mb_dang},
                                    {"ao",PY_mb_dao},
                                    {"e",PY_mb_de},
                                    {"eng",PY_mb_deng},
                                    {"i",PY_mb_di},
                                    {"ian",PY_mb_dian},
                                    {"iao",PY_mb_diao},
                                    {"ie",PY_mb_die},
                                    {"ing",PY_mb_ding},
                                    {"iu",PY_mb_diu},
                                    {"ong",PY_mb_dong},
                                    {"ou",PY_mb_dou},
                                    {"u",PY_mb_du},
                                    {"uan",PY_mb_duan},
                                    {"ui",PY_mb_dui},
                                    {"un",PY_mb_dun},
                                    {"uo",PY_mb_duo}};
PY_index const __code PY_index_e[]={{"",PY_mb_e},
                                    {"n",PY_mb_en},
                                    {"r",PY_mb_er}};
PY_index const __code PY_index_f[]={{"a",PY_mb_fa},
                                    {"an",PY_mb_fan},
                                    {"ang",PY_mb_fang},
                                    {"ei",PY_mb_fei},
                                    {"en",PY_mb_fen},
                                    {"eng",PY_mb_feng},
                                    {"o",PY_mb_fo},
                                    {"ou",PY_mb_fou},
                                    {"u",PY_mb_fu}};
PY_index const __code PY_index_g[]={{"a",PY_mb_ga},
                                    {"ai",PY_mb_gai},
                                    {"an",PY_mb_gan},
                                    {"ang",PY_mb_gang},
                                    {"ao",PY_mb_gao},
                                    {"e",PY_mb_ge},
                                    {"ei",PY_mb_gei},
                                    {"en",PY_mb_gen},
                                    {"eng",PY_mb_geng},
                                    {"ong",PY_mb_gong},
                                    {"ou",PY_mb_gou},
                                    {"u",PY_mb_gu},
                                    {"ua",PY_mb_gua},
                                    {"uai",PY_mb_guai},
                                    {"uan",PY_mb_guan},
                                    {"uang",PY_mb_guang},
                                    {"ui",PY_mb_gui},
                                    {"un",PY_mb_gun},
                                    {"uo",PY_mb_guo}};
PY_index const __code PY_index_h[]={{"a",PY_mb_ha},
                                    {"ai",PY_mb_hai},
                                    {"an",PY_mb_han},
                                    {"ang",PY_mb_hang},
                                    {"ao",PY_mb_hao},
                                    {"e",PY_mb_he},
                                    {"ei",PY_mb_hei},
                                    {"en",PY_mb_hen},
                                    {"eng",PY_mb_heng},
                                    {"ong",PY_mb_hong},
                                    {"ou",PY_mb_hou},
                                    {"u",PY_mb_hu},
                                    {"ua",PY_mb_hua},
                                    {"uai",PY_mb_huai},
                                    {"uan",PY_mb_huan},
                                    {"uang",PY_mb_huang},
                                    {"ui",PY_mb_hui},
                                    {"un",PY_mb_hun},
                                    {"uo",PY_mb_huo}};
PY_index const __code PY_index_i[]={"",PY_mb_space};
PY_index const __code PY_index_j[]={{"i",PY_mb_ji},
                                    {"ia",PY_mb_jia},
                                    {"ian",PY_mb_jian},
                                    {"iang",PY_mb_jiang},
                                    {"iao",PY_mb_jiao},
                                    {"ie",PY_mb_jie},
                                    {"in",PY_mb_jin},
                                    {"ing",PY_mb_jing},
                                    {"iong",PY_mb_jiong},
                                    {"iu",PY_mb_jiu},
                                    {"u",PY_mb_ju},
                                    {"uan",PY_mb_juan},
                                    {"ue",PY_mb_jue},
                                    {"un",PY_mb_jun}};
PY_index const __code PY_index_k[]={{"a",PY_mb_ka},
                                    {"ai",PY_mb_kai},
                                    {"an",PY_mb_kan},
                                    {"ang",PY_mb_kang},
                                    {"ao",PY_mb_kao},
                                    {"e",PY_mb_ke},
                                    {"en",PY_mb_ken},
                                    {"eng",PY_mb_keng},
                                    {"ong",PY_mb_kong},
                                    {"ou",PY_mb_kou},
                                    {"u",PY_mb_ku},
                                    {"ua",PY_mb_kua},
                                    {"uai",PY_mb_kuai},
                                    {"uan",PY_mb_kuan},
                                    {"uang",PY_mb_kuang},
                                    {"ui",PY_mb_kui},
                                    {"un",PY_mb_kun},
                                    {"uo",PY_mb_kuo}};
PY_index const __code PY_index_l[]={{"a",PY_mb_la},
                                    {"ai",PY_mb_lai},
                                    {"an",PY_mb_lan},
                                    {"ang",PY_mb_lang},
                                    {"ao",PY_mb_lao},
                                    {"e",PY_mb_le},
                                    {"ei",PY_mb_lei},
                                    {"eng",PY_mb_leng},
                                    {"i",PY_mb_li},
                                    {"ian",PY_mb_lian},
                                    {"iang",PY_mb_liang},
                                    {"iao",PY_mb_liao},
                                    {"ie",PY_mb_lie},
                                    {"in",PY_mb_lin},
                                    {"ing",PY_mb_ling},
                                    {"iu",PY_mb_liu},
                                    {"ong",PY_mb_long},
                                    {"ou",PY_mb_lou},
                                    {"u",PY_mb_lu},
                                    {"uan",PY_mb_luan},
                                    {"ue",PY_mb_lue},
                                    {"un",PY_mb_lun},
                                    {"uo",PY_mb_luo},
                                    {"v",PY_mb_lv}};
PY_index const __code PY_index_m[]={{"a",PY_mb_ma},
                                    {"ai",PY_mb_mai},
                                    {"an",PY_mb_man},
                                    {"ang",PY_mb_mang},
                                    {"ao",PY_mb_mao},
                                    {"e",PY_mb_me},
                                    {"ei",PY_mb_mei},
                                    {"en",PY_mb_men},
                                    {"eng",PY_mb_meng},
                                    {"i",PY_mb_mi},
                                    {"ian",PY_mb_mian},
                                    {"iao",PY_mb_miao},
                                    {"ie",PY_mb_mie},
                                    {"in",PY_mb_min},
                                    {"ing",PY_mb_ming},
                                    {"iu",PY_mb_miu},
                                    {"o",PY_mb_mo},
                                    {"ou",PY_mb_mou},
                                    {"u",PY_mb_mu}};
PY_index const __code PY_index_n[]={{"a",PY_mb_na},
                                    {"ai",PY_mb_nai},
                                    {"an",PY_mb_nan},
                                    {"ang",PY_mb_nang},
                                    {"ao",PY_mb_nao},
                                    {"e",PY_mb_ne},
                                    {"ei",PY_mb_nei},
                                    {"en",PY_mb_nen},
                                    {"eng",PY_mb_neng},
                                    {"i",PY_mb_ni},
                                    {"ian",PY_mb_nian},
                                    {"iang",PY_mb_niang},
                                    {"iao",PY_mb_niao},
                                    {"ie",PY_mb_nie},
                                    {"in",PY_mb_nin},
                                    {"ing",PY_mb_ning},
                                    {"iu",PY_mb_niu},
                                    {"ong",PY_mb_nong},
                                    {"u",PY_mb_nu},
                                    {"uan",PY_mb_nuan},
                                    {"ue",PY_mb_nue},
                                    {"uo",PY_mb_nuo},
                                    {"v",PY_mb_nv}};
PY_index const __code PY_index_o[]={{"",PY_mb_o},
                                    {"u",PY_mb_ou}};
PY_index const __code PY_index_p[]={{"a",PY_mb_pa},
                                    {"ai",PY_mb_pai},
                                    {"an",PY_mb_pan},
                                    {"ang",PY_mb_pang},
                                    {"ao",PY_mb_pao},
                                    {"ei",PY_mb_pei},
                                    {"en",PY_mb_pen},
                                    {"eng",PY_mb_peng},
                                    {"i",PY_mb_pi},
                                    {"ian",PY_mb_pian},
                                    {"iao",PY_mb_piao},
                                    {"ie",PY_mb_pie},
                                    {"in",PY_mb_pin},
                                    {"ing",PY_mb_ping},
                                    {"o",PY_mb_po},
                                    {"ou",PY_mb_pou},
                                    {"u",PY_mb_pu}};
PY_index const __code PY_index_q[]={{"i",PY_mb_qi},
                                    {"ia",PY_mb_qia},
                                    {"ian",PY_mb_qian},
                                    {"iang",PY_mb_qiang},
                                    {"iao",PY_mb_qiao},
                                    {"ie",PY_mb_qie},
                                    {"in",PY_mb_qin},
                                    {"ing",PY_mb_qing},
                                    {"iong",PY_mb_qiong},
                                    {"iu",PY_mb_qiu},
                                    {"u",PY_mb_qu},
                                    {"uan",PY_mb_quan},
                                    {"ue",PY_mb_que},
                                    {"un",PY_mb_qun}};
PY_index const __code PY_index_r[]={{"an",PY_mb_ran},
                                    {"ang",PY_mb_rang},
                                    {"ao",PY_mb_rao},
                                    {"e",PY_mb_re},
                                    {"en",PY_mb_ren},
                                    {"eng",PY_mb_reng},
                                    {"i",PY_mb_ri},
                                    {"ong",PY_mb_rong},
                                    {"ou",PY_mb_rou},
                                    {"u",PY_mb_ru},
                                    {"uan",PY_mb_ruan},
                                    {"ui",PY_mb_rui},
                                    {"un",PY_mb_run},
                                    {"uo",PY_mb_ruo}};
PY_index const __code PY_index_s[]={{"a",PY_mb_sa},
                                    {"ai",PY_mb_sai},
                                    {"an",PY_mb_san},
                                    {"ang",PY_mb_sang},
                                    {"ao",PY_mb_sao},
                                    {"e",PY_mb_se},
                                    {"en",PY_mb_sen},
                                    {"eng",PY_mb_seng},
                                    {"ha",PY_mb_sha},
                                    {"hai",PY_mb_shai},
                                    {"han",PY_mb_shan},
                                    {"hang",PY_mb_shang},
                                    {"hao",PY_mb_shao},
                                    {"he",PY_mb_she},
                                    {"hen",PY_mb_shen},
                                    {"heng",PY_mb_sheng},
                                    {"hi",PY_mb_shi},
                                    {"hou",PY_mb_shou},
                                    {"hu",PY_mb_shu},
                                    {"hua",PY_mb_shua},
                                    {"huai",PY_mb_shuai},
                                    {"huan",PY_mb_shuan},
                                    {"huang",PY_mb_shuang},
                                    {"hui",PY_mb_shui},
                                    {"hun",PY_mb_shun},
                                    {"huo",PY_mb_shuo},
                                    {"i",PY_mb_si},
                                    {"ong",PY_mb_song},
                                    {"ou",PY_mb_sou},
                                    {"u",PY_mb_su},
                                    {"uan",PY_mb_suan},
                                    {"ui",PY_mb_sui},
                                    {"un",PY_mb_sun},
                                    {"uo",PY_mb_suo}};
PY_index const __code PY_index_t[]={{"a",PY_mb_ta},
                                    {"ai",PY_mb_tai},
                                    {"an",PY_mb_tan},
                                    {"ang",PY_mb_tang},
                                    {"ao",PY_mb_tao},
                                    {"e",PY_mb_te},
                                    {"eng",PY_mb_teng},
                                    {"i",PY_mb_ti},
                                    {"ian",PY_mb_tian},
                                    {"iao",PY_mb_tiao},
                                    {"ie",PY_mb_tie},
                                    {"ing",PY_mb_ting},
                                    {"ong",PY_mb_tong},
                                    {"ou",PY_mb_tou},
                                    {"u",PY_mb_tu},
                                    {"uan",PY_mb_tuan},
                                    {"ui",PY_mb_tui},
                                    {"un",PY_mb_tun},
                                    {"uo",PY_mb_tuo}};
PY_index const __code PY_index_u[]={{"",PY_mb_space}};
PY_index const __code PY_index_v[]={{"",PY_mb_space}};
PY_index const __code PY_index_w[]={{"a",PY_mb_wa},
                                    {"ai",PY_mb_wai},
                                    {"an",PY_mb_wan},
                                    {"ang",PY_mb_wang},
                                    {"ei",PY_mb_wei},
                                    {"en",PY_mb_wen},
                                    {"eng",PY_mb_weng},
                                    {"o",PY_mb_wo},
                                    {"u",PY_mb_wu}};
PY_index const __code PY_index_x[]={{"i",PY_mb_xi},
                                    {"ia",PY_mb_xia},
                                    {"ian",PY_mb_xian},
                                    {"iang",PY_mb_xiang},
                                    {"iao",PY_mb_xiao},
                                    {"ie",PY_mb_xie},
                                    {"in",PY_mb_xin},
                                    {"ing",PY_mb_xing},
                                    {"iong",PY_mb_xiong},
                                    {"iu",PY_mb_xiu},
                                    {"u",PY_mb_xu},
                                    {"uan",PY_mb_xuan},
                                    {"ue",PY_mb_xue},
                                    {"un",PY_mb_xun}};
PY_index const __code PY_index_y[]={{"a",PY_mb_ya},
                                    {"an",PY_mb_yan},
                                    {"ang",PY_mb_yang},
                                    {"ao",PY_mb_yao},
                                    {"e",PY_mb_ye},
                                    {"i",PY_mb_yi},
                                    {"in",PY_mb_yin},
                                    {"ing",PY_mb_ying},
                                    {"o",PY_mb_yo},
                                    {"ong",PY_mb_yong},
                                    {"ou",PY_mb_you},
                                    {"u",PY_mb_yu},
                                    {"uan",PY_mb_yuan},
                                    {"ue",PY_mb_yue},
                                    {"un",PY_mb_yun}};
PY_index const __code PY_index_z[]={{"a",PY_mb_za},
                                    {"ai",PY_mb_zai},
                                    {"an",PY_mb_zan},
                                    {"ang",PY_mb_zang},
                                    {"ao",PY_mb_zao},
                                    {"e",PY_mb_ze},
                                    {"ei",PY_mb_zei},
                                    {"en",PY_mb_zen},
                                    {"eng",PY_mb_zeng},
                                    {"ha",PY_mb_zha},
                                    {"hai",PY_mb_zhai},
                                    {"han",PY_mb_zhan},
                                    {"hang",PY_mb_zhang},
                                    {"hao",PY_mb_zhao},
                                    {"he",PY_mb_zhe},
                                    {"hen",PY_mb_zhen},
                                    {"heng",PY_mb_zheng},
                                    {"hi",PY_mb_zhi},
                                    {"hong",PY_mb_zhong},
                                    {"hou",PY_mb_zhou},
                                    {"hu",PY_mb_zhu},
                                    {"hua",PY_mb_zhua},
                                    {"huai",PY_mb_zhuai},
                                    {"huan",PY_mb_zhuan},
                                    {"huang",PY_mb_zhuang},
                                    {"hui",PY_mb_zhui},
                                    {"hun",PY_mb_zhun},
                                    {"huo",PY_mb_zhuo},
                                    {"i",PY_mb_zi},
                                    {"ong",PY_mb_zong},
                                    {"ou",PY_mb_zou},
                                    {"u",PY_mb_zu},
                                    {"uan",PY_mb_zuan},
                                    {"ui",PY_mb_zui},
                                    {"un",PY_mb_zun},
                                    {"uo",PY_mb_zuo}};
PY_index const __code PY_index_end[]={"",PY_mb_space};

/*��������ĸ������*/
PY_index const __code *PY_index_headletter[]={PY_index_a,
                                            PY_index_b,
                                            PY_index_c,
                                            PY_index_d,
                                            PY_index_e,
                                            PY_index_f,
                                            PY_index_g,
                                            PY_index_h,
                                            PY_index_i,
                                            PY_index_j,
                                            PY_index_k,
                                            PY_index_l,
                                            PY_index_m,
                                            PY_index_n,
                                            PY_index_o,
                                            PY_index_p,
                                            PY_index_q,
                                            PY_index_r,
                                            PY_index_s,
                                            PY_index_t,
                                            PY_index_u,
                                            PY_index_v,
                                            PY_index_w,
                                            PY_index_x,
                                            PY_index_y,
                                            PY_index_z,
                                            PY_index_end};
extern uint8 const __code * py_ime(uint8 *strInput_py_str);
uint8 const __code * py_ime(uint8 *strInput_py_str)
{
    PY_index const __code *cpHZ, *cpHZedge;
    uint8 i, cInputStrLength;
    uint8 *InputStr = strInput_py_str;

    cInputStrLength=osal_strlen((char*)InputStr);    /*����ƴ��������*/
    if(*InputStr == '\0')return(0);       /*���������ַ�����0*/

    for(i=0;i<cInputStrLength;i++)
        *(InputStr+i)|=0x20;             /*����ĸ��תΪСд*/

    if(*InputStr == 'i')return(0);        /*����ƴ������*/
    if(*InputStr == 'u')return(0);
    if(*InputStr == 'v')return(0);

    cpHZ=PY_index_headletter[InputStr[0]-'a'];        /*������ĸ����*/
    cpHZedge=PY_index_headletter[InputStr[0]-'a'+1];  /*����ָ�����*/

    InputStr++;                           /*ָ��ƴ�����ڶ�����ĸ*/
    while(cpHZ<cpHZedge)                         /*����������*/
    {
        for(i=0;i<cInputStrLength;i++)
        {
            if(*(InputStr+i)!=*(cpHZ->PY+i))break;    /*������ĸ������,�˳�*/
        }
        if(i==cInputStrLength)        /*��ĸ��ȫ��*/
        {
            return (cpHZ->PY_mb);
        }
        cpHZ++;
    }
    return 0;                      /*�޹�����*/
}
