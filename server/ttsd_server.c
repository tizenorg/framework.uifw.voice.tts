/*
*  Copyright (c) 2012-2014 Samsung Electronics Co., Ltd All Rights Reserved 
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*  http://www.apache.org/licenses/LICENSE-2.0
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*/

#include <Ecore.h>
#include <aul.h>
#include "ttsd_main.h"
#include "ttsd_player.h"
#include "ttsd_data.h"
#include "ttsd_engine_agent.h"
#include "ttsd_server.h"
#include "ttsd_dbus_server.h"
#include "ttsp.h"
#include "ttsd_dbus.h"
#include "ttsd_config.h"
#include "ttsd_network.h"

typedef enum {
	TTSD_SYNTHESIS_CONTROL_DOING	= 0,
	TTSD_SYNTHESIS_CONTROL_DONE	= 1,
	TTSD_SYNTHESIS_CONTROL_EXPIRED	= 2
}ttsd_synthesis_control_e;

typedef struct {
	int uid;
	int uttid;
}utterance_t;

/* If current engine exist */
static bool	g_is_engine;

/* If engine is running */
static ttsd_synthesis_control_e	g_synth_control;

static Ecore_Timer* g_wait_timer = NULL;

/* Function definitions */
int __server_next_synthesis(int uid);


int __server_set_synth_control(ttsd_synthesis_control_e control)
{
	g_synth_control = control;
	return 0;
}

ttsd_synthesis_control_e __server_get_synth_control()
{
	return g_synth_control;
}

int __server_send_error(int uid, int utt_id, int error_code)
{
	int pid = ttsd_data_get_pid(uid);

	/* send error */
	if ( 0 != ttsdc_send_error_message(pid, uid, utt_id, error_code)) {
		ttsd_data_delete_client(uid);			
	} 
	
	return 0;
}

Eina_Bool __wait_synthesis(void *data)
{
	/* get current play */
	int uid = ttsd_data_is_current_playing();

	if (uid > 0) {
		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			usleep(100000);
			return EINA_TRUE;
		} else {
			g_wait_timer = NULL;
			if (TTSD_SYNTHESIS_CONTROL_DONE == __server_get_synth_control()) {
				/* Start next synthesis */
				__server_next_synthesis(uid);
			}
		}	
	} else {
		g_wait_timer = NULL;
	}

	return EINA_FALSE;
}

int __synthesis(int uid)
{
	speak_data_s sdata;
	if (0 == ttsd_data_get_speak_data(uid, &sdata)) {

		if (NULL == sdata.lang || NULL == sdata.text) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Current data is NOT valid");
			ttsd_server_stop(uid);
			
			int pid = ttsd_data_get_pid(uid);
			ttsdc_send_set_state_message(pid, uid, APP_STATE_READY);
			
			return 0;
		}

		utterance_t* utt = (utterance_t*)calloc(1, sizeof(utterance_t));

		if (NULL == utt) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to allocate memory");
			return TTSD_ERROR_OUT_OF_MEMORY;
		}

		utt->uid = uid;
		utt->uttid = sdata.utt_id;

		SLOG(LOG_DEBUG, get_tag(), "-----------------------------------------------------------");
		SECURE_SLOG(LOG_DEBUG, get_tag(), "ID : uid (%d), uttid(%d) ", utt->uid, utt->uttid );
		SECURE_SLOG(LOG_DEBUG, get_tag(), "Voice : langauge(%s), type(%d), speed(%d)", sdata.lang, sdata.vctype, sdata.speed);
		SECURE_SLOG(LOG_DEBUG, get_tag(), "Text : %s", sdata.text);
		SLOG(LOG_DEBUG, get_tag(), "-----------------------------------------------------------");

		int ret = 0;
		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DOING);
		ret = ttsd_engine_start_synthesis(sdata.lang, sdata.vctype, sdata.text, sdata.speed, (void*)utt);
		if (0 != ret) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR][%s] * FAIL to start SYNTHESIS !!!! * ", __FUNCTION__);

			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);

			ttsd_config_save_error(utt->uid, utt->uttid, sdata.lang, sdata.vctype, sdata.text, __FUNCTION__, __LINE__, "Fail to start synthesis");

			free(utt);

			ttsd_server_stop(uid);

			int pid = ttsd_data_get_pid(uid);
			ttsdc_send_set_state_message(pid, uid, APP_STATE_READY);
		} else {
			g_wait_timer = ecore_timer_add(0, __wait_synthesis, NULL);
		}

		free(sdata.lang);
		free(sdata.text);
	}

	return 0;
}

