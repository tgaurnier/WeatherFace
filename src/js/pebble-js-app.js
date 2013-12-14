/**
 *
 * @author Tory Gaurnier
 */


var unit = window.localStorage.getItem("Unit") || "Celsius";


function configClosed(e) {
	console.log("Configuration window returned: " + e.response);

	if(e.response) {
		if(e.response != unit && e.response != "Cancelled") {
			unit = e.response;
			window.localStorage.setItem("Unit", unit);

			Pebble.sendAppMessage({"status": "configUpdated"})
		}
	}

	else console.log("No options returned\n");
}


function showConfigWindow(e) {
	var url	=	"https://rawgithub.com/tgaurnier/WeatherFace/master/config_window/" +
				"configuration.html?units" + "=" + unit;

	console.log("Showing config page: " + url);
	Pebble.openURL(url);
}


function receivedHandler(message) {
	if(message.payload.status == "retrieve") {
		console.log("Recieved status \"retrieve\"")
		if(unit == "Celsius") format = "metric";
		else format = "imperial";

		console.log("Getting location");
		navigator.geolocation.getCurrentPosition(
			function(location) {
				console.log("Getting weather");
				var req	=	new XMLHttpRequest();
				var url	=	"http://api.openweathermap.org/data/2.5/weather?lat=" +
							location.coords.latitude + "&lon=" + location.coords.longitude +
							"&units=" + format + "&appid=9da7aa4c37d50f0595f5d457fcfa3327";

				req.open("GET", url, true);
				req.onload = function(e) {
					if(req.readyState == 4 && req.status == 200) {
						var response	=	JSON.parse(req.responseText);
						var city		=	"" + response.name; // Make sure these are strings
						var temp		= 	"" + response.main.temp + " \u00B0" + unit[0];
						var	desc		=	"" + response.weather[0].description;
						var icon		=	"" + response.weather[0].icon;

						console.log("Submitting weather");
						Pebble.sendAppMessage(
							{
								"status": "reporting",
								"city": city,
								"temp": temp,
								"desc": desc,
								"icon": icon
							}
						);
					}

					else console.log("Error communicating with Open Weather Map");
				}

				req.send(null);
			},
			function(error) {
				console.log("Failed to get coords: " + error.message + "\n");
			}
		);
	}
}


function readyHandler(e) {
	Pebble.sendAppMessage({"status": "ready"});
}


Pebble.addEventListener("ready", readyHandler);
Pebble.addEventListener("appmessage", receivedHandler);
Pebble.addEventListener("showConfiguration", showConfigWindow);
Pebble.addEventListener("webviewclosed", configClosed);