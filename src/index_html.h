#include <Arduino.h>
// https://www.aconvert.com/image/png-to-svg/
// https://realfavicongenerator.net/
// https://github.com/me-no-dev/ESPAsyncWebServer#send-binary-content-from-progmem

const char log_html[] PROGMEM = R"=====(
<div>
	<div class=log>%s</div>
	<div class=log>%s</div>
</div>
<table>
<thead>
	<tr><td>&nbsp;</td><td>&nbsp;</td></tr>
</thead>
<tbody>
	<tr><td>brightness</td><td>%d (sensor: %d)</td></tr> 
	<tr><td>current time</td><td>%s</td></tr> 
	<tr><td>uptime</td><td>%s</td></tr> 
	<tr><td>sunset</td><td>%s</td></tr> 
	<tr><td>sunrise</td><td>%s</td></tr> 
</tbody>
</table>
)=====";

const char stats_html[] PROGMEM = R"=====(
<table>
<thead>
	<tr><td>&nbsp;</td><td>&nbsp;</td></tr>
</thead>
<tbody>
	<tr><td>hostname</td><td>%s</td></tr> 
	<tr><td>started at</td><td>%s</td></tr> 
	<tr><td>current time</td><td>%s</td></tr> 
	<tr><td>build time</td><td>%s %s</td></tr> 
	<tr><td>runtime</td><td>%s</td></tr> 
	<tr><td>uptime</td><td>%s</td></tr> 
	<tr><td>sunset</td><td>%s</td></tr> 
	<tr><td>sunrise</td><td>%s</td></tr> 
	<tr><td>free heap</td><td>%s</td></tr> 
	<tr><td>sketch space</td><td>%s </td></tr> 
	<tr><td>chip size (real)</td><td>%s (%s)</td></tr> 
	<tr><td>reset reason</td><td>%s</td></tr> 
</tbody>
</table>
)=====";
	// <tr><td>cpu freq mhz</td><td>%d</td></tr> 
	// <tr><td>chip speed</td><td>%lu</td></tr> 
	// <tr><td>core version</td><td>%s</td></tr> 
	// <tr><td>sw version</td><td>%s</td></tr> 
	// <tr><td>sdk version</td><td>%s</td></tr> 
	// <tr><td>core version</td><td>%s</td></tr> 
	// <tr><td>boot version</td><td>%d</td></tr> 
	// <tr><td>ide version</td><td>%s</td></tr> 

const char network_html[] PROGMEM = R"=====(
<table>
<thead>
	<tr><td>&nbsp;</td><td>&nbsp;</td></tr>
</thead>
<tbody>
	<tr><td>hostname</td><td>%s</td></tr> 
	<tr><td>local ip</td><td>%s</td></tr> 
	<tr><td>ssid</td><td>%s</td></tr> 
	<tr><td>gateway ip</td><td>%s</td></tr> 
	<tr><td>channel</td><td>%d</td></tr> 
	<tr><td>mac address</td><td>%s</td></tr> 
	<tr><td>subnet mask</td><td>%s</td></tr> 
	<tr><td>dns ip</td><td>%s</td></tr> 
	<tr><td>rssi</td><td>%d dB</td></tr> 
</tbody>
</table>
)=====";

const char content_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
    <div>%PLACEHOLDER_CONTENT%</div>
</html>
)=====";


