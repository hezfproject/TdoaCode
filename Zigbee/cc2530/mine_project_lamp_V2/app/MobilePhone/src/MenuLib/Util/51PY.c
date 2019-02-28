/********************Æ´ÒôÊäÈë·¨Ä£¿é*******************
/                      Ô­×÷:ÕÅ ¿­
/                      ¸ÄĞ´:Àî Ç¿(mail2li@21cn.com)
/                  ±àÒë»·¾³£ºKeil C 6.14
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

//"Æ´ÒôÊäÈë·¨ºº×ÖÅÅÁĞ±í,Âë±í(mb)"
const  uint8 PY_mb_a[]     = {"°¢°¡"};
const  uint8 PY_mb_ai[]    = {"°¥°§°¦°£°¤°¨°©°«°ª°¬°®°¯°­"};
const  uint8 PY_mb_an[]    = {"°²°±°°°³°¶°´°¸°·°µ"};
const  uint8 PY_mb_ang[]   = {"°¹°º°»"};
const  uint8 PY_mb_ao[]    = {"°¼°½°¾°¿°À°Á°Â°Ä°Ã"};
const  uint8 PY_mb_ba[]    = {"°Ë°Í°È°Ç°É°Å°Ì°Æ°Ê°Î°Ï°Ñ°Ğ°Ó°Ö°Õ°Ô"};
const  uint8 PY_mb_bai[]   = {"°×°Ù°Û°Ø°Ú°Ü°İ°Ş"};
const  uint8 PY_mb_ban[]   = {"°â°à°ã°ä°ß°á°å°æ°ì°ë°é°ç°è°í°ê"};
const  uint8 PY_mb_bang[]  = {"°î°ï°ğ°ó°ñ°ò°ö°ø°ô°ù°õ°÷"};
const  uint8 PY_mb_bao[]   = {"°ü°ú°û°ı±¢±¦±¥±£±¤±¨±§±ª±«±©±¬°ş±¡ÆÙ"};
const  uint8 PY_mb_bei[]   = {"±°±­±¯±®±±±´±·±¸±³±µ±¶±»±¹±º±²"};
const  uint8 PY_mb_ben[]   = {"±¼±¾±½±¿º»"};
const  uint8 PY_mb_beng[]  = {"±À±Á±Â±Ã±Å±Ä"};
const  uint8 PY_mb_bi[]    = {"±Æ±Ç±È±Ë±Ê±É±Ò±Ø±Ï±Õ±Ó±Ñ±İ±Ğ±Ö±Ô±Í±×±Ì±Î±Ú±Ü±Û"};
const  uint8 PY_mb_bian[]  = {"±ß±à±Ş±á±â±å±ã±ä±é±æ±ç±è"};
const  uint8 PY_mb_biao[]  = {"±ë±ê±ì±í"};
const  uint8 PY_mb_bie[]   = {"±ï±î±ğ±ñ"};
const  uint8 PY_mb_bin[]   = {"±ö±ò±ó±õ±ô±÷"};
const  uint8 PY_mb_bing[]  = {"±ù±ø±û±ü±ú±ş±ı²¢²¡"};
const  uint8 PY_mb_bo[]    = {"²¦²¨²£²§²±²¤²¥²®²µ²¯²´²ª²¬²°²©²³²«²­²²²·"};
const  uint8 PY_mb_bu[]    = {"²¹²¸²¶²»²¼²½²À²¿²º²¾"};
const  uint8 PY_mb_ca[]    = {"²Á"};
const  uint8 PY_mb_cai[]   = {"²Â²Å²Ä²Æ²Ã²É²Ê²Ç²È²Ë²Ì"};
const  uint8 PY_mb_can[]   = {"²Î²Í²Ğ²Ï²Ñ²Ò²Ó"};
const  uint8 PY_mb_cang[]  = {"²Ö²×²Ô²Õ²Ø"};
const  uint8 PY_mb_cao[]   = {"²Ù²Ú²Ü²Û²İ"};
const  uint8 PY_mb_ce[]    = {"²á²à²Ş²â²ß"};
const  uint8 PY_mb_ceng[]  = {"²ã²äÔø"};
const  uint8 PY_mb_cha[]   = {"²æ²å²é²ç²è²ë²ì²ê²í²ï²îÉ²"};
const  uint8 PY_mb_chai[]  = {"²ğ²ñ²ò"};
const  uint8 PY_mb_chan[]  = {"²ô²ó²÷²ö²ø²õ²ú²ù²û²ü"};
const  uint8 PY_mb_chang[] = {"²ı²ş³¦³¢³¥³£³§³¡³¨³©³«³ª"};
const  uint8 PY_mb_chao[]  = {"³­³®³¬³²³¯³°³±³³³´´Â"};
const  uint8 PY_mb_che[]   = {"³µ³¶³¹³¸³·³º"};
const  uint8 PY_mb_chen[]  = {"³»³¾³¼³À³Á³½³Â³¿³Ä³Ã"};
const  uint8 PY_mb_cheng[] = {"³Æ³Å³É³Ê³Ğ³Ï³Ç³Ë³Í³Ì³Î³È³Ñ³Ò³Ó"};
const  uint8 PY_mb_chi[]   = {"³Ô³Õ³Ú³Ø³Û³Ù³Ö³ß³Ş³İ³Ü³â³à³ã³á"};
const  uint8 PY_mb_chong[] = {"³ä³å³æ³ç³è"};
const  uint8 PY_mb_chou[]  = {"³é³ğ³ñ³ë³î³í³ï³ê³ì³ó³ò³ô"};
const  uint8 PY_mb_chu[]   = {"³ö³õ³ı³ø³ü³ú³û³÷³ù´¡´¢³ş´¦´¤´¥´£Ğó"};
const  uint8 PY_mb_chuai[] = {"´§"};
const  uint8 PY_mb_chuan[] = {"´¨´©´«´¬´ª´­´®"};
const  uint8 PY_mb_chuang[] = {"´³´¯´°´²´´"};
const  uint8 PY_mb_chui[]  = {"´µ´¶´¹´·´¸"};
const  uint8 PY_mb_chun[]  = {"´º´»´¿´½´¾´¼´À"};
const  uint8 PY_mb_chuo[]  = {"´Á"};
const  uint8 PY_mb_ci[]    = {"´Ã´Ê´Ä´É´È´Ç´Å´Æ´Ë´Î´Ì´Í"};
const  uint8 PY_mb_cong[]  = {"´Ñ´Ó´Ò´Ğ´Ï´Ô"};
const  uint8 PY_mb_cou[]   = {"´Õ"};
const  uint8 PY_mb_cu[]    = {"´Ö´Ù´×´Ø"};
const  uint8 PY_mb_cuan[]  = {"´Ú´Ü´Û"};
const  uint8 PY_mb_cui[]   = {"´Ş´ß´İ´à´ã´á´â´ä"};
const  uint8 PY_mb_cun[]   = {"´å´æ´ç"};
const  uint8 PY_mb_cuo[]   = {"´ê´è´é´ì´ë´í"};
const  uint8 PY_mb_da[]    = {"´î´ï´ğ´ñ´ò´ó"};
const  uint8 PY_mb_dai[]   = {"´ô´õ´ö´ú´ø´ıµ¡´ù´û´ü´ş´÷"};
const  uint8 PY_mb_dan[]   = {"µ¤µ¥µ£µ¢µ¦µ¨µ§µ©µ«µ®µ¯µ¬µ­µ°µª"};
const  uint8 PY_mb_dang[]  = {"µ±µ²µ³µ´µµ"};
const  uint8 PY_mb_dao[]   = {"µ¶µ¼µºµ¹µ·µ»µ¸µ½µ¿µÁµÀµ¾"};
const  uint8 PY_mb_de[]    = {"µÃµÂµÄ"};
const  uint8 PY_mb_deng[]  = {"µÆµÇµÅµÈµËµÊµÉ"};
const  uint8 PY_mb_di[]    = {"µÍµÌµÎµÒµÏµĞµÓµÑµÕµ×µÖµØµÜµÛµİµÚµŞµÙ"};
const  uint8 PY_mb_dian[]  = {"µàµáµßµäµãµâµçµèµéµêµæµëµíµìµîµå"};
const  uint8 PY_mb_diao[]  = {"µóµğµòµïµñµõµöµô"};
const  uint8 PY_mb_die[]   = {"µùµøµüµıµşµúµû"};
const  uint8 PY_mb_ding[]  = {"¶¡¶£¶¢¶¤¶¥¶¦¶©¶¨¶§"};
const  uint8 PY_mb_diu[]   = {"¶ª"};
const  uint8 PY_mb_dong[]  = {"¶«¶¬¶­¶®¶¯¶³¶±¶²¶°¶´"};
const  uint8 PY_mb_dou[]   = {"¶¼¶µ¶·¶¶¶¸¶¹¶º¶»"};
const  uint8 PY_mb_du[]    = {"¶½¶¾¶Á¶¿¶À¶Â¶Ä¶Ã¶Ê¶Å¶Ç¶È¶É¶Æ"};
const  uint8 PY_mb_duan[]  = {"¶Ë¶Ì¶Î¶Ï¶Ğ¶Í"};
const  uint8 PY_mb_dui[]   = {"¶Ñ¶Ó¶Ô¶Ò"};
const  uint8 PY_mb_dun[]   = {"¶Ö¶Ø¶Õ¶×¶Ü¶Û¶Ù¶İ"};
const  uint8 PY_mb_duo[]   = {"¶à¶ß¶á¶Ş¶ä¶â¶ã¶ç¶é¶æ¶è¶å"};
const  uint8 PY_mb_e[]     = {"¶ï¶í¶ğ¶ë¶ì¶ê¶î¶ò¶ó¶ñ¶ö¶õ¶ô"};
const  uint8 PY_mb_en[]    = {"¶÷"};
const  uint8 PY_mb_er[]    = {"¶ù¶ø¶û¶ú¶ı¶ü¶ş·¡"};
const  uint8 PY_mb_fa[]    = {"·¢·¦·¥·£·§·¤·¨·©"};
const  uint8 PY_mb_fan[]   = {"·«·¬·­·ª·²·¯·°·³·®·±·´·µ·¸·º·¹·¶··"};
const  uint8 PY_mb_fang[]  = {"·½·»·¼·À·Á·¿·¾·Â·Ã·Ä·Å"};
const  uint8 PY_mb_fei[]   = {"·É·Ç·È·Æ·Ê·Ë·Ì·Í·Ï·Ğ·Î·Ñ"};
const  uint8 PY_mb_fen[]   = {"·Ö·Ô·×·Ò·Õ·Ó·Ø·Ú·Ù·Û·İ·Ü·Ş·ß·à"};
const  uint8 PY_mb_feng[]  = {"·á·ç·ã·â·è·å·é·æ·ä·ë·ê·ì·í·ï·î"};
const  uint8 PY_mb_fo[]    = {"·ğ"};
const  uint8 PY_mb_fou[]   = {"·ñ"};
const  uint8 PY_mb_fu[]    = {"·ò·ô·õ·ó¸¥·ü·ö·÷·ş·ı·ú¸¡¸¢·û¸¤·ù¸£·ø¸§¸¦¸®¸«¸©¸ª¸¨¸­¸¯¸¸¸¼¸¶¸¾¸º¸½¸À¸·¸´¸°¸±¸µ¸»¸³¸¿¸¹¸²"};
const  uint8 PY_mb_ga[]    = {"¸Â¸Á"};
const  uint8 PY_mb_gai[]   = {"¸Ã¸Ä¸Æ¸Ç¸È¸Å"};
const  uint8 PY_mb_gan[]   = {"¸É¸Ê¸Ë¸Î¸Ì¸Í¸Ñ¸Ï¸Ò¸Ğ¸Ó"};
const  uint8 PY_mb_gang[]  = {"¸Ô¸Õ¸Ú¸Ù¸Ø¸×¸Ö¸Û¸Ü"};
const  uint8 PY_mb_gao[]   = {"¸Ş¸á¸ß¸à¸İ¸â¸ã¸å¸ä¸æ"};
const  uint8 PY_mb_ge[]    = {"¸ê¸í¸ç¸ì¸ë¸î¸é¸è¸ó¸ï¸ñ¸ğ¸ô¸ö¸÷¸õ¿©"};
const  uint8 PY_mb_gei[]   = {"¸ø"};
const  uint8 PY_mb_gen[]   = {"¸ù¸ú"};
const  uint8 PY_mb_geng[]  = {"¸ü¸ı¸û¸ş¹¡¹¢¹£"};
const  uint8 PY_mb_gong[]  = {"¹¤¹­¹«¹¦¹¥¹©¹¬¹§¹ª¹¨¹®¹¯¹°¹²¹±"};
const  uint8 PY_mb_gou[]   = {"¹´¹µ¹³¹·¹¶¹¹¹º¹¸¹»"};
const  uint8 PY_mb_gu[]    = {"¹À¹¾¹Ã¹Â¹Á¹½¹¼¹¿¹Å¹È¹É¹Ç¹Æ¹Ä¹Ì¹Ê¹Ë¹Í"};
const  uint8 PY_mb_gua[]   = {"¹Ï¹Î¹Ğ¹Ñ¹Ò¹Ó"};
const  uint8 PY_mb_guai[]  = {"¹Ô¹Õ¹Ö"};
const  uint8 PY_mb_guan[]  = {"¹Ø¹Û¹Ù¹Ú¹×¹İ¹Ü¹á¹ß¹à¹Ş"};
const  uint8 PY_mb_guang[] = {"¹â¹ã¹ä"};
const  uint8 PY_mb_gui[]   = {"¹é¹ç¹ê¹æ¹ë¹è¹å¹ì¹î¹ï¹í¹ô¹ñ¹ó¹ğ¹ò"};
const  uint8 PY_mb_gun[]   = {"¹õ¹ö¹÷"};
const  uint8 PY_mb_guo[]   = {"¹ù¹ø¹ú¹û¹ü¹ı"};
const  uint8 PY_mb_ha[]    = {"¸ò¹ş"};
const  uint8 PY_mb_hai[]   = {"º¢º¡º£º¥º§º¦º¤"};
const  uint8 PY_mb_han[]   = {"º¨º©º¬ºªº¯º­º®º«º±º°ººº¹ºµº·º´º¸º¶º³º²"};
const  uint8 PY_mb_hang[]  = {"º¼º½ĞĞ"};
const  uint8 PY_mb_hao[]   = {"ºÁºÀº¿º¾ºÃºÂºÅºÆºÄ"};
const  uint8 PY_mb_he[]    = {"ºÇºÈºÌºÏºÎºÍºÓºÒºËºÉºÔºĞºÊºØºÖºÕº×"};
const  uint8 PY_mb_hei[]   = {"ºÚºÙ"};
const  uint8 PY_mb_hen[]   = {"ºÛºÜºİºŞ"};
const  uint8 PY_mb_heng[]  = {"ºàºßºãºáºâ"};
const  uint8 PY_mb_hong[]  = {"ºäºåºæºëºìºêºéºçºè"};
const  uint8 PY_mb_hou[]   = {"ºîºíºïºğºóºñºò"};
const  uint8 PY_mb_hu[]    = {"ºõºôºö»¡ºüºúºøºşºùº÷ºıºû»¢»£»¥»§»¤»¦"};
const  uint8 PY_mb_hua[]   = {"»¨»ª»©»¬»«»¯»®»­»°"};
const  uint8 PY_mb_huai[]  = {"»³»²»´»±»µ"};
const  uint8 PY_mb_huan[]  = {"»¶»¹»·»¸»º»Ã»Â»½»»»Á»¼»À»¾»¿"};
const  uint8 PY_mb_huang[] = {"»Ä»Å»Ê»Ë»Æ»Ì»Í»È»Ç»É»Ğ»Î»Ñ»Ï"};
const  uint8 PY_mb_hui[]   = {"»Ò»Ö»Ó»Ô»Õ»Ø»×»Ú»Ü»ã»á»ä»æ»å»â»ß»Ş»à»İ»Ù»Û"};
const  uint8 PY_mb_hun[]   = {"»è»ç»é»ë»ê»ì"};
const  uint8 PY_mb_huo[]   = {"»í»î»ğ»ï»ò»õ»ñ»ö»ó»ô"};
const  uint8 PY_mb_ji[]    = {"¼¥»÷¼¢»ø»ú¼¡¼¦¼£¼§»ı»ù¼¨¼©»û»ş»ü¼¤¼°¼ª¼³¼¶¼´¼«¼±¼²¼¬¼¯¼µ¼­¼®¼¸¼º¼·¼¹¼Æ¼Ç¼¿¼Í¼Ë¼É¼¼¼Ê¼Á¼¾¼È¼Ã¼Ì¼Å¼Ä¼Â¼À¼»¼½½å"};
const  uint8 PY_mb_jia[]   = {"¼Ó¼Ğ¼Ñ¼Ï¼Ò¼Î¼Ô¼Õ¼×¼Ö¼Ø¼Û¼İ¼Ü¼Ù¼Ş¼ÚĞ®"};
const  uint8 PY_mb_jian[]  = {"¼é¼â¼á¼ß¼ä¼ç¼è¼æ¼à¼ã¼ê¼å¼ğ¼ó¼í¼ë¼ñ¼õ¼ô¼ì¼ï¼ò¼î¼û¼ş½¨½¤½£¼ö¼ú½¡½§½¢½¥½¦¼ù¼ø¼ü¼ı"};
const  uint8 PY_mb_jiang[] = {"½­½ª½«½¬½©½®½²½±½°½¯½³½µ½´"};
const  uint8 PY_mb_jiao[]  = {"½»½¼½¿½½½¾½º½·½¹½¶½¸½Ç½Æ½Ê½È½Ã½Å½Â½Á½Ë½É½Ğ½Î½Ï½Ì½Ñ½Í¾õ½À"};
const  uint8 PY_mb_jie[]   = {"½×½Ô½Ó½Õ½Ò½Ö½Ú½Ù½Ü½à½á½İ½Ş½Ø½ß½ã½â½é½ä½æ½ì½ç½ê½ë½è"};
const  uint8 PY_mb_jin[]   = {"½í½ñ½ï½ğ½ò½î½ó½ö½ô½÷½õ¾¡¾¢½ü½ø½ú½ş½ı½û½ù"};
const  uint8 PY_mb_jing[]  = {"¾©¾­¾¥¾£¾ª¾§¾¦¾¬¾¤¾«¾¨¾®¾±¾°¾¯¾»¾¶¾·¾º¾¹¾´¾¸¾³¾²¾µ"};
const  uint8 PY_mb_jiong[] = {"¾¼¾½"};
const  uint8 PY_mb_jiu[]   = {"¾À¾¿¾¾¾Å¾Ã¾Ä¾Á¾Â¾Æ¾É¾Ê¾Ì¾Î¾Ç¾È¾Í¾Ë"};
const  uint8 PY_mb_ju[]    = {"¾Ó¾Ğ¾Ñ¾Ô¾Ò¾Ï¾Ö½Û¾Õ¾×¾Ú¾Ù¾Ø¾ä¾Ş¾Ü¾ß¾æ¾ã¾ç¾å¾İ¾à¾â¾Û¾á"};
const  uint8 PY_mb_juan[]  = {"¾ê¾è¾é¾í¾ë¾î¾ì"};
const  uint8 PY_mb_jue[]   = {"¾ï¾ö¾÷¾ñ¾ø¾ó¾ò¾ô¾ğ"};
const  uint8 PY_mb_jun[]   = {"¾ü¾ı¾ù¾û¾ú¿¡¿¤¾ş¿£¿¥¿¢"};
const  uint8 PY_mb_ka[]    = {"¿§¿¦¿¨"};
const  uint8 PY_mb_kai[]   = {"¿ª¿«¿­¿®¿¬"};
const  uint8 PY_mb_kan[]   = {"¼÷¿¯¿±¿°¿²¿³¿´"};
const  uint8 PY_mb_kang[]  = {"¿µ¿¶¿·¿¸¿º¿¹¿»"};
const  uint8 PY_mb_kao[]   = {"¿¼¿½¿¾¿¿"};
const  uint8 PY_mb_ke[]    = {"¿À¿Á¿Â¿Æ¿Ã¿Å¿Ä¿Ç¿È¿É¿Ê¿Ë¿Ì¿Í¿Î"};
const  uint8 PY_mb_ken[]   = {"¿Ï¿Ñ¿Ò¿Ğ"};
const  uint8 PY_mb_keng[]  = {"¿Ô¿Ó"};
const  uint8 PY_mb_kong[]  = {"¿Õ¿×¿Ö¿Ø"};
const  uint8 PY_mb_kou[]   = {"¿Ù¿Ú¿Û¿Ü"};
const  uint8 PY_mb_ku[]    = {"¿İ¿Ş¿ß¿à¿â¿ã¿á"};
const  uint8 PY_mb_kua[]   = {"¿ä¿å¿æ¿è¿ç"};
const  uint8 PY_mb_kuai[]  = {"¿é¿ì¿ë¿ê"};
const  uint8 PY_mb_kuan[]  = {"¿í¿î"};
const  uint8 PY_mb_kuang[] = {"¿ï¿ğ¿ñ¿ö¿õ¿ó¿ò¿ô"};
const  uint8 PY_mb_kui[]   = {"¿÷¿ù¿ø¿ú¿ü¿û¿ı¿şÀ¢À£À¡"};
const  uint8 PY_mb_kun[]   = {"À¤À¥À¦À§"};
const  uint8 PY_mb_kuo[]   = {"À©À¨À«Àª"};
const  uint8 PY_mb_la[]    = {"À¬À­À²À®À°À¯À±"};
const  uint8 PY_mb_lai[]   = {"À´À³Àµ"};
const  uint8 PY_mb_lan[]   = {"À¼À¹À¸À·À»À¶À¾À½ÀºÀÀÀ¿ÀÂÀÁÀÃÀÄ"};
const  uint8 PY_mb_lang[]  = {"ÀÉÀÇÀÈÀÅÀÆÀÊÀË"};
const  uint8 PY_mb_lao[]   = {"ÀÌÀÍÀÎÀÏÀĞÀÑÀÔÀÓÀÒ"};
const  uint8 PY_mb_le[]    = {"ÀÖÀÕÁË"};
const  uint8 PY_mb_lei[]   = {"À×ÀØÀİÀÚÀÙÀÜÀßÀáÀàÀÛÀŞ"};
const  uint8 PY_mb_leng[]  = {"ÀâÀãÀä"};
const  uint8 PY_mb_li[]    = {"ÀåÀæÀêÀëÀòÀçÀìÁ§ÀèÀéÀñÀîÀïÁ¨ÀíÀğÁ¦ÀúÀ÷Á¢ÀôÀöÀûÀøÁ¤ÀıÁ¥ÀşÀóÀõÀùÁ£ÀüÁ¡"};
const  uint8 PY_mb_lian[]  = {"Á¬Á±Á¯Á°Á«ÁªÁ®Á­Á²Á³Á·Á¶ÁµÁ´"};
const  uint8 PY_mb_liang[] = {"Á©Á¼Á¹ÁºÁ¸Á»Á½ÁÁÁÂÁ¾ÁÀÁ¿"};
const  uint8 PY_mb_liao[]  = {"ÁÊÁÉÁÆÁÄÁÅÁÈÁÎÁÃÁÇÁÍÁÏÁÌ"};
const  uint8 PY_mb_lie[]   = {"ÁĞÁÓÁÒÁÔÁÑ"};
const  uint8 PY_mb_lin[]   = {"ÁÚÁÖÁÙÁÜÁÕÁØÁ×ÁÛÁİÁßÁŞÁà"};
const  uint8 PY_mb_ling[]  = {"ÁæÁéÁëÁáÁèÁåÁêÁçÁâÁãÁäÁìÁîÁí"};
const  uint8 PY_mb_liu[]   = {"ÁïÁõÁ÷ÁôÁğÁòÁóÁñÁöÁøÁù"};
const  uint8 PY_mb_long[]  = {"ÁúÁüÁıÁûÂ¡ÁşÂ¤Â¢Â£"};
const  uint8 PY_mb_lou[]   = {"Â¦Â¥Â§Â¨ÂªÂ©"};
const  uint8 PY_mb_lu[]    = {"Â¶Â¬Â®Â«Â¯Â­Â±Â²Â°Â³Â½Â¼Â¸Â¹Â»ÂµÂ·Â¾ÂºÂ´"};
const  uint8 PY_mb_luan[]  = {"ÂÏÂÍÂÎÂĞÂÑÂÒ"};
const  uint8 PY_mb_lue[]   = {"ÂÓÂÔ"};
const  uint8 PY_mb_lun[]   = {"ÂÕÂØÂ×ÂÙÂÚÂÖÂÛ"};
const  uint8 PY_mb_luo[]   = {"ÂŞÂÜÂßÂàÂáÂâÂİÂãÂåÂçÂæÂä"};
const  uint8 PY_mb_lv[]    = {"ÂËÂ¿ÂÀÂÂÂÃÂÁÂÅÂÆÂÄÂÉÂÇÂÊÂÌÂÈ"};
const  uint8 PY_mb_ma[]    = {"ÂèÂéÂíÂêÂëÂìÂîÂğÂï"};
const  uint8 PY_mb_mai[]   = {"ÂñÂòÂõÂóÂôÂö"};
const  uint8 PY_mb_man[]   = {"ÂùÂøÂ÷ÂúÂüÃ¡ÂıÂşÂû"};
const  uint8 PY_mb_mang[]  = {"Ã¦Ã¢Ã¤Ã£Ã§Ã¥"};
const  uint8 PY_mb_mao[]   = {"Ã¨Ã«Ã¬Ã©ÃªÃ®Ã­Ã¯Ã°Ã³Ã±Ã²"};
const  uint8 PY_mb_me[]    = {"Ã´"};
const  uint8 PY_mb_mei[]   = {"Ã»Ã¶ÃµÃ¼Ã·Ã½ÃºÃ¸Ã¹Ã¿ÃÀÃ¾ÃÃÃÁÃÄÃÂ"};
const  uint8 PY_mb_men[]   = {"ÃÅÃÆÃÇ"};
const  uint8 PY_mb_meng[]  = {"ÃÈÃËÃÊÃÍÃÉÃÌÃÏÃÎ"};
const  uint8 PY_mb_mi[]    = {"ÃÖÃÔÃÕÃÑÃÓÃÒÃ×ÃĞÃÚÃÙÃØÃÜÃİÃÛ"};
const  uint8 PY_mb_mian[]  = {"ÃßÃàÃŞÃâÃãÃäÃáÃåÃæ"};
const  uint8 PY_mb_miao[]  = {"ÃçÃèÃéÃëÃìÃêÃîÃí"};
const  uint8 PY_mb_mie[]   = {"ÃğÃï"};
const  uint8 PY_mb_min[]   = {"ÃñÃóÃòÃöÃõÃô"};
const  uint8 PY_mb_ming[]  = {"ÃûÃ÷ÃùÃúÃøÃü"};
const  uint8 PY_mb_miu[]   = {"Ãı"};
const  uint8 PY_mb_mo[]    = {"ºÑÃşÄ¡Ä£Ä¤Ä¦Ä¥Ä¢Ä§Ä¨Ä©Ä­Ä°ÄªÄ¯Ä®Ä«Ä¬"};
const  uint8 PY_mb_mou[]   = {"Ä²Ä±Ä³"};
const  uint8 PY_mb_mu[]    = {"Ä¸Ä¶ÄµÄ·Ä´Ä¾Ä¿ÄÁÄ¼Ä¹Ä»ÄÀÄ½ÄºÄÂ"};
const  uint8 PY_mb_na[]    = {"ÄÃÄÄÄÇÄÉÄÈÄÆÄÅ"};
const  uint8 PY_mb_nai[]   = {"ÄËÄÌÄÊÄÎÄÍ"};
const  uint8 PY_mb_nan[]   = {"ÄĞÄÏÄÑ"};
const  uint8 PY_mb_nang[]  = {"ÄÒ"};
const  uint8 PY_mb_nao[]   = {"ÄÓÄÕÄÔÄÖÄ×"};
const  uint8 PY_mb_ne[]    = {"ÄØ"};
const  uint8 PY_mb_nei[]   = {"ÄÚÄÙ"};
const  uint8 PY_mb_nen[]   = {"ÄÛ"};
const  uint8 PY_mb_neng[]  = {"ÄÜ"};
const  uint8 PY_mb_ni[]    = {"ÄİÄáÄàÄßÄŞÄãÄâÄæÄäÄçÄå"};
const  uint8 PY_mb_nian[]  = {"ÄéÄêÄíÄìÄëÄîÄè"};
const  uint8 PY_mb_niang[] = {"ÄïÄğ"};
const  uint8 PY_mb_niao[]  = {"ÄñÄò"};
const  uint8 PY_mb_nie[]   = {"ÄóÄùÄôÄöÄ÷ÄøÄõ"};
const  uint8 PY_mb_nin[]   = {"Äú"};
const  uint8 PY_mb_ning[]  = {"ÄşÅ¡ÄüÄûÄıÅ¢"};
const  uint8 PY_mb_niu[]   = {"Å£Å¤Å¦Å¥"};
const  uint8 PY_mb_nong[]  = {"Å©Å¨Å§Åª"};
const  uint8 PY_mb_nu[]    = {"Å«Å¬Å­"};
const  uint8 PY_mb_nuan[]  = {"Å¯"};
const  uint8 PY_mb_nue[]   = {"Å±Å°"};
const  uint8 PY_mb_nuo[]   = {"Å²ÅµÅ³Å´"};
const  uint8 PY_mb_nv[]    = {"Å®"};
const  uint8 PY_mb_o[]     = {"Å¶"};
const  uint8 PY_mb_ou[]    = {"Å·Å¹Å¸Å»Å¼ÅºÅ½"};
const  uint8 PY_mb_pa[]    = {"Å¿Å¾ÅÀ°ÒÅÃÅÁÅÂ"};
const  uint8 PY_mb_pai[]   = {"ÅÄÅÇÅÅÅÆÅÉÅÈ"};
const  uint8 PY_mb_pan[]   = {"ÅËÅÊÅÌÅÍÅĞÅÑÅÎÅÏ"};
const  uint8 PY_mb_pang[]  = {"ÅÒÅÓÅÔÅÕÅÖ"};
const  uint8 PY_mb_pao[]   = {"Å×ÅÙÅØÅÚÅÛÅÜÅİ"};
const  uint8 PY_mb_pei[]   = {"ÅŞÅßÅãÅàÅâÅáÅæÅåÅä"};
const  uint8 PY_mb_pen[]   = {"ÅçÅè"};
const  uint8 PY_mb_peng[]  = {"ÅêÅéÅëÅóÅíÅïÅğÅîÅôÅìÅñÅòÅõÅö"};
const  uint8 PY_mb_pi[]    = {"±ÙÅúÅ÷ÅûÅøÅüÅùÆ¤ÅşÆ£Æ¡ÅıÆ¢Æ¥Æ¦Æ¨Æ§Æ©"};
const  uint8 PY_mb_pian[]  = {"Æ¬Æ«ÆªÆ­"};
const  uint8 PY_mb_piao[]  = {"Æ¯Æ®Æ°Æ±"};
const  uint8 PY_mb_pie[]   = {"Æ²Æ³"};
const  uint8 PY_mb_pin[]   = {"Æ´Æ¶ÆµÆ·Æ¸"};
const  uint8 PY_mb_ping[]  = {"Æ¹Æ½ÆÀÆ¾ÆºÆ»ÆÁÆ¿Æ¼"};
const  uint8 PY_mb_po[]    = {"ÆÂÆÃÆÄÆÅÆÈÆÆÆÉÆÇ"};
const  uint8 PY_mb_pou[]   = {"ÆÊ"};
const  uint8 PY_mb_pu[]    = {"¸¬ÆÍÆËÆÌÆÎÆĞÆÏÆÑÆÓÆÔÆÒÆÖÆÕÆ×ÆØ"};
const  uint8 PY_mb_qi[]    = {"ÆßÆãÆŞÆâÆàÆÜÆİÆÚÆÛÆáÆîÆëÆäÆæÆçÆíÆêÆéÆèÆïÆåÆìÆòÆóÆñÆôÆğÆøÆıÆùÆúÆûÆüÆõÆöÆ÷"};
const  uint8 PY_mb_qia[]   = {"ÆşÇ¡Ç¢"};
const  uint8 PY_mb_qian[]  = {"Ç§ÇªÇ¤Ç¨Ç¥Ç£Ç¦Ç«Ç©Ç°Ç®Ç¯Ç¬Ç±Ç­Ç³Ç²Ç´Ç·ÇµÇ¶Ç¸"};
const  uint8 PY_mb_qiang[] = {"ÇºÇ¼Ç¹Ç»Ç¿Ç½Ç¾ÇÀ"};
const  uint8 PY_mb_qiao[]  = {"ÇÄÇÃÇÂÇÁÇÇÇÈÇÅÇÆÇÉÇÎÇÍÇÏÇÌÇËÇÊ"};
const  uint8 PY_mb_qie[]   = {"ÇĞÇÑÇÒÇÓÇÔ"};
const  uint8 PY_mb_qin[]   = {"Ç×ÇÖÇÕÇÛÇØÇÙÇİÇÚÇÜÇŞÇß"};
const  uint8 PY_mb_qing[]  = {"ÇàÇâÇáÇãÇäÇåÇéÇçÇèÇæÇêÇëÇì"};
const  uint8 PY_mb_qiong[] = {"ÇîÇí"};
const  uint8 PY_mb_qiu[]   = {"ÇğÇñÇïÇôÇóÇöÇõÇò"};
const  uint8 PY_mb_qu[]    = {"ÇøÇúÇıÇüÇùÇûÇ÷ÇşÈ¡È¢È£È¥È¤"};
const  uint8 PY_mb_quan[]  = {"È¦È«È¨ÈªÈ­È¬È©È§È®È°È¯"};
const  uint8 PY_mb_que[]   = {"È²È±È³È´È¸È·ÈµÈ¶"};
const  uint8 PY_mb_qun[]   = {"È¹Èº"};
const  uint8 PY_mb_ran[]   = {"È»È¼È½È¾"};
const  uint8 PY_mb_rang[]  = {"È¿ÈÂÈÀÈÁÈÃ"};
const  uint8 PY_mb_rao[]   = {"ÈÄÈÅÈÆ"};
const  uint8 PY_mb_re[]    = {"ÈÇÈÈ"};
const  uint8 PY_mb_ren[]   = {"ÈËÈÊÈÉÈÌÈĞÈÏÈÎÈÒÈÑÈÍ"};
const  uint8 PY_mb_reng[]  = {"ÈÓÈÔ"};
const  uint8 PY_mb_ri[]    = {"ÈÕ"};
const  uint8 PY_mb_rong[]  = {"ÈÖÈŞÈ×ÈÙÈİÈÜÈØÈÛÈÚÈß"};
const  uint8 PY_mb_rou[]   = {"ÈáÈàÈâ"};
const  uint8 PY_mb_ru[]    = {"ÈçÈãÈåÈæÈäÈêÈéÈèÈëÈì"};
const  uint8 PY_mb_ruan[]  = {"ÈîÈí"};
const  uint8 PY_mb_rui[]   = {"ÈïÈñÈğ"};
const  uint8 PY_mb_run[]   = {"ÈòÈó"};
const  uint8 PY_mb_ruo[]   = {"ÈôÈõ"};
const  uint8 PY_mb_sa[]    = {"ÈöÈ÷Èø"};
const  uint8 PY_mb_sai[]   = {"ÈûÈùÈúÈü"};
const  uint8 PY_mb_san[]   = {"ÈıÈşÉ¡É¢"};
const  uint8 PY_mb_sang[]  = {"É£É¤É¥"};
const  uint8 PY_mb_sao[]   = {"É¦É§É¨É©"};
const  uint8 PY_mb_se[]    = {"É«É¬Éª"};
const  uint8 PY_mb_sen[]   = {"É­"};
const  uint8 PY_mb_seng[]  = {"É®"};
const  uint8 PY_mb_sha[]   = {"É±É³É´É°É¯ÉµÉ¶É·ÏÃ"};
const  uint8 PY_mb_shai[]  = {"É¸É¹"};
const  uint8 PY_mb_shan[]  = {"É½É¾É¼ÉÀÉºÉ¿ÉÁÉÂÉÇÉ»ÉÈÉÆÉÉÉÃÉÅÉÄÕ¤"};
const  uint8 PY_mb_shang[] = {"ÉËÉÌÉÊÉÑÉÎÉÍÉÏÉĞ"};
const  uint8 PY_mb_shao[]  = {"ÉÓÉÒÉÕÉÔÉ×ÉÖÉØÉÙÉÛÉÜÉÚ"};
const  uint8 PY_mb_she[]   = {"ÉİÉŞÉàÉßÉáÉèÉçÉäÉæÉâÉåÉã"};
const  uint8 PY_mb_shen[]  = {"ÉêÉìÉíÉëÉğÉïÉéÉîÉñÉòÉóÉôÉöÉõÉøÉ÷Ê²"};
const  uint8 PY_mb_sheng[] = {"ÉıÉúÉùÉüÊ¤ÉûÉşÊ¡Ê¥Ê¢Ê£"};
const  uint8 PY_mb_shi[]   = {"³×Ê¬Ê§Ê¦Ê­Ê«Ê©Ê¨ÊªÊ®Ê¯Ê±Ê¶ÊµÊ°Ê´Ê³Ê·Ê¸Ê¹Ê¼Ê»ÊºÊ¿ÊÏÊÀÊËÊĞÊ¾Ê½ÊÂÊÌÊÆÊÓÊÔÊÎÊÒÊÑÊÃÊÇÊÁÊÊÊÅÊÍÊÈÊÄÊÉËÆ"};
const  uint8 PY_mb_shou[]  = {"ÊÕÊÖÊØÊ×ÊÙÊÜÊŞÊÛÊÚÊİ"};
const  uint8 PY_mb_shu[]   = {"ÊéÊãÊåÊàÊâÊáÊçÊèÊæÊäÊßÊëÊêÊìÊîÊòÊğÊóÊñÊíÊïÊõÊùÊøÊöÊ÷ÊúË¡ÊüÊıÊûÊşÊô"};
const  uint8 PY_mb_shua[]  = {"Ë¢Ë£"};
const  uint8 PY_mb_shuai[] = {"Ë¥Ë¤Ë¦Ë§"};
const  uint8 PY_mb_shuan[] = {"Ë©Ë¨"};
const  uint8 PY_mb_shuang[] = {"Ë«ËªË¬"};
const  uint8 PY_mb_shui[]  = {"Ë­Ë®Ë°Ë¯"};
const  uint8 PY_mb_shun[]  = {"Ë±Ë³Ë´Ë²"};
const  uint8 PY_mb_shuo[]  = {"ËµË¸Ë·Ë¶"};
const  uint8 PY_mb_si[]    = {"Ë¿Ë¾Ë½Ë¼Ë¹Ë»ËºËÀËÈËÄËÂËÅËÇËÃËÁ"};
const  uint8 PY_mb_song[]  = {"ËÉËËËÊËÏËÎËĞËÍËÌ"};
const  uint8 PY_mb_sou[]   = {"ËÔËÑËÒËÓ"};
const  uint8 PY_mb_su[]    = {"ËÕËÖË×ËßËàËØËÙËÚËÜËİËÛ"};
const  uint8 PY_mb_suan[]  = {"ËáËâËã"};
const  uint8 PY_mb_sui[]   = {"ËäËçËåËæËèËêËîËìËéËíËë"};
const  uint8 PY_mb_sun[]   = {"ËïËğËñ"};
const  uint8 PY_mb_suo[]   = {"ËôËóËòËõËùË÷ËöËø"};
const  uint8 PY_mb_ta[]    = {"ËıËûËüËúËşÌ¡Ì¢Ì¤Ì£"};
const  uint8 PY_mb_tai[]   = {"Ì¥Ì¨Ì§Ì¦Ì«Ì­Ì¬Ì©Ìª"};
const  uint8 PY_mb_tan[]   = {"Ì®Ì°Ì¯Ì²Ì±Ì³Ì¸ÌµÌ·Ì¶Ì´Ì¹Ì»ÌºÌ¾Ì¿Ì½Ì¼"};
const  uint8 PY_mb_tang[]  = {"ÌÀÌÆÌÃÌÄÌÁÌÂÌÅÌÇÌÈÌÊÌÉÌÌÌË"};
const  uint8 PY_mb_tao[]   = {"ÌÎÌĞÌÍÌÏÌÓÌÒÌÕÌÔÌÑÌÖÌ×"};
const  uint8 PY_mb_te[]    = {"ÌØ"};
const  uint8 PY_mb_teng[]  = {"ÌÛÌÚÌÜÌÙ"};
const  uint8 PY_mb_ti[]    = {"ÌŞÌİÌàÌßÌäÌáÌâÌãÌåÌëÌêÌéÌèÌæÌç"};
const  uint8 PY_mb_tian[]  = {"ÌìÌíÌïÌñÌğÌîÌóÌò"};
const  uint8 PY_mb_tiao[]  = {"µ÷ÌôÌõÌöÌ÷Ìø"};
const  uint8 PY_mb_tie[]   = {"ÌùÌúÌû"};
const  uint8 PY_mb_ting[]  = {"ÌüÍ¡ÌıÌşÍ¢Í¤Í¥Í£Í¦Í§"};
const  uint8 PY_mb_tong[]  = {"Í¨Í¬Í®Í©Í­Í¯ÍªÍ«Í³Í±Í°Í²Í´"};
const  uint8 PY_mb_tou[]   = {"ÍµÍ·Í¶Í¸"};
const  uint8 PY_mb_tu[]    = {"Í¹ÍºÍ»Í¼Í½Í¿Í¾ÍÀÍÁÍÂÍÃ"};
const  uint8 PY_mb_tuan[]  = {"ÍÄÍÅ"};
const  uint8 PY_mb_tui[]   = {"ÍÆÍÇÍÈÍËÍÉÍÊ"};
const  uint8 PY_mb_tun[]   = {"¶ÚÍÌÍÍÍÎ"};
const  uint8 PY_mb_tuo[]   = {"ÍĞÍÏÍÑÍÔÍÓÍÕÍÒÍ×ÍÖÍØÍÙ"};
const  uint8 PY_mb_wa[]    = {"ÍÛÍŞÍÚÍİÍÜÍßÍà"};
const  uint8 PY_mb_wai[]   = {"ÍáÍâ"};
const  uint8 PY_mb_wan[]   = {"ÍäÍåÍãÍèÍêÍæÍçÍéÍğÍìÍíÍñÍïÍîÍëÍòÍó"};
const  uint8 PY_mb_wang[]  = {"ÍôÍöÍõÍøÍùÍ÷ÍıÍüÍúÍû"};
const  uint8 PY_mb_wei[]   = {"Î£ÍşÎ¢Î¡ÎªÎ¤Î§Î¥Î¦Î¨Î©Î¬Î«Î°Î±Î²Î³Î­Î¯Î®ÎÀÎ´Î»Î¶Î·Î¸Î¾Î½Î¹Î¼ÎµÎ¿Îº"};
const  uint8 PY_mb_wen[]   = {"ÎÂÎÁÎÄÎÆÎÅÎÃÎÇÎÉÎÈÎÊ"};
const  uint8 PY_mb_weng[]  = {"ÎÌÎËÎÍ"};
const  uint8 PY_mb_wo[]    = {"ÎÎÎĞÎÑÎÏÎÒÎÖÎÔÎÕÎÓ"};
const  uint8 PY_mb_wu[]    = {"ÎÚÎÛÎØÎ×ÎİÎÜÎÙÎŞÎãÎâÎáÎßÎàÎåÎçÎéÎëÎäÎêÎæÎèÎğÎñÎìÎïÎóÎòÎîÎí"};
const  uint8 PY_mb_xi[]    = {"Ï¦Ï«Î÷ÎüÏ£ÎôÎöÎùÏ¢ÎşÏ¤Ï§Ï©ÎøÎúÏ¬Ï¡ÏªÎıÏ¨ÎõÎûÏ¥Ï°Ï¯Ï®Ï±Ï­Ï´Ï²Ï·ÏµÏ¸Ï¶"};
const  uint8 PY_mb_xia[]   = {"ÏºÏ¹Ï»ÏÀÏ¿ÏÁÏ¾Ï½Ï¼ÏÂÏÅÏÄ"};
const  uint8 PY_mb_xian[]  = {"Ï³ÏÉÏÈÏËÏÆÏÇÏÊÏĞÏÒÏÍÏÌÏÑÏÏÏÎÏÓÏÔÏÕÏØÏÖÏßÏŞÏÜÏİÏÚÏÛÏ×ÏÙ"};
const  uint8 PY_mb_xiang[] = {"ÏçÏàÏãÏáÏæÏäÏåÏâÏêÏéÏèÏíÏìÏëÏòÏïÏîÏóÏñÏğ"};
const  uint8 PY_mb_xiao[]  = {"ÏüÏûÏôÏõÏúÏöÏùÏıĞ¡ÏşĞ¢Ğ¤ÏøĞ§Ğ£Ğ¦Ğ¥"};
const  uint8 PY_mb_xie[]   = {"Ğ©Ğ¨ĞªĞ«Ğ­Ğ°Ğ²Ğ±Ğ³Ğ¯Ğ¬Ğ´Ğ¹ĞºĞ¶Ğ¼ĞµĞ»Ğ¸Ğ·"};
const  uint8 PY_mb_xin[]   = {"ĞÄĞÃĞ¾ĞÁĞÀĞ¿ĞÂĞ½ĞÅĞÆ"};
const  uint8 PY_mb_xing[]  = {"ĞËĞÇĞÊĞÉĞÈĞÌĞÏĞÎĞÍĞÑĞÓĞÕĞÒĞÔ"};
const  uint8 PY_mb_xiong[] = {"Ğ×ĞÖĞÙĞÚĞØĞÛĞÜ"};
const  uint8 PY_mb_xiu[]   = {"ËŞĞİĞŞĞßĞàĞãĞåĞäĞâĞá"};
const  uint8 PY_mb_xu[]    = {"ĞçĞëĞéĞêĞèĞæĞìĞíĞñĞòĞğĞôĞ÷ĞøĞïĞöĞõĞîÓõ"};
const  uint8 PY_mb_xuan[]  = {"ĞùĞûĞúĞşĞüĞıÑ¡Ñ¢Ñ¤Ñ£"};
const  uint8 PY_mb_xue[]   = {"Ï÷Ñ¥Ñ¦Ñ¨Ñ§Ñ©Ñª"};
const  uint8 PY_mb_xun[]   = {"Ñ«Ñ¬Ñ°Ñ²Ñ®Ñ±Ñ¯Ñ­ÑµÑ¶Ñ´Ñ¸Ñ·Ñ³"};
const  uint8 PY_mb_ya[]    = {"Ñ¾Ñ¹Ñ½ÑºÑ»Ñ¼ÑÀÑ¿ÑÁÑÂÑÄÑÃÑÆÑÅÑÇÑÈ"};
const  uint8 PY_mb_yan[]   = {"ÑÊÑÌÑÍÑÉÑËÑÓÑÏÑÔÑÒÑØÑ×ÑĞÑÎÑÖÑÑÑÕÑÙÑÜÑÚÑÛÑİÑáÑåÑâÑäÑçÑŞÑéÑèÑßÑæÑãÑà"};
const  uint8 PY_mb_yang[]  = {"ÑëÑêÑíÑìÑïÑòÑôÑîÑğÑñÑóÑöÑøÑõÑ÷ÑùÑú"};
const  uint8 PY_mb_yao[]   = {"½ÄÑıÑüÑûÒ¢Ò¦Ò¤Ò¥Ò¡Ò£ÑşÒ§Ò¨Ò©ÒªÒ«Ô¿"};
const  uint8 PY_mb_ye[]    = {"Ò¬Ò­Ò¯Ò®Ò²Ò±Ò°ÒµÒ¶Ò·Ò³Ò¹Ò´ÒºÒ¸"};
const  uint8 PY_mb_yi[]    = {"Ò»ÒÁÒÂÒ½ÒÀÒ¿Ò¼Ò¾ÒÇÒÄÒÊÒËÒÌÒÈÒÆÒÅÒÃÒÉÒÍÒÒÒÑÒÔÒÓÒÏÒĞÒÎÒåÒÚÒäÒÕÒéÒàÒÙÒìÒÛÒÖÒëÒØÒ×ÒïÒèÒßÒæÒêÒîÒİÒâÒçÒŞÒáÒãÒíÒÜ"};
const  uint8 PY_mb_yin[]   = {"ÒòÒõÒöÒğÒñÒôÒóÒ÷ÒúÒùÒøÒüÒıÒûÒşÓ¡"};
const  uint8 PY_mb_ying[]  = {"Ó¦Ó¢Ó¤Ó§Ó£Ó¥Ó­Ó¯Ó«Ó¨Ó©ÓªÓ¬Ó®Ó±Ó°Ó³Ó²"};
const  uint8 PY_mb_yo[]    = {"Ó´"};
const  uint8 PY_mb_yong[]  = {"Ó¶ÓµÓ¸Ó¹ÓºÓ·ÓÀÓ½Ó¾ÓÂÓ¿ÓÁÓ¼Ó»ÓÃ"};
const  uint8 PY_mb_you[]   = {"ÓÅÓÇÓÄÓÆÓÈÓÉÓÌÓÊÓÍÓËÓÎÓÑÓĞÓÏÓÖÓÒÓ×ÓÓÓÕÓÔ"};
const  uint8 PY_mb_yu[]    = {"ÓØÓÙÓåÓÚÓèÓàÓÛÓãÓáÓéÓæÓçÓäÓâÓŞÓÜÓİÓßÓëÓîÓìÓğÓêÓíÓïÓñÔ¦ÓóÓıÓôÓüÓøÔ¡Ô¤ÓòÓûÓ÷Ô¢ÓùÔ£ÓöÓúÓşÔ¥"};
const  uint8 PY_mb_yuan[]  = {"Ô©Ô§Ô¨ÔªÔ±Ô°Ô«Ô­Ô²Ô¬Ô®ÔµÔ´Ô³Ô¯Ô¶Ô·Ô¹ÔºÔ¸"};
const  uint8 PY_mb_yue[]   = {"Ô»Ô¼ÔÂÔÀÔÃÔÄÔ¾ÔÁÔ½"};
const  uint8 PY_mb_yun[]   = {"ÔÆÔÈÔÇÔÅÔÊÔÉÔĞÔËÔÎÔÍÔÏÔÌ"};
const  uint8 PY_mb_za[]    = {"ÔÑÔÓÔÒÕ¦"};
const  uint8 PY_mb_zai[]   = {"ÔÖÔÕÔÔÔ×ÔØÔÙÔÚ×Ğ"};
const  uint8 PY_mb_zan[]   = {"ÔÛÔÜÔİÔŞ"};
const  uint8 PY_mb_zang[]  = {"ÔßÔàÔá"};
const  uint8 PY_mb_zao[]   = {"ÔâÔãÔäÔçÔæÔéÔèÔåÔîÔíÔìÔëÔïÔê"};
const  uint8 PY_mb_ze[]    = {"ÔòÔñÔóÔğ"};
const  uint8 PY_mb_zei[]   = {"Ôô"};
const  uint8 PY_mb_zen[]   = {"Ôõ"};
const  uint8 PY_mb_zeng[]  = {"ÔöÔ÷Ôù"};
const  uint8 PY_mb_zha[]   = {"ÔûÔüÔúÔıÔşÕ¢Õ¡Õ£Õ§Õ©Õ¨Õ¥×õ"};
const  uint8 PY_mb_zhai[]  = {"Õ«ÕªÕ¬µÔÕ­Õ®Õ¯"};
const  uint8 PY_mb_zhan[]  = {"Õ´Õ±Õ³Õ²Õ°Õ¶Õ¹ÕµÕ¸Õ·Õ¼Õ½Õ»Õ¾ÕÀÕ¿Õº"};
const  uint8 PY_mb_zhang[] = {"³¤ÕÅÕÂÕÃÕÄÕÁÕÇÕÆÕÉÕÌÕÊÕÈÕÍÕËÕÏÕÎ"};
const  uint8 PY_mb_zhao[]  = {"ÕĞÕÑÕÒÕÓÕÙÕ×ÕÔÕÕÕÖÕØ×¦"};
const  uint8 PY_mb_zhe[]   = {"ÕÚÕÛÕÜÕİÕŞÕßÕàÕâÕãÕá×Å"};
const  uint8 PY_mb_zhen[]  = {"ÕêÕëÕìÕäÕæÕèÕåÕçÕéÕïÕíÕîÕóÕñÕòÕğÖ¡"};
const  uint8 PY_mb_zheng[] = {"ÕùÕ÷ÕúÕõÕøÕöÕôÕüÕûÕıÖ¤Ö£ÕşÖ¢"};
const  uint8 PY_mb_zhi[]   = {"Ö®Ö§Ö­Ö¥Ö¨Ö¦ÖªÖ¯Ö«Ö¬Ö©Ö´Ö¶Ö±ÖµÖ°Ö²Ö³Ö¹Ö»Ö¼Ö·Ö½Ö¸ÖºÖÁÖ¾ÖÆÖÄÖÎÖËÖÊÖÅÖ¿ÖÈÖÂÖÀÖÌÖÏÖÇÖÍÖÉÖÃ"};
const  uint8 PY_mb_zhong[] = {"ÖĞÖÒÖÕÖÑÖÓÖÔÖ×ÖÖÖÙÖÚÖØ"};
const  uint8 PY_mb_zhou[]  = {"ÖİÖÛÖßÖÜÖŞÖàÖáÖâÖãÖäÖæÖçÖåÖè"};
const  uint8 PY_mb_zhu[]   = {"ÖìÖïÖêÖéÖîÖíÖëÖñÖòÖğÖ÷ÖôÖóÖöÖõ×¡Öú×¢Öü×¤Öù×£ÖøÖûÖşÖı"};
const  uint8 PY_mb_zhua[]  = {"×¥"};
const  uint8 PY_mb_zhuai[] = {"×§"};
const  uint8 PY_mb_zhuan[] = {"×¨×©×ª×«×­"};
const  uint8 PY_mb_zhuang[] = {"×±×¯×®×°×³×´´±×²"};
const  uint8 PY_mb_zhui[]  = {"×·×µ×¶×¹×º×¸"};
const  uint8 PY_mb_zhun[]  = {"×»×¼"};
const  uint8 PY_mb_zhuo[]  = {"×¿×¾×½×À×Æ×Â×Ç×Ã×Ä×Á"};
const  uint8 PY_mb_zi[]    = {"×Î×È×É×Ë×Ê×Í×Ì×Ñ×Ó×Ï×Ò×Ö×Ô×Õ"};
const  uint8 PY_mb_zong[]  = {"×Ú×Û×Ø×Ù×××Ü×İ"};
const  uint8 PY_mb_zou[]   = {"×Ş×ß×à×á"};
const  uint8 PY_mb_zu[]    = {"×â×ã×ä×å×ç×è×é×æ"};
const  uint8 PY_mb_zuan[]  = {"×¬×ë×ê"};
const  uint8 PY_mb_zui[]   = {"×ì×î×ï×í"};
const  uint8 PY_mb_zun[]   = {"×ğ×ñ"};
const  uint8 PY_mb_zuo[]   = {"×ò×ó×ô×÷×ø×ù×ö"};
const  uint8 PY_mb_space[] = {""};

/*"Æ´ÒôÊäÈë·¨²éÑ¯Âë±í,¶ş¼¶×ÖÄ¸Ë÷Òı±í(index)"*/
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

