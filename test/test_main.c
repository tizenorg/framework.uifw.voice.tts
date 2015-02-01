/*
*  Copyright (c) 2011-2014 Samsung Electronics Co., Ltd All Rights Reserved 
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

#include <stdio.h>
#include <Ecore.h>

#include <tts.h>

#define TTS_STRDUP(src) 		((src != NULL) ? strdup(src) : NULL )

static tts_h g_tts;
static char* g_text = NULL;

Eina_Bool __tts_test_destroy(void *data);

static bool __tts_test_get_text_from_file(const char* path, char** text)
{
	if(!path) return 0;
	if(!text) return 0;

	FILE *fp = NULL;

	if((fp = fopen(path, "rb")) == NULL ) {
		printf("Fail to open file (%s)", path);
		return 0;
	}

	fseek(fp , 0 , SEEK_END);  
	
	int text_len = ftell(fp);
	if (0 >= text_len) {
		printf("File has no contents\n");
		fclose(fp);
		return 0;
	}
	printf("text_len(%d)\n", text_len);
	rewind(fp);

	*text = (char*)calloc(1, text_len+1);

	int result_len = 1;
	while (!feof(fp)) {
		result_len = fread(*text, sizeof(char), text_len, fp);
		if (result_len != text_len) {
			printf("Fail to read\n");
			break;
		}
	}
	*text[result_len] = '\0';

	fclose(fp);
	return 1;
}

Eina_Bool __tts_test_play(void *data)
{
	int utt_id;
	int ret;
	char* lang = NULL;

	lang = (char*)data;
	
	ret = tts_add_text(g_tts, g_text, lang, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &utt_id);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to add text\n");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

	printf("Play : utt id(%d)\n", utt_id);
	ret = tts_play(g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to play\n");
		ecore_timer_add(0, __tts_test_destroy, NULL);
		return EINA_FALSE;
	}

	return EINA_FALSE;
}

Eina_Bool __tts_test_destroy(void *data)
{
	int ret;
	printf("Stop\n");
	ret = tts_stop(g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to stop\n");
	}

	printf("Unprepare (Disconnection)\n");
	ret = tts_unprepare(g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to unprepare\n");
	}

	printf("Destory tts client\n");
	ret = tts_destroy(g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to destroy\n");
	}

	ecore_main_loop_quit();

	return EINA_FALSE;
}

static void __tts_test_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data)
{
	if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current) {
		printf("State is ready after prepare\n");
		ecore_timer_add(0, __tts_test_play, user_data);
	}

	return;
}

static void __tts_test_utt_started_cb(tts_h tts, int utt_id, void* user_data)
{
	printf("Utterance started : utt id(%d) \n", utt_id);

	return;
}

static void __tts_test_utt_completed_cb(tts_h tts, int utt_id, void* user_data)
{
	printf("Utterance completed : utt id(%d) \n", utt_id);
	ecore_timer_add(0, __tts_test_destroy, NULL);

	return;
}

int main (int argc, char *argv[])
{
	if (1 == argc || 5 < argc) {
		printf("Please check parameter\n");
		printf("Ex> tts-test 'text'\n");
		printf("Specific mode> tts-test 'text' '-sr || -noti'\n");
		return 0;
	}

	char* lang = NULL;
	char* src_path = NULL;

	int n = 0;

	while(NULL != argv[n]) {

		if(!strcmp("-h", argv[n])) {
			printf("\n");
			printf(" ==========================================\n");
			printf("  TTS test usage\n");
			printf(" ==========================================\n\n");
			printf("  -t : Synthesize text \n");
			printf("  -l : Determine langage to synthesize text, ex) en_US, ko_KR ...\n");
			printf("  -f : Determine file path which include text\n\n");
			printf(" ***************************************************\n");
			printf("    Example : #tts-test -l en_US -t \"1 2 3 4\" \n");
			printf(" ***************************************************\n");
			printf("\n");
			return 0;
		}
		
		// check langage option
		if(!strcmp("-l", argv[n])) {
			lang = TTS_STRDUP(argv[n+1]);
			printf("Language : %s\n", lang);
		}
		// check text to synthesize
		else if (!strcmp("-t", argv[n])) {
			g_text = TTS_STRDUP(argv[n+1]);
			printf("Text : %s\n", g_text);
		}
		// check file path to synthesize
		else if (!strcmp("-f", argv[n])) {
			src_path = TTS_STRDUP(argv[n+1]);
			printf("File path : %s\n", src_path);
			if(!__tts_test_get_text_from_file(src_path, &g_text)) {
				return 0;
			}
		}
		n++;
	}
	
	if(!g_text && !src_path) {
		printf("Invalid parameter, check help with command tts-test -h");
		return 0;
	}

//===================================

	tts_mode_e mode = TTS_MODE_DEFAULT;

	printf("  ");
	printf("  ");
	printf("===== TTS Sample start =====\n");

	printf("Input text : %s\n", g_text ? g_text : "NULL");
	printf("Input lang : %s\n", lang ? lang : "NULL");
	printf("Input file path : %s\n", src_path ? src_path : "NULL");

	if (!ecore_init()) {
		printf("[Main ERROR] Fail ecore_init()\n");
		return 0;
	}

	int ret;

	printf("Create tts client\n");
	ret = tts_create(&g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to create\n");
		return 0;
	}

	printf("Set tts mode - %d\n", mode);
	ret = tts_set_mode(g_tts, mode);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to set mode\n");
		tts_destroy(g_tts);
		return 0;
	}

	printf("Set Callback func\n");
	ret = tts_set_state_changed_cb(g_tts, __tts_test_state_changed_cb, (void*)lang);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to set state changed cb\n");
		tts_destroy(g_tts);
		return 0;
	}

	ret = tts_set_utterance_started_cb(g_tts, __tts_test_utt_started_cb, NULL);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to set utt started cb\n");
		tts_destroy(g_tts);
		return 0;
	}

	ret = tts_set_utterance_completed_cb(g_tts, __tts_test_utt_completed_cb, NULL);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to set utt completed cb\n");
		tts_destroy(g_tts);
		return 0;
	}

	printf("Prepare (Daemon Connection) asynchronously : Wait for ready state \n");
	ret = tts_prepare(g_tts);
	if (TTS_ERROR_NONE != ret) {
		printf("Fail to prepare\n");
		tts_destroy(g_tts);
		return 0;
	}

	ecore_main_loop_begin();

	ecore_shutdown();

	if(src_path) free(src_path);
	if(lang) free(lang);
	if(g_text) free(g_text);

	printf("===== TTS END =====\n\n\n");

	return 0;
}
