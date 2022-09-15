#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>// https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>// https://github.com/me-no-dev/ESPAsyncWebServer

#include "util.h"
#define I1PWM 0
#define I2PWM 1
#define I3PWM 2
#define I4PWM 3
#ifdef STATLED
  #define RPWM 4
  #define GPWM 5
#endif

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
const long PWM_MAX=pow(2,PWM_BIT),_Z=0;
const int ZPAD=1+(int)log10(PWM_MAX*2);

long v[2]={0,0};//L,R

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
	switch(type){
		case WS_EVT_CONNECT:
			Serial.printf("ws[%u] connect: %s\n", client->id(), client->remoteIP().toString().c_str());
			client->printf("{\"purpose\":\"init\",\"cid\":%u,\"cip\":\"%s\",\"max\":%u,\"zpad\":%u}", client->id(), client->remoteIP().toString().c_str(), PWM_MAX, ZPAD);
			//client->ping();
			break;
		case WS_EVT_DISCONNECT:
			Serial.printf("ws[%u] disconnect\n", client->id());
			v[0]=0;v[1]=0;
			break;
		case WS_EVT_ERROR:
			Serial.printf("ws[%u] error(%u): %s\n", client->id(), *((uint16_t*)arg), (char*)data);
			break;
		case WS_EVT_PONG:
			Serial.printf("ws[%u] pong\n", client->id());
			break;
		case WS_EVT_DATA:
			{
				AwsFrameInfo *info=(AwsFrameInfo*)arg;
				if(info->final && info->index==0 && info->len==len && info->opcode==WS_TEXT){
					data[len]=0;
					String str=(char*)data;
					Serial.printf("ws[%u] text-msg[%llu]: %s\n", client->id(), info->len, str.c_str());
					ws.printfAll("{\"purpose\":\"pong\",\"pong\":\"%s\"}",str.c_str());
					if(str.startsWith("V_CTR")){
						// "V_CTR[L<int>][R<int>]"
						// 0padding needed
						// example: "V_CTR256256"
						v[0]=str.substring(5,5+ZPAD).toInt()-PWM_MAX;
						v[1]=str.substring(5+ZPAD,5+ZPAD+ZPAD).toInt()-PWM_MAX;
						//v[0]=v[0]*5/6;v[1]=v[1]*5/6;
						Serial.printf("%d %d\n",v[0],v[1]);
						ws.printfAll("{\"purpose\":\"check\",\"vl\":%d,\"vr\":%d}",v[0],v[1]);
					}
				}
			}
			break;
	}
}
//class CP:public AsyncWebHandler{
//public:
//  CaptiveRequestHandler(){}
//  virtual ~CaptiveRequestHandler(){}
//
//  bool canHandle(AsyncWebServerRequest *request){return true;}
//  void handleRequest(AsyncWebServerRequest *request){request->send_P(200,"text/html",html);}
//};

void setup(){
  #ifdef STATLED
    ledcSetup(RPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(RPIN,RPWM);
    ledcSetup(GPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(GPIN,GPWM);
    ledcWrite(RPWM,PWM_MAX/2);
  #endif
	Serial.begin(115200);
	delay(1000);

	// softAP https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiAP.h
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(ssid,pass);
	delay(100);//https://github.com/espressif/arduino-esp32/issues/985
	WiFi.softAPConfig(ip,ip,subnet);
	Serial.printf("SSID: %s\nPASS: %s\nAPIP: %s\n",ssid,pass,WiFi.softAPIP().toString().c_str());

	ws.onEvent(onEvent);
	server.addHandler(&ws);
  //server.addHandler(new CP());
	server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){request->send_P(200,"text/html",html);});
	server.begin();
	Serial.println("server started");

	ArduinoOTA
		.setPassword("sazanka_")
		.onStart([](){Serial.printf("Start updating %s\n",ArduinoOTA.getCommand()==U_FLASH?"sketch":"filesystem");})
		.onEnd([](){Serial.println("\nEnd");})
		.onProgress([](unsigned int progress, unsigned int total){Serial.printf("Progress: %u%%\r",(progress/(total/100)));})
		.onError([](ota_error_t error){Serial.printf("Error[%u]", error);})
		.begin();

	ledcSetup(I1PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I1,I1PWM);
	ledcSetup(I2PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I2,I2PWM);
	ledcSetup(I3PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I3,I3PWM);
	ledcSetup(I4PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I4,I4PWM);
}

void loop(){
	ArduinoOTA.handle();
	ws.cleanupClients();
  //short break
  ledcWrite(I1PWM,PWM_MAX-max(_Z,v[0]));ledcWrite(I2PWM,PWM_MAX-max(_Z,-v[0]));//if(v[0]>0){ledcWrite(I1PWM,PWM_MAX-v[0]);ledcWrite(I2PWM,PWM_MAX);}else{ledcWrite(I1PWM,PWM_MAX);ledcWrite(I2PWM,PWM_MAX+v[0]);}
  ledcWrite(I3PWM,PWM_MAX-max(_Z,v[1]));ledcWrite(I4PWM,PWM_MAX-max(_Z,-v[1]));//if(v[1]>0){ledcWrite(I3PWM,PWM_MAX-v[1]);ledcWrite(I4PWM,PWM_MAX);}else{ledcWrite(I3PWM,PWM_MAX);ledcWrite(I4PWM,PWM_MAX+v[1]);}
  //no break
	//ledcWrite(I1PWM,min(0,-v[0]));ledcWrite(I2PWM,max(0,v[0]));//if(v[0]>0){ledcWrite(I1PWM,0);ledcWrite(I2PWM,v[0]);}else{ledcWrite(I1PWM,-v[0]);ledcWrite(I2PWM,0);}
	//ledcWrite(I3PWM,min(0,-v[1]));ledcWrite(I4PWM,max(0,v[1]));//if(v[1]>0){ledcWrite(I3PWM,0);ledcWrite(I4PWM,v[1]);}else{ledcWrite(I3PWM,-v[1]);ledcWrite(I4PWM,0);}
  #ifdef STATLED
    if(ws.count()>0){
      ledcWrite(RPWM,(v[0]+PWM_MAX)/4);
      ledcWrite(GPWM,(v[1]+PWM_MAX)/4);
    }else{
      ledcWrite(RPWM,0);
      ledcWrite(GPWM,(sin(millis()/1000.*3.1415)*.5+.5)*PWM_MAX/2);
    }
  #endif
}
