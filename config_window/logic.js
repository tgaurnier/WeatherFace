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
	if(value) document.getElementById("units" + value).checked = "checked";
}


function cancel() {
	alert("TEST");
	document.location="pebblejs://close#Cancelled";
}


function submit() {
	var celcius		=	document.getElementById("unitsCelcius");
	var fahrenheit	=	document.getElementById("unitsFahrenheit");
	var value = "NULL";

	if(celcius.checked)
		value = "Celcius"

	else if(fahrenheit.checked)
		value = "Fahrenheit"

	document.location = "pebblejs://close#" + value;
}