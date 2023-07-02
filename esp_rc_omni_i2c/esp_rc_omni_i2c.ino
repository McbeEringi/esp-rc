#include "util.h"

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);
AsyncWebServer svr(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *op=NULL;
int16_t v[3]={0};
Ticker ticker;
Adafruit_SSD1306 display(128,64,&Wire,-1);
bool flag=false;

void interval(){flag=true;}

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
			v[0]=0;v[1]=0;v[2]=0;
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
  ledcSetup(M0PPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M0PPIN,M0PPWM);
  ledcSetup(M0NPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M0NPIN,M0NPWM);
  ledcSetup(M1PPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M1PPIN,M1PPWM);
  ledcSetup(M1NPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M1NPIN,M1NPWM);
  ledcSetup(M2PPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M2PPIN,M2PPWM);
  ledcSetup(M2NPWM,PWM_FREQ,PWM_BIT);ledcAttachPin(M2NPIN,M2NPWM);
  pinMode(STBY,OUTPUT);digitalWrite(STBY,LOW);
  pinMode(I2CG,OUTPUT);digitalWrite(I2CG,LOW);
  pinMode(I2CV,OUTPUT);digitalWrite(I2CV,HIGH);
	delay(100);

  Serial.begin(115200);while(!Serial)delay(10);
	Wire.begin(I2CD,I2CC);
	display.begin(SSD1306_SWITCHCAPVCC,0x3c);
	display.setTextColor(SSD1306_WHITE);
	display.setRotation(2);
	display.clearDisplay();
	display.drawBitmap(32,0,icon,64,64,SSD1306_WHITE);
	display.display();
	delay(1000);

  // https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
	WiFi.softAP(SSID,PASS);
	delay(100);// https://github.com/espressif/arduino-esp32/issues/985
	WiFi.softAPConfig(ip,ip,subnet);
  display.clearDisplay();display.setCursor(0,0);
	display.printf("SSID: %s\nPASS: %s\nAPIP: %s\n",SSID,PASS,WiFi.softAPIP().toString().c_str());
	display.display();

	ws.onEvent(onWS);
	svr.addHandler(&ws);
	svr.on("/",HTTP_GET,[](AsyncWebServerRequest *request){request->send_P(200,"text/html",html);});
	svr.onNotFound([](AsyncWebServerRequest *request){request->send_P(302,"text/html",html);});
	svr.begin();
	display.printf("Server OK\n");
	display.display();

	ArduinoOTA
    .setHostname(SSID).setPassword(PASS)
		.onStart([](){digitalWrite(STBY,LOW);ws.enable(false);ws.printfAll("update started: %s\n",ArduinoOTA.getCommand()==U_FLASH?"flash":"spiffs");ws.closeAll();})
		.onProgress([](unsigned int x,unsigned int a){display.clearDisplay();display.drawBitmap(32,0,icon,64,64,SSD1306_WHITE);display.drawFastHLine(0,62,128,SSD1306_WHITE);display.fillRect(1,61,x*126/a,3,SSD1306_WHITE);display.display();})
		.onError([](ota_error_t e){display.clearDisplay();display.setCursor(0,0);display.printf("%s update\nErr[%u]: %s_ERROR",ArduinoOTA.getCommand()==U_FLASH?"flash":"spiffs",e,e==0?"AUTH":e==1?"BEGIN":e==2?"CONNECT":e==3?"RECIEVE":e==4?"END":"UNKNOWN");display.display();delay(5000);})
		.begin();
	display.printf("OTA OK\n");
	display.display();
  digitalWrite(STBY,HIGH);

	display.printf("\nReady!\n");
	display.display();
	Serial.printf("SSID: %s\nPASS: %s\nAPIP: %s\n",SSID,PASS,WiFi.softAPIP().toString().c_str());
	delay(2000);

	ticker.attach_ms(20,interval);
}

void loop(){
	ws.cleanupClients();
  ArduinoOTA.handle();
	if(flag){
		display.clearDisplay();
		display.setCursor(0,0);
		display.printf("%4d,%4d,%4d\n cli_cnt: %d\n op: %d",v[0],v[1],v[2],ws.count(),op==NULL?0:op->id());
		display.display();
		flag=false;
	}
  ledcWrite(M0PPWM,PWM_MAX-max(0,+v[0]));ledcWrite(M0NPWM,PWM_MAX-max(0,-v[0]));
  ledcWrite(M1PPWM,PWM_MAX-max(0,+v[1]));ledcWrite(M1NPWM,PWM_MAX-max(0,-v[1]));
  ledcWrite(M2PPWM,PWM_MAX-max(0,+v[2]));ledcWrite(M2NPWM,PWM_MAX-max(0,-v[2]));
}
