#define I1 18
#define I2 19
#define I3 17
#define I4 16

#define PWM_FREQ 1045.0
#define PWM_BIT 8
#define PWM_MAX 256
//PWM_MAX=2^PWM_BIT
//2^16=65536
#define WS_ZERO_PAD 3
//shuuld be grater than length of PWM_MAX


const char* ssid="esp_rc_proto";
const char* pass="sazanka_";
// QR
// WIFI:S:<SSID>;T:<WEP|WPA|無記入>;P:<パスワード>;H:<true|false|無記入>;
const char html[] PROGMEM=R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
	<meta charset="utf-8">
	<title>esp_rc</title>
	<meta name="viewport" content="width=device-width,initial-scale=1">
</head>
<body>
	<input type="range" id="vl">
	<input type="range" id="vr">
	<pre id="log">Connecting…</pre>
	<script>
		'use strict';
		let ws,data={};
		const ws_init=()=>{
			console.log('ws_init');
			ws=new WebSocket(`ws://${window.location.hostname}/ws`);
			ws.onopen=e=>{
				console.log('opened');
			};
			ws.onclose=e=>{
				console.log('closed');
				setTimeout(ws_init,5000);
			};
			ws.onmessage=e=>{
				(async()=>{
					data={...data,...JSON.parse(e.data)};
					log.textContent=JSON.stringify(data,null,'\t');
					if(data.purpose=="init"){
						vl.max=vr.max=data.max*2;
						vl.value=vr.value=data.max;
						vl.onchange=vr.onchange=()=>ws.send(`V_CTR${vl.value.padStart(data.zpad,'0')}${vr.value.padStart(data.zpad,'0')}`);
					}
				})().catch(()=>{
					log.textContent+=`\n${e.data}`;
				});
			};
			ws.onerror=e=>{
				console.log(e);
			};
		};

		window.onload=ws_init;
	</script>
</body>
</html>
)rawliteral";
