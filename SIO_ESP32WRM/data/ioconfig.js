var lastGCF = '';
var chRL = false;
var chST = false;

domReady(function() {
document.getElementById("action").innerHTML = "Fetching Settings...";
document.getElementById("loader").classList.remove('hidden');
document.getElementById("action").classList.remove('hidden');


GetIOSettings();
/*
document.getElementById("loader").classList.add('hidden');
document.getElementById("gcontainer").classList.remove('hidden');
document.getElementById("action").innerHTML = "";
document.getElementById("action").classList.add('hidden');		
*/



});


function GetIOSettings(){
	
	var xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onload = function() {
		var reads = JSON.parse(this.responseText);
		//existing IO Config
		document.getElementById("cbo_io0_type").value = reads.settings.io0_type;
		document.getElementById("cbo_io1_type").value = reads.settings.io1_type;
		document.getElementById("cbo_io2_type").value = reads.settings.io2_type;
		document.getElementById("cbo_io3_type").value = reads.settings.io3_type;
		document.getElementById("cbo_io4_type").value = reads.settings.io4_type;
		document.getElementById("cbo_io5_type").value = reads.settings.io5_type;
		document.getElementById("cbo_io6_type").value = reads.settings.io6_type;


		//other settings
		document.getElementById("status").innerHTML = reads.settings.LastStatus;
		document.getElementById("deviceInfo").innerHTML = reads.settings.hostname;
		document.getElementById("firmwareV").innerHTML = reads.settings.firmwareV;

		document.getElementById("loader").classList.add('hidden');
		document.getElementById("gcontainer").classList.remove('hidden');
		document.getElementById("action").innerHTML = "";
		document.getElementById("action").classList.add('hidden');		
		
		
	};
	
	xmlhttp.open("GET", "/APIGetIOSettings", true);
	xmlhttp.send();

}


