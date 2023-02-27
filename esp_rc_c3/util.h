// 0: init o
// 1: op io
// 2: vel io
// 3: txt io

//#define CAPTIVE_PORTAL // since websocket is disabled in ios captive portal this does not make sense
//#define SIGMA_DELTA

#include <WiFi.h>
#ifdef CAPTIVE_PORTAL
  #include <DNSServer.h>
#endif
#include <AsyncTCP.h>// https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>// https://github.com/me-no-dev/ESPAsyncWebServer
// requires bug fix: https://github.com/me-no-dev/ESPAsyncWebServer/issues/1101
#include <ArduinoOTA.h>

#define SSID "esp_rc_proto"
#define PASS "sazanka_"
#define PWM_FREQ 20000
#define PWM_BIT 10
#define I1PIN 5
#define I2PIN 4
#define I3PIN 6
#define I4PIN 7
#define RPIN 0
#define GPIN 1

#define I1PWM 0
#define I2PWM 1
#define I3PWM 2
#define I4PWM 3
#define RPWM 4
#define GPWM 5
#define PWM_MAX (1<<PWM_BIT)

const char html[] PROGMEM=R"rawliteral(
<!DOCTYPE html>
<html lang="en" dir="ltr">
	<head>
		<meta charset="utf-8">
		<meta name="viewport" content="width=device-width,initial-scale=1">
		<title>esp_rc</title>
	</head>
	<body>
		<style>
			:root{--box:min(100vmin,480px);--cir:0.7;--dot:0.2;}
			@media(prefers-color-scheme:dark){:root,.sty{background-color:#222;color:#fff;}}
			#stick{position:relative;width:calc(var(--box)*var(--cir));height:calc(var(--box)*var(--cir));box-shadow:0 0 0 calc(var(--box)*calc(calc(var(--cir)*var(--dot))/2)) #8882;border-radius:50%;image-rendering:pixelated;background:#8884 0 0/100% url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAGklEQVQI12NkYGDg/f//PwPT////GRgZGRkAObwGDol2alsAAAAASUVORK5CYII=);margin:calc(var(--box) * calc(calc(1 - var(--cir)) / 2)) auto;user-select:none;-webkit-user-select:none;}
			#stick>*{position:absolute;width:calc(100%*var(--dot));height:calc(100%*var(--dot));border-radius:50%;background-color:#888;pointer-events:none;top:50%;left:50%;transition:.05s;will-change:transform;}
			#log{position:fixed;top:0;opacity:.5;pointer-events:none;white-space:pre-wrap;overflow-wrap:anywhere;}
		</style>
		<div id="stick"><div></div></div>
		
		<select class="sty" onchange="this.value&&send(this.value.split(','),console.log(1))"><option value="" disabled selected>select command to run...</option><optgroup id="cmd"></optgroup></select><br>
		<input class="sty" placeholder="chat..." onchange="send([3,...te.encode(`<${w.id}> ${this.value}`)],this.value='')">
		<pre id="log"></pre>
		<script>
			'use strict';
			let ws={},timer,stick_stat;
			const
				w={},
				te=new TextEncoder(),td=new TextDecoder(),
				strfy=(w,n=0)=>['Array','Object'].includes(w.constructor.name)?'\n'+Object.entries(w).map(([x,y])=>'\t'.repeat(n)+x+': '+strfy(y,n+1)).join(''):w+'\n',
				main=_=>(
					log.textContent+='Connecting…\n',
					ws=Object.assign(new WebSocket(`ws://${location.hostname}/ws`),{
						binaryType:'arraybuffer',
						onopen:_=>console.log(log.textContent+='Opened\n'),
						onclose:_=>console.log(log.textContent+='Closed\n',setTimeout(main,2000)),
						onmessage:e=>(
							e.data.constructor.name=='String'?w.msg=e.data:
							(
								e=new Uint8Array(e.data),
								([
									_=>(w.id=e[1],w.max=2**e[2]),
									_=>flush(w.op=e[1],w.clis=[...e].slice(2)),
									_=>w.vel=[(e[1]<<8|e[2])-w.max,(e[3]<<8|e[4])-w.max],
									_=>w.chat=[...(w.chat||[]),td.decode(e.slice(1))].slice(-8),
								][e[0]]||(_=>w.msg=e))()
							),
							log.textContent=strfy(w)
						)
					})
				),
				send=w=>ws.readyState==1&&ws.send(new Uint8Array(w).buffer),
				flush=_=>(cmd.innerHTML=w.clis.reduce((a,x)=>(x!=w.id&&w.id==w.op&&(a+=`<option value="1,${x}">/op @a[id=${x}]</option>`),a),''),cmd.previousElementSibling.selected=1);

			stick.ontouchstart=e=>e.preventDefault();
			stick.onpointerdown=e=>(stick_stat=true,onpointermove(e));
			(onpointerup=onpointerleave=onpointercancel=_=>(stick_stat=false,clearTimeout(timer),timer=0,send([2,w.max>>8,w.max&0xff,w.max>>8,w.max&0xff]),stick.children[0].style.transform='translate(-50%,-50%)'))();
			onpointermove=(e,bcr=stick.getBoundingClientRect(),p=[
				(e.clientX-bcr.left-bcr.width*.5)||0,
				(e.clientY-bcr.top-bcr.height*.5)||0
			],l=Math.hypot(...p))=>stick_stat&&!timer&&(
				p=p.map(x=>x*Math.min(bcr.width*.5,l)/l),
				stick.children[0].style.transform=`translate(calc(${p[0]}px - 50%),calc(${p[1]}px - 50%))`,
				//https://openrtm.org/openrtm/sites/default/files/6357/171108-04.pdf
				p=[Math.atan2(...p)-Math.PI*.75,Math.min(1,l/bcr.width*2)],
				send([2,...[Math.cos(p[0])*p[1],Math.sin(p[0])*p[1]].flatMap(x=>(x=(x+1)*w.max,[x>>8,x&0xff]))]),
				timer=setTimeout(()=>timer=0,50)
			);
			window.onload=main;
		</script>
	</body>
</html>

)rawliteral";