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
    const  uint8 *PY_mb;
} PY_index;

//"ƴ�����뷨�������б�,���(mb)"
const  uint8 PY_mb_a[]     = {"����"};
const  uint8 PY_mb_ai[]    = {"��������������������������"};
const  uint8 PY_mb_an[]    = {"������������������"};
const  uint8 PY_mb_ang[]   = {"������"};
const  uint8 PY_mb_ao[]    = {"�������������°İ�"};
const  uint8 PY_mb_ba[]    = {"�˰ͰȰǰɰṴ̋ưʰΰϰѰаӰְհ�"};
const  uint8 PY_mb_bai[]   = {"�װٰ۰ذڰܰݰ�"};
const  uint8 PY_mb_ban[]   = {"�����߰����������"};
const  uint8 PY_mb_bang[]  = {"������������������"};
const  uint8 PY_mb_bao[]   = {"������������������������������������"};
const  uint8 PY_mb_bei[]   = {"������������������������������"};
const  uint8 PY_mb_ben[]   = {"����������"};
const  uint8 PY_mb_beng[]  = {"�����±ñű�"};
const  uint8 PY_mb_bi[]    = {"�ƱǱȱ˱ʱɱұرϱձӱѱݱбֱԱͱױ̱αڱܱ�"};
const  uint8 PY_mb_bian[]  = {"�߱�ޱ���������"};
const  uint8 PY_mb_biao[]  = {"�����"};
const  uint8 PY_mb_bie[]   = {"�����"};
const  uint8 PY_mb_bin[]   = {"����������"};
const  uint8 PY_mb_bing[]  = {"������������������"};
const  uint8 PY_mb_bo[]    = {"����������������������������������������"};
const  uint8 PY_mb_bu[]    = {"��������������������"};
const  uint8 PY_mb_ca[]    = {"��"};
const  uint8 PY_mb_cai[]   = {"�²ŲĲƲòɲʲǲȲ˲�"};
const  uint8 PY_mb_can[]   = {"�βͲвϲѲҲ�"};
const  uint8 PY_mb_cang[]  = {"�ֲײԲղ�"};
const  uint8 PY_mb_cao[]   = {"�ٲڲܲ۲�"};
const  uint8 PY_mb_ce[]    = {"���޲��"};
const  uint8 PY_mb_ceng[]  = {"�����"};
const  uint8 PY_mb_cha[]   = {"������������ɲ"};
const  uint8 PY_mb_chai[]  = {"����"};
const  uint8 PY_mb_chan[]  = {"�������������������"};
const  uint8 PY_mb_chang[] = {"������������������������"};
const  uint8 PY_mb_chao[]  = {"��������������������"};
const  uint8 PY_mb_che[]   = {"������������"};
const  uint8 PY_mb_chen[]  = {"�������������³��ĳ�"};
const  uint8 PY_mb_cheng[] = {"�Ƴųɳʳгϳǳ˳ͳ̳γȳѳҳ�"};
const  uint8 PY_mb_chi[]   = {"�Գճڳس۳ٳֳ߳޳ݳܳ����"};
const  uint8 PY_mb_chong[] = {"������"};
const  uint8 PY_mb_chou[]  = {"�������������"};
const  uint8 PY_mb_chu[]   = {"����������������������������������"};
const  uint8 PY_mb_chuai[] = {"��"};
const  uint8 PY_mb_chuan[] = {"��������������"};
const  uint8 PY_mb_chuang[] = {"����������"};
const  uint8 PY_mb_chui[]  = {"����������"};
const  uint8 PY_mb_chun[]  = {"��������������"};
const  uint8 PY_mb_chuo[]  = {"��"};
const  uint8 PY_mb_ci[]    = {"�ôʴĴɴȴǴŴƴ˴δ̴�"};
const  uint8 PY_mb_cong[]  = {"�ѴӴҴдϴ�"};
const  uint8 PY_mb_cou[]   = {"��"};
const  uint8 PY_mb_cu[]    = {"�ִٴ״�"};
const  uint8 PY_mb_cuan[]  = {"�ڴܴ�"};
const  uint8 PY_mb_cui[]   = {"�޴ߴݴ�����"};
const  uint8 PY_mb_cun[]   = {"����"};
const  uint8 PY_mb_cuo[]   = {"�������"};
const  uint8 PY_mb_da[]    = {"�������"};
const  uint8 PY_mb_dai[]   = {"������������������������"};
const  uint8 PY_mb_dan[]   = {"������������������������������"};
const  uint8 PY_mb_dang[]  = {"����������"};
const  uint8 PY_mb_dao[]   = {"������������������������"};
const  uint8 PY_mb_de[]    = {"�õµ�"};
const  uint8 PY_mb_deng[]  = {"�Ƶǵŵȵ˵ʵ�"};
const  uint8 PY_mb_di[]    = {"�͵̵εҵϵеӵѵյ׵ֵصܵ۵ݵڵ޵�"};
const  uint8 PY_mb_dian[]  = {"���ߵ�������������"};
const  uint8 PY_mb_diao[]  = {"�����������"};
const  uint8 PY_mb_die[]   = {"��������������"};
const  uint8 PY_mb_ding[]  = {"������������������"};
const  uint8 PY_mb_diu[]   = {"��"};
const  uint8 PY_mb_dong[]  = {"��������������������"};
const  uint8 PY_mb_dou[]   = {"����������������"};
const  uint8 PY_mb_du[]    = {"�����������¶ĶöʶŶǶȶɶ�"};
const  uint8 PY_mb_duan[]  = {"�˶̶ζ϶ж�"};
const  uint8 PY_mb_dui[]   = {"�ѶӶԶ�"};
const  uint8 PY_mb_dun[]   = {"�ֶضն׶ܶ۶ٶ�"};
const  uint8 PY_mb_duo[]   = {"��߶�޶��������"};
const  uint8 PY_mb_e[]     = {"����������������"};
const  uint8 PY_mb_en[]    = {"��"};
const  uint8 PY_mb_er[]    = {"����������������"};
const  uint8 PY_mb_fa[]    = {"����������������"};
const  uint8 PY_mb_fan[]   = {"����������������������������������"};
const  uint8 PY_mb_fang[]  = {"���������������·÷ķ�"};
const  uint8 PY_mb_fei[]   = {"�ɷǷȷƷʷ˷̷ͷϷзη�"};
const  uint8 PY_mb_fen[]   = {"�ַԷ׷ҷշӷطڷٷ۷ݷܷ޷߷�"};
const  uint8 PY_mb_feng[]  = {"����������������"};
const  uint8 PY_mb_fo[]    = {"��"};
const  uint8 PY_mb_fou[]   = {"��"};
const  uint8 PY_mb_fu[]    = {"������󸥷�����������������������������������������������������������������������������"};
const  uint8 PY_mb_ga[]    = {"�¸�"};
const  uint8 PY_mb_gai[]   = {"�øĸƸǸȸ�"};
const  uint8 PY_mb_gan[]   = {"�ɸʸ˸θ̸͸ѸϸҸи�"};
const  uint8 PY_mb_gang[]  = {"�Ըոڸٸظ׸ָ۸�"};
const  uint8 PY_mb_gao[]   = {"�޸�߸�ݸ�����"};
const  uint8 PY_mb_ge[]    = {"����������������������"};
const  uint8 PY_mb_gei[]   = {"��"};
const  uint8 PY_mb_gen[]   = {"����"};
const  uint8 PY_mb_geng[]  = {"��������������"};
const  uint8 PY_mb_gong[]  = {"������������������������������"};
const  uint8 PY_mb_gou[]   = {"������������������"};
const  uint8 PY_mb_gu[]    = {"�����ù¹��������ŹȹɹǹƹĹ̹ʹ˹�"};
const  uint8 PY_mb_gua[]   = {"�Ϲιйѹҹ�"};
const  uint8 PY_mb_guai[]  = {"�Թչ�"};
const  uint8 PY_mb_guan[]  = {"�ع۹ٹڹ׹ݹܹ�߹��"};
const  uint8 PY_mb_guang[] = {"����"};
const  uint8 PY_mb_gui[]   = {"������������������"};
const  uint8 PY_mb_gun[]   = {"������"};
const  uint8 PY_mb_guo[]   = {"������������"};
const  uint8 PY_mb_ha[]    = {"���"};
const  uint8 PY_mb_hai[]   = {"��������������"};
const  uint8 PY_mb_han[]   = {"��������������������������������������"};
const  uint8 PY_mb_hang[]  = {"������"};
const  uint8 PY_mb_hao[]   = {"���������úºźƺ�"};
const  uint8 PY_mb_he[]    = {"�ǺȺ̺ϺκͺӺҺ˺ɺԺкʺغֺպ�"};
const  uint8 PY_mb_hei[]   = {"�ں�"};
const  uint8 PY_mb_hen[]   = {"�ۺܺݺ�"};
const  uint8 PY_mb_heng[]  = {"��ߺ���"};
const  uint8 PY_mb_hong[]  = {"����������"};
const  uint8 PY_mb_hou[]   = {"��������"};
const  uint8 PY_mb_hu[]    = {"������������������������������������"};
const  uint8 PY_mb_hua[]   = {"������������������"};
const  uint8 PY_mb_huai[]  = {"����������"};
const  uint8 PY_mb_huan[]  = {"�����������û»�������������"};
const  uint8 PY_mb_huang[] = {"�ĻŻʻ˻ƻ̻ͻȻǻɻлλѻ�"};
const  uint8 PY_mb_hui[]   = {"�һֻӻԻջػ׻ڻܻ������߻޻�ݻٻ�"};
const  uint8 PY_mb_hun[]   = {"�������"};
const  uint8 PY_mb_huo[]   = {"�������������"};
const  uint8 PY_mb_ji[]    = {"���������������������������������������������������������������������ƼǼ��ͼ˼ɼ��ʼ����ȼü̼żļ¼�������"};
const  uint8 PY_mb_jia[]   = {"�ӼмѼϼҼμԼռ׼ּؼۼݼܼټ޼�Ю"};
const  uint8 PY_mb_jian[]  = {"����߼����������������������������������������������������"};
const  uint8 PY_mb_jiang[] = {"��������������������������"};
const  uint8 PY_mb_jiao[]  = {"���������������������ǽƽʽȽýŽ½��˽ɽнνϽ̽ѽ;���"};
const  uint8 PY_mb_jie[]   = {"�׽Խӽսҽֽڽٽܽ��ݽ޽ؽ߽����������"};
const  uint8 PY_mb_jin[]   = {"���������������������������������"};
const  uint8 PY_mb_jing[]  = {"��������������������������������������������������"};
const  uint8 PY_mb_jiong[] = {"����"};
const  uint8 PY_mb_jiu[]   = {"�������žþľ��¾ƾɾʾ̾ξǾȾ;�"};
const  uint8 PY_mb_ju[]    = {"�ӾоѾԾҾϾֽ۾վ׾ھپؾ�޾ܾ߾����ݾ��۾�"};
const  uint8 PY_mb_juan[]  = {"��������"};
const  uint8 PY_mb_jue[]   = {"��������������"};
const  uint8 PY_mb_jun[]   = {"����������������������"};
const  uint8 PY_mb_ka[]    = {"������"};
const  uint8 PY_mb_kai[]   = {"����������"};
const  uint8 PY_mb_kan[]   = {"��������������"};
const  uint8 PY_mb_kang[]  = {"��������������"};
const  uint8 PY_mb_kao[]   = {"��������"};
const  uint8 PY_mb_ke[]    = {"�����¿ƿÿſĿǿȿɿʿ˿̿Ϳ�"};
const  uint8 PY_mb_ken[]   = {"�Ͽѿҿ�"};
const  uint8 PY_mb_keng[]  = {"�Կ�"};
const  uint8 PY_mb_kong[]  = {"�տ׿ֿ�"};
const  uint8 PY_mb_kou[]   = {"�ٿڿۿ�"};
const  uint8 PY_mb_ku[]    = {"�ݿ޿߿����"};
const  uint8 PY_mb_kua[]   = {"������"};
const  uint8 PY_mb_kuai[]  = {"�����"};
const  uint8 PY_mb_kuan[]  = {"���"};
const  uint8 PY_mb_kuang[] = {"�����������"};
const  uint8 PY_mb_kui[]   = {"����������������������"};
const  uint8 PY_mb_kun[]   = {"��������"};
const  uint8 PY_mb_kuo[]   = {"��������"};
const  uint8 PY_mb_la[]    = {"��������������"};
const  uint8 PY_mb_lai[]   = {"������"};
const  uint8 PY_mb_lan[]   = {"������������������������������"};
const  uint8 PY_mb_lang[]  = {"��������������"};
const  uint8 PY_mb_lao[]   = {"������������������"};
const  uint8 PY_mb_le[]    = {"������"};
const  uint8 PY_mb_lei[]   = {"����������������������"};
const  uint8 PY_mb_leng[]  = {"������"};
const  uint8 PY_mb_li[]    = {"��������������������������������������������������������������������"};
const  uint8 PY_mb_lian[]  = {"����������������������������"};
const  uint8 PY_mb_liang[] = {"������������������������"};
const  uint8 PY_mb_liao[]  = {"������������������������"};
const  uint8 PY_mb_lie[]   = {"����������"};
const  uint8 PY_mb_lin[]   = {"������������������������"};
const  uint8 PY_mb_ling[]  = {"����������������������������"};
const  uint8 PY_mb_liu[]   = {"����������������������"};
const  uint8 PY_mb_long[]  = {"��������¡��¤¢£"};
const  uint8 PY_mb_lou[]   = {"¦¥§¨ª©"};
const  uint8 PY_mb_lu[]    = {"¶¬®«¯­±²°³½¼¸¹»µ·¾º´"};
const  uint8 PY_mb_luan[]  = {"������������"};
const  uint8 PY_mb_lue[]   = {"����"};
const  uint8 PY_mb_lun[]   = {"��������������"};
const  uint8 PY_mb_luo[]   = {"������������������������"};
const  uint8 PY_mb_lv[]    = {"��¿������������������������"};
const  uint8 PY_mb_ma[]    = {"������������������"};
const  uint8 PY_mb_mai[]   = {"������������"};
const  uint8 PY_mb_man[]   = {"����������á������"};
const  uint8 PY_mb_mang[]  = {"æâäãçå"};
const  uint8 PY_mb_mao[]   = {"èëìéêîíïðóñò"};
const  uint8 PY_mb_me[]    = {"ô"};
const  uint8 PY_mb_mei[]   = {"ûöõü÷ýúøùÿ��þ��������"};
const  uint8 PY_mb_men[]   = {"������"};
const  uint8 PY_mb_meng[]  = {"����������������"};
const  uint8 PY_mb_mi[]    = {"����������������������������"};
const  uint8 PY_mb_mian[]  = {"������������������"};
const  uint8 PY_mb_miao[]  = {"����������������"};
const  uint8 PY_mb_mie[]   = {"����"};
const  uint8 PY_mb_min[]   = {"������������"};
const  uint8 PY_mb_ming[]  = {"������������"};
const  uint8 PY_mb_miu[]   = {"��"};
const  uint8 PY_mb_mo[]    = {"����ġģĤĦĥĢħĨĩĭİĪįĮīĬ"};
const  uint8 PY_mb_mou[]   = {"Ĳıĳ"};
const  uint8 PY_mb_mu[]    = {"ĸĶĵķĴľĿ��ļĹĻ��Ľĺ��"};
const  uint8 PY_mb_na[]    = {"��������������"};
const  uint8 PY_mb_nai[]   = {"����������"};
const  uint8 PY_mb_nan[]   = {"������"};
const  uint8 PY_mb_nang[]  = {"��"};
const  uint8 PY_mb_nao[]   = {"����������"};
const  uint8 PY_mb_ne[]    = {"��"};
const  uint8 PY_mb_nei[]   = {"����"};
const  uint8 PY_mb_nen[]   = {"��"};
const  uint8 PY_mb_neng[]  = {"��"};
const  uint8 PY_mb_ni[]    = {"����������������������"};
const  uint8 PY_mb_nian[]  = {"��������������"};
const  uint8 PY_mb_niang[] = {"����"};
const  uint8 PY_mb_niao[]  = {"����"};
const  uint8 PY_mb_nie[]   = {"��������������"};
const  uint8 PY_mb_nin[]   = {"��"};
const  uint8 PY_mb_ning[]  = {"��š������Ţ"};
const  uint8 PY_mb_niu[]   = {"ţŤŦť"};
const  uint8 PY_mb_nong[]  = {"ũŨŧŪ"};
const  uint8 PY_mb_nu[]    = {"ūŬŭ"};
const  uint8 PY_mb_nuan[]  = {"ů"};
const  uint8 PY_mb_nue[]   = {"űŰ"};
const  uint8 PY_mb_nuo[]   = {"ŲŵųŴ"};
const  uint8 PY_mb_nv[]    = {"Ů"};
const  uint8 PY_mb_o[]     = {"Ŷ"};
const  uint8 PY_mb_ou[]    = {"ŷŹŸŻżźŽ"};
const  uint8 PY_mb_pa[]    = {"ſž����������"};
const  uint8 PY_mb_pai[]   = {"������������"};
const  uint8 PY_mb_pan[]   = {"����������������"};
const  uint8 PY_mb_pang[]  = {"����������"};
const  uint8 PY_mb_pao[]   = {"��������������"};
const  uint8 PY_mb_pei[]   = {"������������������"};
const  uint8 PY_mb_pen[]   = {"����"};
const  uint8 PY_mb_peng[]  = {"����������������������������"};
const  uint8 PY_mb_pi[]    = {"��������������Ƥ��ƣơ��ƢƥƦƨƧƩ"};
const  uint8 PY_mb_pian[]  = {"Ƭƫƪƭ"};
const  uint8 PY_mb_piao[]  = {"ƯƮưƱ"};
const  uint8 PY_mb_pie[]   = {"ƲƳ"};
const  uint8 PY_mb_pin[]   = {"ƴƶƵƷƸ"};
const  uint8 PY_mb_ping[]  = {"ƹƽ��ƾƺƻ��ƿƼ"};
const  uint8 PY_mb_po[]    = {"����������������"};
const  uint8 PY_mb_pou[]   = {"��"};
const  uint8 PY_mb_pu[]    = {"������������������������������"};
const  uint8 PY_mb_qi[]    = {"������������������������������������������������������������������������"};
const  uint8 PY_mb_qia[]   = {"��ǡǢ"};
const  uint8 PY_mb_qian[]  = {"ǧǪǤǨǥǣǦǫǩǰǮǯǬǱǭǳǲǴǷǵǶǸ"};
const  uint8 PY_mb_qiang[] = {"ǺǼǹǻǿǽǾ��"};
const  uint8 PY_mb_qiao[]  = {"������������������������������"};
const  uint8 PY_mb_qie[]   = {"����������"};
const  uint8 PY_mb_qin[]   = {"����������������������"};
const  uint8 PY_mb_qing[]  = {"��������������������������"};
const  uint8 PY_mb_qiong[] = {"����"};
const  uint8 PY_mb_qiu[]   = {"����������������"};
const  uint8 PY_mb_qu[]    = {"����������������ȡȢȣȥȤ"};
const  uint8 PY_mb_quan[]  = {"ȦȫȨȪȭȬȩȧȮȰȯ"};
const  uint8 PY_mb_que[]   = {"Ȳȱȳȴȸȷȵȶ"};
const  uint8 PY_mb_qun[]   = {"ȹȺ"};
const  uint8 PY_mb_ran[]   = {"ȻȼȽȾ"};
const  uint8 PY_mb_rang[]  = {"ȿ��������"};
const  uint8 PY_mb_rao[]   = {"������"};
const  uint8 PY_mb_re[]    = {"����"};
const  uint8 PY_mb_ren[]   = {"��������������������"};
const  uint8 PY_mb_reng[]  = {"����"};
const  uint8 PY_mb_ri[]    = {"��"};
const  uint8 PY_mb_rong[]  = {"��������������������"};
const  uint8 PY_mb_rou[]   = {"������"};
const  uint8 PY_mb_ru[]    = {"��������������������"};
const  uint8 PY_mb_ruan[]  = {"����"};
const  uint8 PY_mb_rui[]   = {"������"};
const  uint8 PY_mb_run[]   = {"����"};
const  uint8 PY_mb_ruo[]   = {"����"};
const  uint8 PY_mb_sa[]    = {"������"};
const  uint8 PY_mb_sai[]   = {"��������"};
const  uint8 PY_mb_san[]   = {"����ɡɢ"};
const  uint8 PY_mb_sang[]  = {"ɣɤɥ"};
const  uint8 PY_mb_sao[]   = {"ɦɧɨɩ"};
const  uint8 PY_mb_se[]    = {"ɫɬɪ"};
const  uint8 PY_mb_sen[]   = {"ɭ"};
const  uint8 PY_mb_seng[]  = {"ɮ"};
const  uint8 PY_mb_sha[]   = {"ɱɳɴɰɯɵɶɷ��"};
const  uint8 PY_mb_shai[]  = {"ɸɹ"};
const  uint8 PY_mb_shan[]  = {"ɽɾɼ��ɺɿ������ɻ������������դ"};
const  uint8 PY_mb_shang[] = {"����������������"};
const  uint8 PY_mb_shao[]  = {"����������������������"};
const  uint8 PY_mb_she[]   = {"������������������������"};
const  uint8 PY_mb_shen[]  = {"��������������������������������ʲ"};
const  uint8 PY_mb_sheng[] = {"��������ʤ����ʡʥʢʣ"};
const  uint8 PY_mb_shi[]   = {"��ʬʧʦʭʫʩʨʪʮʯʱʶʵʰʴʳʷʸʹʼʻʺʿ��������ʾʽ������������������������������������"};
const  uint8 PY_mb_shou[]  = {"��������������������"};
const  uint8 PY_mb_shu[]   = {"������������������������������������������������������ˡ����������"};
const  uint8 PY_mb_shua[]  = {"ˢˣ"};
const  uint8 PY_mb_shuai[] = {"˥ˤ˦˧"};
const  uint8 PY_mb_shuan[] = {"˩˨"};
const  uint8 PY_mb_shuang[] = {"˫˪ˬ"};
const  uint8 PY_mb_shui[]  = {"˭ˮ˰˯"};
const  uint8 PY_mb_shun[]  = {"˱˳˴˲"};
const  uint8 PY_mb_shuo[]  = {"˵˸˷˶"};
const  uint8 PY_mb_si[]    = {"˿˾˽˼˹˻˺����������������"};
const  uint8 PY_mb_song[]  = {"����������������"};
const  uint8 PY_mb_sou[]   = {"��������"};
const  uint8 PY_mb_su[]    = {"����������������������"};
const  uint8 PY_mb_suan[]  = {"������"};
const  uint8 PY_mb_sui[]   = {"����������������������"};
const  uint8 PY_mb_sun[]   = {"������"};
const  uint8 PY_mb_suo[]   = {"����������������"};
const  uint8 PY_mb_ta[]    = {"����������̡̢̤̣"};
const  uint8 PY_mb_tai[]   = {"̨̧̥̦̫̭̬̩̪"};
const  uint8 PY_mb_tan[]   = {"̸̵̷̶̴̮̰̯̲̱̳̹̻̺̼̾̿̽"};
const  uint8 PY_mb_tang[]  = {"��������������������������"};
const  uint8 PY_mb_tao[]   = {"����������������������"};
const  uint8 PY_mb_te[]    = {"��"};
const  uint8 PY_mb_teng[]  = {"��������"};
const  uint8 PY_mb_ti[]    = {"������������������������������"};
const  uint8 PY_mb_tian[]  = {"����������������"};
const  uint8 PY_mb_tiao[]  = {"������������"};
const  uint8 PY_mb_tie[]   = {"������"};
const  uint8 PY_mb_ting[]  = {"��͡����ͤͥͣͦͧ͢"};
const  uint8 PY_mb_tong[]  = {"ͨͬͮͩͭͯͪͫͳͱͰͲʹ"};
const  uint8 PY_mb_tou[]   = {"͵ͷͶ͸"};
const  uint8 PY_mb_tu[]    = {"͹ͺͻͼͽͿ;��������"};
const  uint8 PY_mb_tuan[]  = {"����"};
const  uint8 PY_mb_tui[]   = {"������������"};
const  uint8 PY_mb_tun[]   = {"��������"};
const  uint8 PY_mb_tuo[]   = {"����������������������"};
const  uint8 PY_mb_wa[]    = {"��������������"};
const  uint8 PY_mb_wai[]   = {"����"};
const  uint8 PY_mb_wan[]   = {"����������������������������������"};
const  uint8 PY_mb_wang[]  = {"��������������������"};
const  uint8 PY_mb_wei[]   = {"Σ��΢ΡΪΤΧΥΦΨΩάΫΰαβγέίή��δλζηθξνιμεοκ"};
const  uint8 PY_mb_wen[]   = {"��������������������"};
const  uint8 PY_mb_weng[]  = {"������"};
const  uint8 PY_mb_wo[]    = {"������������������"};
const  uint8 PY_mb_wu[]    = {"����������������������������������������������������������"};
const  uint8 PY_mb_xi[]    = {"Ϧϫ����ϣ������Ϣ��Ϥϧϩ����ϬϡϪ��Ϩ����ϥϰϯϮϱϭϴϲϷϵϸ϶"};
const  uint8 PY_mb_xia[]   = {"ϺϹϻ��Ͽ��ϾϽϼ������"};
const  uint8 PY_mb_xian[]  = {"ϳ����������������������������������������������������"};
const  uint8 PY_mb_xiang[] = {"����������������������������������������"};
const  uint8 PY_mb_xiao[]  = {"����������������С��ТФ��ЧУЦХ"};
const  uint8 PY_mb_xie[]   = {"ЩШЪЫЭавбгЯЬдйкжмелиз"};
const  uint8 PY_mb_xin[]   = {"����о����п��н����"};
const  uint8 PY_mb_xing[]  = {"����������������������������"};
const  uint8 PY_mb_xiong[] = {"��������������"};
const  uint8 PY_mb_xiu[]   = {"��������������������"};
const  uint8 PY_mb_xu[]    = {"��������������������������������������"};
const  uint8 PY_mb_xuan[]  = {"������������ѡѢѤѣ"};
const  uint8 PY_mb_xue[]   = {"��ѥѦѨѧѩѪ"};
const  uint8 PY_mb_xun[]   = {"ѫѬѰѲѮѱѯѭѵѶѴѸѷѳ"};
const  uint8 PY_mb_ya[]    = {"ѾѹѽѺѻѼ��ѿ����������������"};
const  uint8 PY_mb_yan[]   = {"������������������������������������������������������������������"};
const  uint8 PY_mb_yang[]  = {"����������������������������������"};
const  uint8 PY_mb_yao[]   = {"��������ҢҦҤҥҡң��ҧҨҩҪҫԿ"};
const  uint8 PY_mb_ye[]    = {"ҬҭүҮҲұҰҵҶҷҳҹҴҺҸ"};
const  uint8 PY_mb_yi[]    = {"һ����ҽ��ҿҼҾ������������������������������������������������������������������������������������������"};
const  uint8 PY_mb_yin[]   = {"������������������������������ӡ"};
const  uint8 PY_mb_ying[]  = {"ӦӢӤӧӣӥӭӯӫӨөӪӬӮӱӰӳӲ"};
const  uint8 PY_mb_yo[]    = {"Ӵ"};
const  uint8 PY_mb_yong[]  = {"ӶӵӸӹӺӷ��ӽӾ��ӿ��Ӽӻ��"};
const  uint8 PY_mb_you[]   = {"����������������������������������������"};
const  uint8 PY_mb_yu[]    = {"����������������������������������������������������Ԧ����������ԡԤ������Ԣ��ԣ������ԥ"};
const  uint8 PY_mb_yuan[]  = {"ԩԧԨԪԱ԰ԫԭԲԬԮԵԴԳԯԶԷԹԺԸ"};
const  uint8 PY_mb_yue[]   = {"ԻԼ��������Ծ��Խ"};
const  uint8 PY_mb_yun[]   = {"������������������������"};
const  uint8 PY_mb_za[]    = {"������զ"};
const  uint8 PY_mb_zai[]   = {"����������������"};
const  uint8 PY_mb_zan[]   = {"��������"};
const  uint8 PY_mb_zang[]  = {"������"};
const  uint8 PY_mb_zao[]   = {"����������������������������"};
const  uint8 PY_mb_ze[]    = {"��������"};
const  uint8 PY_mb_zei[]   = {"��"};
const  uint8 PY_mb_zen[]   = {"��"};
const  uint8 PY_mb_zeng[]  = {"������"};
const  uint8 PY_mb_zha[]   = {"����������բագէթըե��"};
const  uint8 PY_mb_zhai[]  = {"իժլ��խծկ"};
const  uint8 PY_mb_zhan[]  = {"մձճղհնչյոշռսջվ��տպ"};
const  uint8 PY_mb_zhang[] = {"��������������������������������"};
const  uint8 PY_mb_zhao[]  = {"��������������������צ"};
const  uint8 PY_mb_zhe[]   = {"����������������������"};
const  uint8 PY_mb_zhen[]  = {"��������������������������������֡"};
const  uint8 PY_mb_zheng[] = {"��������������������֤֣��֢"};
const  uint8 PY_mb_zhi[]   = {"ְֱֲֳִֵֶַָֹֺֻּֽ֧֥֦֪֭֮֨֯֫֬֩��־������������ֿ������������������"};
const  uint8 PY_mb_zhong[] = {"����������������������"};
const  uint8 PY_mb_zhou[]  = {"����������������������������"};
const  uint8 PY_mb_zhu[]   = {"������������������������������ס��ע��פ��ף��������"};
const  uint8 PY_mb_zhua[]  = {"ץ"};
const  uint8 PY_mb_zhuai[] = {"ק"};
const  uint8 PY_mb_zhuan[] = {"רשת׫׭"};
const  uint8 PY_mb_zhuang[] = {"ױׯ׮װ׳״��ײ"};
const  uint8 PY_mb_zhui[]  = {"׷׵׶׹׺׸"};
const  uint8 PY_mb_zhun[]  = {"׻׼"};
const  uint8 PY_mb_zhuo[]  = {"׿׾׽��������������"};
const  uint8 PY_mb_zi[]    = {"����������������������������"};
const  uint8 PY_mb_zong[]  = {"��������������"};
const  uint8 PY_mb_zou[]   = {"��������"};
const  uint8 PY_mb_zu[]    = {"����������������"};
const  uint8 PY_mb_zuan[]  = {"׬����"};
const  uint8 PY_mb_zui[]   = {"��������"};
const  uint8 PY_mb_zun[]   = {"����"};
const  uint8 PY_mb_zuo[]   = {"��������������"};
const  uint8 PY_mb_space[] = {""};

