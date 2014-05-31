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

#include <dirent.h>
#include <Ecore.h>
#include <iconv.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <vconf.h>
#include <vconf-internal-keys.h>

#include "tts_client.h"
#include "tts_config_mgr.h"
#include "tts_dbus.h"
#include "tts_main.h"


#define MAX_TEXT_COUNT 2000

static bool g_is_daemon_started = false;

static bool g_is_noti_daemon_started = false;

static bool g_is_sr_daemon_started = false;

static Ecore_Timer* g_connect_timer = NULL;

static bool g_screen_reader;

/* Function definition */
static Eina_Bool __tts_notify_state_changed(void *data);
static Eina_Bool __tts_notify_error(void *data);

char* tts_tag()
{
	return "ttsc";
}

char* __tts_get_error_code(tts_error_e err)
{
	switch(err) {
	case TTS_ERROR_NONE:			return "TTS_ERROR_NONE";
	case TTS_ERROR_OUT_OF_MEMORY:		return "TTS_ERROR_OUT_OF_MEMORY";
	case TTS_ERROR_IO_ERROR:		return "TTS_ERROR_IO_ERROR";
	case TTS_ERROR_INVALID_PARAMETER:	return "TTS_ERROR_INVALID_PARAMETER";
	case TTS_ERROR_OUT_OF_NETWORK:		return "TTS_ERROR_OUT_OF_NETWORK";
	case TTS_ERROR_INVALID_STATE:		return "TTS_ERROR_INVALID_STATE";
	case TTS_ERROR_INVALID_VOICE:		return "TTS_ERROR_INVALID_VOICE";
	case TTS_ERROR_ENGINE_NOT_FOUND:	return "TTS_ERROR_ENGINE_NOT_FOUND";
	case TTS_ERROR_TIMED_OUT:		return "TTS_ERROR_TIMED_OUT";
	case TTS_ERROR_OPERATION_FAILED:	return "TTS_ERROR_OPERATION_FAILED";
	case TTS_ERROR_AUDIO_POLICY_BLOCKED:	return "TTS_ERROR_AUDIO_POLICY_BLOCKED";
	default:
		return "Invalid error code";
	}
	return NULL;
}

void __tts_config_voice_changed_cb(const char* before_lang, int before_voice_type, const char* language, int voice_type, bool auto_voice, void* user_data)
{
	SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "Voice changed : Before lang(%s) type(%d) , Current lang(%s), type(%d)", 
		before_lang, before_voice_type, language, voice_type);

	GList* client_list = NULL;
	client_list = tts_client_get_client_list();

	GList *iter = NULL;
	tts_client_s *data = NULL;

	if (g_list_length(client_list) > 0) {
		/* Get a first item */
		iter = g_list_first(client_list);

		while (NULL != iter) {
			data = iter->data;
			if (NULL != data->default_voice_changed_cb) {
				SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "Call default voice changed callback : uid(%d)", data->uid);
				data->default_voice_changed_cb(data->tts, before_lang, before_voice_type, 
					language, voice_type, data->default_voice_changed_user_data);
			}

			/* Next item */
			iter = g_list_next(iter);
		}
	}

	return; 
}

