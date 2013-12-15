/**
 * Weather Face
 * A simple watch face for monitoring weather in your area
 *
 * Copyright 2013 Tory Gaurnier
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <pebble.h>

static const time_t INTERVAL		=	15 * 60; // Seconds between weather checks
static const time_t ERROR_TIMEOUT	=	120; // Wait time after error before retrying get_weather
static const uint32_t INBOUND_SIZE	=	128; // Inbound app message size
static const uint32_t OUTBOUND_SIZE	=	128; // Outbound app message size
static const uint16_t SCR_W			=	144; // Screen width
static const uint16_t SCR_H			=	168; // Screen height

static Window *window;
static TextLayer *city_layer;
static TextLayer *temp_layer;
static TextLayer *desc_layer;
static BitmapLayer *icon_layer;
static BitmapLayer *noti_layer;
static GFont font_tiny;
static GFont font_small;
static GFont font_medium;
static GFont font_large;
static GBitmap *icon	=	NULL;
static GBitmap *refresh	=	NULL;
static GBitmap *warning	=	NULL;
static GBitmap *waiting	=	NULL;
static GBitmap *empty	=	NULL;

static uint16_t padding_left	=	3; // Padding for left of screen
static uint16_t padding_right	=	3; // Padding for right of screen
static uint16_t padding_top		=	6; // Padding for top of screen
static uint16_t padding_bottom	=	6; // Padding for bottom of screen
static bool ready				=	false; // Is PebbleKit ready?
static bool tick_timer_started	=	false; // Has the tick timer been started yet?
static time_t last_tick_time	=	0; // Last time tick_handler ran

static char city_buff[100]	=	"\0";
static char temp_buff[50]	=	"\0";
static char desc_buff[50]	=	"\0";
static char icon_buff[50]	=	"\0";
static time_t time_stamp	=	0;

enum dict_keys {
	STATUS,
	CITY,
	TEMP,
	DESC,
	ICON,
	TIME
};


/**
 * Compare time stamp of last refresh to current time, if INTERVAL has been reached return true, if
 * not, or no time stamp exists, then return false
 */
static bool time_to_refresh() {
	time_t time_passed = time(NULL) - time_stamp;
	if(time_stamp == 0 || time_passed >= INTERVAL) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Time to refresh");
		bitmap_layer_set_bitmap(noti_layer, waiting);
		return true;
	}

	else {
		APP_LOG(
			APP_LOG_LEVEL_DEBUG,
			"Not time to refresh, %d seconds left",
			(int)(INTERVAL - time_passed)
		);

		return false;
	}
}


/**
 * Set layer frame sizes
 */
static void set_layer_frames() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting layer frames");

	uint16_t city_w	=	SCR_W - (padding_left + padding_right);
	uint16_t city_h	=	40;
	uint16_t city_x	=	padding_left;
	uint16_t city_y	=	padding_top;

	uint16_t desc_w	=	SCR_W - (padding_left + padding_right);
	uint16_t desc_h	=	20;
	uint16_t desc_x	=	padding_left;
	uint16_t desc_y	=	SCR_H - (desc_h + padding_bottom);

	uint16_t temp_w	=	SCR_W - (padding_left + padding_right);
	uint16_t temp_h	=	20;
	uint16_t temp_x	=	padding_left;
	uint16_t temp_y	=	SCR_H - (desc_h + temp_h + padding_bottom);

	uint16_t icon_w	=	SCR_W - (padding_left + padding_right);
	uint16_t icon_h	=	SCR_H - (city_h + temp_h + desc_h + padding_top);
	uint16_t icon_x	=	padding_left;
	uint16_t icon_y	=	city_h;

	uint16_t noti_w	=	20;
	uint16_t noti_h	=	20;
	uint16_t noti_x	=	SCR_W - (noti_w + padding_right);
	uint16_t noti_y	=	SCR_H - (noti_h + padding_bottom);

	layer_set_frame(text_layer_get_layer(city_layer), GRect(city_x, city_y, city_w, city_h));
	layer_set_bounds(text_layer_get_layer(city_layer), GRect(0, 0, city_w, city_h));
	layer_set_frame(text_layer_get_layer(desc_layer), GRect(desc_x, desc_y, desc_w, desc_h));
	layer_set_bounds(text_layer_get_layer(desc_layer), GRect(0, 0, desc_w, desc_h));
	layer_set_frame(text_layer_get_layer(temp_layer), GRect(temp_x, temp_y, temp_w, temp_h));
	layer_set_bounds(text_layer_get_layer(temp_layer), GRect(0, 0, temp_w, temp_h));
	layer_set_frame(bitmap_layer_get_layer(icon_layer), GRect(icon_x, icon_y, icon_w, icon_h));
	layer_set_bounds(bitmap_layer_get_layer(icon_layer), GRect(0, 0, icon_w, icon_h));
	layer_set_frame(bitmap_layer_get_layer(noti_layer), GRect(noti_x, noti_y, noti_w, noti_h));
	layer_set_bounds(bitmap_layer_get_layer(noti_layer), GRect(0, 0, noti_w, noti_h));
}