int __server_next_synthesis(int uid)
{
	SLOG(LOG_DEBUG, get_tag(), "===== NEXT SYNTHESIS & PLAY START");

	/* get current playing client */
	int current_uid = ttsd_data_get_current_playing();

	if (0 > current_uid) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] Current uid is not valid");
		SLOG(LOG_DEBUG, get_tag(), "=====");
		SLOG(LOG_DEBUG, get_tag(), "  ");
		return 0;
	}

	if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] Engine has already been running");
		SLOG(LOG_DEBUG, get_tag(), "=====");
		SLOG(LOG_DEBUG, get_tag(), "  ");
		return 0;
	}

	__synthesis(current_uid);

	if (0 != ttsd_player_play(current_uid)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to play sound");

		int pid;
		int uid;
		int uttid;
		int reason;
		if (0 == ttsd_data_get_error_data(&uid, &uttid, &reason)) {
			if (current_uid == uid) {
				/* Send error message */
				pid = ttsd_data_get_pid(uid);
				ttsdc_send_error_message(pid, uid, uttid, reason);
			} else {
				SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Not matched uid : uid(%d) uttid(%d) reason(%d)", uid, uttid, reason);
			}
		}

		/* Change ready state */
		ttsd_server_stop(current_uid);
		
		pid = ttsd_data_get_pid(current_uid);
		ttsdc_send_set_state_message(pid, current_uid, APP_STATE_READY);
	}

	SLOG(LOG_DEBUG, get_tag(), "===== NEXT SYNTHESIS & PLAY END");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

