var lastGCF = '';
var chRL = false;
var chST = false;

domReady(function() {
document.getElementById("action").innerHTML = "Fetching Settings...";
document.getElementById("loader").classList.remove('hidden');
document.getElementById("action").classList.remove('hidden');


GetIOSettings();

});


function GetIOSettings(){
	
	var xmlhttp = new XMLHttpRequest();
	
	xmlhttp.onload = function() {
		var reads = JSON.parse(this.responseText);
		//existing IO Config
		tbl = document.getElementById('ioPoints');
		var IORows = "<tr><th>IO POINT</th><th>IO POINT Type</th>"; //header
		var ioIndex = 0;
		for(let i in reads.settings.IO) {
			var id = "cbo_" + i ;
			IORows += "<tr><td>IO "+ ioIndex++ +"</td><td><select name='" +id+"' id='"+id+"'><option value='2'>OUTPUT</option><option value='18'>OUTPUT_OPEN_DRAIN</option><option value='1'>INPUT</option><option value='5'>INPUT_PULLUP</option><option value='9'>INPUT_PULLDOWN</option></select></td></tr>";
		}
		
		tbl.innerHTML = IORows;
		
		for(let i in reads.settings.IO) {
			var id = "cbo_" + i ;
			document.getElementById(id).value = reads.settings.IO[i];
		}
		
		
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


