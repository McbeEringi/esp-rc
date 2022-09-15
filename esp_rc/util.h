#define REV2

#ifdef REV2
  #define I1 25
  #define I2 26
  #define I3 33
  #define I4 32

  #define STATLED
  #define RPIN 14
  #define GPIN 27
#else
  #define I1 18
  #define I2 19
  #define I3 17
  #define I4 16
#endif

// https://lang-ship.com/blog/work/esp32-pwm-max/
#define PWM_FREQ 39062.5
#define PWM_BIT 11

const char* ssid="esp_rc_proto";
const char* pass="sazanka_";
// QR
// WIFI:S:<SSID>;T:<WEP|WPA|無記入>;P:<パスワード>;H:<true|false|無記入>;
const char html[] PROGMEM=R"rawliteral(
<!DOCTYPE html>
<html lang="en" dir="ltr">
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>esp_rc</title>
    <meta name="theme-color" content="#912b31"/>
  </head>
  <body>
    <style>
      :root{transition:.5s;--box:min(100vmin,480px);--cir:0.8;--dot:0.1;image-rendering:pixelated;}@media(prefers-color-scheme:dark){:root{background-color:#222;color:#fff;}}body{margin:0;}
      #stick{position:relative;width:calc(var(--box)*var(--cir));height:calc(var(--box)*var(--cir));box-shadow:0 0 0 calc(var(--box)*calc(calc(var(--cir)*var(--dot))/2)) #8882;border-radius:50%;background:#8884 0 0/100% url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAGklEQVQI12NkYGDg/f//PwPT////GRgZGRkAObwGDol2alsAAAAASUVORK5CYII=);margin:calc(var(--box) * calc(calc(1 - var(--cir)) / 2)) auto;user-select:none;-webkit-user-select:none;}
      #stick>*{position:absolute;width:calc(100% * var(--dot));height:calc(100% * var(--dot));border-radius:50%;background-color:#888;pointer-events:none;top:calc(50% * calc(1 - var(--dot)));left:calc(50% * calc(1 - var(--dot)));transition:.05s;will-change:transform;}
    </style>
    <div id="stick"><div></div></div>
    <pre id="log">Connecting…</pre>
    <script>
      'use strict';
      let ws,ws_send=()=>console.log('uninitialized'),recieved={},timer=0,stick_stat=false;
      const ws_init=()=>{
        console.log('ws_init');
        ws=new WebSocket(`ws://${window.location.hostname}/ws`);
        ws.onopen=e=>{
          console.log(log.textContent='Opened :)');
        };
        ws.onclose=e=>{
          console.log(log.textContent='Closed :(');
          ws_send=()=>{};
          setTimeout(ws_init,2000);
        };
        ws.onmessage=e=>{
          if(ws.timer)return;ws.timer=!0;
          Object.assign(recieved,JSON.parse(e.data));
          log.textContent=JSON.stringify(recieved,null,'\t');
          if(recieved.purpose=='init')
            ws_send=lr=>ws.send(`V_CTR${lr.map(x=>String(Math.round((Math.sqrt(Math.abs(x))*Math.sign(x)+1)*recieved.max)).padStart(recieved.zpad,'0')).join('')}`);
          delete ws.timer;
        };
        ws.onerror=console.log;
      };

      stick.ontouchstart=e=>e.preventDefault();
      stick.onpointerdown=e=>{
        stick_stat=true;
        window.onpointermove(e);
      };
      window.onpointerup=window.onpointerleave=window.onpointercancel=()=>{
        stick_stat=false;
        clearTimeout(timer);timer=0;ws_send([0,0]);
        stick.children[0].style.transform='';
      };
      window.onpointermove=e=>{
        if(!stick_stat||timer)return;
        const stick_style=stick.getBoundingClientRect();
        let pos=[
          (e.clientX-stick_style.left-stick_style.width*.5)||0,
          (e.clientY-stick_style.top-stick_style.height*.5)||0
        ],l=Math.sqrt(pos[0]*pos[0]+pos[1]*pos[1]);
        pos=pos.map(x=>x*Math.min(stick_style.width*.5,l)/l);
        stick.children[0].style.transform=`translate(${pos.join('px,')}px)`;
        //https://openrtm.org/openrtm/sites/default/files/6357/171108-04.pdf
        pos=[Math.atan2(...pos)-Math.PI*.75,Math.min(1,l/stick_style.width*2)];
        ws_send([Math.cos(pos[0])*pos[1],Math.sin(pos[0])*pos[1]]);
        timer=setTimeout(()=>timer=0,50);
      };

      window.onload=ws_init;
    </script>
  </body>
</html>
)rawliteral";