/*
* TTS Server Callback Functions	
*/
int __synthesis_result_callback(ttsp_result_event_e event, const void* data, unsigned int data_size, 
				ttsp_audio_type_e audio_type, int rate, void *user_data)
{
	SLOG(LOG_DEBUG, get_tag(), "===== SYNTHESIS RESULT CALLBACK START");

	utterance_t* utt_get_param;
	utt_get_param = (utterance_t*)user_data;

	if (NULL == utt_get_param) {
		SLOG(LOG_ERROR, get_tag(), "[SERVER ERROR] Utterence data is NULL");
		SLOG(LOG_DEBUG, get_tag(), "=====");
		SLOG(LOG_DEBUG, get_tag(), "  ");
		return -1;
	}

	int uid = utt_get_param->uid;
	int uttid = utt_get_param->uttid;

	/* Synthesis is success */
	if (TTSP_RESULT_EVENT_START == event || TTSP_RESULT_EVENT_CONTINUE == event || TTSP_RESULT_EVENT_FINISH == event) {
		
		if (TTSP_RESULT_EVENT_START == event)		SLOG(LOG_DEBUG, get_tag(), "[SERVER] Event : TTSP_RESULT_EVENT_START");
		if (TTSP_RESULT_EVENT_CONTINUE == event)	SLOG(LOG_DEBUG, get_tag(), "[SERVER] Event : TTSP_RESULT_EVENT_CONTINUE");
		if (TTSP_RESULT_EVENT_FINISH == event)		SLOG(LOG_DEBUG, get_tag(), "[SERVER] Event : TTSP_RESULT_EVENT_FINISH");

		if (false == ttsd_data_is_uttid_valid(uid, uttid)) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
			SECURE_SLOG(LOG_ERROR, get_tag(), "[SERVER ERROR] uttid is NOT valid !!!! - uid(%d), uttid(%d)", uid, uttid);
			SLOG(LOG_DEBUG, get_tag(), "=====");
			SLOG(LOG_DEBUG, get_tag(), "  ");
			return 0;
		}

		SECURE_SLOG(LOG_DEBUG, get_tag(), "[SERVER] Result Info : uid(%d), utt(%d), data(%p), data size(%d) audiotype(%d) rate(%d)", 
			uid, uttid, data, data_size, audio_type, rate);

		if (rate <= 0 || audio_type < 0 || audio_type > TTSP_AUDIO_TYPE_MAX) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
			SECURE_SLOG(LOG_ERROR, get_tag(), "[SERVER ERROR] audio data is invalid");
			SLOG(LOG_DEBUG, get_tag(), "=====");
			SLOG(LOG_DEBUG, get_tag(), "  ");
			return 0;
		}

		/* add wav data */
		sound_data_s temp_data;
		temp_data.data = NULL;
		temp_data.rate = 0;
		temp_data.data_size = 0;

		if (0 < data_size) {
			temp_data.data = (char*)calloc(data_size, sizeof(char));
			memcpy(temp_data.data, data, data_size);
		} else {
			SLOG(LOG_ERROR, get_tag(), "Sound data is NULL");
		}

		temp_data.data_size = data_size;
		temp_data.utt_id = utt_get_param->uttid;
		temp_data.event = event;
		temp_data.audio_type = audio_type;
		temp_data.rate = rate;
		
		if (0 != ttsd_data_add_sound_data(uid, temp_data)) {
			SECURE_SLOG(LOG_ERROR, get_tag(), "[SERVER ERROR] Fail to add sound data : uid(%d)", utt_get_param->uid);
		}

		if (event == TTSP_RESULT_EVENT_FINISH) {
			__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_DONE);
		}
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[SERVER] Event : TTSP_RESULT_EVENT_ERROR");
		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);
	} 

	if (TTSP_RESULT_EVENT_FINISH == event || TTSP_RESULT_EVENT_FAIL == event) {
		if (NULL != utt_get_param)	free(utt_get_param);
	}

	SLOG(LOG_DEBUG, get_tag(), "===== SYNTHESIS RESULT CALLBACK END");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return 0;
}

bool __get_client_cb(int pid, int uid, app_state_e state, void* user_data)
{
	/* clear client data */
	ttsd_data_clear_data(uid);			
	ttsd_data_set_client_state(uid, APP_STATE_READY);

	/* send message */
	if ( 0 != ttsdc_send_set_state_message(pid, uid, APP_STATE_READY)) {
		/* remove client */
		ttsd_data_delete_client(uid);
	} 

	return true;
}

