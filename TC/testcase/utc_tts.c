#include <tet_api.h>
#include <tts.h>
#include <stdlib.h>
#include <Ecore.h>

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_tts_create_p(void);
static void utc_tts_create_n(void);
static void utc_tts_destroy_p(void);
static void utc_tts_destroy_n(void);
static void utc_tts_set_mode_p(void);
static void utc_tts_set_mode_n(void);
static void utc_tts_get_mode_p(void);
static void utc_tts_get_mode_n(void);
static void utc_tts_prepare_p(void);
static void utc_tts_prepare_n(void);
static void utc_tts_unprepare_p(void);
static void utc_tts_unprepare_n(void);
static void utc_tts_foreach_supported_voices_p(void);
static void utc_tts_foreach_supported_voices_n(void);
static void utc_tts_get_default_voice_p(void);
static void utc_tts_get_default_voice_n(void);
static void utc_tts_get_max_text_count_p(void);
static void utc_tts_get_max_text_count_n(void);
static void utc_tts_get_state_p(void);
static void utc_tts_get_state_n(void);
static void utc_tts_add_text_p(void);
static void utc_tts_add_text_n(void);
static void utc_tts_play_p(void);
static void utc_tts_play_n(void);
static void utc_tts_stop_p(void);
static void utc_tts_stop_n(void);
static void utc_tts_pause_p(void);
static void utc_tts_pause_n(void);
static void utc_tts_set_state_changed_cb_p(void);
static void utc_tts_set_state_changed_cb_n(void);
static void utc_tts_unset_state_changed_cb_p(void);
static void utc_tts_unset_state_changed_cb_n(void);
static void utc_tts_set_utterance_started_cb_p(void);
static void utc_tts_set_utterance_started_cb_n(void);
static void utc_tts_unset_utterance_started_cb_p(void);
static void utc_tts_unset_utterance_started_cb_n(void);
static void utc_tts_set_utterance_completed_cb_p(void);
static void utc_tts_set_utterance_completed_cb_n(void);
static void utc_tts_unset_utterance_completed_cb_p(void);
static void utc_tts_unset_utterance_completed_cb_n(void);
static void utc_tts_set_error_cb_p(void);
static void utc_tts_set_error_cb_n(void);
static void utc_tts_unset_error_cb_p(void);
static void utc_tts_unset_error_cb_n(void);
static void utc_tts_set_default_voice_changed_cb_p(void);
static void utc_tts_set_default_voice_changed_cb_n(void);
static void utc_tts_unset_default_voice_changed_cb_p(void);
static void utc_tts_unset_default_voice_changed_cb_n(void);

struct tet_testlist tet_testlist[] = {
	{utc_tts_create_p,	POSITIVE_TC_IDX},
	{utc_tts_create_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_state_changed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_set_state_changed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_utterance_started_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_set_utterance_started_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_utterance_completed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_set_utterance_completed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_default_voice_changed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_set_default_voice_changed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_error_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_set_error_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_set_mode_p,	POSITIVE_TC_IDX},
	{utc_tts_set_mode_n,	NEGATIVE_TC_IDX},
	{utc_tts_get_mode_p,	POSITIVE_TC_IDX},
	{utc_tts_get_mode_n,	NEGATIVE_TC_IDX},
	{utc_tts_prepare_p,	POSITIVE_TC_IDX},
	{utc_tts_prepare_n,	NEGATIVE_TC_IDX},
	{utc_tts_foreach_supported_voices_p,	POSITIVE_TC_IDX},
	{utc_tts_foreach_supported_voices_n,	NEGATIVE_TC_IDX},
	{utc_tts_get_default_voice_p,	POSITIVE_TC_IDX},
	{utc_tts_get_default_voice_n,	NEGATIVE_TC_IDX},
	{utc_tts_get_max_text_count_p,	POSITIVE_TC_IDX},
	{utc_tts_get_max_text_count_n,	NEGATIVE_TC_IDX},
	{utc_tts_get_state_p,	POSITIVE_TC_IDX},
	{utc_tts_get_state_n,	NEGATIVE_TC_IDX},
	{utc_tts_add_text_p,	POSITIVE_TC_IDX},
	{utc_tts_add_text_n,	NEGATIVE_TC_IDX},
	{utc_tts_play_p,	POSITIVE_TC_IDX},
	{utc_tts_play_n,	NEGATIVE_TC_IDX},
	{utc_tts_pause_p,	POSITIVE_TC_IDX},
	{utc_tts_pause_n,	NEGATIVE_TC_IDX},
	{utc_tts_stop_p,	POSITIVE_TC_IDX},
	{utc_tts_stop_n,	NEGATIVE_TC_IDX},
	{utc_tts_unprepare_p,	POSITIVE_TC_IDX},
	{utc_tts_unprepare_n,	NEGATIVE_TC_IDX},
	{utc_tts_unset_state_changed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_unset_state_changed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_unset_utterance_started_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_unset_utterance_started_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_unset_utterance_completed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_unset_utterance_completed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_unset_default_voice_changed_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_unset_default_voice_changed_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_unset_error_cb_p,	POSITIVE_TC_IDX},
	{utc_tts_unset_error_cb_n,	NEGATIVE_TC_IDX},
	{utc_tts_destroy_p,	POSITIVE_TC_IDX},
	{utc_tts_destroy_n,	NEGATIVE_TC_IDX},
	{NULL, 0},
};