const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="de">
<head>
	<title>%SETTINGS_HOSTNAME%</title>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
	<link rel="shortcut icon" type="image/x-icon" href="favicon.ico">
	<link rel="mask-icon" type="image/svg" href="mask-icon.svg" color="#48A1BE">
	<meta name="theme-color" content="#ffffff">	
	<style type="text/css"></style>
	<link rel="stylesheet" type="text/css" href="style.css"/>
	<script type="text/javascript"> 
	var ws = null;
	function startEvents(){
		var es = new EventSource('/events');
		es.onopen = function(e) {
			var element = document.getElementById('debug');
			element.innerHTML = "Events Opened";
		};
		es.onerror = function(e) {
			if (e.target.readyState != EventSource.OPEN) {
				var element = document.getElementById('debug');
				element.innerHTML = "Events Closed";
			}
		};
		es.onmessage = function(e) {
			var element = document.getElementById('debug');
			element.innerHTML = e.data;
		};
		es.addEventListener('debug', function(e) {
			var element = document.getElementById('debug');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('home', function(e) {
			var element = document.getElementById('home');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('stats', function(e) {
			var element = document.getElementById('stats');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('network', function(e) {
			var element = document.getElementById('network');
			element.innerHTML = e.data;
		}, false);
		es.addEventListener('loader', function(e) {
			document.getElementById("loader").style.display = e.data;
		}, false);
	}
	function startSocket(){
		ws = new WebSocket('ws://'+document.location.host+'/ws',['arduino']);
		ws.binaryType = "arraybuffer";
		ws.onopen = function(e) {
			console.log("Connected: ", e);
		};
		ws.onclose = function(e) {
			console.log("Disconnected: ", e);
		};
		ws.onerror = function(e) {
			console.log("ws error", e);
		};
		ws.onmessage = function(e) {
			var msg = "";
			if(e.data instanceof ArrayBuffer)
			{
				msg = "BIN:";
				var bytes = new Uint8Array(e.data);
				for (var i = 0; i < bytes.length; i++) 
				{
					msg += String.fromCharCode(bytes[i]);
				}
			} else 
			{
				msg = "TXT:"+e.data;
			}				
			console.log(msg);
		};
    }	
	</script>
</head>
<body onload=onBodyLoad()>
<header>
	<nav>
			<div class="navBar" id="mainNavBar">
			<img class="logo" src='data:image/svg+xml;utf8,<svg style="fill-rule:evenodd;clip-rule:evenodd;stroke-linecap:round;stroke-linejoin:round;" xmlns:vectornator="http://vectornator.io" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns="http://www.w3.org/2000/svg" xml:space="preserve" version="1.1" viewBox="0 0 256 256"><path stroke="#00a3c2" stroke-width="20.99" d="M128+242.89C191.452+242.89+242.89+191.452+242.89+128C242.89+64.5481+191.452+13.11+128+13.11C64.5481+13.11+13.11+64.5481+13.11+128C13.11+191.452+64.5481+242.89+128+242.89Z" fill="none" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" fill-rule="evenodd" stroke-width="8" d="M31.3777+141.266L31.3777+107.818C31.3777+105.943+32.0437+104.29+33.3757+102.86C34.7077+101.429+36.311+100.714+38.1857+100.714C40.0603+100.714+41.7377+101.429+43.2177+102.86C44.6977+104.29+45.4377+105.943+45.4377+107.818L45.4377+143.042C45.4377+149.652+47.411+155.03+51.3577+159.174C55.3043+163.318+60.139+165.39+65.8617+165.39C71.683+165.39+76.567+163.367+80.5137+159.322C84.4603+155.276+86.4337+149.85+86.4337+143.042L86.4337+107.818C86.4337+105.943+87.1243+104.29+88.5057+102.86C89.887+101.429+91.515+100.714+93.3897+100.714C95.2643+100.714+96.917+101.429+98.3477+102.86C99.7783+104.29+100.493+105.943+100.493+107.818L100.493+143.042C100.493+149.85+102.442+155.276+106.339+159.322C110.237+163.367+115.096+165.39+120.917+165.39C126.739+165.39+131.623+163.367+135.569+159.322C139.516+155.276+141.489+149.85+141.489+143.042L141.489+107.818C141.489+105.943+142.205+104.29+143.635+102.86C145.066+101.429+146.719+100.714+148.593+100.714C150.468+100.714+152.121+101.429+153.551+102.86C154.982+104.29+155.697+105.943+155.697+107.818L155.697+141.266C155.697+152.02+152.367+160.777+145.707+167.536C139.047+174.294+130.735+177.674+120.769+177.674C110.212+177.674+101.085+172.938+93.3897+163.466C85.891+172.938+76.715+177.674+65.8617+177.674C55.995+177.674+47.781+174.294+41.2197+167.536C34.6583+160.777+31.3777+152.02+31.3777+141.266Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path stroke="#f59e13" fill-rule="evenodd" stroke-width="8.98" d="M147.927+74.0739L173.308+74.0739C189.927+74.0739+203.018+78.9332+212.58+88.6519C222.141+98.3706+226.922+110.63+226.922+125.43C226.922+140.23+222.141+152.637+212.58+162.652C203.018+172.667+189.927+177.674+173.308+177.674L147.927+177.674C146.025+177.674+144.448+177.033+143.197+175.75C141.945+174.467+141.319+172.839+141.319+170.866L141.319+80.8819C141.319+78.9086+141.945+77.2806+143.197+75.9979C144.448+74.7152+146.025+74.0739+147.927+74.0739ZM173.308+86.6539L155.436+86.6539L155.436+165.094L173.308+165.094C186.223+165.094+196.11+161.271+202.968+153.624C209.826+145.977+213.256+136.579+213.256+125.43C213.256+114.281+209.851+105.031+203.043+97.6799C196.235+90.3292+186.323+86.6539+173.308+86.6539Z" fill="#f59e13" stroke-linecap="butt" opacity="1" stroke-linejoin="miter"/><path d="M130.204+81.3807C129.57+71.7308+136.878+63.3934+146.528+62.7585C156.178+62.1238+164.516+69.4318+165.149+79.0817C165.785+88.7317+158.477+97.0691+148.827+97.7039C139.177+98.3386+130.839+91.0306+130.204+81.3807Z" fill-rule="evenodd" fill="#00a3c2" opacity="1"/></svg>
' width="42" height="42">
			<a class="tablinks" id="defaultOpen" onclick="doTab(event, 'tab_home')" href="#"> home </a>
			<a class="tablinks" onclick="doTab(event, 'tab_stats')" href="#"> stats </a>
			<a class="tablinks" onclick="doTab(event, 'tab_network')" href="#"> network </a>
			<a class="tablinks" onclick="doTab(event, 'tab_settings')" href="#"> settings </a>
			<span class="title">%SETTINGS_HOSTNAME%</span>
			<a href="javascript:void(0);" class="drawer" onclick="openDrawerMenu()">&#9776;</a>
		</div>
	</nav>
</header>

<section>    
	<div class="row">
        %PLACEHOLDER_ONOFFBUTTON%
		<a class="button" href="/reconnect">reconnect</a>
		<a class="button" href="/reboot">reboot</a>
	</div>
</section>

<div class="loader" style="display:block;" id="loader"></div>

<section id="tab_home" class="tabcontent">            
	<div class="row">
		<div id="home">&nbsp;</div>
	</div>
</section>    

<section id="tab_stats" class="tabcontent">            
	<div class="row">
		<div id="stats">&nbsp;</div>
	</div>
</section>    

<section id="tab_network" class="tabcontent">            
	<div class="row">
		<div id="network">&nbsp;</div>
	</div>
</section>    

<section id="tab_settings" class="tabcontent">            
	<form id="settings" action="#" method="post">
    	<div class="row">
    		<div class="col-50"> 
                <label for="hostname"> hostname</label>
    		</div>
            <div class="col-50"> 
                <input type="text" name="hostname" id="hostname" placeholder="hostname" value="%SETTINGS_HOSTNAME%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">
    		        <label for="ssid">ssid</label>
    		</div>
    		 <div class="col-50">
    		        <input type="text" name="ssid" id="ssid" placeholder="ssid" value="%SETTINGS_SSID%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">
    		    <label for="password">password</label>
    		</div>
    		 <div class="col-50">
    		    <input type="text" name="password" id="password" placeholder="password" value="%SETTINGS_PASSWORD%">
    		</div>
    	</div>

    	<div class="row">
    		 <div class="col-50">		
    			<label for="mqtt_server">mqtt server</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="mqtt_server" id="mqtt_server" placeholder="mqtt_server" value="%SETTINGS_MQTT_SERVER%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">		
    			<label for="mqtt_subscription">subscribe topic</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="mqtt_subscription" id="mqtt_subscription" placeholder="mqtt_subscription" value="%SETTINGS_MQTT_SUBSCRIPTION%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">		
    			<label for="mqtt_user">mqtt user</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="mqtt_user" id="mqtt_user" placeholder="mqtt_user" value="%SETTINGS_MQTT_USER%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">
    			<label for="mqtt_password">mqtt password</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="mqtt_password" id="mqtt_password" placeholder="mqtt_password" value="%SETTINGS_MQTT_PASSWORD%">
    		</div>
    	</div>

    	<div class="row">
    		 <div class="col-50">		
    			<label for="api_key">open weather api key</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="api_key" id="api_key" placeholder="api_key" value="%SETTINGS_APIKEYR%">
    		</div>
    	</div>
		<!--
    	<div class="row">
    		 <div class="col-50">		
    			<label for="longitude">city id(s)</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="cityid" id="cityid" placeholder="cityid" value="%SETTINGS_CITYID%">
    		</div>
    	</div>
		/-->
		<div class="row">
    		 <div class="col-50">		
    			<label for="latitude">latitude</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="latitude" id="latitude" placeholder="latitude" value="%SETTINGS_LATITUDE%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">		
    			<label for="longitude">longitude</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="longitude" id="longitude" placeholder="longitude" value="%SETTINGS_LONGITUDE%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">		
    			<label for="offset">offset (h)</label>
    		</div>
    		 <div class="col-50">
    			<input type="text" name="offset" id="offset" placeholder="offset" value="%SETTINGS_OFFSET%">
    		</div>
    	</div>
    	<div class="row">
    		 <div class="col-50">		
    			<label for="twilight">switch display by twilight</label>
    		</div>
    		 <div class="col-50">
    		    <input type="checkbox" name="twilight" id="twilight" %SETTINGS_TWILIGHT%>
    		</div>
    	</div>

    	<div class="row">
    		 <div class="col-50">
    			<label for="brightness">dim by sensor</label>
    		</div>
    		 <div class="col-50">
    		    <input type="checkbox" id="ldr" name="ldr" %SETTINGS_USELDR%>
    		</div>
   		</div>
    	<div class="row">
    		 <div class="col-50">
    			<label for="brightness">brightness</label>
    		</div>
    		 <div class="col-50">
    		 	<span id="sliderAmount">%SETTINGS_BRIGHTNESS%</span>
    			​<input id="brightness" name="brightness" type="range" min="0" max="15" step="1" onchange="updateSlider(this.id, this.value);" value="%SETTINGS_BRIGHTNESS%">
    		</div>
    	</div>
    	
    	<div class="row">
    		 <div class="col-33">&nbsp;</div>
    		 <div class="col-33">
    			<button type="button" class="button center" onClick="safeConfig('settings')">save</button>
    		</div>
    		 <div class="col-33">&nbsp;</div>
    	</div> 
	</form>
</section>    

<section>    
	<div class="row">
		<div id="debug">[debug]</div>
	</div>
</section>

</body>
<script type="text/javascript"> 
	function onBodyLoad(){
		startEvents();
		startSocket();
		document.getElementById("defaultOpen").click();
	}
	function doTab(evt, section) {
		var i, tabcontent, tablinks;
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i = 0; i < tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}
		tablinks = document.getElementsByClassName("tablinks");
			for (i = 0; i < tablinks.length; i++) {
		tablinks[i].className = tablinks[i].className.replace(" active", "");
		}
		document.getElementById(section).style.display = "block";
		evt.currentTarget.className += " active";
		ws.send(section);
	}
	function openDrawerMenu(){
		var x = document.getElementById("mainNavBar");
		if (x.className === "navBar"){
			x.className += " responsive";
		} else {
			x.className = "navBar";
		}
	}
	function updateSlider(slider, value) {
		var sliderDiv = document.getElementById("sliderAmount");
		sliderDiv.innerHTML = value;
	}
	function toJSONString( form ) {
		var obj = {};
		var elements = form.querySelectorAll( "input" ); 
		for( var i = 0; i < elements.length; ++i ) {
			var element = elements[i];
			var name = element.name;
			// console.log('element.type' + ':' + element.type);
			var value;
			if(element.type == 'checkbox') {
				if (element.checked == true) value = '1';
				if (element.checked == false) value = '0';
			} else value = element.value;
			if( name ) {
				obj[ name ] = value;
			}
		}
		return JSON.stringify( obj );
	}
	function safeConfig(elementId){
		var form = document.getElementById(elementId);
		var json = toJSONString(form);
		console.log(json);
		var request = new XMLHttpRequest();
		//request.open("POST", "/settings", true);
		request.open("PUT", "/json", true);
		request.setRequestHeader("Content-Type", "application/json"); 
		request.send(JSON.stringify(json));
		console.log(JSON.stringify(json));
		//request.send("json=" + JSON.stringify(json));
	}	
</script>
</html>
)=====";
