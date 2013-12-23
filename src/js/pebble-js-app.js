/**
 *
 * @author Tory Gaurnier
 */


var theme			=	window.localStorage.getItem("Theme") || 1;
var unit			=	window.localStorage.getItem("Unit") || "Celsius";
var timeout			=	window.localStorage.getItem("Timeout") || 900;
var staticLocation	=	(window.localStorage.getItem("StaticLocation") === "true");
var city			=	window.localStorage.getItem("City") || "";
var territory		=	window.localStorage.getItem("Territory") || "";
var appid			=	"9da7aa4c37d50f0595f5d457fcfa3327";


/**
 * Create array of JSON objects defining form elements for config page
 */
function createJSONArray() {
	var objects = new Array();
	var i = 0;
	objects[i++] = {
		"type": "title",
		"value": "Weather Face Configuration"
	};

	objects[i++] = {
		"objectName": "theme",
		"type": "radioGroup",
		"name": "theme",
		"title": "Color theme",
		"list": [
			{
				"value": 1,
				"text": "White on black",
				"id": "whiteOnBlack",
				"selected": (theme == 1)
			},
			{
				"value": 2,
				"text": "Black on white",
				"id": "blackOnWhite",
				"selected": (theme == 2)
			}
		]
	};

	objects[i++] = {
		"objectName": "unit",
		"type": "radioGroup",
		"name": "units",
		"title": "Units of measurement",
		"list": [
			{
				"value": "Celsius",
				"text": "Celsius",
				"id": "Celsius",
				"selected": (unit == "Celsius")
			},
			{
				"value": "Fahrenheit",
				"text": "Fahrenheit",
				"id": "Fahrenheit",
				"selected": (unit == "Fahrenheit")
			}
		]
	};

	objects[i++] = {
		"objectName": "timeout",
		"type": "select",
		"title": "Time between checks for weather",
		"id": "timeout",
		"list": [
			{"value": 300, "text": "5 minutes", "selected": (timeout == 300)},
			{"value": 600, "text": "10 minutes", "selected": (timeout == 600)},
			{"value": 900, "text": "15 minutes", "selected": (timeout == 900)},
			{"value": 1200, "text": "20 minutes", "selected": (timeout == 1200)},
			{"value": 1500, "text": "25 minutes", "selected": (timeout == 1500)},
			{"value": 1800, "text": "30 minutes", "selected": (timeout == 1800)},
			{"value": 2100, "text": "35 minutes", "selected": (timeout == 2100)},
			{"value": 2400, "text": "40 minutes", "selected": (timeout == 2400)},
			{"value": 2700, "text": "45 minutes", "selected": (timeout == 2700)},
			{"value": 3000, "text": "50 minutes", "selected": (timeout == 3000)},
			{"value": 3300, "text": "55 minutes", "selected": (timeout == 3300)},
			{"value": 3600, "text": "1 hour", "selected": (timeout == 3600)}
		]
	};

	objects[i++] = {
		"objectName": "staticLocation",
		"type": "inputToggle",
		"title": "Enable static location",
		"id": "enableStaticLocation",
		"selected": staticLocation,
		"list": [
			{
				"objectName": "city",
				"type": "text",
				"title": "City name",
				"id": "cityName",
				"value": city
			},
			{
				"objectName": "territory",
				"type": "text",
				"title": "Territory/State",
				"id": "territoryName",
				"value": territory
			}
		]
	};

	return objects;
}


function configClosed(e) {
	console.log("Configuration window returned: " + e.response);

	if(e.response) {
		if(e.response != "Cancelled") {
			// Get config values from response
			var config		=	JSON.parse(e.response);
			theme			=	config.theme;
			unit			=	config.unit;
			timeout			=	config.timeout;
			staticLocation	=	config.staticLocation;
			city			=	config.city;
			territory		=	config.territory;

			// Save config values to local storage
			window.localStorage.setItem("Theme", theme);
			window.localStorage.setItem("Unit", unit);
			window.localStorage.setItem("Timeout", timeout);
			window.localStorage.setItem("StaticLocation", staticLocation);
			window.localStorage.setItem("City", city);
			window.localStorage.setItem("Territory", territory);

			Pebble.sendAppMessage(
				{
					"status": "configUpdated",
					"theme": theme,
					"timeout": timeout
				}
			);
		}
	}

	else console.log("No options returned\n");
}