void __config_changed_cb(tts_config_type_e type, const char* str_param, int int_param)
{
	switch(type) {
	case TTS_CONFIG_TYPE_ENGINE:
	{
		if (NULL == str_param) {
			SLOG(LOG_ERROR, get_tag(), "[Server] engine id from config is NULL");
			return;
		}

		if (true == ttsd_engine_agent_is_same_engine(str_param)) {
			SLOG(LOG_DEBUG, get_tag(), "[Server Setting] new engine is the same as current engine");
			return;
		}

		/* stop all player */ 
		ttsd_player_all_stop();

		/* send interrupt message to  all clients */
		ttsd_data_foreach_clients(__get_client_cb, NULL);

		ttsd_engine_cancel_synthesis();

		/* set engine */
		int ret = 0;
		ret = ttsd_engine_agent_set_default_engine(str_param);
		if (0 != ret) {
			SLOG(LOG_WARN, get_tag(), "[Server Setting WARNING] Fail to set current engine : result(%d)", ret);
		}

		break;
	}
		
	case TTS_CONFIG_TYPE_VOICE:
	{
		if (NULL == str_param) {
			SLOG(LOG_ERROR, get_tag(), "[Server] language from config is NULL");
			return;
		}

		char* out_lang = NULL;
		ttsp_voice_type_e out_type;
		int ret = -1;

		if (true == ttsd_engine_select_valid_voice(str_param, int_param, &out_lang, &out_type)) {
			SECURE_SLOG(LOG_ERROR, get_tag(), "[Server] valid language : lang(%s), type(%d)", out_lang, out_type);
			ret = ttsd_engine_agent_set_default_voice(out_lang, out_type);
			if (0 != ret)
				SECURE_SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set valid language : lang(%s), type(%d)", out_lang, out_type);

			if (NULL != out_lang)	free(out_lang);
		} else {
			/* Current language is not available */
			SECURE_SLOG(LOG_WARN, get_tag(), "[Server WARNING] Fail to set voice : lang(%s), type(%d)", str_param, int_param);
		}
		break;
	}
	
	case TTS_CONFIG_TYPE_SPEED:
	{
		if (TTSP_SPEED_VERY_SLOW <= int_param && int_param <= TTSP_SPEED_VERY_FAST) {
			/* set default speed */
			int ret = 0;
			ret = ttsd_engine_agent_set_default_speed((ttsp_speed_e)int_param);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set default speed : result(%d)", ret);
			}	
		}
		break;
	}

	case TTS_CONFIG_TYPE_PITCH:
	{
		if (TTSP_PITCH_VERY_LOW <= int_param && int_param <= TTSP_PITCH_VERY_HIGH) {
			/* set default speed */
			int ret = 0;
			ret = ttsd_engine_agent_set_default_pitch((ttsp_pitch_e)int_param);
			if (0 != ret) {
				SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set default pitch : result(%d)", ret);
			}	
		}
		break;
	}

	default:
		break;
	}
	
	return;
}

bool __terminate_client(int pid, int uid, app_state_e state, void* user_data)
{
	SLOG(LOG_DEBUG, get_tag(), "=== Start to terminate client [%d] ===", uid);
	ttsd_server_finalize(uid);
	return true;
}

Eina_Bool ttsd_terminate_daemon(void *data) 
{
	ttsd_data_foreach_clients(__terminate_client, NULL);
	return EINA_FALSE;
}

void __screen_reader_changed_cb(bool value)
{
	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode() && false == value) {
		SLOG(LOG_DEBUG, get_tag(), "[Server] Screen reader is OFF. Start to terminate tts daemon");
		ecore_timer_add(1, ttsd_terminate_daemon, NULL);
	} else {
		SLOG(LOG_DEBUG, get_tag(), "[Server] Screen reader is %s", value ? "ON" : "OFF");
	}
	return;
}

/*
* Server APIs
*/

int ttsd_initialize()
{
	if (ttsd_config_initialize(__config_changed_cb)) {
		SLOG(LOG_ERROR, get_tag(), "[Server WARNING] Fail to initialize config.");
	}

	/* player init */
	if (ttsd_player_init()) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to initialize player init.");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* Engine Agent initialize */
	if (0 != ttsd_engine_agent_init(__synthesis_result_callback)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to engine agent initialize.");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	/* set current engine */
	if (0 != ttsd_engine_agent_initialize_current_engine()) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] No Engine !!!" );
		g_is_engine = false;
	} else 
		g_is_engine = true;
	
	__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);

	if (TTSD_MODE_SCREEN_READER == ttsd_get_mode()) {
		ttsd_config_set_screen_reader_callback(__screen_reader_changed_cb);
	}

	ttsd_file_clean_up();

	return TTSD_ERROR_NONE;
}

int ttsd_finalize()
{
	ttsd_config_finalize();
	
	ttsd_player_release();

	ttsd_engine_agent_release();

	return TTSD_ERROR_NONE;
}

