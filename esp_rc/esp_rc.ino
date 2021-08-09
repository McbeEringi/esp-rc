#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>//https://github.com/me-no-dev/ESPAsyncWebServer

#define I1PWM 0
#define I2PWM 1
#define I3PWM 2
#define I4PWM 3
#include "util.h"

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

int v[2]={0,0};//L,R

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){
	switch(type){
		case WS_EVT_CONNECT:
			Serial.printf("ws[%u] connect: %s\n", client->id(), client->remoteIP().toString().c_str());
			client->printf("{\"purpose\":\"init\",\"cid\":%u,\"cip\":\"%s\",\"max\":%u,\"zpad\":%u}", client->id(), client->remoteIP().toString().c_str(),PWM_MAX,WS_ZERO_PAD);
			client->ping();
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
						v[0]=str.substring(5,5+WS_ZERO_PAD).toInt()-PWM_MAX;
						v[1]=str.substring(5+WS_ZERO_PAD,5+WS_ZERO_PAD+WS_ZERO_PAD).toInt()-PWM_MAX;
						v[0]=v[0]*5/6;
						v[1]=v[1]*5/6;
						Serial.printf("%d %d\n",v[0],v[1]);
						ws.printfAll("{\"purpose\":\"check\",\"vl\":%d,\"vr\":%d}",v[0],v[1]);
					}
				}
			}
			break;
	}
}

void setup(){
	Serial.begin(115200);
	delay(100);

	// softAP https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/src/WiFiAP.h
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(ssid,pass);
	delay(100);//https://github.com/espressif/arduino-esp32/issues/985
	WiFi.softAPConfig(ip,ip,subnet);
	Serial.printf("SSID: %s\n",ssid);//Serial.print("SSID: ");Serial.println(ssid);
	Serial.printf("PASS: %s\n",pass);//Serial.print("PASS: ");Serial.println(pass);
	Serial.printf("APIP: %s\n",WiFi.softAPIP().toString().c_str());//Serial.print("APIP: ");Serial.println(myIP);

	ws.onEvent(onEvent);
	server.addHandler(&ws);
	server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
		request->send_P(200,"text/html",html);
	});

	server.begin();
	Serial.println("server started");
	/*PACKAGE BUG? does not work after first OTA
		// OTA	https://web.is.tokushima-u.ac.jp/wp/blog/2018/04/12/esp32-arduino-softapでotaプログラム書き込み/
		ArduinoOTA.setPassword("sazanka_");
		ArduinoOTA
			.onStart([](){Serial.print("Start updating ");Serial.println(ArduinoOTA.getCommand()==U_FLASH?"sketch":"filesystem");})
			.onEnd([](){Serial.println("\nEnd");})
			.onProgress([](unsigned int progress, unsigned int total){Serial.printf("Progress: %u%%\r",(progress/(total/100)));})
			.onError([](ota_error_t error){
				if(error==OTA_AUTH_ERROR)Serial.println("Auth Error");
				else if(error==OTA_BEGIN_ERROR)Serial.println("Begin Error");
				else if(error==OTA_CONNECT_ERROR)Serial.println("Connect Error");
				else if(error==OTA_RECEIVE_ERROR)Serial.println("Receive Error");
				else if(error==OTA_END_ERROR)Serial.println("End Error");
			});
		ArduinoOTA.begin();
	*/
	ledcSetup(I1PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I1,I1PWM);
	ledcSetup(I2PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I2,I2PWM);
	ledcSetup(I3PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I3,I3PWM);
	ledcSetup(I4PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I4,I4PWM);
}

void loop(){
	//ArduinoOTA.handle();
	ws.cleanupClients();
	if(v[0]>0){ledcWrite(I3PWM,0);ledcWrite(I4PWM,v[0]);}else{ledcWrite(I3PWM,-v[0]);ledcWrite(I4PWM,0);}
	if(v[1]>0){ledcWrite(I1PWM,0);ledcWrite(I2PWM,v[1]);}else{ledcWrite(I1PWM,-v[1]);ledcWrite(I2PWM,0);}
}