function showConfigWindow(e) {
	var objects = createJSONArray();
	var url	=	"https://rawgithub.com/tgaurnier/PebbleGlobalConfig/master/configuration.html?";
	for(i = 0; i < objects.length; i++) {
		if(i > 0) url += "&";
		url += i + "=" + JSON.stringify(objects[i]);
	}

	console.log("Showing config page: " + url);
	Pebble.openURL(url);
}


function receivedHandler(message) {
	if(message.payload.status == "retrieve") {
		console.log("Recieved status \"retrieve\"")
		if(unit == "Celsius") format = "metric";
		else format = "imperial";

		// If staticLocation is enabled, get weather with the set city and territory.
		/*NOTE*
		 * Much of the code is duplicated from the 'if' statement to the 'else' statement because
		 * using defined functions for the callbacks wasn't working correctly, specifically,
		 * variables were becoming undefined when passing to the functions.
		 */
		if(staticLocation) {
			console.log("Getting weather");
			var req	=	new XMLHttpRequest();
			var url	=	"http://api.openweathermap.org/data/2.5/weather?q=" +
						city + "," + territory + "&units=" + format + "&appid=" + appid;

			req.open("GET", url, true);
			req.onload = function(e) {
				if(req.readyState == 4 && req.status == 200) {
					var response	=	JSON.parse(req.responseText);
					var cityName	=	"" + response.name; // Make sure these are strings
					var temp		= 	"" + response.main.temp + " \u00B0" + unit[0];
					var	desc		=	"" + response.weather[0].description;
					var icon		=	"" + response.weather[0].icon;

					console.log("Submitting weather");
					Pebble.sendAppMessage(
						{
							"status": "reporting",
							"city": cityName,
							"temp": temp,
							"desc": desc,
							"icon": icon
						}
					);
				}

				else {
					console.log("Error communicating with Open Weather Map");
					Pebble.sendAppMessage({"status": "failed"});
				}
			}

			req.send(null);
		}

		// Else get coordinates and get weather with them
		else {
			console.log("Getting location");
			navigator.geolocation.getCurrentPosition(
				function(location) {
					console.log("Getting weather");
					var req	=	new XMLHttpRequest();
					var url	=	"http://api.openweathermap.org/data/2.5/weather?lat=" +
								location.coords.latitude + "&lon=" + location.coords.longitude +
								"&units=" + format + "&appid=" + appid;

					req.open("GET", url, true);
					req.onload = function(e) {
						if(req.readyState == 4 && req.status == 200) {
							var response	=	JSON.parse(req.responseText);
							var cityName	=	"" + response.name; // Make sure these are strings
							var temp		= 	"" + response.main.temp + " \u00B0" + unit[0];
							var	desc		=	"" + response.weather[0].description;
							var icon		=	"" + response.weather[0].icon;

							console.log("Submitting weather");
							Pebble.sendAppMessage(
								{
									"status": "reporting",
									"city": cityName,
									"temp": temp,
									"desc": desc,
									"icon": icon
								}
							);
						}

						else {
							console.log("Error communicating with Open Weather Map");
							Pebble.sendAppMessage({"status": "failed"});
						}
					}

					req.send(null);
				},
				function(error) {
					console.log("Failed to get coords: " + error.message + "\n");
					Pebble.sendAppMessage({"status": "failed"});
				},
				{
					timeout: 60000
				}
			);
		}
	}
}


function readyHandler(e) {
	Pebble.sendAppMessage({"status": "ready"});
}


Pebble.addEventListener("ready", readyHandler);
Pebble.addEventListener("appmessage", receivedHandler);
Pebble.addEventListener("showConfiguration", showConfigWindow);
Pebble.addEventListener("webviewclosed", configClosed);