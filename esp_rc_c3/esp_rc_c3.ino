#include "util.h"

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);
AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *op=NULL;
int16_t v[2]={0};

void cliflush(AsyncWebSocket *ws){
  std::string x="";
  for(uint8_t i=0;i<ws->count();i++)x=x+std::to_string((*(ws->getClients().nth(i)))->id())+",";
  if(!x.empty())x.pop_back();
  ws->printfAll("[\"clis\",%s]\n",x.c_str());
  ws->printfAll("[\"op\",%u]\n",op->id());
  Serial.printf("[\"clis\",%s]\n[\"op\",%u]\n",x.c_str(),op->id());
}

void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *client,AwsEventType type,void *arg,uint8_t *data,size_t len){
	switch(type){
		case WS_EVT_CONNECT:
			Serial.printf("ws[%u] conn: %s\n",client->id(),client->remoteIP().toString().c_str());
			client->printf("[\"init\",{\"id\":%u,\"bit\":%u}]\n",client->id(),PWM_BIT);
      if(op==NULL)op=client;
      cliflush(ws);
			break;
		case WS_EVT_DISCONNECT:
			Serial.printf("ws[%u] disconn\n",client->id());
      if(ws->count()>0){
        if(op==client)op=*(ws->getClients().nth(ws->count()-1));
        cliflush(ws);
      }else op=NULL;
			v[0]=0;v[1]=0;
			break;
		case WS_EVT_PONG:Serial.printf("ws[%u] pong\n",client->id());break;
		case WS_EVT_ERROR:Serial.printf("ws[%u] error(%u): %s\n",client->id(),*((uint16_t*)arg),(char*)data);break;
		case WS_EVT_DATA:{
      AwsFrameInfo *info=(AwsFrameInfo*)arg;
      if(info->final&&info->index==0&&info->len==len&&op==client){
        if(data[0]==1){
          // op [1,op]
          AsyncWebSocketClient *tmp;
          for(uint8_t i=0;i<ws->count();i++){
            tmp=*(ws->getClients().nth(i));
            if(tmp->id()==data[1]){op=tmp;cliflush(ws);break;}
          }
        }else{
          // velocity [0,L,L,R,R]
          // BE
          v[0]=((data[1]<<1)|data[2])-PWM_MAX;
          v[1]=((data[3]<<1)|data[4])-PWM_MAX;
          ws->printfAll("[\"vel\",[%d,%d]]",v[0],v[1]);
          Serial.printf("[\"vel\",[%d,%d]]\n",v[0],v[1]);
        }
      }
    }break;
	}
}


void setup() {
  ledcSetup(I1PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I1,I1PWM);
	ledcSetup(I2PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I2,I2PWM);
	ledcSetup(I3PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I3,I3PWM);
	ledcSetup(I4PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(I4,I4PWM);
  ledcSetup(RPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(RPIN,RPWM);
  ledcSetup(GPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(GPIN,GPWM);
  ledcWrite(RPWM,PWM_MAX>>2);

  Serial.begin(115200);
	delay(1000);

  // https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
	WiFi.softAP(SSID,PASS);
	delay(100);// https://github.com/espressif/arduino-esp32/issues/985
	WiFi.softAPConfig(ip,ip,subnet);
  Serial.printf("SSID: %s\nPASS: %s\nAPIP: %s\n",SSID,PASS,WiFi.softAPIP().toString().c_str());

	ws.onEvent(onWS);
	svr.addHandler(&ws);
	svr.on("/",HTTP_GET,[](AsyncWebServerRequest *request){request->send_P(200,"text/html",html);});

	svr.begin();
	Serial.printf("svr started\n");

	ArduinoOTA.setPassword("sazanka_").begin();
  Serial.printf("OTA started\n");
}

void loop() {
  ArduinoOTA.handle();
	ws.cleanupClients();

  ledcWrite(I1PWM,max(0,+v[0]));ledcWrite(I2PWM,max(0,-v[0]));
  ledcWrite(I3PWM,max(0,+v[1]));ledcWrite(I4PWM,max(0,-v[1]));
  #if 1
    if(op){
      ledcWrite(RPWM,(v[0]+PWM_MAX)>>3);
      ledcWrite(GPWM,(v[1]+PWM_MAX)>>3);
    }else{
      ledcWrite(RPWM,0);
      ledcWrite(GPWM,(sin(millis()/1000.*3.1415)*.5+.5)*PWM_MAX/4.);
    }
  #endif
}
