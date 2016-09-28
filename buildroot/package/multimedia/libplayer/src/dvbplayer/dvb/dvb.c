
#include "dvb.h"
#include "am_fend.h"
#include "am_av.h"
#include <log_print.h>
#include "am_dmx.h"
#include "am_util.h"
#include "am_parser.h"
#include <pthread.h>
#include <time.h>
#include <errno.h> 


#include <stdio.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/mman.h>

enum dvb_status_code
{
  AM_DVB_STATUS_INIT,
  AM_DVB_STATUS_PAT,
  AM_DVB_STATUS_PMT,
  AM_DVB_STATUS_SDT,
  AM_DVB_STATUS_AV,
  AM_DVB_STATUS_STOP,
	AM_DVB_STATUS_END
};

typedef struct dvb_section_struct
{
  int freq;
	int pmt_numbers;
	int pmt_no;
	pat_section_info_t *pat_info;
	pmt_section_info_t **pmt_info;
	sdt_section_info_t *sdt_info;
	struct  dvb_section_struct *next;
}dvb_section_struct_t;

typedef struct dvb_struct
{
	char initd;
  int  mode;
	int  freq;
	int  symbol_rate;
	int  qam;
	int  v_pid;
	int  a_pid;
	char v_type;
	char a_type;
	int  fe_id;
	int  dmx_id;
	int  av_id;
	int  av_source;
	int  dmx_source;
	int  status;
	int  fid;
	int  freq_numbers;
	int  *freq_list;
	dvb_section_struct_t   *section;
	dvb_section_struct_t   *current_section;
	aml_dvb_channel_info_t *listChannels;
}dvb_struct_t;

static dvb_struct_t dvb;

static pthread_mutex_t   mutex;
static pthread_cond_t    cond; 

static dvb_section_struct_t *new_dvb_section_info(void);
static void section_parser(int dev_no, int fid, const uint8_t *data, int len, void *user_data);
static void aml_free_section(dvb_section_struct_t *tmp);

static dvb_section_struct_t *new_dvb_section_info(void)
{
    dvb_section_struct_t *tmp = NULL;

    tmp = malloc(sizeof(dvb_section_struct_t));
    if(tmp)
    {
        tmp->freq        = 0;
        tmp->pmt_numbers = 0;
        tmp->pmt_no      = 0;
        tmp->pat_info    = dvb_psi_new_pat_info();
        tmp->pmt_info    = NULL;
        tmp->sdt_info    = dvb_si_new_sdt_info();
        tmp->next        = NULL;
    }

    return tmp;
}

