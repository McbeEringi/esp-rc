#include <WiFi.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>// https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>// https://github.com/me-no-dev/ESPAsyncWebServer
// requires bug fix: https://github.com/me-no-dev/ESPAsyncWebServer/issues/1101

#define SSID "esp_rc_proto"
#define PASS "sazanka_"
#define PWM_FREQ 4000
#define PWM_BIT 10
#define I1 4
#define I2 5
#define I3 6
#define I4 7
#define RPIN 0
#define GPIN 1

#define I1PWM 0
#define I2PWM 1
#define I3PWM 2
#define I4PWM 3
#define RPWM 4
#define GPWM 5
#define PWM_MAX ((1<<PWM_BIT+1)-1)

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
			:root{transition:.5s;--box:min(100vmin,480px);--cir:0.7;--dot:0.2;}
			@media(prefers-color-scheme:dark){:root{background-color:#222;color:#fff;}}
			#stick{position:relative;width:calc(var(--box)*var(--cir));height:calc(var(--box)*var(--cir));box-shadow:0 0 0 calc(var(--box)*calc(calc(var(--cir)*var(--dot))/2)) #8882;border-radius:50%;image-rendering:pixelated;background:#8884 0 0/100% url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAGklEQVQI12NkYGDg/f//PwPT////GRgZGRkAObwGDol2alsAAAAASUVORK5CYII=);margin:calc(var(--box) * calc(calc(1 - var(--cir)) / 2)) auto;user-select:none;-webkit-user-select:none;}
			#stick>*{position:absolute;width:calc(100%*var(--dot));height:calc(100%*var(--dot));border-radius:50%;background-color:#888;pointer-events:none;top:50%;left:50%;transition:.05s;will-change:transform;}
		</style>
		<div id="stick"><div></div></div>
		<pre id="log">Connecting…</pre>
		<script>
			'use strict';

			let ws={},d={},timer,stick_stat;
			const main=_=>{
				ws=Object.assign(new WebSocket(`ws://${location.hostname}/ws`),{
					onopen:_=>console.log(log.textContent='Opened :)'),
					onclose:_=>console.log(log.textContent='Closed :(',setTimeout(main,2000)),
					onmessage:e=>{
						e=JSON.parse(e.data);
						({
							init:_=>d={...d,...e[1]},
							clis:_=>d.clis=e.slice(1),
							op:_=>d.op=e[1],
							vel:_=>d.vel=e[1]
						}[e[0]]||(_=>_))();
						log.textContent=JSON.stringify(d,null,'\t');
					}
				});
			},
			send=lr=>ws.readyState==1&&ws.send(new Uint8Array([0,...lr.flatMap(x=>(x=(x+1)*(2**d.bit-1),[x>>8,x&0xff]))]).buffer);

			stick.ontouchstart=e=>e.preventDefault();
			stick.onpointerdown=e=>{
				stick_stat=true;
				onpointermove(e);
			};
			(onpointerup=onpointerleave=onpointercancel=()=>{
				stick_stat=false;
				clearTimeout(timer);timer=0;send([0,0]);
				stick.children[0].style.transform='translate(-50%,-50%)';
			})();
			onpointermove=e=>{
				if(!stick_stat||timer)return;
				const stick_style=stick.getBoundingClientRect();
				let pos=[
					(e.clientX-stick_style.left-stick_style.width*.5)||0,
					(e.clientY-stick_style.top-stick_style.height*.5)||0
				],l=Math.sqrt(pos[0]*pos[0]+pos[1]*pos[1]);
				pos=pos.map(x=>x*Math.min(stick_style.width*.5,l)/l);
				stick.children[0].style.transform=`translate(calc(${pos[0]}px - 50%),calc(${pos[1]}px - 50%))`;
				//https://openrtm.org/openrtm/sites/default/files/6357/171108-04.pdf
				pos=[Math.atan2(...pos)-Math.PI*.75,Math.min(1,l/stick_style.width*2)];
				send([Math.cos(pos[0])*pos[1],Math.sin(pos[0])*pos[1]]);
				timer=setTimeout(()=>timer=0,50);
			};

			window.onload=main;

		</script>
	</body>
</html>

)rawliteral";