int tts_create(tts_h* tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Create TTS");
	
	/* check param */
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_open_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to open dbus connection");
			SLOG(LOG_DEBUG, TAG_TTSC, "=====");
			SLOG(LOG_DEBUG, TAG_TTSC, " ");
			return TTS_ERROR_OPERATION_FAILED;
		}
	}

	if (0 != tts_client_new(tts)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to create client!!!!!");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_OUT_OF_MEMORY;
	}

	tts_client_s* client = tts_client_get(*tts);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get client");
		return TTS_ERROR_OPERATION_FAILED;
	}

	int ret = tts_config_mgr_initialize(client->uid);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to init config manager : %d", ret);
		tts_client_destroy(*tts);
		return TTS_ERROR_OPERATION_FAILED;
	}

	ret = tts_config_mgr_set_callback(client->uid, NULL, __tts_config_voice_changed_cb, NULL, NULL, NULL);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to set config changed : %d", ret);
		tts_client_destroy(*tts);
		return TTS_ERROR_OPERATION_FAILED;
	}

	g_is_daemon_started = false;
	g_is_noti_daemon_started = false;
	g_is_sr_daemon_started = false;

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_destroy(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Destroy TTS");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check used callback */
	if (0 != tts_client_get_use_callback(client)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Cannot destroy in Callback function");
		return TTS_ERROR_OPERATION_FAILED;
	}

	tts_config_mgr_finalize(client->uid);

	int ret = -1;
	int count = 0;

	/* check state */
	switch (client->current_state) {
	case TTS_STATE_PAUSED:
	case TTS_STATE_PLAYING:
	case TTS_STATE_READY:
		if (!(false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode)) {
			while (0 != ret) {
				ret = tts_dbus_request_finalize(client->uid);
				if (0 != ret) {
					if (TTS_ERROR_TIMED_OUT != ret) {
						SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
						break;
					} else {
						SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry finalize");
						usleep(10);
						count++;
						if (10 == count) {
							SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
							break;
						}
					}
				}
			}
		} else {
			SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Do not request finalize : g_sr(%d) mode(%d)", g_screen_reader, client->mode);
		}

		if (TTS_MODE_SCREEN_READER == client->mode)
			g_is_sr_daemon_started = false;
		else if (TTS_MODE_NOTIFICATION == client->mode)
			g_is_noti_daemon_started = false;
		else
			g_is_daemon_started = false;

		if (0 == tts_client_get_mode_client_count(client->mode)) {
			SLOG(LOG_DEBUG, TAG_TTSC, "Close file msg connection : mode(%d)", client->mode);
			ret = tts_file_msg_close_connection(client->mode);
			if (0 != ret)
				SLOG(LOG_WARN, TAG_TTSC, "[ERROR] Fail to close file message connection");
		}

		client->before_state = client->current_state;
		client->current_state = TTS_STATE_CREATED;

	case TTS_STATE_CREATED:
		if (NULL != g_connect_timer) {
			SLOG(LOG_DEBUG, TAG_TTSC, "Connect Timer is deleted");
			ecore_timer_del(g_connect_timer);
		}
		/* Free resources */
		tts_client_destroy(tts);
		break;
	}
 
	if (0 == tts_client_get_size()) {
		if (0 != tts_dbus_close_connection()) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to close connection");
		}
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

void __tts_screen_reader_changed_cb(bool value)
{
	g_screen_reader = value;
}

int tts_set_mode(tts_h tts, tts_mode_e mode)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Set TTS mode");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'"); 
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (TTS_MODE_DEFAULT <= mode && mode <= TTS_MODE_SCREEN_READER) {
		client->mode = mode;
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] mode is not valid : %d", mode);
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_MODE_SCREEN_READER == mode) {
		int ret;
		int screen_reader;
		ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &screen_reader);
		if (0 != ret) {
			SLOG(LOG_ERROR, tts_tag(), "[Config ERROR] Fail to get screen reader");
			return TTS_ERROR_OPERATION_FAILED;
		}
		g_screen_reader = (bool)screen_reader;
		tts_config_set_screen_reader_callback(client->uid, __tts_screen_reader_changed_cb);
	} else {
		tts_config_unset_screen_reader_callback(client->uid);
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_get_mode(tts_h tts, tts_mode_e* mode)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get TTS mode");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'"); 
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (NULL == mode) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter(mode) is NULL");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	} 

	*mode = client->mode;
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

static void* __fork_tts_daemon(void* data)
{
	char daemon_path[64] = {'\0',};
	int pid, i;
	tts_mode_e mode;

	mode = (tts_mode_e)data;

	if (TTS_MODE_DEFAULT == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon");
	} else if (TTS_MODE_NOTIFICATION == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon-noti");
	} else if (TTS_MODE_SCREEN_READER == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon-sr");
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "mode is not valid");
		return (void*)-1;
	}
	
	/* fork-exec daemom */
	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_TTSC, "Fail to create daemon");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl(daemon_path, daemon_path, NULL);
		break;

	default:
		break;
	}

	return (void*) 1;
}

