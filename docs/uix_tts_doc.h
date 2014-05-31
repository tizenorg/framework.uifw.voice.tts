/*
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */


#ifndef __TIZEN_UIX_TTS_DOC_H__
#define __TIZEN_UIX_TTS_DOC_H__

/**
 * @defgroup CAPI_UIX_TTS_MODULE TTS
 * @ingroup CAPI_UIX_FRAMEWORK
 * @brief The @ref CAPI_UIX_TTS_MODULE API provides functions for synthesizing voice from text and playing synthesized sound data.
 * 
 * @section CAPI_UIX_TTS_MODULE_HEADER Required Header
 *   \#include <tts.h>
 * 
 * @section CAPI_UIX_TTS_MODULE_OVERVIEW Overview
 * A main function of Text-To-Speech(below TTS) API reads sound data transformed by engine using input text.
 * Applications can add input-text to queue for reading continuously and control player that can play, pause, and stop sound data synthesized from text.
 *
 * To use of TTS, use the following steps:<br>
 * 1. Create handle <br>
 * 2. Register callback functions for notifications <br> 
 * 3. Prepare tts-daemon asynchronously <br>
 * 4. Add text to queue for speech <br>
 * 5. Start synthesizing voice from text and play synthesized sound data <br>
 * 6. Pause the player <br>
 * 7. Resume the player <br>
 * 8. Stop the player <br>
 * 9. Destroy handle <br>
 * 
 * The TTS API also notifies you (by callback mechanism) when the state of TTS is changed, 
 * utterance is started and completed, default voice is changed or error occured.
 * An application should register callback functions: tts_state_changed_cb(), tts_utterance_started_cb(), 
 * tts_utterance_completed_cb(), tts_default_voice_changed_cb(), tts_error_cb().
 *
 * @section CAPI_UIX_TTS_MODULE_STATE_DIAGRAM State Diagram
 * The following diagram shows the life cycle and the states of the TTS.
 *
 * @image html capi_uix_tts_state_diagram.png "State diagram"
 *
 * @section CAPI_UIX_TTS_MODULE_STATE_TRANSITIONS State Transitions
 *
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>PRE-STATE</th>
 * <th>POST-STATE</th>
 * <th>SYNC TYPE</th>
 * </tr>
 * <tr>
 * <td>tts_prepare()</td>
 * <td>Created</td>
 * <td>Ready</td>
 * <td>ASYNC</td>
 * </tr>
 * <tr>
 * <td>tts_play()</td>
 * <td>Ready, Paused</td>
 * <td>Playing</td>
 * <td>SYNC</td>
 * </tr>
 * <tr>
 * <td>tts_stop()</td>
 * <td>Playing, Paused</td>
 * <td>Ready</td>
 * <td>SYNC</td>
 * </tr>
 * <tr>
 * <td>tts_pause()</td>
 * <td>Playing</td>
 * <td>Paused</td>
 * <td>SYNC</td>
 * </tr>
 * </table>
 *
 * @section CAPI_UIX_TTS_MODULE_STATE_DEPENDENT_FUNCTION_CALLS State Dependent Function Calls
 * The following table shows state-dependent function calls.
 * It is forbidden to call functions listed below in wrong states.
 * Violation of this rule may result in an unpredictable behavior.
 * 
 * <table>
 * <tr>
 * <th>FUNCTION</th>
 * <th>VALID STATES</th>
 * <th>DESCRIPTION</th>
 * </tr>
 * <tr>
 * <td>tts_create()</td>
 * <td>None</td>
 * <td>All functions must be called after tts_create()</td>
 * </tr>
 * <tr>
 * <td>tts_destroy()</td>
 * <td>Created, Ready, Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_set_mode()</td>
 * <td>Created</td>
 * <td>The application should set mode to use TTS for screen reader or notification like driving mode.</td>
 * </tr>
 * <tr>
 * <td>tts_get_mode()</td>
 * <td>Created</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_prepare()</td>
 * <td>Created</td>
 * <td>This function works asynchronously. If daemon fork is failed, application gets the error callback.</td>
 * </tr>
 * <tr>
 * <td>tts_unprepare()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_foreach_supported_voices()</td>
 * <td>Created, Ready, Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_get_default_voice()</td>
 * <td>Created, Ready, Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_get_state()</td>
 * <td>Created, Ready, Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_get_max_text_count()</td>
 * <td>Ready</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_add_text()</td>
 * <td>Ready, Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_play()</td>
 * <td>Ready, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_stop()</td>
 * <td>Playing, Paused</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>tts_pause()</td>
 * <td>Playing</td>
 * <td></td>
 * </tr>
 * <tr>
 * <td>
 * tts_set_state_changed_cb()<br>
 * tts_unset_state_changed_cb()<br>
 * tts_set_utterance_started_cb()<br>
 * tts_unset_utterance_started_cb()<br>
 * tts_set_utterance_completed_cb()<br>
 * tts_unset_utterance_completed_cb()<br>
 * tts_set_default_voice_changed_cb()<br>
 * tts_unset_default_voice_changed_cb()<br>
 * tts_set_error_cb()<br>
 * tts_unset_error_cb()</td>
 * <td>Created</td>
 * <td> All callback function should be registered in Created state </td>
 * </tr>
 * </table>
 * 
 *
 */

#endif /* __TIZEN_UIX_TTS_DOC_H__ */