static void startup(void)
{
	/* start of TC */
	tet_printf("\n TC start");
	ecore_init();
}

static void cleanup(void)
{
	/* end of TC */
	ecore_shutdown();
	tet_printf("\n TC end");
}

static tts_h g_tts;
static char *g_language;
static tts_voice_type_e g_voice_type;
static tts_state_e g_current_state;
static bool __tts_supported_voice_cb(tts_h tts, const char* language, tts_voice_type_e voice_type, void* user_data)
{
	return true;
}
static void __tts_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* user_data)
{
	g_current_state = current;
}
static void __tts_utterance_started_cb(tts_h tts, int utt_id, void* user_data)
{
}
static void __tts_utterance_completed_cb(tts_h tts, int utt_id, void *user_data)
{
}

static void __tts_error_cb(tts_h tts, int utt_id, tts_error_e reason, void* user_data)
{
}

static void __tts_default_voice_changed_cb(tts_h tts, const char* previous_language, tts_voice_type_e previous_voice_type, const char* current_language, tts_voice_type_e current_voice_type, void* user_data)
{
}

static void utc_tts_create_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_create(&g_tts);
	
	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_CREATED != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_create", ret, TTS_ERROR_NONE, "Fail to create tts");
}

static void utc_tts_create_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_create(NULL);
	dts_check_eq("tts_create", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_destroy_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_destroy(g_tts);
	dts_check_eq("tts_destroy", ret, TTS_ERROR_NONE, "Fail to destroy tts");
}

static void utc_tts_destroy_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_destroy(NULL);
	dts_check_eq("tts_destroy", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_mode_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_mode(g_tts, TTS_MODE_DEFAULT);
	dts_check_eq("tts_set_mode", ret, TTS_ERROR_NONE, "Fail to set mode");
}

