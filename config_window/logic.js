/**
 *
 * @author Tory Gaurnier
 */


function cancel() {
	document.location="pebblejs://close#";
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