static Eina_Bool __tts_connect_daemon(void *data)
{
	tts_h tts = (tts_h)data;
	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		g_connect_timer = NULL;
		return EINA_FALSE;
	}

	/* Send hello */
	if (0 != tts_dbus_request_hello(client->uid)) {
		pthread_t thread;
		int thread_id;

		if (TTS_MODE_SCREEN_READER == client->mode) {
			if (false == g_is_sr_daemon_started) {
				g_is_sr_daemon_started = true;
				thread_id = pthread_create(&thread, NULL, __fork_tts_daemon, (void*)TTS_MODE_SCREEN_READER);
				if (thread_id < 0) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to make thread");
					g_connect_timer = NULL;
					return EINA_FALSE;
				}
				pthread_detach(thread);
			}
		} else if (TTS_MODE_NOTIFICATION == client->mode) {
			if (false == g_is_noti_daemon_started) {
				g_is_noti_daemon_started = true;
				thread_id = pthread_create(&thread, NULL, __fork_tts_daemon, (void*)TTS_MODE_NOTIFICATION);
				if (thread_id < 0) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to make thread");
					g_connect_timer = NULL;
					return EINA_FALSE;
				}
				pthread_detach(thread);
			}
		} else {
			if (false == g_is_daemon_started) {
				g_is_daemon_started = true;
				thread_id = pthread_create(&thread, NULL, __fork_tts_daemon, (void*)TTS_MODE_DEFAULT);
				if (thread_id < 0) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to make thread");
					g_connect_timer = NULL;
					return EINA_FALSE;
				}
				pthread_detach(thread);
			}
		}
		
		return EINA_TRUE;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "===== Connect daemon");

	/* do request initialize */
	int ret = -1;
	ret = tts_dbus_request_initialize(client->uid);

	if (TTS_ERROR_ENGINE_NOT_FOUND == ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to initialize : %s", __tts_get_error_code(ret));
		
		client->reason = TTS_ERROR_ENGINE_NOT_FOUND;
		client->utt_id = -1;

		ecore_timer_add(0, __tts_notify_error, (void*)client->tts);
		g_connect_timer = NULL;
		return EINA_FALSE;

	} else if (TTS_ERROR_NONE != ret) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to connection. Retry to connect : %s", __tts_get_error_code(ret));
		return EINA_TRUE;

	} else {
		/* success to connect tts-daemon */
	}

	g_connect_timer = NULL;

	if (0 == tts_client_get_mode_client_count(client->mode)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Open file msg connection : mode(%d)", client->mode);
		ret = tts_file_msg_open_connection(client->mode);
		if (0 != ret) {
			SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to open file message connection");
			ecore_timer_add(0, __tts_notify_error, (void*)client->tts);
			return EINA_FALSE;
		}
	}

	client = tts_client_get(tts);
	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		return EINA_FALSE;
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "State changed callback is NULL");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return EINA_FALSE;
}

int tts_prepare(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Prepare TTS");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_CREATED) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'CREATED'"); 
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	g_connect_timer = ecore_timer_add(0, __tts_connect_daemon, (void*)tts);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

int tts_unprepare(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Unprepare TTS");

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not available");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* check state */
	if (client->current_state != TTS_STATE_READY) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Invalid State: Current state is not 'READY'"); 
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	if (!(false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode)) {
		while (0 != ret) {
			ret = tts_dbus_request_finalize(client->uid);
			if (0 != ret) {
				if (TTS_ERROR_TIMED_OUT != ret) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
					break;
				} else {
					SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry finalize : %s", __tts_get_error_code(ret));
					usleep(10);
					count++;
					if (10 == count) {
						SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
						break;
					}
				}
			}
		}
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Do not request finalize : g_sr(%d) mode(%d)", g_screen_reader, client->mode);
	}

	if (TTS_MODE_SCREEN_READER == client->mode) 
		g_is_sr_daemon_started = false;
	else if (TTS_MODE_NOTIFICATION == client->mode) 
		g_is_noti_daemon_started = false;
	else 
		g_is_daemon_started = false;

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_CREATED;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	/* Close file message connection */
	if (0 == tts_client_get_mode_client_count(client->mode)) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Close file msg connection : mode(%d)", client->mode);
		ret = tts_file_msg_close_connection(client->mode);
		if (0 != ret)
			SLOG(LOG_WARN, TAG_TTSC, "[ERROR] Fail to close file message connection");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