/*"ƴ�����뷨��ѯ���,������ĸ������(index)"*/
PY_index const  PY_index_a[] = {{"", PY_mb_a},
    {"i", PY_mb_ai},
    {"n", PY_mb_an},
    {"ng", PY_mb_ang},
    {"o", PY_mb_ao}
};
PY_index const  PY_index_b[] = {{"a", PY_mb_ba},
    {"ai", PY_mb_bai},
    {"an", PY_mb_ban},
    {"ang", PY_mb_bang},
    {"ao", PY_mb_bao},
    {"ei", PY_mb_bei},
    {"en", PY_mb_ben},
    {"eng", PY_mb_beng},
    {"i", PY_mb_bi},
    {"ian", PY_mb_bian},
    {"iao", PY_mb_biao},
    {"ie", PY_mb_bie},
    {"in", PY_mb_bin},
    {"ing", PY_mb_bing},
    {"o", PY_mb_bo},
    {"u", PY_mb_bu}
};
PY_index const  PY_index_c[] = {{"a", PY_mb_ca},
    {"ai", PY_mb_cai},
    {"an", PY_mb_can},
    {"ang", PY_mb_cang},
    {"ao", PY_mb_cao},
    {"e", PY_mb_ce},
    {"eng", PY_mb_ceng},
    {"ha", PY_mb_cha},
    {"hai", PY_mb_chai},
    {"han", PY_mb_chan},
    {"hang", PY_mb_chang},
    {"hao", PY_mb_chao},
    {"he", PY_mb_che},
    {"hen", PY_mb_chen},
    {"heng", PY_mb_cheng},
    {"hi", PY_mb_chi},
    {"hong", PY_mb_chong},
    {"hou", PY_mb_chou},
    {"hu", PY_mb_chu},
    {"huai", PY_mb_chuai},
    {"huan", PY_mb_chuan},
    {"huang", PY_mb_chuang},
    {"hui", PY_mb_chui},
    {"hun", PY_mb_chun},
    {"huo", PY_mb_chuo},
    {"i", PY_mb_ci},
    {"ong", PY_mb_cong},
    {"ou", PY_mb_cou},
    {"u", PY_mb_cu},
    {"uan", PY_mb_cuan},
    {"ui", PY_mb_cui},
    {"un", PY_mb_cun},
    {"uo", PY_mb_cuo}
};
PY_index const  PY_index_d[] = {{"a", PY_mb_da},
    {"ai", PY_mb_dai},
    {"an", PY_mb_dan},
    {"ang", PY_mb_dang},
    {"ao", PY_mb_dao},
    {"e", PY_mb_de},
    {"eng", PY_mb_deng},
    {"i", PY_mb_di},
    {"ian", PY_mb_dian},
    {"iao", PY_mb_diao},
    {"ie", PY_mb_die},
    {"ing", PY_mb_ding},
    {"iu", PY_mb_diu},
    {"ong", PY_mb_dong},
    {"ou", PY_mb_dou},
    {"u", PY_mb_du},
    {"uan", PY_mb_duan},
    {"ui", PY_mb_dui},
    {"un", PY_mb_dun},
    {"uo", PY_mb_duo}
};
PY_index const  PY_index_e[] = {{"", PY_mb_e},
    {"n", PY_mb_en},
    {"r", PY_mb_er}
};
PY_index const  PY_index_f[] = {{"a", PY_mb_fa},
    {"an", PY_mb_fan},
    {"ang", PY_mb_fang},
    {"ei", PY_mb_fei},
    {"en", PY_mb_fen},
    {"eng", PY_mb_feng},
    {"o", PY_mb_fo},
    {"ou", PY_mb_fou},
    {"u", PY_mb_fu}
};
PY_index const  PY_index_g[] = {{"a", PY_mb_ga},
    {"ai", PY_mb_gai},
    {"an", PY_mb_gan},
    {"ang", PY_mb_gang},
    {"ao", PY_mb_gao},
    {"e", PY_mb_ge},
    {"ei", PY_mb_gei},
    {"en", PY_mb_gen},
    {"eng", PY_mb_geng},
    {"ong", PY_mb_gong},
    {"ou", PY_mb_gou},
    {"u", PY_mb_gu},
    {"ua", PY_mb_gua},
    {"uai", PY_mb_guai},
    {"uan", PY_mb_guan},
    {"uang", PY_mb_guang},
    {"ui", PY_mb_gui},
    {"un", PY_mb_gun},
    {"uo", PY_mb_guo}
};
PY_index const  PY_index_h[] = {{"a", PY_mb_ha},
    {"ai", PY_mb_hai},
    {"an", PY_mb_han},
    {"ang", PY_mb_hang},
    {"ao", PY_mb_hao},
    {"e", PY_mb_he},
    {"ei", PY_mb_hei},
    {"en", PY_mb_hen},
    {"eng", PY_mb_heng},
    {"ong", PY_mb_hong},
    {"ou", PY_mb_hou},
    {"u", PY_mb_hu},
    {"ua", PY_mb_hua},
    {"uai", PY_mb_huai},
    {"uan", PY_mb_huan},
    {"uang", PY_mb_huang},
    {"ui", PY_mb_hui},
    {"un", PY_mb_hun},
    {"uo", PY_mb_huo}
};
PY_index const  PY_index_i[] = {"", PY_mb_space};
PY_index const  PY_index_j[] = {{"i", PY_mb_ji},
    {"ia", PY_mb_jia},
    {"ian", PY_mb_jian},
    {"iang", PY_mb_jiang},
    {"iao", PY_mb_jiao},
    {"ie", PY_mb_jie},
    {"in", PY_mb_jin},
    {"ing", PY_mb_jing},
    {"iong", PY_mb_jiong},
    {"iu", PY_mb_jiu},
    {"u", PY_mb_ju},
    {"uan", PY_mb_juan},
    {"ue", PY_mb_jue},
    {"un", PY_mb_jun}
};
PY_index const  PY_index_k[] = {{"a", PY_mb_ka},
    {"ai", PY_mb_kai},
    {"an", PY_mb_kan},
    {"ang", PY_mb_kang},
    {"ao", PY_mb_kao},
    {"e", PY_mb_ke},
    {"en", PY_mb_ken},
    {"eng", PY_mb_keng},
    {"ong", PY_mb_kong},
    {"ou", PY_mb_kou},
    {"u", PY_mb_ku},
    {"ua", PY_mb_kua},
    {"uai", PY_mb_kuai},
    {"uan", PY_mb_kuan},
    {"uang", PY_mb_kuang},
    {"ui", PY_mb_kui},
    {"un", PY_mb_kun},
    {"uo", PY_mb_kuo}
};
PY_index const  PY_index_l[] = {{"a", PY_mb_la},
    {"ai", PY_mb_lai},
    {"an", PY_mb_lan},
    {"ang", PY_mb_lang},
    {"ao", PY_mb_lao},
    {"e", PY_mb_le},
    {"ei", PY_mb_lei},
    {"eng", PY_mb_leng},
    {"i", PY_mb_li},
    {"ian", PY_mb_lian},
    {"iang", PY_mb_liang},
    {"iao", PY_mb_liao},
    {"ie", PY_mb_lie},
    {"in", PY_mb_lin},
    {"ing", PY_mb_ling},
    {"iu", PY_mb_liu},
    {"ong", PY_mb_long},
    {"ou", PY_mb_lou},
    {"u", PY_mb_lu},
    {"uan", PY_mb_luan},
    {"ue", PY_mb_lue},
    {"un", PY_mb_lun},
    {"uo", PY_mb_luo},
    {"v", PY_mb_lv}
};
PY_index const  PY_index_m[] = {{"a", PY_mb_ma},
    {"ai", PY_mb_mai},
    {"an", PY_mb_man},
    {"ang", PY_mb_mang},
    {"ao", PY_mb_mao},
    {"e", PY_mb_me},
    {"ei", PY_mb_mei},
    {"en", PY_mb_men},
    {"eng", PY_mb_meng},
    {"i", PY_mb_mi},
    {"ian", PY_mb_mian},
    {"iao", PY_mb_miao},
    {"ie", PY_mb_mie},
    {"in", PY_mb_min},
    {"ing", PY_mb_ming},
    {"iu", PY_mb_miu},
    {"o", PY_mb_mo},
    {"ou", PY_mb_mou},
    {"u", PY_mb_mu}
};
PY_index const  PY_index_n[] = {{"a", PY_mb_na},
    {"ai", PY_mb_nai},
    {"an", PY_mb_nan},
    {"ang", PY_mb_nang},
    {"ao", PY_mb_nao},
    {"e", PY_mb_ne},
    {"ei", PY_mb_nei},
    {"en", PY_mb_nen},
    {"eng", PY_mb_neng},
    {"i", PY_mb_ni},
    {"ian", PY_mb_nian},
    {"iang", PY_mb_niang},
    {"iao", PY_mb_niao},
    {"ie", PY_mb_nie},
    {"in", PY_mb_nin},
    {"ing", PY_mb_ning},
    {"iu", PY_mb_niu},
    {"ong", PY_mb_nong},
    {"u", PY_mb_nu},
    {"uan", PY_mb_nuan},
    {"ue", PY_mb_nue},
    {"uo", PY_mb_nuo},
    {"v", PY_mb_nv}
};
PY_index const  PY_index_o[] = {{"", PY_mb_o},
    {"u", PY_mb_ou}
};
PY_index const  PY_index_p[] = {{"a", PY_mb_pa},
    {"ai", PY_mb_pai},
    {"an", PY_mb_pan},
    {"ang", PY_mb_pang},
    {"ao", PY_mb_pao},
    {"ei", PY_mb_pei},
    {"en", PY_mb_pen},
    {"eng", PY_mb_peng},
    {"i", PY_mb_pi},
    {"ian", PY_mb_pian},
    {"iao", PY_mb_piao},
    {"ie", PY_mb_pie},
    {"in", PY_mb_pin},
    {"ing", PY_mb_ping},
    {"o", PY_mb_po},
    {"ou", PY_mb_pou},
    {"u", PY_mb_pu}
};
PY_index const  PY_index_q[] = {{"i", PY_mb_qi},
    {"ia", PY_mb_qia},
    {"ian", PY_mb_qian},
    {"iang", PY_mb_qiang},
    {"iao", PY_mb_qiao},
    {"ie", PY_mb_qie},
    {"in", PY_mb_qin},
    {"ing", PY_mb_qing},
    {"iong", PY_mb_qiong},
    {"iu", PY_mb_qiu},
    {"u", PY_mb_qu},
    {"uan", PY_mb_quan},
    {"ue", PY_mb_que},
    {"un", PY_mb_qun}
};
PY_index const  PY_index_r[] = {{"an", PY_mb_ran},
    {"ang", PY_mb_rang},
    {"ao", PY_mb_rao},
    {"e", PY_mb_re},
    {"en", PY_mb_ren},
    {"eng", PY_mb_reng},
    {"i", PY_mb_ri},
    {"ong", PY_mb_rong},
    {"ou", PY_mb_rou},
    {"u", PY_mb_ru},
    {"uan", PY_mb_ruan},
    {"ui", PY_mb_rui},
    {"un", PY_mb_run},
    {"uo", PY_mb_ruo}
};
PY_index const  PY_index_s[] = {{"a", PY_mb_sa},
    {"ai", PY_mb_sai},
    {"an", PY_mb_san},
    {"ang", PY_mb_sang},
    {"ao", PY_mb_sao},
    {"e", PY_mb_se},
    {"en", PY_mb_sen},
    {"eng", PY_mb_seng},
    {"ha", PY_mb_sha},
    {"hai", PY_mb_shai},
    {"han", PY_mb_shan},
    {"hang", PY_mb_shang},
    {"hao", PY_mb_shao},
    {"he", PY_mb_she},
    {"hen", PY_mb_shen},
    {"heng", PY_mb_sheng},
    {"hi", PY_mb_shi},
    {"hou", PY_mb_shou},
    {"hu", PY_mb_shu},
    {"hua", PY_mb_shua},
    {"huai", PY_mb_shuai},
    {"huan", PY_mb_shuan},
    {"huang", PY_mb_shuang},
    {"hui", PY_mb_shui},
    {"hun", PY_mb_shun},
    {"huo", PY_mb_shuo},
    {"i", PY_mb_si},
    {"ong", PY_mb_song},
    {"ou", PY_mb_sou},
    {"u", PY_mb_su},
    {"uan", PY_mb_suan},
    {"ui", PY_mb_sui},
    {"un", PY_mb_sun},
    {"uo", PY_mb_suo}
};
PY_index const  PY_index_t[] = {{"a", PY_mb_ta},
    {"ai", PY_mb_tai},
    {"an", PY_mb_tan},
    {"ang", PY_mb_tang},
    {"ao", PY_mb_tao},
    {"e", PY_mb_te},
    {"eng", PY_mb_teng},
    {"i", PY_mb_ti},
    {"ian", PY_mb_tian},
    {"iao", PY_mb_tiao},
    {"ie", PY_mb_tie},
    {"ing", PY_mb_ting},
    {"ong", PY_mb_tong},
    {"ou", PY_mb_tou},
    {"u", PY_mb_tu},
    {"uan", PY_mb_tuan},
    {"ui", PY_mb_tui},
    {"un", PY_mb_tun},
    {"uo", PY_mb_tuo}
};
PY_index const  PY_index_u[] = {{"", PY_mb_space}};
PY_index const  PY_index_v[] = {{"", PY_mb_space}};
PY_index const  PY_index_w[] = {{"a", PY_mb_wa},
    {"ai", PY_mb_wai},
    {"an", PY_mb_wan},
    {"ang", PY_mb_wang},
    {"ei", PY_mb_wei},
    {"en", PY_mb_wen},
    {"eng", PY_mb_weng},
    {"o", PY_mb_wo},
    {"u", PY_mb_wu}
};
PY_index const  PY_index_x[] = {{"i", PY_mb_xi},
    {"ia", PY_mb_xia},
    {"ian", PY_mb_xian},
    {"iang", PY_mb_xiang},
    {"iao", PY_mb_xiao},
    {"ie", PY_mb_xie},
    {"in", PY_mb_xin},
    {"ing", PY_mb_xing},
    {"iong", PY_mb_xiong},
    {"iu", PY_mb_xiu},
    {"u", PY_mb_xu},
    {"uan", PY_mb_xuan},
    {"ue", PY_mb_xue},
    {"un", PY_mb_xun}
};
PY_index const  PY_index_y[] = {{"a", PY_mb_ya},
    {"an", PY_mb_yan},
    {"ang", PY_mb_yang},
    {"ao", PY_mb_yao},
    {"e", PY_mb_ye},
    {"i", PY_mb_yi},
    {"in", PY_mb_yin},
    {"ing", PY_mb_ying},
    {"o", PY_mb_yo},
    {"ong", PY_mb_yong},
    {"ou", PY_mb_you},
    {"u", PY_mb_yu},
    {"uan", PY_mb_yuan},
    {"ue", PY_mb_yue},
    {"un", PY_mb_yun}
};
PY_index const  PY_index_z[] = {{"a", PY_mb_za},
    {"ai", PY_mb_zai},
    {"an", PY_mb_zan},
    {"ang", PY_mb_zang},
    {"ao", PY_mb_zao},
    {"e", PY_mb_ze},
    {"ei", PY_mb_zei},
    {"en", PY_mb_zen},
    {"eng", PY_mb_zeng},
    {"ha", PY_mb_zha},
    {"hai", PY_mb_zhai},
    {"han", PY_mb_zhan},
    {"hang", PY_mb_zhang},
    {"hao", PY_mb_zhao},
    {"he", PY_mb_zhe},
    {"hen", PY_mb_zhen},
    {"heng", PY_mb_zheng},
    {"hi", PY_mb_zhi},
    {"hong", PY_mb_zhong},
    {"hou", PY_mb_zhou},
    {"hu", PY_mb_zhu},
    {"hua", PY_mb_zhua},
    {"huai", PY_mb_zhuai},
    {"huan", PY_mb_zhuan},
    {"huang", PY_mb_zhuang},
    {"hui", PY_mb_zhui},
    {"hun", PY_mb_zhun},
    {"huo", PY_mb_zhuo},
    {"i", PY_mb_zi},
    {"ong", PY_mb_zong},
    {"ou", PY_mb_zou},
    {"u", PY_mb_zu},
    {"uan", PY_mb_zuan},
    {"ui", PY_mb_zui},
    {"un", PY_mb_zun},
    {"uo", PY_mb_zuo}
};
PY_index const  PY_index_end[] = {"", PY_mb_space};

