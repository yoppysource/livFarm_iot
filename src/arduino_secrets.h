
// This ID is defined when the arduino
//TODO: Before connect ardunio with planter module you need to define the ID
/* 아두이노 ID 작성 원리
	행정구역(동) 사용용도(store or farm) 재배기번호(1~)
For example,
 세곡동 매장 1번 모듈 => sgs1
 세곡동 농장 4번 모듈 => sgf4
 성복동 매장 2번 모듈 => sbs1
*/
#define SECRET_PLANTER_ID "sgf13"
#define SECREP_PLANTER_NUMBER 13
#define SECRET_PUBLIC_IP "12.123.12.3"
// #define SECREP_LOCAL_IP "2.3.2.1"
#define SECRET_PUBLIC_PORT 101

#define SECRET_SSID "segokFarm"
#define SECRET_PASSWORD "alwaysgreen12"
#define SECRET_HTTP_PORT 80

#define SECRET_CLOUD_ADDRESS "8f21-59-10-187-220.ngrok.io" // hostname of web server:
#define SECRET_CAMERA_IP "10.12.12.12"
#define SECRET_PATH_ROOT  "/"
#define SECRET_PATH_INIT  "/planters/init/"