bool __get_client_for_clean_up(int pid, int uid, app_state_e state, void* user_data)
{
	char appid[128] = {0, };
	aul_app_get_appid_bypid(pid, appid, sizeof(appid));
	if (0 < strlen(appid)) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%d] is running app - %s", pid, appid);
	} else {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[%d] is daemon or no_running app", pid);

		int result = 1;
		result = ttsdc_send_hello(pid, uid);

		if (0 == result) {
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Server] uid(%d) should be removed.", uid); 
			ttsd_server_finalize(uid);
		} else if (-1 == result) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Hello result has error"); 
		}
	}
	return true;
}


Eina_Bool ttsd_cleanup_client(void *data)
{
	SLOG(LOG_DEBUG, get_tag(), "===== CLEAN UP CLIENT START");
	ttsd_data_foreach_clients(__get_client_for_clean_up, NULL);
	SLOG(LOG_DEBUG, get_tag(), "=====");
	SLOG(LOG_DEBUG, get_tag(), "  ");

	return EINA_TRUE;
}

/*
* TTS Server Functions for Client
*/

int ttsd_server_initialize(int pid, int uid)
{
	if (false == g_is_engine) {
		if (0 != ttsd_engine_agent_initialize_current_engine()) {
			SLOG(LOG_WARN, get_tag(), "[Server WARNING] No Engine !!!");
			g_is_engine = false;

			return TTSD_ERROR_ENGINE_NOT_FOUND;
		} else {
			g_is_engine = true;
		}
	}

	if (-1 != ttsd_data_is_client(uid)) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] Uid has already been registered");
		return TTSD_ERROR_NONE;
	}

	if (0 == ttsd_data_get_client_count()) {
		if (0 != ttsd_engine_agent_load_current_engine()) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to load current engine");
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	if (0 == ttsd_data_get_same_pid_client_count(pid)) {
		if (0 != ttsd_file_msg_open_connection(pid)) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to open file message connection");
			return TTSD_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != ttsd_data_new_client(pid, uid)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to add client info");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != ttsd_player_create_instance(uid)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to create player");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}

static Eina_Bool __quit_ecore_loop(void *data)
{
	ecore_main_loop_quit();
	return EINA_FALSE;
}

void __used_voice_cb(const char* lang, ttsp_voice_type_e type)
{
	SLOG(LOG_DEBUG, get_tag(), "[Server] Request to unload voice (%s,%d)", lang, type);
	if (0 != ttsd_engine_unload_voice(lang, type)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to unload voice");
	}
}

int ttsd_server_finalize(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] ttsd_server_finalize : uid is not valid");
	}

	ttsd_server_stop(uid);
	
	ttsd_player_destroy_instance(uid);

	/* Need to unload voice when used voice is unregistered */
	if (0 != ttsd_data_reset_used_voice(uid, __used_voice_cb)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set used voice");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	int pid = ttsd_data_get_pid(uid);

	ttsd_data_delete_client(uid);

	if (0 == ttsd_data_get_same_pid_client_count(pid)) {
		ttsd_file_msg_close_connection(pid);
	}

	/* unload engine, if ref count of client is 0 */
	if (0 == ttsd_data_get_client_count()) {
		SLOG(LOG_DEBUG, get_tag(), "[Server] Quit main loop");
		ecore_timer_add(0, __quit_ecore_loop, NULL);
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_set_sound_type(int uid, int type)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] ttsd_server_add_queue : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_READY != state) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Current state(%d) is NOT 'ready' ", uid);
		return TTSD_ERROR_INVALID_STATE;
	}

	if (0 != ttsd_data_set_client_sound_type(uid, (ttsd_sound_type_e)type)) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set sound type : uid(%d) type(%d)", uid, type);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	return TTSD_ERROR_NONE;
}