PY_index  const  *PY_index_headletter[] =
{
    PY_index_a,
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
    PY_index_end
};

/*��������ĸ������*/
uint8 const  PY_index_size[] = {sizeof(PY_index_a)/sizeof(PY_index),
        sizeof(PY_index_b)/sizeof(PY_index),
        sizeof(PY_index_c)/sizeof(PY_index),
        sizeof(PY_index_d)/sizeof(PY_index),
        sizeof(PY_index_e)/sizeof(PY_index),
        sizeof(PY_index_f)/sizeof(PY_index),
        sizeof(PY_index_g)/sizeof(PY_index),
        sizeof(PY_index_h)/sizeof(PY_index),
        sizeof(PY_index_i)/sizeof(PY_index),
        sizeof(PY_index_j)/sizeof(PY_index),
        sizeof(PY_index_k)/sizeof(PY_index),
        sizeof(PY_index_l)/sizeof(PY_index),
        sizeof(PY_index_m)/sizeof(PY_index),
        sizeof(PY_index_n)/sizeof(PY_index),
        sizeof(PY_index_o)/sizeof(PY_index),
        sizeof(PY_index_p)/sizeof(PY_index),
        sizeof(PY_index_q)/sizeof(PY_index),
        sizeof(PY_index_r)/sizeof(PY_index),
        sizeof(PY_index_s)/sizeof(PY_index),
        sizeof(PY_index_t)/sizeof(PY_index),
        sizeof(PY_index_u)/sizeof(PY_index),
        sizeof(PY_index_v)/sizeof(PY_index),
        sizeof(PY_index_w)/sizeof(PY_index),
        sizeof(PY_index_x)/sizeof(PY_index),
        sizeof(PY_index_y)/sizeof(PY_index),
        sizeof(PY_index_z)/sizeof(PY_index),
        sizeof(PY_index_end)/sizeof(PY_index)
};