bool __tts_supported_voice_cb(const char* engine_id, const char* language, int type, void* user_data)
{
	tts_h tts = (tts_h)user_data;

	tts_client_s* client = tts_client_get(tts);
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[WARNING] A handle is not valid");
		return false;
	}

	/* call callback function */
	if (NULL != client->supported_voice_cb) {
		return client->supported_voice_cb(tts, language, (tts_voice_type_e)type, client->supported_voice_user_data);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of supported voice");
	}

	return false;
}

int tts_foreach_supported_voices(tts_h tts, tts_supported_voice_cb callback, void* user_data)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Foreach supported voices");

	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	int ret = 0;
	char* current_engine = NULL;
	ret = tts_config_mgr_get_engine(&current_engine);
	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to get current engine : %d", ret);
		return TTS_ERROR_OPERATION_FAILED;
	}

	client->supported_voice_cb = callback;
	client->supported_voice_user_data = user_data;

	ret = tts_config_mgr_get_voice_list(current_engine, __tts_supported_voice_cb, client->tts);

	if (NULL != current_engine)
		free(current_engine);

	client->supported_voice_cb = NULL;
	client->supported_voice_user_data = NULL;

	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Result : %d", ret);
		ret = TTS_ERROR_OPERATION_FAILED;
	}    

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_default_voice(tts_h tts, char** lang, tts_voice_type_e* vctype)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Get default voice");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;

	}
	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	/* Request call remote method */
	int ret = 0;
	ret = tts_config_mgr_get_voice(lang, (int*)vctype);
    	if (0 != ret) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %d", ret);
		ret = TTS_ERROR_OPERATION_FAILED;
	} else {
		SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "[DEBUG] Default language(%s), type(%d)", *lang, *vctype);
	}
	
	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_get_max_text_count(tts_h tts, int* count)
{
	if (NULL == tts || NULL == count) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}
	
	if (TTS_STATE_READY != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get max text count : Current state is NOT 'READY'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	*count = MAX_TEXT_COUNT;

	SLOG(LOG_DEBUG, TAG_TTSC, "Get max text count : %d", *count);
	return TTS_ERROR_NONE;
}