/**
 * Set layer text and image
 */
static void set_layer_values() {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting layer values");

	// Change font size and padding according to city_buff length
	if(strlen(city_buff) > 12 && strlen(city_buff) <= 26) {
		text_layer_set_font(city_layer, font_medium);
		padding_top = 0;
	}
	else if(strlen(city_buff) > 26) {
		text_layer_set_font(city_layer, font_tiny);
		padding_top = 0;
	}
	else {
		text_layer_set_font(city_layer, font_large);
		padding_top = 6;
	}

	// Set frame sizes
	set_layer_frames();

	// Set text layers
	text_layer_set_text(city_layer, city_buff);
	text_layer_set_text(temp_layer, temp_buff);
	text_layer_set_text(desc_layer, desc_buff);

	// If icon is already set, free it first
	if(icon != NULL)
		gbitmap_destroy(icon);

	// Set appropriate icon based on icon code
	if(strcmp(icon_buff, "01d") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_DAY);

	else if(strcmp(icon_buff, "01n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_NIGHT);

	else if(strcmp(icon_buff, "02d") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_FEW_CLOUDS_DAY);

	else if(strcmp(icon_buff, "02n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_FEW_CLOUDS_NIGHT);

	else if(strcmp(icon_buff, "03d") == 0 || strcmp(icon_buff, "04d") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_SCATTERED_CLOUDS_DAY);

	else if(strcmp(icon_buff, "03n") == 0 || strcmp(icon_buff, "04n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_SCATTERED_CLOUDS_NIGHT);

	else if(strcmp(icon_buff, "09d") == 0 || strcmp(icon_buff, "09n") == 0 ||
			strcmp(icon_buff, "10d") == 0 || strcmp(icon_buff, "10n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_RAIN);

	else if(strcmp(icon_buff, "11d") == 0 || strcmp(icon_buff, "11n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_THUNDERSTORM);

	else if(strcmp(icon_buff, "13d") == 0 || strcmp(icon_buff, "13n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_SNOW);

	else if(strcmp(icon_buff, "50d") == 0 || strcmp(icon_buff, "50n") == 0)
		icon = gbitmap_create_with_resource(RESOURCE_ID_MIST);

	else {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Can't find icon: %s", icon_buff);
		icon = NULL;
	}

	// Set icon (as long as it's not NULL)
	if(icon != NULL)
		bitmap_layer_set_bitmap(icon_layer, icon);
}

/**
 * Returns true if all data in buffs have values
 * If any values are empty returns false
 */
static bool data_to_save() {
	return (
		city_buff[0] != '\0' &&
		temp_buff[0] != '\0' &&
		desc_buff[0] != '\0' &&
		icon_buff[0] != '\0' &&
		time_stamp != 0
	);
}

/**
 * Returns true if all data in persistent storage have values
 * If any values are empty returns false
 */
static bool data_to_load() {
	return (
		persist_exists(CITY) &&
		persist_exists(TEMP) &&
		persist_exists(DESC) &&
		persist_exists(ICON) &&
		persist_exists(TIME)
	);
}


/**
 * Save data to persistent storage
 */
static void save_data() {
	if(data_to_save()) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Saving data to persistent storage");

		persist_write_string(CITY, city_buff);
		persist_write_string(TEMP, temp_buff);
		persist_write_string(DESC, desc_buff);
		persist_write_string(ICON, icon_buff);
		persist_write_int(TIME, (int)time_stamp);
	}

	else
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Some or all buffs are empty, not saving");
}


/**
 * Load data from persistent storage
 */
static void load_data() {
	// If data saved, then load it
	if(data_to_load()) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading data from persistent storage");

		persist_read_string(CITY, city_buff, 100);
		persist_read_string(TEMP, temp_buff, 50);
		persist_read_string(DESC, desc_buff, 50);
		persist_read_string(ICON, icon_buff, 50);
		time_stamp = (time_t)persist_read_int(TIME);

		set_layer_values();
	}

	// Else set text to "loading..." and set layer frames
	else {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Some or all save data is empty, not loading");
		text_layer_set_text(city_layer, "loading...");
		set_layer_frames();
	}
}


/**
 * Send signal to Javascript to check the weather
 */
static void get_weather() {
	// Reset time_stamp back to INTERVAL to avoid simultanious runs of get_weather
	time_stamp = time(NULL) - INTERVAL;

	bitmap_layer_set_bitmap(noti_layer, refresh);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	Tuplet value = TupletCString(STATUS, "retrieve");
	dict_write_tuplet(iter, &value);

	app_message_outbox_send();
}


/**
 * Outgoing message sent
 */
static void sent_handler(DictionaryIterator *sent, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent");
}


/**
 * Outgoing message failed to send
 */
static void failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send message: %s", (char*)reason);
}


/**
 * Message received
 */
static void received_handler(DictionaryIterator *message, void *context) {
	char *status = (char*)dict_find(message, STATUS)->value;

	if(strcmp(status, "ready") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"ready\"");
		ready = true;
		if(time_to_refresh() && tick_timer_started && time(NULL) - last_tick_time >= 30)
			get_weather();
	}

	else if(strcmp(status, "reporting") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"reporting\"");

		// Set notification area to empty bitmap
		bitmap_layer_set_bitmap(noti_layer, empty);

		// Get values from dictionary
		strcpy(city_buff, (char*)dict_find(message, CITY)->value);
		strcpy(temp_buff, (char*)dict_find(message, TEMP)->value);
		strcpy(desc_buff, (char*)dict_find(message, DESC)->value);
		strcpy(icon_buff, (char*)dict_find(message, ICON)->value);

		time_stamp = time(NULL);
		set_layer_values();
	}

	else if(strcmp(status, "configUpdated") == 0) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Recieved status \"configUpdate\"");

		get_weather();
	}

	else if(strcmp(status, "failed") == 0) {
		bitmap_layer_set_bitmap(noti_layer, warning);
		time_stamp = time(NULL) - (INTERVAL - ERROR_TIMEOUT);
	}
}


/**
 * Failed to receive message
 */
static void dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Message dropped: %s", (char*)reason);
}


/**
 * Check weather every INTERVAL minutes
 */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	last_tick_time = time(NULL);

	if(!tick_timer_started) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Tick timer started");
		tick_timer_started = true;
	}

	if(time_to_refresh() && ready)
		get_weather();
}


static void init(void) {
	// Init notification icons
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing notification icons");
	refresh	=	gbitmap_create_with_resource(RESOURCE_ID_REFRESH);
	warning	=	gbitmap_create_with_resource(RESOURCE_ID_WARNING);
	waiting	=	gbitmap_create_with_resource(RESOURCE_ID_WAITING);
	empty	=	gbitmap_create_with_resource(RESOURCE_ID_EMPTY);

	// Init fonts
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing fonts");
	font_tiny	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_12));
	font_small	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_16));
	font_medium	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_18));
	font_large	=	fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_20));

	// Init window
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing window");
	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);
	Layer *root_layer = window_get_root_layer(window);
	GRect frame = layer_get_frame(root_layer);

	// Init city name layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing city layer");
	city_layer = text_layer_create(frame);
	text_layer_set_font(city_layer, font_large);
	text_layer_set_text_alignment(city_layer, GTextAlignmentCenter);
	text_layer_set_text_color(city_layer, GColorWhite);
	text_layer_set_background_color(city_layer, GColorClear);
	text_layer_set_overflow_mode(city_layer, GTextOverflowModeFill);
	layer_add_child(root_layer, text_layer_get_layer(city_layer));

	// Init description layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing description layer");
	desc_layer = text_layer_create(frame);
	text_layer_set_font(desc_layer, font_small);
	text_layer_set_text_alignment(desc_layer, GTextAlignmentCenter);
	text_layer_set_text_color(desc_layer, GColorWhite);
	text_layer_set_background_color(desc_layer, GColorClear);
	layer_add_child(root_layer, text_layer_get_layer(desc_layer));

	// Init temperature layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing temperature layer");
	temp_layer = text_layer_create(frame);
	text_layer_set_font(temp_layer, font_small);
	text_layer_set_text_alignment(temp_layer, GTextAlignmentCenter);
	text_layer_set_text_color(temp_layer, GColorWhite);
	text_layer_set_background_color(temp_layer, GColorClear);
	layer_add_child(root_layer, text_layer_get_layer(temp_layer));

	// Init icon layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing icon layer");
	icon_layer = bitmap_layer_create(frame);
	bitmap_layer_set_background_color(icon_layer, GColorClear);
	bitmap_layer_set_alignment(icon_layer, GAlignCenter);
	layer_add_child(root_layer, bitmap_layer_get_layer(icon_layer));

	// Init notification layer
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing notification layer");
	noti_layer = bitmap_layer_create(frame);
	bitmap_layer_set_background_color(noti_layer, GColorClear);
	bitmap_layer_set_alignment(noti_layer, GAlignTop);
	layer_add_child(root_layer, bitmap_layer_get_layer(noti_layer));

	// Init app message and message handlers
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Initializing handlers");
	app_message_register_outbox_sent(sent_handler);
	app_message_register_outbox_failed(failed_handler);
	app_message_register_inbox_received(received_handler);
	app_message_register_inbox_dropped(dropped_handler);
	app_message_open(INBOUND_SIZE, OUTBOUND_SIZE);

	// Load data from persistent storage
	load_data();
	// Set tick timer handler
	tick_timer_service_subscribe(MINUTE_UNIT, &tick_handler);
}


static void deinit(void) {
	save_data();
	text_layer_destroy(city_layer);
	text_layer_destroy(temp_layer);
	text_layer_destroy(desc_layer);
	bitmap_layer_destroy(icon_layer);
	bitmap_layer_destroy(noti_layer);
	fonts_unload_custom_font(font_tiny);
	fonts_unload_custom_font(font_small);
	fonts_unload_custom_font(font_medium);
	fonts_unload_custom_font(font_large);
	if(icon != NULL)
		gbitmap_destroy(icon);
	gbitmap_destroy(refresh);
	gbitmap_destroy(warning);
	gbitmap_destroy(waiting);
	gbitmap_destroy(empty);
	window_destroy(window);
}


int main(void) {
	init();
	app_event_loop();
	deinit();
}