extern uint8 const  * py_ime(uint8 *strInput_py_str);
uint8 const  * py_ime(uint8 *strInput_py_str)
{
    PY_index const  *cpHZ, *cpHZedge;
    uint8 i, cInputStrLength;
    uint8 *InputStr = strInput_py_str;

    cInputStrLength = osal_strlen((char*)InputStr);  /*����ƴ��������*/
    if(*InputStr == '\0')return(0);       /*���������ַ�����0*/

    for(i = 0; i < cInputStrLength; i++)
        *(InputStr + i) |= 0x20;         /*����ĸ��תΪСд*/

    if(*InputStr == 'i')return(0);        /*����ƴ������*/
    if(*InputStr == 'u')return(0);
    if(*InputStr == 'v')return(0);

    cpHZ = PY_index_headletter[InputStr[0] - 'a'];    /*������ĸ����*/
    //cpHZedge=PY_index_headletter[InputStr[0]-'a'+1];  /*����ָ�����*/
    cpHZedge = cpHZ + PY_index_size[InputStr[0]-'a'];

    InputStr++;                           /*ָ��ƴ�����ڶ�����ĸ*/
    while(cpHZ < cpHZedge)                       /*����������*/
    {
        for(i = 0; i < cInputStrLength; i++)
        {
            if(*(InputStr + i) != *(cpHZ->PY + i))break; /*������ĸ������,�˳�*/
        }
        if(i == cInputStrLength)      /*��ĸ��ȫ��*/
        {
            return (cpHZ->PY_mb);
        }
        cpHZ++;
    }
    return 0;                      /*�޹�����*/
}