int tts_get_state(tts_h tts, tts_state_e* state)
{
	if (NULL == tts || NULL == state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get state : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get state : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	*state = client->current_state;

	switch(*state) {
		case TTS_STATE_CREATED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Created'");	break;
		case TTS_STATE_READY:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Ready'");		break;
		case TTS_STATE_PLAYING:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Playing'");	break;
		case TTS_STATE_PAUSED:	SLOG(LOG_DEBUG, TAG_TTSC, "Current state is 'Paused'");		break;
	}

	return TTS_ERROR_NONE;
}

int tts_add_text(tts_h tts, const char* text, const char* language, tts_voice_type_e voice_type, tts_speed_e speed, int* utt_id)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Add text");

	if (NULL == tts || NULL == utt_id) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input parameter is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Current state is 'CREATED'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	if (MAX_TEXT_COUNT < strlen(text)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input text size is too big.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	/* check valid utf8 */
	iconv_t *ict;
	ict = iconv_open("utf-8", "");
	if ((iconv_t)-1 == ict) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to init for text check");
		return TTS_ERROR_OPERATION_FAILED;
	}

	size_t len = strlen(text);
	char *in_tmp = NULL;
	char in_buf[MAX_TEXT_COUNT];
	char *out_tmp = NULL;
	char out_buf[MAX_TEXT_COUNT];
	size_t len_tmp = sizeof(out_buf);

	sprintf(in_buf, "%s", text);
	in_tmp = in_buf;

	memset(out_buf, 0, MAX_TEXT_COUNT);
	out_tmp = out_buf;

	size_t st;
	st = iconv(ict, &in_tmp, &len, &out_tmp, &len_tmp);
	if ((size_t)-1 == st) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Text is invalid - '%s'", in_buf);
		iconv_close(ict);
		return TTS_ERROR_INVALID_PARAMETER;
	}
	iconv_close(ict);
	SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "Text is valid - Converted text is '%s'", out_buf);

	/* change default language value */
	char* temp = NULL;

	if (NULL == language)
		temp = strdup("default");
	else 
		temp = strdup(language);

	client->current_utt_id ++;
	if (client->current_utt_id == 10000) {
		client->current_utt_id = 1;
	}

	/* do request */
	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_add_text(client->uid, out_buf, temp, voice_type, speed, client->current_utt_id);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				break;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry add text : %s", __tts_get_error_code(ret));
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					break;
				}
			}
		} else {
			*utt_id = client->current_utt_id;
		}
	}

	if (NULL != temp)	free(temp);

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return ret;
}

int tts_play(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Play tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid.");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_PLAYING == client->current_state || TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] The current state is invalid."); 
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_play(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry play : %s", __tts_get_error_code(ret));
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PLAYING;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_stop(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Stop tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED == client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Current state is 'CREATED'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_stop(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry stop : %s", __tts_get_error_code(ret));
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}

	client->before_state = client->current_state;
	client->current_state = TTS_STATE_READY;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}


int tts_pause(tts_h tts)
{
	SLOG(LOG_DEBUG, TAG_TTSC, "===== Pause tts");

	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Input handle is null");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] A handle is not valid");
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_PLAYING != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[Error] The Current state is NOT 'playing'. So this request should be not running.");    
		SLOG(LOG_DEBUG, TAG_TTSC, "=====");
		SLOG(LOG_DEBUG, TAG_TTSC, " ");
		return TTS_ERROR_INVALID_STATE;
	}

	if (false == g_screen_reader && TTS_MODE_SCREEN_READER == client->mode) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Screen reader option is NOT available. Ignore this request");
		return TTS_ERROR_INVALID_STATE;
	}

	int ret = -1;
	int count = 0;
	while (0 != ret) {
		ret = tts_dbus_request_pause(client->uid);
		if (0 != ret) {
			if (TTS_ERROR_TIMED_OUT != ret) {
				SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] result : %s", __tts_get_error_code(ret));
				return ret;
			} else {
				SLOG(LOG_WARN, TAG_TTSC, "[WARNING] retry pause : %s", __tts_get_error_code(ret));
				usleep(10);
				count++;
				if (10 == count) {
					SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to request");
					return ret;
				}
			}
		}
	}
	
	client->before_state = client->current_state;
	client->current_state = TTS_STATE_PAUSED;

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called");
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "=====");
	SLOG(LOG_DEBUG, TAG_TTSC, " ");

	return TTS_ERROR_NONE;
}

static Eina_Bool __tts_notify_error(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "Fail to notify error msg : A handle is not valid");
		return EINA_FALSE;
	}

	SLOG(LOG_DEBUG, TAG_TTSC, "Error data : uttid(%d) reason(%s)", client->utt_id, __tts_get_error_code(client->reason));

	if (NULL != client->error_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of error");
		tts_client_use_callback(client);
		client->error_cb(client->tts, client->utt_id, client->reason, client->error_user_data );
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of error ");
	}

	return EINA_FALSE;
}

