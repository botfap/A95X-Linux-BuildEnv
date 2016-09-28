
#include "am_chmgr.h"

#include "chmgr_file.h"
#include "log_print.h"

int aml_load_channel_info(const char *filename,aml_dvb_param_t *param)
{

	int key_value=0;
	int ret=0;

	log_print("aml_load_channel_info %s\n",filename);

	ret=load_key_value(filename,"freq=",&key_value);
	if(ret)
	{
		log_print("**can not find freq\n");
		return -1;
	}
	param->freq=key_value;
	log_print("**param->freq %d\n", param->freq);

	ret=load_key_value(filename,"symbol_rate=",&key_value);
	if(ret)
	{
		log_print("**can not find symbol_rate\n");
		return -1;
	}
	param->symbol_rate=key_value;
	log_print("**param->symbol_rate %d\n", param->symbol_rate);

	ret=load_key_value(filename,"qam=",&key_value);
	if(ret)
	{
		log_print("**can not find qam\n");
		return -1;
	}
	param->qam=key_value;
  log_print("**param->qam %d\n", param->qam);

	ret=load_key_value(filename,"v_pid=",&key_value);
	if(ret)
	{
		log_print("**can not find v_pid\n");
		return -1;
	}
	param->v_pid=key_value;
	log_print("**param->v_pid %d\n", param->v_pid);

	ret=load_key_value(filename,"a_pid=",&key_value);
	if(ret)
	{
		log_print("**can not find a_pid\n");
		return -1;
	}
	param->a_pid=key_value;
	log_print("**param->a_pid %d\n", param->a_pid);

	ret=load_key_value(filename,"v_type=",&key_value);
	if(ret)
	{
		log_print("**can not find v_type\n");
		return -1;
	}
	param->v_type=key_value;
	log_print("**param->v_type %d\n", param->v_type);

	ret=load_key_value(filename,"a_type=",&key_value);
	if(ret)
	{
		log_print("**can not find a_type\n");
		return -1;
	}
	param->a_type=key_value;
	log_print("**param->a_type %d\n", param->a_type);

	return 0;


}

int aml_load_dvb_param(const char *filename,aml_dvb_init_param_t *param)
{
	int key_value=0;
	int *freq_list=0;
	int ret=0;

	if((!filename) || (!param))
	{
		return -1;		
	}

  log_print("aml_load_dvb_param %s\n",filename);

  ret=load_key_value(filename,"dvb_mode=",&key_value);
  if(ret)
  {
    log_print("**can not find dvb_mode\n");
    return -1;
  }
  param->dvb_mode=key_value;
  log_print("**param->dvb_mode %d\n", param->dvb_mode);

  ret=load_key_value(filename,"dvb_symbol_rate=",&key_value);
  if(ret)
  {
    log_print("**can not find symbol_rate\n");
    return -1;
  }
  param->symbol_rate=key_value;
  log_print("**param->symbol_rate %d\n", param->symbol_rate);

  ret=load_key_value(filename,"dvb_qam=",&key_value);
  if(ret)
  {
    log_print("**can not find qam\n");
    return -1;
  }
  param->qam=key_value;
  log_print("**param->qam %d\n", param->qam);
  
  ret=load_key_value(filename,"fe_id=",&key_value);
	if(ret)
	{
		log_print("**can not find fe_id\n");
		return -1;
	}
	param->fe_id=key_value;
	log_print("**param->fe_id %d\n", param->fe_id);

	ret=load_key_value(filename,"dmx_id=",&key_value);
	if(ret)
	{
		log_print("**can not find dmx_id\n");
		return -1;
	}
	param->dmx_id=key_value;
	log_print("**param->dmx_id %d\n", param->dmx_id);

	ret=load_key_value(filename,"av_id=",&key_value);
	if(ret)
	{
		log_print("**can not find av_id\n");
		return -1;
	}
	param->av_id=key_value;
	log_print("**param->av_id %d\n", param->av_id);

	ret=load_key_value(filename,"demux_source=",&key_value);
	if(ret)
	{
		log_print("**can not find demux_source\n");
		return -1;
	}
	param->dmx_source=key_value;
  log_print("**param->dmx_source %d\n", param->dmx_source);

	ret=load_key_value(filename,"av_source=",&key_value);
	if(ret)
	{
		log_print("**can not find av_source\n");
		return -1;
	}
	param->av_source=key_value;
	log_print("**param->av_source %d\n", param->av_source);

	ret=load_freq_value(filename,"freq=",&key_value, &freq_list);
	if(ret)
	{
		log_print("**can not find freq\n");
		return -1;
	}
	param->freq_numbers=key_value;
	param->freq_list=freq_list;
	log_print("**param->freq_numbers %d\n", param->freq_numbers);
	for(key_value=0; key_value<param->freq_numbers; key_value++) {
		log_print("  **param->freq_list[%d] %d\n", key_value, param->freq_list[key_value]);
	}

	return 0;


}

int aml_check_is_channel_info_file(const char *filename)
{
	int key_value=0;
	int ret=0;
	ret=load_key_value(filename,"channel_info=",&key_value);
	if(ret)
	{
		key_value=0;	
	}
	return key_value;	

}