static void section_parser(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
  dvb_struct_t *dvb = (dvb_struct_t *)user_data;
	
	switch(dvb->status) {
		case AM_DVB_STATUS_PAT:
			log_print("section_parser PAT\n");
      if(dvb_psi_parse_pat(data, len, dvb->current_section->pat_info) == AM_SUCCESS) {
	      dvb_psi_dump_pat_info(dvb->current_section->pat_info);

	      if(AM_DMX_StopFilter(dvb->dmx_id, dvb->fid) != AM_SUCCESS) {
		      log_print("AM_DMX_StopFilter PAT failed\n");
	      }

        pthread_mutex_lock(&mutex);
        dvb->current_section->pmt_numbers = dvb_psi_get_pmt_numbers(dvb->current_section->pat_info);
        dvb->status = dvb->current_section->pmt_numbers ? AM_DVB_STATUS_PMT : AM_DVB_STATUS_END;
        pthread_cond_signal(&cond);
        log_print("dvb->status = AM_DVB_STATUS_PMT %d\n", dvb->current_section->pmt_numbers);
        pthread_mutex_unlock(&mutex);
      }
      break;
		case AM_DVB_STATUS_PMT:
			log_print("section_parser PMT %d\n",dvb->current_section->pmt_no);
      if(dvb_psi_parse_pmt(data, len, dvb->current_section->pmt_info[dvb->current_section->pmt_no]) == AM_SUCCESS) {
	      dvb_psi_dump_pmt_info(dvb->current_section->pmt_info[dvb->current_section->pmt_no]);

	      if(AM_DMX_StopFilter(dvb->dmx_id, dvb->fid) != AM_SUCCESS) {
		      log_print("AM_DMX_StopFilter PMT failed\n");
	      }

        pthread_mutex_lock(&mutex);
        //dvb->status = AM_DVB_STATUS_SDT;
        dvb->current_section->pmt_no++;
        pthread_cond_signal(&cond);
        log_print("dvb->status = AM_DVB_STATUS_PMT\n");
        pthread_mutex_unlock(&mutex);
      }
      break;
		case AM_DVB_STATUS_SDT:
			log_print("section_parser SDT\n");
      if(dvb_si_parse_sdt(data, len, dvb->current_section->sdt_info) == AM_SUCCESS) {
	      dvb_si_dump_sdt_info(dvb->current_section->sdt_info, &dvb->listChannels, dvb->freq);

	      if(AM_DMX_StopFilter(dvb->dmx_id, dvb->fid) != AM_SUCCESS) {
		      log_print("AM_DMX_StopFilter SDT failed\n");
	      }

        pthread_mutex_lock(&mutex);
        dvb->status = AM_DVB_STATUS_END;
        pthread_cond_signal(&cond);
        log_print("dvb->status = AM_DVB_STATUS_END\n");
        pthread_mutex_unlock(&mutex);
      }
      break;
		default:
			break;
	}
}

int aml_dvb_init(aml_dvb_init_param_t param_init)
{
	AM_FEND_OpenPara_t para;
	AM_AV_OpenPara_t para_av;
	AM_DMX_OpenPara_t para_dmx;

	if(dvb.initd)
	{
		return 0;
	}

  log_print("start aml_dvb_init\n");
	memset(&para,0,sizeof(AM_FEND_OpenPara_t));
	memset(&para_av,0,sizeof(AM_AV_OpenPara_t));
	memset(&para_dmx,0,sizeof(AM_DMX_OpenPara_t));
  memset(&dvb,0,sizeof(dvb_struct_t));
	dvb.freq=-1;
	dvb.symbol_rate=-1;
	dvb.qam=-1;
	dvb.v_pid=0x1fff;
	dvb.a_pid=0x1fff;
	dvb.v_type=0xff;
	dvb.a_type=0xff;
	dvb.fid = -1;

	dvb.mode=param_init.dvb_mode;
  dvb.fe_id=param_init.fe_id;
	dvb.dmx_id=param_init.dmx_id;
	dvb.av_id=param_init.av_id;
	dvb.av_source=param_init.av_source;
	dvb.dmx_source=param_init.dmx_source;
	dvb.freq_numbers = param_init.freq_numbers;
	if(!dvb.freq_numbers) {
		log_print("error: no freq lists\n");
		return -1;
	}
	dvb.freq_list = malloc(sizeof(int) * dvb.freq_numbers);
	if(!dvb.freq_list) {
		log_print("error: no mem for freq lists\n");
		return -1;
	}
	memcpy(dvb.freq_list, param_init.freq_list, sizeof(int) * dvb.freq_numbers);

	if(AM_FEND_Open(dvb.fe_id,&para) != AM_SUCCESS) {
		log_print("AM_FEND_Open failed\n");
		return -1;
	}
	if(AM_AV_Open(dvb.av_id,&para_av) != AM_SUCCESS) {
		log_print("AM_AV_Open failed\n");
		return -1;
	}
	if(AM_DMX_Open(dvb.dmx_id,&para_dmx) != AM_SUCCESS) {
		log_print("AM_DMX_Open failed\n");
		return -1;
	}

  pthread_mutex_init(&mutex,NULL);
  pthread_cond_init(&cond,NULL); 

	dvb.status = AM_DVB_STATUS_INIT;
	dvb.initd=1;

	log_print("aml_dvb_init done\n");
	return 0;
}

