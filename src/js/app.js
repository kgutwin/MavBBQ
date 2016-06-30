var lastTemp = {
	"temp1": 0,
	"temp2": 0,
};

// Send a calendar update, only if an update needs to be sent.
function sendTemp(temp1, temp2) {
	if (temp1 != lastTemp.temp1 || temp2 != lastTemp.temp2) {
		lastTemp.temp1 = temp1;
		lastTemp.temp2 = temp2;
		console.log("sendTemp " + JSON.stringify(lastTemp));
		Pebble.sendAppMessage(lastTemp);
	}
}

var lastResponse = {
	"temperature_F_1": 0.0,
	"temperature_F_2": 0.0,
};

function tryTemp() {
	var req = new XMLHttpRequest();
	req.open('GET', "http://www.gutwin.org/ebw/maverick.json?cache=" + (Math.random() * 100000), true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
				//console.log(req.responseText);
				lastResponse = JSON.parse(req.responseText);
				var temp1 = Math.floor(lastResponse.temperature_F_1 * 10);
				var temp2 = Math.floor(lastResponse.temperature_F_2 * 10);
				sendTemp(temp1, temp2);
			} else {
				console.log("Error " + req.status);
				sendTemp(-10, -10);
			}
		} else {
			console.log("temp readyState " + req.readyState);
		}
	};
	req.timeout = 15000;
	req.ontimeout = function() {
		console.log("timeout");
		sendTemp(-20, -20);
	};
	req.send(null);
}

// Initialize application
Pebble.addEventListener(
	"ready",
	function(e) {
		console.log("CONNECTION ESTABLISHED " + e.ready);
	});

// Handle incoming AppMessages, dispatch to try* functions as appropriate.
Pebble.addEventListener(
	"appmessage",
	function(e) {
		//console.log("RECEIVED MESSAGE:");
		//console.log(JSON.stringify(e));
		if (e.payload.tempGet) {
			tryTemp();
		}
	});