int ttsd_server_add_queue(int uid, const char* text, const char* lang, int voice_type, int speed, int utt_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] ttsd_server_add_queue : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* check valid voice */
	char* temp_lang = NULL;
	ttsp_voice_type_e temp_type;
	if (true != ttsd_engine_select_valid_voice((const char*)lang, (const ttsp_voice_type_e)voice_type, &temp_lang, &temp_type)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to select valid voice");
		return TTSD_ERROR_INVALID_VOICE;
	}

	if (NULL == temp_lang) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to select valid voice : result lang is NULL");
		return TTSD_ERROR_INVALID_VOICE;
	}
	
	speak_data_s data;

	data.lang = strdup(lang);
	data.vctype = (ttsp_voice_type_e)voice_type;

	data.speed = (ttsp_speed_e)speed;
	data.utt_id = utt_id;

	data.text = strdup(text);

	/* if state is APP_STATE_READY , APP_STATE_PAUSED , only need to add speak data to queue*/
	if (0 != ttsd_data_add_speak_data(uid, data)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to add speak data");
		if (NULL != temp_lang)	free(temp_lang);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (0 != ttsd_data_set_used_voice(uid, temp_lang, temp_type)) {
		/* Request load voice */
		SLOG(LOG_DEBUG, get_tag(), "[Server] Request to load voice");
		if (0 != ttsd_engine_load_voice(temp_lang, temp_type)) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to load voice");
		}
	}

	if (NULL != temp_lang)	free(temp_lang);

	if (APP_STATE_PLAYING == state) {
		/* check if engine use network */
		if (ttsd_engine_agent_need_network()) {
			if (false == ttsd_network_is_connected()) {
				SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Disconnect network. Current engine needs network.");
				return TTSD_ERROR_OPERATION_FAILED;
			}
		}

		/* Check whether tts-engine is running or not */
		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			SLOG(LOG_WARN, get_tag(), "[Server WARNING] Engine has already been running.");
		} else {
			__synthesis(uid);
		}
	}

	return TTSD_ERROR_NONE;
}

Eina_Bool __send_interrupt_client(void *data)
{
	int* uid = (int*)data;
	
	if (NULL != uid) {
		int pid = ttsd_data_get_pid(*uid);

		if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
			/* send message to client about changing state */
			ttsdc_send_set_state_message (pid, *uid, APP_STATE_READY);
		} else {
			ttsdc_send_set_state_message (pid, *uid, APP_STATE_PAUSED);
		}

		free(uid);
	}	
	return EINA_FALSE;
}

int ttsd_server_play(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Server ERROR] uid(%d) is NOT valid  ", uid);
		return TTSD_ERROR_INVALID_PARAMETER;
	}
	
	if (APP_STATE_PLAYING == state) {
		SECURE_SLOG(LOG_WARN, get_tag(), "[Server WARNING] Current state(%d) is 'play' ", uid);
		return TTSD_ERROR_NONE;
	}

	/* check if engine use network */
	if (ttsd_engine_agent_need_network()) {
		if (false == ttsd_network_is_connected()) {
			SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Disconnect network. Current engine needs network service!!!.");
			return TTSD_ERROR_OUT_OF_NETWORK;
		}
	}

	int current_uid = ttsd_data_get_current_playing();

	if (uid != current_uid && -1 != current_uid) {
		if (TTSD_MODE_DEFAULT != ttsd_get_mode()) {
			/* Send interrupt message */
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Server] Old uid(%d) will be interrupted into 'Stop' state ", current_uid);

			/* pause player */
			if (0 != ttsd_server_stop(current_uid)) {
				SECURE_SLOG(LOG_WARN, get_tag(), "[Server ERROR] Fail to stop : uid (%d)", current_uid);
			} 
			
			int* temp_uid = (int*)malloc(sizeof(int));
			*temp_uid = current_uid;
			ecore_timer_add(0, __send_interrupt_client, temp_uid);
		} else {
			/* Default mode policy of interrupt is "Pause" */

			/* Send interrupt message */
			SECURE_SLOG(LOG_DEBUG, get_tag(), "[Server] Old uid(%d) will be interrupted into 'Pause' state ", current_uid);

			/* pause player */
			if (0 != ttsd_player_pause(current_uid)) {
				SECURE_SLOG(LOG_WARN, get_tag(), "[Server ERROR] Fail to ttsd_player_pause() : uid (%d)", current_uid);
			}

			/* change state */
			ttsd_data_set_client_state(current_uid, APP_STATE_PAUSED);

			int* temp_uid = (int*)malloc(sizeof(int));
			*temp_uid = current_uid;
			ecore_timer_add(0, __send_interrupt_client, temp_uid);
		}
	}

	/* Change current play */
	if (0 != ttsd_data_set_client_state(uid, APP_STATE_PLAYING)) {
		SECURE_SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to set state : uid(%d)", uid);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	if (APP_STATE_PAUSED == state) {
		SECURE_SLOG(LOG_DEBUG, get_tag(), "[Server] uid(%d) is 'Pause' state : resume player", uid);

		/* Resume player */
		if (0 != ttsd_player_resume(uid)) {
			SLOG(LOG_WARN, get_tag(), "[Server WARNING] Fail to ttsd_player_resume()");
		}
	}

	/* Check whether tts-engine is running or not */
	if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] Engine has already been running.");
	} else {
		__synthesis(uid);
	}

	return TTSD_ERROR_NONE;
}