int __tts_cb_error(int uid, tts_error_e reason, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	client->utt_id = utt_id;
	client->reason = reason;

	/* call callback function */
	if (NULL != client->error_cb) {
		ecore_timer_add(0, __tts_notify_error, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of error ");
	}
	
	return 0;
}

static Eina_Bool __tts_notify_state_changed(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "Fail to notify state changed : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->state_changed_cb) {
		tts_client_use_callback(client);
		client->state_changed_cb(client->tts, client->before_state, client->current_state, client->state_changed_user_data); 
		tts_client_not_use_callback(client);
		SLOG(LOG_DEBUG, TAG_TTSC, "State changed callback is called : pre(%d) cur(%d)", client->before_state, client->current_state);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	return EINA_FALSE;
}

int __tts_cb_set_state(int uid, int state)
{
	tts_client_s* client = tts_client_get_by_uid(uid);
	if( NULL == client ) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] The handle is not valid");
		return -1;
	}

	tts_state_e state_from_daemon = (tts_state_e)state;

	if (client->current_state == state_from_daemon) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Current state has already been %d", client->current_state);
		return 0;
	}

	if (NULL != client->state_changed_cb) {
		ecore_timer_add(0, __tts_notify_state_changed, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] State changed callback is null");
	}

	client->before_state = client->current_state;
	client->current_state = state_from_daemon;

	return 0;
}

static Eina_Bool __tts_notify_utt_started(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to notify utt started : A handle is not valid");
		return EINA_FALSE;
	}
	
	if (NULL != client->utt_started_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance started ");
		tts_client_use_callback(client);
		client->utt_started_cb(client->tts, client->utt_id, client->utt_started_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance started ");
	}

	return EINA_FALSE;
}

int __tts_cb_utt_started(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "utterance started : utt id(%d) ", utt_id);

	client->utt_id = utt_id;

	/* call callback function */
	if (NULL != client->utt_started_cb) {
		ecore_timer_add(0, __tts_notify_utt_started, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance started ");
	}

	return 0;
}

static Eina_Bool __tts_notify_utt_completed(void *data)
{
	tts_h tts = (tts_h)data;

	tts_client_s* client = tts_client_get(tts);

	/* check handle */
	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] Fail to notify utt completed : A handle is not valid");
		return EINA_FALSE;
	}

	if (NULL != client->utt_completeted_cb) {
		SLOG(LOG_DEBUG, TAG_TTSC, "Call callback function of utterance completed ");
		tts_client_use_callback(client);
		client->utt_completeted_cb(client->tts, client->utt_id, client->utt_completed_user_data);
		tts_client_not_use_callback(client);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance completed ");
	}

	return EINA_FALSE;
}

int __tts_cb_utt_completed(int uid, int utt_id)
{
	tts_client_s* client = tts_client_get_by_uid(uid);

	if (NULL == client) {
		SLOG(LOG_WARN, TAG_TTSC, "[WARNING] A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "utterance completed : uttid(%d) ", utt_id);

	client->utt_id = utt_id;

	/* call callback function */
	if (NULL != client->utt_completeted_cb) {
		ecore_timer_add(0, __tts_notify_utt_completed, client->tts);
	} else {
		SLOG(LOG_WARN, TAG_TTSC, "No registered callback function of utterance completed ");
	}

	return 0;
}

int tts_set_state_changed_cb(tts_h tts, tts_state_changed_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set state changed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = callback;
	client->state_changed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set state changed cb");

	return 0;
}

int tts_unset_state_changed_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset state changed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->state_changed_cb = NULL;
	client->state_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset state changed cb");

	return 0;
}

int tts_set_utterance_started_cb(tts_h tts, tts_utterance_started_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt started cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_started_cb = callback;
	client->utt_started_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set utt started cb");
	
	return 0;
}

int tts_unset_utterance_started_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt started cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_started_cb = NULL;
	client->utt_started_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset utt started cb");
	
	return 0;
}

int tts_set_utterance_completed_cb(tts_h tts, tts_utterance_completed_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set utt completed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_completeted_cb = callback;
	client->utt_completed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set utt completed cb");
	
	return 0;
}

int tts_unset_utterance_completed_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset utt completed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->utt_completeted_cb = NULL;
	client->utt_completed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset utt completed cb");
	return 0;
}

