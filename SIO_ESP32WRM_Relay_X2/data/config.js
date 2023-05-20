var lastGCF = '';
var chRL = false;
var chST = false;

domReady(function() {
document.getElementById("action").innerHTML = "Fetching Settings...";
document.getElementById("loader").classList.remove('hidden');
document.getElementById("action").classList.remove('hidden');
GetSettings();
});

function toggleAccessPassword(){
	var el = document.getElementById("tx_APW");
	togglePassword(el);
}
function toggleWiFiPassword(){
	var el = document.getElementById("tx_WFPW");
	togglePassword(el);
}
function togglePassword(el){
	
	if(el.type === "text"){
		el.type = "password";
	}else{
		el.type = "text";
	}
}

function GetSettings(){
	
	var xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onload = function() {
		var reads = JSON.parse(this.responseText);
		//wifi settings	
		document.getElementById("cbo_WFMD").value = reads.settings.wifimode;
		document.getElementById("tx_HNAM").value = reads.settings.hostname;
		document.getElementById("tx_SSID").value = reads.settings.ssid;
		document.getElementById("tx_WFPW").value = reads.settings.password;
		document.getElementById("tx_WFCA").value = reads.settings.attempts;
		document.getElementById("tx_WFAD").value = reads.settings.attemptdelay;
		document.getElementById("tx_APW").value = reads.settings.apPassword;
		
		//MQTT setting (Future)
		/*
		if(reads.settings.MQTTEN == "1"){
			document.getElementById("ch_MQTTEN").checked = true;
		}

		document.getElementById("tx_MQTTSRV").value = reads.settings.MQTTSrvIp;		
		document.getElementById("tx_MQTTPort").value = reads.settings.MQTTPort;
		document.getElementById("tx_MQTTUSR").value = reads.settings.MQTTUser;
		document.getElementById("tx_MQTTPW").value = reads.settings.MQTTPW;
		
		*/
		
		//Octoprint connection settings
		document.getElementById("tx_OPURI").value = reads.settings.OPURI;
		document.getElementById("tx_OPPort").value = reads.settings.OPPort;
		document.getElementById("tx_OPAK").value = reads.settings.OPAK;
		document.getElementById("tx_STI").value = reads.settings.StatusIntervalSec;								
		document.getElementById("tx_TZOS").value = reads.settings.TimeZoneOffsetHours;
		
		//deruved settings
		document.getElementById("status").innerHTML = reads.settings.LastStatus;
		document.getElementById("deviceInfo").innerHTML = reads.settings.hostname;
		document.getElementById("firmwareV").innerHTML = reads.settings.firmwareV;
		
		document.getElementById("loader").classList.add('hidden');
		document.getElementById("gcontainer").classList.remove('hidden');
		document.getElementById("action").innerHTML = "";
		document.getElementById("action").classList.add('hidden');
		
		
		
		
	};
	xmlhttp.open("GET", "/APIGetSettings", true);
	xmlhttp.send();
	
}


function GetNetworks(){
	document.getElementById("wfdts").open = true;
	document.getElementById("networks").innerHTML = "Scanning Networks...";
	RequestNetScan();
	setTimeout(() => { ReadNetworks();}, 3000);
}

function ReadNetworks(){
	var xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onload = function() {
		var resp = this.responseText;
		var SSIDs = JSON.parse(resp);
		if (SSIDs.networks.length > 0){
			
			var nets = document.getElementById("networks");
			nets.innerHTML = "";
			var list = document.getElementById("SSIDs");
			for(i = 0; i < SSIDs.networks.length; i++) {
				var option = document.createElement('option');
				option.value = SSIDs.networks[i].split(' ')[0];
				list.appendChild(option);
				nets.innerHTML += SSIDs.networks[i] + "<br/>";
				console.log(SSIDs.networks[i]);
			}
			
		}
	}
	
	xmlhttp.open("GET", "/APIGetNetworks", true);
	xmlhttp.send();
}

function RequestNetScan(){
	var res = fetch("/APIScanWifi");
}
