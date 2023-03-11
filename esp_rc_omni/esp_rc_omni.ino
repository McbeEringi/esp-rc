#include "util.h"

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);
AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *op=NULL;
int16_t v[3]={0};

void flush(AsyncWebSocket *ws){// op tx [1,op,...clis]
  uint8_t l=ws->count()+2,a[l]={1,(uint8_t)op->id()};
  for(uint8_t i=2;i<l;i++)a[i]=(*(ws->getClients().nth(i-2)))->id();
  ws->binaryAll(a,l);
  Serial.printf("clis: ");
  for(uint8_t i=2;i<l;i++)Serial.printf("%u ",a[i]);
  Serial.printf(", op: %u\n",a[1]);
}

void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *client,AwsEventType type,void *arg,uint8_t *data,size_t len){
	switch(type){
		case WS_EVT_CONNECT:{// init tx [0,id,bit]
			Serial.printf("conn: %u(%s)\n",client->id(),client->remoteIP().toString().c_str());
      uint8_t l=3,a[l]={0,(uint8_t)client->id(),PWM_BIT};
			client->binary(a,l);
      if(op==NULL)op=client;
      flush(ws);
    }break;
		case WS_EVT_DISCONNECT:
			Serial.printf("disconn: %u\n",client->id());
      if(ws->count()>0){
        if(op==client)op=*(ws->getClients().nth(ws->count()-1));
        flush(ws);
      }else op=NULL;
			v[0]=0;v[1]=0;
			break;
		case WS_EVT_PONG:Serial.printf("pong: %u\n",client->id());break;
		case WS_EVT_ERROR:Serial.printf("error: %u(%s)\n",client->id(),*((uint16_t*)arg),(char*)data);break;
		case WS_EVT_DATA:{
      AwsFrameInfo *info=(AwsFrameInfo*)arg;
      if(info->final&&info->index==0&&info->len==len){
        switch(data[0]){
          case 1:{// op rx [1,op]
            if(op==client){
              AsyncWebSocketClient *tmp;
              for(uint8_t i=0;i<ws->count();i++){
                tmp=*(ws->getClients().nth(i));
                if(tmp->id()==data[1]){op=tmp;flush(ws);break;}
              }
            }
          }break;
          case 2:{// velocity rxtx [2,L,L,R,R,M,M] BigEndian
            if(op==client){
              v[0]=((data[1]<<8)|data[2])-PWM_MAX;
              v[1]=((data[3]<<8)|data[4])-PWM_MAX;
              v[2]=((data[5]<<8)|data[6])-PWM_MAX;
              ws->binaryAll(data,7);
              Serial.printf("L: %d, R: %d, M: %d\n",v[0],v[1],v[2]);
            }
          }break;
          case 3:{// txt rxtx [3,...txt]
              ws->binaryAll(data,info->len);
          }break;
        }
      }
    }break;
	}
}


void setup(){
  ledcSetup(uI1PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(uI1PIN,uI1PWM);
  ledcSetup(uI2PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(uI2PIN,uI2PWM);
  ledcSetup(uI3PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(uI3PIN,uI3PWM);
  ledcSetup(uI4PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(uI4PIN,uI4PWM);
  ledcSetup(bI3PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(bI3PIN,bI3PWM);
  ledcSetup(bI4PWM,PWM_FREQ,PWM_BIT);ledcAttachPin(bI4PIN,bI4PWM);
  pinMode(STBY,OUTPUT);digitalWrite(STBY,LOW);
  pinMode(RPIN,INPUT_PULLUP);

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
	svr.onNotFound([](AsyncWebServerRequest *request){request->send_P(302,"text/html",html);});
	svr.begin();
	Serial.printf("svr started\n");

	ArduinoOTA
    .setHostname(SSID).setPassword(PASS)
		.onStart([](){Serial.printf("update started: %s\n",ArduinoOTA.getCommand()==U_FLASH?"flash":"spiffs");ws.enable(false);ws.printfAll("update started: %s\n",ArduinoOTA.getCommand()==U_FLASH?"flash":"spiffs");ws.closeAll();})
		.onEnd([](){Serial.printf("End\n");})
		.onProgress([](unsigned int x,unsigned int a){Serial.printf("%d / %d\n",x,a);})
		.onError([](ota_error_t e){Serial.printf("update error: %u\n",e);})
		.begin();
  Serial.printf("OTA started\n");
  digitalWrite(STBY,HIGH);
  pinMode(RPIN,OUTPUT);
  pinMode(GPIN,INPUT_PULLUP);
}

void loop(){
	ws.cleanupClients();
  ArduinoOTA.handle();

  ledcWrite(uI1PWM,PWM_MAX-max(0,+v[0]));ledcWrite(uI2PWM,PWM_MAX-max(0,-v[0]));
  ledcWrite(uI3PWM,PWM_MAX-max(0,+v[1]));ledcWrite(uI4PWM,PWM_MAX-max(0,-v[1]));
  ledcWrite(bI3PWM,PWM_MAX-max(0,+v[2]));ledcWrite(bI4PWM,PWM_MAX-max(0,-v[2]));
}