static void aml_free_section(dvb_section_struct_t *tmp)
{
	if(!tmp) {
		return;
	}
	dvb_psi_free_pat_info(tmp->pat_info);
  for(tmp->pmt_no=0; tmp->pmt_no<tmp->pmt_numbers; tmp->pmt_no++) {
    dvb_psi_free_pmt_info(tmp->pmt_info[tmp->pmt_no]);
  }
  dvb_si_free_sdt_info(tmp->sdt_info);
  free(tmp);

  return;
}

int aml_dvb_start_section(aml_dvb_param_t parm)
{
	int i = 0, ret = 0;
	struct dmx_sct_filter_params param;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	unsigned short program_number;
  unsigned short program_map_pid;
  struct timespec timeout;
  dvb_section_struct_t *tmp_section = NULL;


	log_print("aml_dvb_start_section\n");
	if(!dvb.initd)
	{
		log_print("dvb not initialized\n");
		return -1;
	}

  dvb.status = AM_DVB_STATUS_PAT;
  dvb.current_section = NULL;

  switch(dvb.mode) {
    case 0: // DVB-C
      p.frequency = parm.freq;
      p.u.qam.symbol_rate =parm.symbol_rate;
      p.u.qam.fec_inner = FEC_AUTO;
      p.u.qam.modulation =parm.qam;
      log_print("_____ parm.freq %d\n", parm.freq); 
      log_print("_____ parm.symbol_rate %d\n", parm.symbol_rate); 
      log_print("_____ parm.qam %d\n", parm.qam); 
      break;
    case 1: // DVB-T
      p.frequency = parm.freq;
      p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
      p.u.ofdm.code_rate_HP = FEC_AUTO;
      p.u.ofdm.code_rate_LP = FEC_AUTO;
      p.u.ofdm.constellation = QAM_AUTO;
      p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
      p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
      p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
      log_print("_____ parm.freq %d\n", parm.freq); 
      log_print("_____ parm.bandwidth %d\n", p.u.ofdm.bandwidth); 
      break;
    default:
      log_print("dvb mode error %d\n", dvb.mode);
      return -1;
  }

	if(dvb.freq!=parm.freq)
	{
		dvb.freq=parm.freq;
		dvb.symbol_rate=parm.symbol_rate;
		dvb.qam=parm.qam;
		log_print("*******___lock new freq %d \n",parm.freq);

		if(AM_FEND_Lock(/*FEND_DEV_NO*/dvb.fe_id, &p, &status) != AM_SUCCESS) {
		  log_print("AM_FEND_Lock failed\n");
		  return -1;
		}
	}

	if(AM_DMX_SetSource(dvb.dmx_id,dvb.dmx_source) != AM_SUCCESS) {
		log_print("AM_DMX_SetSource failed\n");
		return -1;
	}

	if(AM_AV_SetTSSource(dvb.av_id,dvb.dmx_source) != AM_SUCCESS) {
		log_print("AM_AV_SetTSSource failed\n");
		return -1;
  }

	if(AM_DMX_AllocateFilter(dvb.dmx_id, &dvb.fid) != AM_SUCCESS) {
		log_print("AM_DMX_AllocateFilter failed\n");
		return -1;
	}

  tmp_section = new_dvb_section_info();
  if(tmp_section == NULL) {
		log_print("can't alloc section structure\n");
		ret = -1;
		goto SECTION_END;
  }
  tmp_section->freq = parm.freq;
  dvb.current_section = tmp_section;

	if(AM_DMX_SetBufferSize(dvb.dmx_id, dvb.fid, 32*1024) != AM_SUCCESS) {
	  log_print("AM_DMX_SetBufferSize failed\n");
		ret = -1;
		goto SECTION_END;
	}
	if(AM_DMX_SetCallback(dvb.dmx_id, dvb.fid, section_parser, &dvb) != AM_SUCCESS) {
		log_print("AM_DMX_SetCallback failed\n");
		ret = -1;
		goto SECTION_END;
	}

  while(dvb.status != AM_DVB_STATUS_END) {

  	switch(dvb.status) {
  		case AM_DVB_STATUS_PAT:
	      memset(&param, 0, sizeof(param));
	      for(i=0; i<DMX_FILTER_SIZE; i++) {
	        param.filter.mode[i] = 0xff;
	      }
	      param.pid = 0;
	      param.filter.filter[0] = 0;
	      param.filter.mask[0] = 0xff;
	      param.filter.mode[0] = 0;

	      param.flags = DMX_CHECK_CRC;
	
	      if(AM_DMX_SetSecFilter(dvb.dmx_id, dvb.fid, &param) != AM_SUCCESS) {
		      log_print("AM_DMX_SetSecFilter PAT failed\n");
		      ret = -1;
		      goto SECTION_END;
	      }

	      if(AM_DMX_StartFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
		      log_print("AM_DMX_StartFilter PAT failed\n");
		      ret = -1;
		      goto SECTION_END;
	      }

        log_print("PAT filter started\n");
        break;
  		case AM_DVB_STATUS_PMT:
  			if(dvb.current_section->pmt_no == 0) {
  				dvb.current_section->pmt_info = (pmt_section_info_t **)malloc(sizeof(pmt_section_info_t *)*dvb.current_section->pmt_numbers);
  				if(dvb.current_section->pmt_info == NULL) {
		        log_print("dvb->pmt_info malloc failed\n");
		        ret = -1;
		        goto SECTION_END;
  				}
  				else {
            for(i=0; i<dvb.current_section->pmt_numbers; i++) {
              dvb.current_section->pmt_info[i] = dvb_psi_new_pmt_info();
            }
          }
  			}
  			if(dvb.current_section->pmt_no < dvb.current_section->pmt_numbers) {
  			  dvb_psi_get_program_info(dvb.current_section->pat_info, dvb.current_section->pmt_no, &program_map_pid, &program_number);

	        memset(&param, 0, sizeof(param));
	        for(i=0; i<DMX_FILTER_SIZE; i++) {
	          param.filter.mode[i] = 0xff;
	        }
	        param.pid = program_map_pid;
	        param.filter.filter[0] = 0x02; // Table ID
	        param.filter.mask[0] = 0xff;
	        param.filter.mode[0] = 0;

				  // the 2 bytes between table id and program number are always skipped by demux driver

	        param.filter.filter[1] = (program_number >> 8) & 0xff; // Program number Hi
	        param.filter.mask[1] = 0xff;
	        param.filter.mode[1] = 0;

	        param.filter.filter[2] = program_number & 0xff; // Program number Lo
	        param.filter.mask[2] = 0xff;
	        param.filter.mode[2] = 0;

	        param.flags = DMX_CHECK_CRC;
	
	        if(AM_DMX_SetSecFilter(dvb.dmx_id, dvb.fid, &param) != AM_SUCCESS) {
		        log_print("AM_DMX_SetSecFilter PMT failed\n");
		        ret = -1;
		        goto SECTION_END;
	        }
	        if(AM_DMX_StartFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
		        log_print("AM_DMX_StartFilter PMT failed\n");
		        ret = -1;
		        goto SECTION_END;
	        }

          log_print("PMT filter started: 0x%x, 0x%x, 0x%x\n", param.filter.filter[0], param.filter.filter[1], param.filter.filter[2]);
  			  break;
        }
        else {
        	log_print("dvb->status = AM_DVB_STATUS_SDT\n");
        	pthread_mutex_lock(&mutex);
        	dvb.status = AM_DVB_STATUS_SDT;
        	pthread_mutex_unlock(&mutex);
        }
  		case AM_DVB_STATUS_SDT:
	      memset(&param, 0, sizeof(param));
	      for(i=0; i<DMX_FILTER_SIZE; i++) {
	        param.filter.mode[i] = 0xff;
	      }
	      param.pid = 0x11;
	      param.filter.filter[0] = 0x42; // table ID
	      param.filter.mask[0] = 0xff;
	      param.filter.mode[0] = 0;

	      param.flags = DMX_CHECK_CRC;
	
	      if(AM_DMX_SetSecFilter(dvb.dmx_id, dvb.fid, &param) != AM_SUCCESS) {
		      log_print("AM_DMX_SetSecFilter SDT failed\n");
		      ret = -1;
		      goto SECTION_END;
	      }

	      if(AM_DMX_StartFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
		      log_print("AM_DMX_StartFilter SDT failed\n");
		      ret = -1;
		      goto SECTION_END;
	      }

        log_print("SDT filter started\n");
        break;
  		default:
  			break;
  	}

    timeout.tv_sec  = time(NULL)+5;
    timeout.tv_nsec = 0;
  	pthread_mutex_lock(&mutex);
  	ret = pthread_cond_timedwait(&cond,&mutex, (const struct timespec *)&timeout);
  	if(ret == ETIMEDOUT) {
  		log_print("*****timeout occurred\n");
  		if((dvb.status == AM_DVB_STATUS_PMT) &&
  			 (dvb.current_section->pmt_no + 1 < dvb.current_section->pmt_numbers)) {
  			 	dvb.current_section->pmt_no++;
  		}
  		else if(dvb.status == AM_DVB_STATUS_SDT) {
        aml_dvb_channel_info_t *tempChannel = NULL;
        aml_dvb_channel_info_t *tmpChannel = NULL;
        unsigned short program_map_pid = 0;

        for(i=0; i<dvb.current_section->pmt_numbers; i++) {
          tempChannel = malloc(sizeof(aml_dvb_channel_info_t));
          if(tempChannel) {
            tempChannel->freq = dvb.freq;
            dvb_psi_get_program_info(dvb.current_section->pat_info, i, &program_map_pid, &tempChannel->service_id);
            tempChannel->service_name = NULL;
            tempChannel->next = NULL;

            if(dvb.listChannels == NULL) {
              dvb.listChannels = tempChannel;
            }
            else {
              tmpChannel = dvb.listChannels;
              while(tmpChannel->next) {
                tmpChannel = tmpChannel->next;
              }
              tmpChannel->next = tempChannel;
            }
          }
        }
        dvb.status = AM_DVB_STATUS_END;
      }
  		else {
  		  dvb.status = AM_DVB_STATUS_END;
  		  ret = -1;
  		}
  	}
  	pthread_mutex_unlock(&mutex);

  }

SECTION_END:
	if(ret < 0) {
		aml_dvb_stop();
		aml_free_section(dvb.current_section);
	}
	else {
		if(dvb.section == NULL) {
			dvb.section = dvb.current_section;
		}
		else {
		  tmp_section = dvb.section;
		  while(tmp_section->next) {
		  	tmp_section = tmp_section->next;
		  }
		  tmp_section->next = dvb.current_section;
		}
	}
	if(AM_DMX_FreeFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
    log_print("AM_DMX_FreeFilterr failed\n");
    ret = -1;
	}
	log_print("END*******___\n");
	return ret;

}

