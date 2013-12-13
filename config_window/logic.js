/**
 *
 * @author Tory Gaurnier
 */


function getQueryString(key) {
	var key		=	key.replace(/[\[]/,"\\\[").replace(/[\]]/,"\\\]");
	var regex	=	new RegExp("[\\?&]" + key + "=([^&#]*)");
	var value	=	regex.exec(window.location.href);
    return value[1];
}


function setValues() {
	value = getQueryString("units");
	if(value) document.getElementById("units" + value).checked = true;
}


function cancel() {
	document.location="pebblejs://close#Cancelled";
}


function submit() {
	var celsius		=	document.getElementById("unitsCelsius");
	var fahrenheit	=	document.getElementById("unitsFahrenheit");
	var value = "NULL";

	if(celsius.checked)
		value = "Celsius"

	else if(fahrenheit.checked)
		value = "Fahrenheit"

	document.location = "pebblejs://close#" + value;
}