/*¶¨ÒåÊ××ÖÄ¸Ë÷Òı±í*/
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

    cInputStrLength = osal_strlen((char*)InputStr);  /*ÊäÈëÆ´Òô´®³¤¶È*/
    if(*InputStr == '\0')return(0);       /*Èç¹ûÊäÈë¿Õ×Ö·û·µ»Ø0*/

    for(i = 0; i < cInputStrLength; i++)
        *(InputStr + i) |= 0x20;         /*½«×ÖÄ¸´®×ªÎªĞ¡Ğ´*/

    if(*InputStr == 'i')return(0);        /*´íÎóÆ´ÒôÊäÈë*/
    if(*InputStr == 'u')return(0);
    if(*InputStr == 'v')return(0);

    cpHZ = PY_index_headletter[InputStr[0] - 'a'];    /*²éÊ××ÖÄ¸Ë÷Òı*/
    //cpHZedge=PY_index_headletter[InputStr[0]-'a'+1];  /*ÉèÖÃÖ¸Õë½çÏŞ*/
    cpHZedge = cpHZ + PY_index_size[InputStr[0]-'a'];

    InputStr++;                           /*Ö¸ÏòÆ´Òô´®µÚ¶ş¸ö×ÖÄ¸*/
    while(cpHZ < cpHZedge)                       /*Ë÷Òı±í²»³¬½ç*/
    {
        for(i = 0; i < cInputStrLength; i++)
        {
            if(*(InputStr + i) != *(cpHZ->PY + i))break; /*·¢ÏÖ×ÖÄ¸´®²»Åä,ÍË³ö*/
        }
        if(i == cInputStrLength)      /*×ÖÄ¸´®È«Åä*/
        {
            return (cpHZ->PY_mb);
        }
        cpHZ++;
    }
    return 0;                      /*ÎŞ¹û¶øÖÕ*/
}