int aml_dvb_start_pes(int program_index)
{
	int i = 0;
	struct dvb_frontend_parameters p;
	fe_status_t status;
  int numChannels = 0;
	aml_dvb_channel_info_t *listChannels = NULL;
	aml_dvb_param_t parm;
	unsigned short program_number;
  unsigned short program_map_pid;

  memset(&parm, 0, sizeof(parm));

	log_print("aml_dvb_start_pes\n");
	if(!dvb.initd)
	{
		log_print("dvb not initialized\n");
		return -1;
	}

  aml_dvb_store_sdt_info(&numChannels, &listChannels);
  if(program_index > numChannels) {
  	log_print("error! program_index %d > numChannels %d!\n", program_index, numChannels);
  	return -1;
  }

  for (i=0; i< program_index; i++) {
  	if(listChannels->next) {
      listChannels = listChannels->next;
    }
  }
  if(listChannels == NULL) {
  	log_print("error! can't find the program index!\n");
  	return -1;
  }
////////////////////////////////

  dvb.current_section = dvb.section;
  while(dvb.current_section) {
  	if(dvb.current_section->freq == listChannels->freq) {
      for(i=0; i<dvb.current_section->pmt_numbers; i++) {
      	dvb_psi_get_program_info(dvb.current_section->pat_info, i, &program_map_pid, &program_number);
        if(program_number == listChannels->service_id) {
      	  log_print("service_id 0x%x found in pmt[%d]\n", listChannels->service_id, i);
          break;
        }
      }
      if(i<dvb.current_section->pmt_numbers) {
        if(dvb_psi_get_program_stream_info(dvb.current_section->pmt_info[i], &parm) != -1) {
          break;
        }
        else {
        	log_print("error! can't find the video/audio pid!\n");
        }
      }
      else {
  	    log_print("error! can't find the program number!\n");
      }
    }
  	dvb.current_section = dvb.current_section->next;
  }
  if(dvb.current_section == NULL) {
    log_print("error! can't find the pes at freq %d!\n", listChannels->freq);
    return -1;
  }
////////////////////////////////


	if(dvb.freq!=listChannels->freq)
	{
		dvb.freq=listChannels->freq;

    switch(dvb.mode) {
      case 0: // DVB-C
        p.frequency = parm.freq;
        p.u.qam.symbol_rate =6875000;
        p.u.qam.fec_inner = FEC_AUTO;
        p.u.qam.modulation =3;
        log_print("_____ parm.freq %d\n", parm.freq); 
        log_print("_____ parm.symbol_rate %d\n", parm.symbol_rate); 
        log_print("_____ parm.qam %s\n", (parm.qam == 3) ? ("64-QAM") : ("Other")); 
        break;
      case 1: // DVB-T
        p.frequency = parm.freq;
        p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
        p.u.ofdm.code_rate_HP = FEC_AUTO;
        p.u.ofdm.code_rate_LP = FEC_AUTO;
        p.u.ofdm.constellation = QAM_AUTO;
        p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
        p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
        p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
        log_print("_____ parm.freq %d\n", parm.freq); 
        log_print("_____ parm.bandwidth %d\n", p.u.ofdm.bandwidth); 
        break;
      default:
        log_print("dvb mode error %d\n", dvb.mode);
        return -1;
    }

		log_print("*******___lock new freq %d \n",dvb.freq);

		if(AM_FEND_Lock(/*FEND_DEV_NO*/dvb.fe_id, &p, &status) != AM_SUCCESS) {
		  log_print("AM_FEND_Lock failed\n");
		  return -1;
		}
	}

  log_print("_____ parm.v_pid %d\n", parm.v_pid);
  log_print("_____ parm.a_pid %d\n", parm.a_pid);

  dvb.a_pid=parm.a_pid;
  dvb.v_pid=parm.v_pid;
  dvb.a_type=parm.a_type;
  dvb.v_type=parm.v_type;
  if(AM_AV_StartTS(dvb.av_id,dvb.v_pid,dvb.a_pid,dvb.v_type,dvb.a_type) !=AM_SUCCESS) {
    log_print("AM_AV_StartTS failed\n");
    return -1;
  }
  log_print("AV filter started!\n");

  dvb.status = AM_DVB_STATUS_AV;

	log_print("END*******___\n");
	return 0;

}