int tts_set_error_cb(tts_h tts, tts_error_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set error cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->error_cb = callback;
	client->error_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set error cb");
	
	return 0;
}

int tts_unset_error_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}
	
	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset error cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->error_cb = NULL;
	client->error_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset error cb");

	return 0;
}

int tts_set_default_voice_changed_cb(tts_h tts, tts_default_voice_changed_cb callback, void* user_data)
{
	if (NULL == tts || NULL == callback) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Set default voice changed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->default_voice_changed_cb = callback;
	client->default_voice_changed_user_data = user_data;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Set default voice changed cb");

	return 0;
}

int tts_unset_default_voice_changed_cb(tts_h tts)
{
	if (NULL == tts) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : Input parameter is null");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	tts_client_s* client = tts_client_get(tts);

	if (NULL == client) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : A handle is not valid");
		return TTS_ERROR_INVALID_PARAMETER;
	}

	if (TTS_STATE_CREATED != client->current_state) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Unset default voice changed cb : Current state is not 'Created'."); 
		return TTS_ERROR_INVALID_STATE;
	}

	client->default_voice_changed_cb = NULL;
	client->default_voice_changed_user_data = NULL;

	SLOG(LOG_DEBUG, TAG_TTSC, "[SUCCESS] Unset default voice changed cb");

	return 0;
}

#if 0
static int __get_cmd_line(char *file, char *buf) 
{
	FILE *fp = NULL;

	fp = fopen(file, "r");
	if (fp == NULL) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Get command line");
		return -1;
	}

	memset(buf, 0, 256);
	if (NULL == fgets(buf, 256, fp)) {
		SLOG(LOG_ERROR, TAG_TTSC, "[ERROR] Fail to fget command line");
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return 0;
}

static bool __tts_is_alive(char* daemon_path)
{
	DIR *dir;
	struct dirent *entry;
	struct stat filestat;
	
	int pid;
	char cmdLine[256] = {'\0',};
	char tempPath[256] = {'\0',};

	dir  = opendir("/proc");
	if (NULL == dir) {
		SLOG(LOG_ERROR, TAG_TTSC, "process checking is FAILED");
		return FALSE;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (0 != lstat(entry->d_name, &filestat)) {
			continue;
		}

		if (!S_ISDIR(filestat.st_mode)) {
			continue;
		}

		pid = atoi(entry->d_name);
		if (pid <= 0) continue;

		sprintf(tempPath, "/proc/%d/cmdline", pid);
		if (0 != __get_cmd_line(tempPath, cmdLine)) {
			continue;
		}

		if (0 == strncmp(cmdLine, daemon_path, strlen(daemon_path))) {
			SECURE_SLOG(LOG_DEBUG, TAG_TTSC, "%s is ALIVE !!", daemon_path);
			closedir(dir);
			return TRUE;
		}
	}

	closedir(dir);
	return FALSE;
}

static int __tts_check_tts_daemon(tts_mode_e mode)
{
	char daemon_path[64] = {'\0',};
	int pid, i;

	if (TTS_MODE_DEFAULT == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon");
	} else if (TTS_MODE_NOTIFICATION == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon-noti");
	} else if (TTS_MODE_SCREEN_READER == mode) {
		strcpy(daemon_path, "/usr/bin/tts-daemon-sr");
	} else {
		SLOG(LOG_ERROR, TAG_TTSC, "mode is not valid");
		return -1;
	}

	if (TRUE == __tts_is_alive(daemon_path)) {
		return 0;
	}
	
	/* fork-exec daemom */
	pid = fork();

	switch(pid) {
	case -1:
		SLOG(LOG_ERROR, TAG_TTSC, "Fail to create daemon");
		break;

	case 0:
		setsid();
		for (i = 0;i < _NSIG;i++)
			signal(i, SIG_DFL);

		execl(daemon_path, daemon_path, NULL);
		break;

	default:
		break;
	}

	return 0;
}
#endif