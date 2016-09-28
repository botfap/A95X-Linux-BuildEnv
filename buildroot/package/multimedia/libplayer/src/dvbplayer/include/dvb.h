
#ifndef _AML_DVB_H
#define _AML_DVB_H

typedef struct aml_dvb_param
{
	int freq;
	int symbol_rate;
	int qam;
	int v_pid;
	int a_pid;
	char v_type;
	char a_type;
	
}aml_dvb_param_t;

typedef struct aml_dvb_init_param
{
  int dvb_mode;
  int symbol_rate;
  int qam;
  int fe_id;
	int dmx_id;
	int av_id;
	int av_source;
	int dmx_source;
	int freq_numbers;
	int *freq_list;
}aml_dvb_init_param_t;

typedef struct aml_dvb_channel_info
{
    int freq;
    unsigned short  service_id;
    unsigned char   *service_name;
    struct aml_dvb_channel_info *next;
}aml_dvb_channel_info_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**\brief Initialize DVB device
 * \param[in] param_init Initialization parameters.
 * \return
 *   - 0  Success.
 *   - -1 Error.
 */
int aml_dvb_init(aml_dvb_init_param_t param_init);

/**\brief Release DVB device
 * \return
 *   - 0  Success.
 *   - -1 Error.
 */
int aml_dvb_deinit();

int aml_dvb_start(aml_dvb_param_t parm);

int aml_dvb_start_section(aml_dvb_param_t parm);

int aml_dvb_start_pes(int program_index);

int aml_dvb_stop();

int aml_dvb_set_volm(int vol);

int aml_dvb_store_sdt_info(int *numChannels, aml_dvb_channel_info_t **listChannels);




#ifdef __cplusplus
}
#endif

#endif