int aml_dvb_stop()
{
	switch(dvb.status) {
		case AM_DVB_STATUS_PAT:
		case AM_DVB_STATUS_PMT:
		case AM_DVB_STATUS_SDT:
			log_print("stop dvb.fid %d\n", dvb.fid);
			if(AM_DMX_StopFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
				log_print("AM_DMX_StopFilter failed\n");
				return -1;
			}
			dvb.status = AM_DVB_STATUS_STOP;
			break;
		case AM_DVB_STATUS_AV:
			log_print("stop dvb.av_id %d\n", dvb.av_id);
      if(AM_AV_StopTS(dvb.av_id) != AM_SUCCESS) {
  	    log_print("AM_AV_StopTS failed\n");
  	    return -1;
      }
      dvb.status = AM_DVB_STATUS_STOP;
			break;
		default:
			break;
	}
	return 0;

}

int aml_dvb_deinit()
{
	int ret=0;
	void *temp;
	void *tmp;

#if 1
        int i;
        volatile unsigned char *cp;
        int fd;
        volatile void *v;
        off_t nvram = 0xcf400000;
        size_t length = 0x300000 /*- 0x1000*/;
        FILE *file;

        if((fd = open("/dev/mem",O_RDWR)) != -1) {
          v = mmap(0, length, PROT_READ | PROT_WRITE, MAP_SHARED,fd,nvram);
          log_print("mmap returns %p\n", v);

          if ( (int)v == -1) {
            log_print("mmap");
          }
          else {
            file = fopen("/system/vbuf.bin", "wb");
            fwrite ((const void *)v, length, 1, file);
            fclose(file);   
          }
        }
        else {
          log_print("open /dev/mem");
        }
#endif


	if(dvb.initd == 0)
	{
		return 0;
	}

  if((dvb.status == AM_DVB_STATUS_PAT) ||
  	 (dvb.status == AM_DVB_STATUS_PMT) ||
  	 (dvb.status == AM_DVB_STATUS_SDT) ||
  	 (dvb.status == AM_DVB_STATUS_AV)) 
  {
    aml_dvb_stop();
  }

	//if(dvb.fid >= 0) {
	//  if(AM_DMX_FreeFilter(dvb.dmx_id, dvb.fid) != AM_SUCCESS) {
  //	  log_print("AM_DMX_FreeFilterr failed\n");
  //	  ret = -1;
	//  }
	//}
	if(AM_DMX_Close(dvb.dmx_id) != AM_SUCCESS) {
  	log_print("AM_DMX_Close failed\n");
  	ret = -1;
	}
	if(AM_AV_Close(dvb.av_id) != AM_SUCCESS) {
  	log_print("AM_AV_Close failed\n");
  	ret = -1;
	}
	if(AM_FEND_Close(dvb.fe_id) != AM_SUCCESS) {
  	log_print("AM_FEND_Close failed\n");
  	ret = -1;
	}

  tmp = (dvb_section_struct_t *)dvb.section;
  while(tmp) {
  	temp = ((dvb_section_struct_t *)tmp)->next;
  	aml_free_section(tmp);
  	tmp = temp;
  }
  dvb.section = NULL;

  tmp = (aml_dvb_channel_info_t *)dvb.listChannels;
  while(tmp) {
  	temp = ((aml_dvb_channel_info_t *)tmp)->next;
  	free(tmp);
  	tmp = temp;
  }
  dvb.listChannels = NULL;

  free(dvb.freq_list);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond); 

	dvb.initd=0;

	return ret;
}

int aml_dvb_store_sdt_info(int *numChannels, aml_dvb_channel_info_t **listChannels)
{
	aml_dvb_channel_info_t *tmp = dvb.listChannels;
	int i = 0;


  while(tmp) {
  	i++;
  	tmp = tmp->next;
  }

	*listChannels = dvb.listChannels;
	*numChannels = i;

	return 0;
}


