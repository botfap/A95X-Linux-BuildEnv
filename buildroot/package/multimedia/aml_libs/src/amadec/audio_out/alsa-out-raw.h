#ifndef ALSA_OUT_RAW_H
#define ALSA_OUT_RAW_H



int alsa_get_aml_card();
int alsa_get_spdif_port();
int alsa_swtich_port(alsa_param_t *alsa_params, int card, int port);
int alsa_init_raw(struct aml_audio_dec* audec);
int alsa_start_raw(struct aml_audio_dec* audec);
int alsa_pause_raw(struct aml_audio_dec* audec);
int alsa_resume_raw(struct aml_audio_dec* audec);
int alsa_stop_raw(struct aml_audio_dec* audec);
int dummy_alsa_control_raw(char * id_string , long vol, int rw, long * value);
int alsa_mute_raw(struct aml_audio_dec* audec, adec_bool_t en);


#endif