int ttsd_server_stop(int uid)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING == state || APP_STATE_PAUSED == state) {
		ttsd_data_set_client_state(uid, APP_STATE_READY);

		if (0 != ttsd_player_stop(uid))
			SLOG(LOG_WARN, get_tag(), "[Server] Fail to ttsd_player_stop()");

		if (TTSD_SYNTHESIS_CONTROL_DOING == __server_get_synth_control()) {
			SLOG(LOG_DEBUG, get_tag(), "[Server] TTS-engine is running");

			int ret = 0;
			ret = ttsd_engine_cancel_synthesis();
			if (0 != ret)
				SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail to cancel synthesis : ret(%d)", ret);
		}

		__server_set_synth_control(TTSD_SYNTHESIS_CONTROL_EXPIRED);
	}

	/* Reset all data */
	ttsd_data_clear_data(uid);

	return TTSD_ERROR_NONE;
}

int ttsd_server_pause(int uid, int* utt_id)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] ttsd_server_pause : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	if (APP_STATE_PLAYING != state)	{
		SLOG(LOG_WARN, get_tag(), "[Server WARNING] Current state is not 'play'");
		return TTSD_ERROR_INVALID_STATE;
	}

	int ret = 0;
	ret = ttsd_player_pause(uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail player_pause() : ret(%d)", ret);
		return TTSD_ERROR_OPERATION_FAILED;
	}

	ttsd_data_set_client_state(uid, APP_STATE_PAUSED);

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_support_voices(int uid, GList** voice_list)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}

	/* get voice list*/
	if (0 != ttsd_engine_get_voice_list(voice_list)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail ttsd_server_get_support_voices()");
		return TTSD_ERROR_OPERATION_FAILED;
	}

	SLOG(LOG_DEBUG, get_tag(), "[Server SUCCESS] Get supported voices");

	return TTSD_ERROR_NONE;
}

int ttsd_server_get_current_voice(int uid, char** language, int* voice_type)
{
	app_state_e state;
	if (0 > ttsd_data_get_client_state(uid, &state)) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] ttsd_server_get_current_voice : uid is not valid");
		return TTSD_ERROR_INVALID_PARAMETER;
	}		

	/* get current voice */
	int ret = ttsd_engine_get_default_voice(language, (ttsp_voice_type_e*)voice_type);
	if (0 != ret) {
		SLOG(LOG_ERROR, get_tag(), "[Server ERROR] Fail ttsd_server_get_support_voices()");
		return ret;
	}

	SECURE_SLOG(LOG_DEBUG, get_tag(), "[Server] Get default language (%s), voice type(%d) ", *language, *voice_type); 

	return TTSD_ERROR_NONE;
}