static void utc_tts_set_mode_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_mode(NULL, TTS_MODE_DEFAULT);
	dts_check_eq("tts_set_mode", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_get_mode_p(void)
{
	tts_mode_e mode;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_mode(g_tts, &mode);
	dts_check_eq("tts_get_mode", ret, TTS_ERROR_NONE, "Fail to get mode");
}

static void utc_tts_get_mode_n(void)
{
	tts_mode_e mode;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_mode(NULL, &mode);
	dts_check_eq("tts_get_mode", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_prepare_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_prepare(g_tts);
		
	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_READY != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_prepare", ret, TTS_ERROR_NONE, "Fail to prepare");
}

static void utc_tts_prepare_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_prepare(NULL);
	dts_check_eq("tts_prepare", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unprepare_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unprepare(g_tts);

	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_CREATED != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_unprepare", ret, TTS_ERROR_NONE, "Fail to unprepare");
}

static void utc_tts_unprepare_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unprepare(NULL);
	dts_check_eq("tts_unprepare", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_foreach_supported_voices_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_foreach_supported_voices(g_tts, __tts_supported_voice_cb, NULL);
	dts_check_eq("tts_foreach_supported_voices", ret, TTS_ERROR_NONE, "Fail to foreach supported voices");
}

static void utc_tts_foreach_supported_voices_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_foreach_supported_voices(NULL, __tts_supported_voice_cb, NULL);
	dts_check_eq("tts_foreach_supported_voices", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_get_default_voice_p(void)
{
	g_language = NULL;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_default_voice(g_tts, &g_language, &g_voice_type);
	dts_check_eq("tts_get_default_voice", ret, TTS_ERROR_NONE, "Fail to get default voice");
}

static void utc_tts_get_default_voice_n(void)
{
	g_language = NULL;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_default_voice(NULL, &g_language, &g_voice_type);
	dts_check_eq("tts_get_default_voice", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_get_max_text_count_p(void)
{
	int count;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_max_text_count(g_tts, &count);
	dts_check_eq("tts_get_max_text_count", ret, TTS_ERROR_NONE, "Fail to get max text count");
}

static void utc_tts_get_max_text_count_n(void)
{
	int count;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_max_text_count(NULL, &count);
	dts_check_eq("tts_get_max_text_count", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_get_state_p(void)
{
	tts_state_e state;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_state(g_tts, &state);
	dts_check_eq("tts_get_state", ret, TTS_ERROR_NONE, "Fail to get state");
}

static void utc_tts_get_state_n(void)
{
	tts_state_e state;
	int ret = TTS_ERROR_NONE;
	ret = tts_get_state(NULL, &state);
	dts_check_eq("tts_get_state", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_add_text_p(void)
{
	int utt_id;
	int ret = TTS_ERROR_NONE;
	ret = tts_add_text(g_tts, "1 2 3", g_language, g_voice_type, TTS_SPEED_AUTO, &utt_id);
	dts_check_eq("tts_add_text", ret, TTS_ERROR_NONE, "Fail to add text");
}

static void utc_tts_add_text_n(void)
{
	int utt_id;
	int ret = TTS_ERROR_NONE;
	ret = tts_add_text(NULL, "1 2 3", g_language, g_voice_type, TTS_SPEED_AUTO, &utt_id);
	dts_check_eq("tts_add_text", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_play_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_play(g_tts);

	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_PLAYING != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_play", ret, TTS_ERROR_NONE, "Fail to play");
}

static void utc_tts_play_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_play(NULL);
	dts_check_eq("tts_play", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_stop_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_stop(g_tts);

	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_READY != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_stop", ret, TTS_ERROR_NONE, "Fail to stop");
}

static void utc_tts_stop_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_stop(NULL);
	dts_check_eq("tts_stop", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_pause_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_pause(g_tts);

	if (TTS_ERROR_NONE == ret) {
		while (TTS_STATE_PAUSED != g_current_state) {
			ecore_main_loop_iterate();
		}
	}

	dts_check_eq("tts_pause", ret, TTS_ERROR_NONE, "Fail to pause");
}

static void utc_tts_pause_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_pause(NULL);
	dts_check_eq("tts_stop", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_state_changed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_state_changed_cb(g_tts, __tts_state_changed_cb, NULL);
	dts_check_eq("tts_set_state_changed_cb", ret, TTS_ERROR_NONE, "Fail to set state changed callback");
}

static void utc_tts_set_state_changed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_state_changed_cb(NULL, __tts_state_changed_cb, NULL);
	dts_check_eq("tts_set_state_changed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unset_state_changed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_state_changed_cb(g_tts);
	dts_check_eq("tts_unset_state_changed_cb", ret, TTS_ERROR_NONE, "Fail to unset state changed callback");
}

static void utc_tts_unset_state_changed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_state_changed_cb(NULL);
	dts_check_eq("tts_unset_state_changed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_utterance_started_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_utterance_started_cb(g_tts, __tts_utterance_started_cb, NULL);
	dts_check_eq("tts_set_utterance_started_cb", ret, TTS_ERROR_NONE, "Fail to set utterance started callback");
}

static void utc_tts_set_utterance_started_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_utterance_started_cb(NULL, __tts_utterance_started_cb, NULL);
	dts_check_eq("tts_set_utterance_started_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unset_utterance_started_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_utterance_started_cb(g_tts);
	dts_check_eq("tts_unset_utterance_started_cb", ret, TTS_ERROR_NONE, "Fail to unset utternace started");
}

static void utc_tts_unset_utterance_started_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_utterance_started_cb(NULL);
	dts_check_eq("tts_unset_utterance_started_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_utterance_completed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_utterance_completed_cb(g_tts, __tts_utterance_completed_cb, NULL);
	dts_check_eq("tts_set_utterance_completed_cb", ret, TTS_ERROR_NONE, "Fail to set utterance completed callback");
}

static void utc_tts_set_utterance_completed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_utterance_completed_cb(NULL, __tts_utterance_completed_cb, NULL);
	dts_check_eq("tts_set_utterance_completed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unset_utterance_completed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_utterance_completed_cb(g_tts);
	dts_check_eq("tts_unset_utterance_completed_cb", ret, TTS_ERROR_NONE, "Fail to unset utterance completed callback");
}

static void utc_tts_unset_utterance_completed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_utterance_completed_cb(NULL);
	dts_check_eq("tts_unset_utterance_completed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_error_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_error_cb(g_tts, __tts_error_cb, NULL);
	dts_check_eq("tts_set_error_cb", ret, TTS_ERROR_NONE, "Fail to set error callback");
}

static void utc_tts_set_error_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_error_cb(NULL, __tts_error_cb, NULL);
	dts_check_eq("tts_set_error_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unset_error_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_error_cb(g_tts);
	dts_check_eq("tts_unset_error_cb", ret, TTS_ERROR_NONE, "Fail to unset error callback");
}

static void utc_tts_unset_error_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_error_cb(NULL);
	dts_check_eq("tts_unset_error_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_set_default_voice_changed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_default_voice_changed_cb(g_tts, __tts_default_voice_changed_cb, NULL);
	dts_check_eq("tts_set_default_voice_changed_cb", ret, TTS_ERROR_NONE, "Fail to set default voice changed callback");
}

static void utc_tts_set_default_voice_changed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_set_default_voice_changed_cb(NULL, __tts_default_voice_changed_cb, NULL);
	dts_check_eq("tts_set_default_voice_changed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}

static void utc_tts_unset_default_voice_changed_cb_p(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_default_voice_changed_cb(g_tts);
	dts_check_eq("tts_unset_default_voice_changed_cb", ret, TTS_ERROR_NONE, "Fail to unset default voice changed callback");
}

static void utc_tts_unset_default_voice_changed_cb_n(void)
{
	int ret = TTS_ERROR_NONE;
	ret = tts_unset_default_voice_changed_cb(NULL);
	dts_check_eq("tts_unset_default_voice_changed_cb", ret, TTS_ERROR_INVALID_PARAMETER, "Must return TTS_ERROR_INVALID_PARAMETER in case of invalid parameter");
}