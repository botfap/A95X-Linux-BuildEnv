

#define AM_DEBUG_LEVEL 1

#include <am_debug.h>
#include <am_mem.h>
#include <amports/vformat.h>
#include <amports/aformat.h>
#include "am_parser_internal.h"
#include <string.h>
#include <assert.h>
#include <log_print.h>

dvb_parser_t pmt_parser[] =
{
/*
    {
        "ca",
        DVB_CA_DESCRIPTOR,
        0xff,
        dvb_ca_descriptor_parse,
        NULL
    },

    {
        "iso639 language",
        DVB_ISO_639_LANGUAGE_DESCRIPTOR,
        0xff,
        dvb_iso639_language_descriptor_parse,
        NULL
    },
*/
    {
        "AC3",
        DVB_AC3_DESCRIPTOR,
        0xff,
        dvb_descriptor_parse,
        NULL
    }
};

static dvb_parser_t sdt_parser[] =
{
    {
        "service",
        DVB_SERVICE_DESCRIPTOR,
        0xff,
        dvb_service_descriptor_parse,
        NULL
    }
#if 0
    ,
    {
        "multi service name",
        DVB_MULTILINGUAL_SERVICE_NAME_DESCRIPTOR,
        0xff,
        dvb_multi_service_name_descriptor_parse,
        NULL
    }
#endif
};

/* ascending sort !!! */
static const struct descriptor_ctx descriptor_map[] = 
{
    {
        "SERVICE",
        DVB_SERVICE_DESCRIPTOR,
        service_descriptor_new,
        service_descriptor_free,
        service_descriptor_dump,
        dvb_service_descriptor_parse
    }
#if 0
    ,

    {
        "MULTILINGUAL SERVICE NAME",
        DVB_MULTILINGUAL_SERVICE_NAME_DESCRIPTOR,
        multi_service_name_descriptor_new,
        multi_service_name_descriptor_free,
        multi_service_name_descriptor_dump,
        dvb_multi_service_name_descriptor_parse
    }
#endif
};

static struct descriptor_ctx *ext_descriptor_map = NULL;
static int ext_descriptor_num = 0;

static int find_descriptor(const struct descriptor_ctx descriptor[], int low, int high, unsigned tag)
{
    int mid = 0;

    while(low <= high)
    {
        mid = (low + high)>>1;
        if(tag > descriptor[mid].tag)
        {
            low = mid + 1;
        }
        else if(tag < descriptor[mid].tag)
        {
            high = mid - 1;
        }
        else
        {
            return mid;
        }
    }

    return -1;
}

static int find_ext_descriptor(unsigned char tag)
{
    int i = 0;
    
    if((ext_descriptor_map != NULL) && (ext_descriptor_num > 0))
    {
        return find_descriptor((const struct descriptor_ctx*)ext_descriptor_map, 0, ext_descriptor_num-1, tag);
    }
    
    return -1;
}

static void dvb_free_descriptor_info(descriptor_info_t *info)
{
    descriptor_info_t *tmp = NULL;
    descriptor_info_t *temp = NULL;
    int index = -1;

    if(info)
    {
        tmp = info->next;
        
        while(tmp)
        {
            if(tmp->descriptor)
            {
                index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, tmp->tag);
                if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
                {
                    descriptor_map[index].free_func((void*)tmp->descriptor);
                    tmp->descriptor = 0;
                }
                else
                {
                    index = find_ext_descriptor(tmp->tag);
                    if(index >= 0 && index < ext_descriptor_num)
                    {
                        ext_descriptor_map[index].free_func((void*)tmp->descriptor);
                        tmp->descriptor = 0;
                    }
                }
            }

            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }

        if(info->descriptor)
        {
            index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, info->tag);
            if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
            {
                descriptor_map[index].free_func((void*)info->descriptor);
                info->descriptor = 0;
            }
            else
            {
                index = find_ext_descriptor(info->tag);
                if(index >= 0 && index < ext_descriptor_num)
                {
                    ext_descriptor_map[index].free_func((void*)info->descriptor);
                    info->descriptor = 0;
                }
            }
        }
        
        free(info);
    }

    return ;
}

static int dvb_descriptor_parse(unsigned char* data, unsigned int length, unsigned int descriptor)
{
    int index = -1;
    
    if(data && length && descriptor)
    {
        index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, data[0]);
        if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
        {
            if(descriptor_map[index].parse_func)
            {
                return descriptor_map[index].parse_func(data, length, descriptor);
            }
        }
        else
        {
            index = find_ext_descriptor(data[0]);
            if(index >= 0 && index < ext_descriptor_num)
            {
                if(ext_descriptor_map[index].parse_func)
                {
                    return ext_descriptor_map[index].parse_func(data, length, descriptor);
                }
            }
        }
    }

    return 0;
}

static int dvb_service_descriptor_parse(unsigned char* data, unsigned int length, unsigned int descriptor)
{
    service_descriptor_t *desp = (service_descriptor_t *)descriptor;
    unsigned char provider_name_len = 0;
    unsigned char service_name_len = 0;

    if(data && length && desp)
    {
        desp->tag = data[0];
        desp->length = data[1];
        desp->service_type = data[2];
        desp->provider_name_len = provider_name_len = data[3];
        desp->service_name_len = service_name_len = data[4+provider_name_len];

        if(provider_name_len)
        {
            desp->provider_name = calloc(provider_name_len+1, sizeof(unsigned char));
            if(desp->provider_name)
            {
                memcpy(desp->provider_name, &data[4], provider_name_len);
            }
            else
            {
                log_print("error out of memory !!\n");
            }
        }

        if(service_name_len)
        {
            desp->service_name = calloc(service_name_len+1, sizeof(unsigned char));
            if(desp->service_name)
            {
                memcpy(desp->service_name, &data[5+provider_name_len], service_name_len);
            }
            else
            {
                log_print("error out of memory !!\n");
            }
        }

        return 0;
    }

    return -1;
}

static void *service_descriptor_new(void)
{
    service_descriptor_t *tmp = NULL;

    tmp = (service_descriptor_t*)malloc(sizeof(service_descriptor_t));
    if(tmp)
    {
        tmp->tag = DVB_SERVICE_DESCRIPTOR;
        tmp->length = 0;
        tmp->service_type = 0;
        tmp->provider_name_len = 0;
        tmp->service_name_len = 0;
        tmp->provider_name = NULL;
        tmp->service_name = NULL;
    }
    else
    {
        log_print("error out of memory !!\n");
    }
    
    return (void*)tmp;
}

void service_descriptor_free(void *desp)
{
    service_descriptor_t *tmp = (service_descriptor_t *)desp;

    if(tmp)
    {
        if(tmp->provider_name)
            free(tmp->provider_name);

        if(tmp->service_name)
            free(tmp->service_name);

        free(tmp);
    }

    return ;
}

void service_descriptor_dump(void *desp)
{
    service_descriptor_t *tmp = (service_descriptor_t*)desp;

    if(tmp)
    {
        log_print("\r\n========== SERVICE DESCRIPTOR ==========\r\n");

        log_print("tag: 0x%02x\r\n", tmp->tag);
        log_print("length: 0x%02x\r\n", tmp->length);
        log_print("service_type: 0x%02x\r\n", tmp->service_type);

        if(tmp->provider_name)
        {
            log_print("provider_name: %s\r\n", tmp->provider_name);
        }

        if(tmp->service_name)
        {
            log_print("service_name: %s\r\n", tmp->service_name);
        }

        log_print("\r\n================== END =================\r\n");
    }

    return ;
}

static int dvb_register_parser(struct dvb_parser **head, struct dvb_parser *parser)
{
    struct dvb_parser **tmp = head;
    struct dvb_parser **temp = head;

    if(parser && head)
    {
        if(*tmp == NULL)
        {
            parser->next = NULL;
            *tmp = parser;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == parser)
                {
                    return 0;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            parser->next = NULL;
            (*temp)->next = parser;
        }

        return 0;
    }

    return -1;
}

static void dvb_unregister_parser(struct dvb_parser **head, struct dvb_parser *parser)
{
    struct dvb_parser **tmp = head;

    if(head && parser)
    {
        while(*tmp)
        {
            if(*tmp == parser)
            {
                *tmp = parser->next;
                return ;
            }

            tmp = &(*tmp)->next;
        }
    }

    return ;
}

static struct dvb_parser * dvb_find_parser(struct dvb_parser **head, char *name, unsigned char id, unsigned char mask)
{
    struct dvb_parser **tmp = head;

    while(*tmp)
    {
        if(name)
        {
            if(strcmp((*tmp)->name, name) == 0)
            {
                return *tmp;
            }
        }
        else
        {
            if((id&mask) == ((*tmp)->id&(*tmp)->mask))
            {
                return *tmp;
            }
        }

        tmp = &(*tmp)->next;
    }

    return NULL;
}

static int dvb_parser_callback(struct dvb_parser * parser, unsigned char* data, unsigned int length, unsigned int para)
{
    if(parser)
    {
        if(parser->callback)
        {
            return parser->callback(data, length, para);
        }
    }

    return -1;
}

static descriptor_info_t *dvb_new_descriptor_info(unsigned char tag)
{
    descriptor_info_t *tmp = NULL;
    int index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, tag);

    if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
    {
        tmp = calloc(1, sizeof(descriptor_info_t));
        if(tmp)
        {
            tmp->tag = tag;
            tmp->next = NULL;
            tmp->descriptor = (unsigned int)descriptor_map[index].new_func();
            if(tmp->descriptor)
            {
                return tmp;
            }
            else
            {
                free(tmp);
                log_print("error out of memory !!\n");
            }
        }
        else
        {
            log_print("error out of memory !!\n");
        }
    }
    else
    {
        index = find_ext_descriptor(tag);
        if(index >= 0 && index < ext_descriptor_num)
        {
            tmp = calloc(1, sizeof(descriptor_info_t));
            if(tmp)
            {
                tmp->tag = tag;
                tmp->next = NULL;
                tmp->descriptor = (unsigned int)ext_descriptor_map[index].new_func();
                if(tmp->descriptor)
                {
                    return tmp;
                }
                else
                {
                    free(tmp);
                    log_print("error out of memory !!\n");
                }
            }
            else
            {
                log_print("error out of memory !!\n");
            }
        }
        else
        {
            tmp = calloc(1, sizeof(descriptor_info_t));
            if(tmp)
            {
                tmp->tag = tag;
                tmp->next = NULL;
                tmp->descriptor = 0;
                return tmp;
            }
            else
            {
                log_print("error out of memory !!\n");
            }

        }
    }
    
    return NULL;
}

static int dvb_add_descriptor_info(descriptor_info_t **head, descriptor_info_t *info)
{
    descriptor_info_t **tmp = head;
    descriptor_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return 0;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            info->next = NULL;
            (*temp)->next = info;
        }

        return 0;
    }

    return -1;
}

void dvb_delete_descriptor_info(descriptor_info_t **head, descriptor_info_t *info)
{
    descriptor_info_t **tmp = head;
    int index = -1;

    if(head && info)
    {
        while(*tmp)
        {
            if(*tmp == info)
            {
                *tmp = info->next;
                if(info->descriptor)
                {
                    index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, info->tag);
                    if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
                    {
                        descriptor_map[index].free_func((void*)info->descriptor);
                        info->descriptor = 0;
                    }
                    else
                    {
                        index = find_ext_descriptor(info->tag);
                        if(index >= 0 && index < ext_descriptor_num)
                        {
                            ext_descriptor_map[index].free_func((void*)info->descriptor);
                            info->descriptor = 0;
                        }
                    }
                }

                free(info);
                
                return ;
            }

            tmp = &(*tmp)->next;
        }    
    }
    
    return ;
}

static pat_section_info_t *new_pat_section_info(void)
{
    pat_section_info_t *tmp = NULL;

    tmp = malloc(sizeof(pat_section_info_t));
    if(tmp)
    {
        tmp->transport_stream_id = DVB_INVALID_ID;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->program_num = 0;
        tmp->program_map = NULL;
        tmp->next = NULL;
    }

    return tmp;
}

static void add_pat_section_info(pat_section_info_t **head, pat_section_info_t *info)
{
    pat_section_info_t **tmp = head;
    pat_section_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return ;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            info->next = NULL;
            (*temp)->next = info;

            return ;
        }
    }

    return ;
}

AM_ErrorCode_t dvb_psi_parse_pat(const unsigned char *data, int length, pat_section_info_t *info)
{
    unsigned char *sect_data = (unsigned char*)data;
    unsigned int  sect_len = length;
    pat_section_info_t *sect_info = info;
    pat_section_header_t *sect_head = NULL;
    pat_program_info_t *prog_info = NULL;
    unsigned char prog_num = 0;
    AM_ErrorCode_t ret = AM_SUCCESS;

    if(data && length && info)
    {
        while(sect_len > 0)
        {
            sect_head = (pat_section_header_t*)sect_data;
            
            if(sect_head->table_id != DVB_PSI_PAT_TID)
            {
                sect_len -= 3;
                sect_len -= MAKE_SHORT_HL(sect_head->section_length);

                sect_data += 3;
                sect_data += MAKE_SHORT_HL(sect_head->section_length);

                continue;
            }
            if(NULL == sect_info)
            {
                sect_info = new_pat_section_info();
                if(sect_info == NULL)
                {
                    log_print("AM_PARSER_PAT error out of memory !!\n");
                    ret = AM_PARSER_ERR_NO_MEM;
                    break;
                }
                else
                {
                    add_pat_section_info(&info, sect_info);
                }
            }
            
            sect_info->transport_stream_id = MAKE_SHORT_HL(sect_head->transport_stream_id);
            sect_info->version_number = sect_head->version_number;

            prog_info = (pat_program_info_t *)&sect_data[PAT_SECTION_HEADER_LEN];
            prog_num = (MAKE_SHORT_HL(sect_head->section_length) - (PAT_SECTION_HEADER_LEN - 3) - 4)/(sizeof(pat_program_info_t));

            if(prog_num)
            {
                sect_info->program_num = prog_num;
                sect_info->program_map = (struct pat_program_map *)malloc(sizeof(pat_program_map_t)*prog_num);
                if(sect_info->program_map)
                {
                    while(prog_num > 0)
                    {
                        sect_info->program_map[sect_info->program_num-prog_num].program_number = MAKE_SHORT_HL(prog_info->program_number);
                        sect_info->program_map[sect_info->program_num-prog_num].program_map_pid = MAKE_SHORT_HL(prog_info->program_map_pid);

                        prog_num--;
                        prog_info++;
                    }
                }
                else
                {
                    log_print("AM_PARSER_PAT error out of memory !!\n");
                    ret = AM_PARSER_ERR_NO_MEM;
                    break;
                }                
            }

            sect_len -= 3;
            sect_len -= MAKE_SHORT_HL(sect_head->section_length);

            sect_data += 3;
            sect_data += MAKE_SHORT_HL(sect_head->section_length);

            sect_info = sect_info->next;
        }
    }
    else {
    	log_print("error: data 0x%x, length 0x%x, info 0x%x\n", (unsigned int)data, (unsigned int)length, (unsigned int)info);
    	ret = AM_PARSER_ERR_SYS;
    }

    return ret;
}

void dvb_psi_free_pat_info(pat_section_info_t *info)
{
    pat_section_info_t *tmp = info;
    pat_section_info_t *temp = NULL;
    
    if(info)
    {
        while(tmp)
        {
            if(tmp->program_map)
            {
                free(tmp->program_map);
                tmp->program_map = NULL;
            }

            tmp->transport_stream_id = DVB_INVALID_ID;
            tmp->version_number = DVB_INVALID_VERSION;
            tmp->program_num = 0;
            tmp->program_map = NULL;
            
            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }
    }

    return ;
}

void dvb_psi_dump_pat_info(pat_section_info_t *info)
{
    pat_section_info_t *tmp = info;
    unsigned char i = 0;
    
    if(info)
    {
        log_print("\r\n============= PAT INFO =============\r\n");

        while(tmp)
        {
            log_print("transport_stream_id: 0x%04x\r\n", tmp->transport_stream_id);
            log_print("version_number: 0x%02x\r\n", tmp->version_number);
            log_print("program_number: %d\r\n", tmp->program_num); 

            for(i=0; i<tmp->program_num; i++)
            {
                log_print("  program_number: 0x%04x program_map_pid: 0x%04x\r\n", tmp->program_map[i].program_number, tmp->program_map[i].program_map_pid);
            }
            
            tmp = tmp->next;
        }

        log_print("\r\n=============== END ================\r\n");
    }

    return ;
}

int dvb_psi_get_pmt_numbers(pat_section_info_t *info)
{
	return info->program_num;
}

void dvb_psi_get_program_info(pat_section_info_t *info, int program_no, unsigned short *program_map_pid, unsigned short *program_number)
{
	if(program_no < info->program_num) {
	  *program_map_pid = info->program_map[program_no].program_map_pid;
	  *program_number  = info->program_map[program_no].program_number;
	}
	else {
		log_print("\r\nerror! program_number %d >= total program_number %d\r\n", program_no, info->program_num);
	  *program_map_pid = info->program_map[0].program_map_pid;
	  *program_number  = info->program_map[0].program_number;
	}

	return;
}

int dvb_psi_get_program_stream_info(pmt_section_info_t *info, aml_dvb_param_t *parm)
{
	int i = 0;
  pmt_section_info_t *tmp = info;
  pmt_stream_info_t *temp = NULL;

  if(tmp)
  {
    if(tmp->stream_info)
    {
      temp = tmp->stream_info;
      while(temp)
      {
      	switch(temp->stream_type) {
      		case DVB_STREAM_TYPE_MPEG1_VIDEO:
          case DVB_STREAM_TYPE_MPEG2_VIDEO:
      			parm->v_pid = temp->elementary_pid;
      			parm->v_type = VFORMAT_MPEG12;
      			log_print("MPEG video found! 0x%x\n", parm->v_pid);
      			i |= 1;
      			break;
          case DVB_STREAM_TYPE_H264_VIDEO:
            parm->v_pid = temp->elementary_pid;
            parm->v_type = VFORMAT_H264;
            log_print("H.264 video found! 0x%x\n", parm->v_pid);
            i |= 1;
            break;
      		case DVB_STREAM_TYPE_MPEG1_AUDIO:
          case DVB_STREAM_TYPE_MPEG2_AUDIO:
      			parm->a_pid = temp->elementary_pid;
      			parm->a_type = AFORMAT_MPEG;
            log_print("MPEG audio found! 0x%x\n", parm->a_pid);
      			i |= 2;
      			break;
          case DVB_STREAM_TYPE_AC3_AUDIO:
          case DVB_STREAM_TYPE_PRIVATE_DATA:
            parm->a_pid = temp->elementary_pid;
            parm->a_type = AFORMAT_AC3;
            log_print("AC3 audio found! 0x%x\n", parm->a_pid);
            i |= 2;
            break;
      		default:
      			break;
      	}
      	if(i == 3) {
      		break;
      	}
        temp = temp->next;
      }
    }
  }
	return ((i==0) ? -1 : 0);
}

void dvb_psi_clear_pat_info(pat_section_info_t *info)
{
    pat_section_info_t *tmp = NULL;
    pat_section_info_t *temp = NULL;
    
    if(info)
    {
        tmp = info->next;
        
        while(tmp)
        {
            if(tmp->program_map)
            {
                free(tmp->program_map);
                tmp->program_map = NULL;
            }
            
            tmp->transport_stream_id = DVB_INVALID_ID;
            tmp->version_number = DVB_INVALID_VERSION;
            tmp->program_num = 0;
            tmp->program_map = NULL;
            
            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }

        if(info->program_map)
            free(info->program_map);
        
        info->transport_stream_id = DVB_INVALID_ID;
        info->version_number = DVB_INVALID_VERSION;
        info->program_num = 0;
        info->program_map = NULL;
        info->next = NULL;
        
    }

    return ;
}

pat_section_info_t *dvb_psi_new_pat_info(void)
{
    return new_pat_section_info();
}

static int dvb_psi_register_pmt_parser(pmt_section_info_t *info, dvb_parser_t *parser, int num)
{
    int count = 0;
    int i = 0;
    
    if(info && parser && num > 0)
    {
        for(i=0; i<num; i++)
        {
            if((parser[i].id == 0xff) || (parser[i].callback == NULL)/* || (parser[i].next != NULL)*/)
            {
                log_print("parser parameter error !!\n");
                continue;
            }

            if(dvb_register_parser(&info->desp_parser, &parser[i]) == 0)
            {
                count++;
            }
            else
            {
                log_print("parser register failed !!\n");
            }
        }
    }
    
    return count;
}

pmt_section_info_t *dvb_psi_new_pmt_info(void)
{
    pmt_section_info_t *tmp = NULL;

    tmp = (pmt_section_info_t*)malloc(sizeof(pmt_section_info_t));
    if(tmp)
    {
        tmp->program_number = 0;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->pcr_pid = DVB_INVALID_PID;
        tmp->program_info = NULL;
        tmp->stream_info = NULL;
        tmp->desp_parser = NULL;
        dvb_psi_register_pmt_parser(tmp, pmt_parser, PMT_PARSER_NUM);
    }
    
    return tmp;
}

static pmt_stream_info_t *new_stream_info(unsigned char type, unsigned short pid)
{
    pmt_stream_info_t *tmp = NULL;

    tmp = malloc(sizeof(pmt_stream_info_t));
    if(tmp)
    {
        tmp->elementary_pid = pid;
        tmp->stream_type = type;
        tmp->es_info = NULL;
        tmp->next = NULL;
    }
    
    return tmp;
}

static void add_stream_info(pmt_stream_info_t **head, pmt_stream_info_t *info)
{
    pmt_stream_info_t **tmp = head;
    pmt_stream_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return ;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            info->next = NULL;
            (*temp)->next = info;

            return ;
        }
    }

    return ;
}

static void free_stream_info(pmt_stream_info_t *info)
{
    pmt_stream_info_t *tmp = NULL;
    pmt_stream_info_t *temp = NULL;
    
    if(info)
    {
        tmp = info->next;
        while(tmp)
        {
            if(tmp->es_info)
            {
                dvb_free_descriptor_info(tmp->es_info);
                tmp->es_info = NULL;
            }

            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }

        if(info->es_info)
        {
            dvb_free_descriptor_info(info->es_info);
            info->es_info = NULL;
        }

        free(info);
    }

    return ;
}

AM_ErrorCode_t dvb_psi_parse_pmt(const unsigned char *data, int length, pmt_section_info_t *info)
{
    unsigned char *sect_data = (unsigned char *)data;
    unsigned int  sect_len = length;
    pmt_section_info_t *sect_info = info;
    pmt_section_header_t *sect_head = NULL;
    pmt_es_info_t  *es_info = NULL;
    unsigned char  *data_info = NULL;
    unsigned short data_length = 0;
    unsigned short es_length = 0;
    struct descriptor *data_desp = NULL;
    struct descriptor_info *desp_info = NULL;
    struct pmt_stream_info *stream_info = NULL;
    struct dvb_parser *data_parser = NULL;
    AM_ErrorCode_t ret = AM_SUCCESS;
    
    if(data && length && info)
    {
        while(sect_len > 0)
        {
            sect_head = (pmt_section_header_t*)sect_data;
            
            if(sect_head->table_id != DVB_PSI_PMT_TID)
            {
				        log_print("error, sect_head->table_id is %d\n", sect_head->table_id);
					      ret = AM_PARSER_ERR_NO_DATA;
					      break;
            }

            sect_info->program_number = MAKE_SHORT_HL(sect_head->program_number);
            sect_info->version_number = sect_head->version_number;
            sect_info->pcr_pid = MAKE_SHORT_HL(sect_head->pcr_pid);
            sect_info->program_info = NULL;
            sect_info->stream_info = NULL;
            
            /* program info */
            data_info = &sect_data[PMT_SECTION_HEADER_LEN];
            data_length = MAKE_SHORT_HL(sect_head->program_info_length);
            
			      while((data_length > 0)&&(data_info!=NULL))
            {
                data_desp = (struct descriptor*)data_info;
				        if(data_desp->length == 0) {
				        	  log_print("error, data_desp->length is 0\n");
					          ret = AM_PARSER_ERR_NO_DATA;
					          return ret;
					      }

                data_parser = dvb_find_parser(&info->desp_parser, NULL, data_desp->tag, 0xff);
                if(data_parser)
                {
                    desp_info = dvb_new_descriptor_info(data_desp->tag);
                    if(desp_info)
                    {
                        if(dvb_add_descriptor_info(&sect_info->program_info, desp_info) != 0)
                        {
                            log_print("add program descriptor info failed !!\n");
                        }
                        else
                        {
                            if(dvb_parser_callback(data_parser, data_info, data_desp->length+2, desp_info->descriptor) != 0)
                            {
                                log_print("parse descriptor 0x%02x failed !!\n", data_desp->tag);
                                dvb_delete_descriptor_info(&sect_info->program_info, desp_info);
                                desp_info = NULL;
                            }
                        }
                    }
                    else
                    {
                        log_print("error out of memory !!\n");
                        ret = AM_PARSER_ERR_NO_MEM;
                        return ret;
                    }
                }
                
                data_length -= data_desp->length+2;
                data_info += data_desp->length+2;
            }
                        
            /* es info */
            data_info = &sect_data[PMT_SECTION_HEADER_LEN + MAKE_SHORT_HL(sect_head->program_info_length)];
            data_length = MAKE_SHORT_HL(sect_head->section_length) - (PMT_SECTION_HEADER_LEN - 3) - MAKE_SHORT_HL(sect_head->program_info_length) - 4;
            while(data_length > 0)
            {
                es_info = (pmt_es_info_t*)data_info;
                es_length = MAKE_SHORT_HL(es_info->es_info_length);
                
                data_info += PMT_STREAM_HEADER_LEN;
                data_length -= PMT_STREAM_HEADER_LEN + es_length;

                stream_info = new_stream_info(es_info->stream_type, MAKE_SHORT_HL(es_info->elementary_pid));
                if(stream_info == NULL)
                {
                    log_print("error out of memory !!\n");
                    ret = AM_PARSER_ERR_NO_MEM;
                    return ret;
                }

                add_stream_info(&sect_info->stream_info, stream_info);
                
                while(es_length > 0)
                {
                    data_desp = (struct descriptor*)data_info;
					          if(data_desp->length == 0) {
				        	      log_print("error, data_desp->length is 0\n");
					              ret = AM_PARSER_ERR_NO_DATA;
					              return ret;
					          }

                    data_parser = dvb_find_parser(&info->desp_parser, NULL, data_desp->tag, 0xff);
                    if(data_parser)
                    {
                        desp_info = dvb_new_descriptor_info(data_desp->tag);
                        if(desp_info)
                        {
                            if(dvb_add_descriptor_info(&stream_info->es_info, desp_info) != 0)
                            {
                                log_print("add es descriptor info failed !!\n");
                            }
                            else
                            {
                                if(dvb_parser_callback(data_parser, data_info, data_desp->length+2, desp_info->descriptor) != 0)
                                {
                                    log_print("parse descriptor 0x%02x failed !!\n", data_desp->tag);
                                    dvb_delete_descriptor_info(&stream_info->es_info, desp_info);
                                    desp_info = NULL;
                                }
                            }
                        }
                        else
                        {
                            log_print("error out of memory !!\n");
                            ret = AM_PARSER_ERR_NO_MEM;
							              return ret;
                        }
                    }
                    
                    es_length -= data_desp->length+2;
                    data_info += data_desp->length+2;
                }
            }
            
            sect_len -= 3;
            sect_len -= MAKE_SHORT_HL(sect_head->section_length);

            sect_data += 3;
            sect_data += MAKE_SHORT_HL(sect_head->section_length);
        }
    }
    else {
    	log_print("error: data 0x%x, length 0x%x, info 0x%x\n", (unsigned int)data, (unsigned int)length, (unsigned int)info);
    	ret = AM_PARSER_ERR_SYS;
    }

    return ret;
}

static void dvb_dump_descriptor_info(descriptor_info_t *info)
{
    descriptor_info_t *tmp = info;
    descriptor_t *desp = NULL;
    int index = -1;
    
    while(tmp)
    {
        if(tmp->descriptor)
        {
            index = find_descriptor(descriptor_map, 0, DESCRIPTOR_SUPPORT_NUM-1, tmp->tag);
            if(index >= 0 && index < DESCRIPTOR_SUPPORT_NUM)
            {
                if(descriptor_map[index].dump_func)
                {
                    descriptor_map[index].dump_func((void*)tmp->descriptor);
                }
            }
            else if((index = find_ext_descriptor(tmp->tag)) != -1)
            {
                if(ext_descriptor_map[index].dump_func)
                {
                    ext_descriptor_map[index].dump_func((void*)tmp->descriptor);
                }
            }
            else
            {
                desp = (descriptor_t*)tmp->descriptor;

                log_print("\r\n========== DESCRIPTOR ==========\r\n");
                
                log_print("tag: 0x%02x\r\n", desp->tag);
                log_print("length: 0x%02x\r\n", desp->length);
                if(index<desp->length)
                {
                    log_print("data: \r\n");
                    for(index=0; index<desp->length; index++)
                    {
                        log_print("0x%02x ", desp->data[index]);
                        
                        if((index+1)%8 == 0)
                            log_print("\r\n");
                    }

                    if(desp->length%8 != 0)
                        log_print("\r\n");
                }

                log_print("\r\n============== END =============\r\n");
                
            }
        }
        
        tmp = tmp->next;
    }
    
    return ;
}

void dvb_psi_dump_pmt_info(pmt_section_info_t *info)
{
    pmt_section_info_t *tmp = info;
    pmt_stream_info_t *temp = NULL;
    
    if(info)
    {
        log_print("\r\n============= PMT INFO =============\r\n");

        log_print("program_number: 0x%04x\r\n", tmp->program_number);
        log_print("version_number: 0x%02x\r\n", tmp->version_number);
        log_print("PCR PID: 0x%04x\r\n", tmp->pcr_pid);

        if(tmp->program_info)
        {
            log_print("program descriptors:\r\n");
            dvb_dump_descriptor_info(tmp->program_info);
        }

        if(tmp->stream_info)
        {
            log_print("streams info:\r\n");
            
            temp = tmp->stream_info;
            while(temp)
            {
                log_print("stream_type: 0x%02x\r\n", temp->stream_type);
                log_print("elementary_pid: 0x%04x\r\n", temp->elementary_pid);

                if(temp->es_info)
                {
                    log_print("element stream descriptors:\r\n");
                    dvb_dump_descriptor_info(temp->es_info);
                }

                temp = temp->next;
            }
        }
        
        log_print("\r\n=============== END ================\r\n");
    }

    return ;
}

void dvb_psi_free_pmt_info(pmt_section_info_t *info)
{
    struct dvb_parser *parser = NULL;
    
    if(info)
    {
        while((parser = info->desp_parser) != NULL)
        {
            dvb_unregister_parser(&info->desp_parser, parser);
        }

        info->desp_parser = NULL;
        
        if(info->program_info)
        {
            dvb_free_descriptor_info(info->program_info);
        }

        if(info->stream_info)
        {
            free_stream_info(info->stream_info);
        }

        info->program_number = 0;
        info->version_number = DVB_INVALID_VERSION;
        info->pcr_pid = DVB_INVALID_PID;
        info->program_info = NULL;
        info->stream_info = NULL;

        free(info);
    }
    
    return ;
}

static sdt_section_info_t *new_sdt_section_info(void)
{
    sdt_section_info_t *tmp = NULL;

    tmp = malloc(sizeof(sdt_section_info_t));
    if(tmp)
    {
        tmp->transport_stream_id = DVB_INVALID_ID;
        tmp->original_network_id = DVB_INVALID_ID;
        tmp->version_number = DVB_INVALID_VERSION;
        tmp->service_info = NULL;
        tmp->desp_parser = NULL;
        tmp->next = NULL;
    }

    return tmp;
}

static void add_sdt_section_info(sdt_section_info_t **head, sdt_section_info_t *info)
{
    sdt_section_info_t **tmp = head;
    sdt_section_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return ;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            info->next = NULL;
            (*temp)->next = info;

            return ;
        }
    }

    return ;
}

static int dvb_si_register_sdt_parser(sdt_section_info_t *info, dvb_parser_t *parser, int num)
{
    int count = 0;
    int i = 0;
    
    if(info && parser && num > 0)
    {
        for(i=0; i<num; i++)
        {
            if((parser[i].id == 0xff) || (parser[i].callback == NULL)/* || (parser[i].next != NULL)*/)
            {
                log_print("parser parameter error !!\n");
                continue;
            }

            if(dvb_register_parser(&info->desp_parser, &parser[i]) == 0)
            {
                count++;
            }
            else
            {
                log_print("parser register failed !!\n");
            }
        }
    }
    
    return count;
}

sdt_section_info_t *dvb_si_new_sdt_info(void)
{
    sdt_section_info_t *sect_info = new_sdt_section_info();
    if(sect_info) {
        dvb_si_register_sdt_parser(sect_info, sdt_parser, SDT_PARSER_NUM);
    }
    return sect_info;
}

static sdt_service_info_t *new_service_info(unsigned short service_id)
{
    sdt_service_info_t *tmp = NULL;

    tmp = malloc(sizeof(sdt_service_info_t));
    if(tmp)
    {
        tmp->service_id = service_id;
        tmp->eit_schedule_flag = 0;
        tmp->eit_present_following_flag = 0;
        tmp->runing_status = 0;
        tmp->free_ca_mode = 0;
        tmp->desp_info = NULL;
        tmp->next = NULL;
    }
    
    return tmp;
}

static void add_service_info(sdt_service_info_t **head, sdt_service_info_t *info)
{
    sdt_service_info_t **tmp = head;
    sdt_service_info_t **temp = head;

    if(info && head)
    {
        if(*tmp == NULL)
        {
            info->next = NULL;
            *tmp = info;
        }
        else
        {
            while(*tmp)
            {
                if(*tmp == info)
                {
                    return ;
                }

                temp = tmp;
                tmp = &(*tmp)->next;
            }

            info->next = NULL;
            (*temp)->next = info;

            return ;
        }
    }

    return ;
}

static void free_service_info(sdt_service_info_t *info)
{
    sdt_service_info_t *tmp = NULL;
    sdt_service_info_t *temp = NULL;
    
    if(info)
    {
        tmp = info->next;
        while(tmp)
        {
            if(tmp->desp_info)
            {
                dvb_free_descriptor_info(tmp->desp_info);
                tmp->desp_info = NULL;
            }

            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }

        if(info->desp_info)
        {
            dvb_free_descriptor_info(info->desp_info);
            info->desp_info = NULL;
        }

        free(info);
    }

    return ;
}

AM_ErrorCode_t dvb_si_parse_sdt(const unsigned char *data, int length, sdt_section_info_t *info)
{
    unsigned char *sect_data = (unsigned char *)data;
    unsigned int sect_len = (unsigned int)length;
    unsigned char *data_info = NULL;
    sdt_section_info_t *sect_info = info;
    sdt_section_header_t *sect_head = NULL;
    sdt_service_info_t *service_info = NULL;
    unsigned short data_length = 0;
    unsigned short descriptor_length = 0;
    struct descriptor *data_desp = NULL;
    struct descriptor_info *desp_info = NULL;
    struct dvb_parser *data_parser = NULL;
    struct sdt_service_header *service_head = NULL;
    AM_ErrorCode_t ret = AM_SUCCESS;

    if(data && length && info)
    {
        while(sect_len > 0)
        {
            sect_head = (sdt_section_header_t*)sect_data;
            
            if((sect_head->table_id != DVB_SI_SDT_ACT_TID) && (sect_head->table_id != DVB_SI_SDT_OTH_TID))
            {
                sect_len -= 3;
                sect_len -= MAKE_SHORT_HL(sect_head->section_length);

                sect_data += 3;
                sect_data += MAKE_SHORT_HL(sect_head->section_length);

                continue;
            }

            if(NULL == sect_info)
            {
                sect_info = new_sdt_section_info();
                if(sect_info == NULL)
                {
                    log_print("error out of memory !!\n");
                    ret = AM_PARSER_ERR_NO_MEM;
                    return ret;
                }
                else
                {
                    add_sdt_section_info(&info, sect_info);
                }
            }

            sect_info->transport_stream_id = MAKE_SHORT_HL(sect_head->transport_stream_id);
            sect_info->original_network_id = MAKE_SHORT_HL(sect_head->original_network_id);
            sect_info->version_number = sect_head->version_number;

            /* service info */
            data_length = MAKE_SHORT_HL(sect_head->section_length) - (SDT_SECTION_HEADER_LEN - 3) - 4;
            data_info = &sect_data[SDT_SECTION_HEADER_LEN];

            while(data_length > 0)
            {
                service_head = (struct sdt_service_header *)data_info;

                descriptor_length = MAKE_SHORT_HL(service_head->descriptors_loop_length);
                
                data_length -= SDT_SERVICE_HEADER_LEN+descriptor_length;
                data_info += SDT_SERVICE_HEADER_LEN;
                
                service_info = new_service_info(MAKE_SHORT_HL(service_head->service_id));
                if(service_info == NULL)
                {
                    log_print("error out of memory !!\n");
                    ret = AM_PARSER_ERR_NO_MEM;
                    return ret;
                }

                service_info->eit_schedule_flag = service_head->eit_schedule_flag;
                service_info->eit_present_following_flag = service_head->eit_present_following_flag;
                service_info->runing_status = service_head->runing_status;
                service_info->free_ca_mode = service_head->free_ca_mode;
                
                add_service_info(&sect_info->service_info, service_info);

                while(descriptor_length > 0)
                {
                    data_desp = (struct descriptor*)data_info;

                    data_parser = dvb_find_parser(&info->desp_parser, NULL, data_desp->tag, 0xff);
                    if(data_parser)
                    {
                        desp_info = dvb_new_descriptor_info(data_desp->tag);
                        if(desp_info)
                        {
                            if(dvb_add_descriptor_info(&service_info->desp_info, desp_info) != 0)
                            {
                                log_print("add service descriptor info failed !!\n");
                            }
                            else
                            {
                                if(dvb_parser_callback(data_parser, data_info, data_desp->length+2, desp_info->descriptor) != 0)
                                {
                                    log_print("parse descriptor 0x%02x failed !!\n", data_desp->tag);
                                    dvb_delete_descriptor_info(&service_info->desp_info, desp_info);
                                    desp_info = NULL;
                                }
                            }
                        }
                        else
                        {
                            log_print("error out of memory !!\n");
                            ret = AM_PARSER_ERR_NO_MEM;
                            return ret;
                        }
                    }
                    
                    descriptor_length -= data_desp->length+2;
                    data_info += data_desp->length+2;
                }
                
            }
            
            sect_len -= 3;
            sect_len -= MAKE_SHORT_HL(sect_head->section_length);

            sect_data += 3;
            sect_data += MAKE_SHORT_HL(sect_head->section_length);

            sect_info = sect_info->next;
        }
    }
    else {
    	log_print("error: data 0x%x, length 0x%x, info 0x%x\n", (unsigned int)data, (unsigned int)length, (unsigned int)info);
    	ret = AM_PARSER_ERR_SYS;
    }

    return ret;
}

void dvb_si_dump_sdt_info(sdt_section_info_t *info, aml_dvb_channel_info_t **listChannels, int freq)
{
    sdt_section_info_t *tmp = info;
    sdt_service_info_t *temp = NULL;
    aml_dvb_channel_info_t *tempChannel = NULL;
    aml_dvb_channel_info_t *tmpChannel = NULL;
    struct descriptor_info *desp_info = NULL;
    unsigned char i = 0;
    
    if(info)
    {
        log_print("\r\n============= SDT INFO =============\r\n");
        
        while(tmp)
        {
            log_print("transport_stream_id: 0x%04x\r\n", tmp->transport_stream_id);
            log_print("original_network_id: 0x%04x\r\n", tmp->original_network_id);
            log_print("version_number: 0x%02x\r\n", tmp->version_number);

            if(tmp->service_info)
            {
                log_print("service info: \r\n");

                temp = tmp->service_info;
                while(temp)
                {
                    log_print("service_id: 0x%04x\r\n", temp->service_id);
                    log_print("eit_schedule_flag: 0x%02x\r\n", temp->eit_schedule_flag);
                    log_print("eit_present_following_flag: 0x%02x\r\n", temp->eit_present_following_flag);
                    log_print("runing_status: 0x%02x\r\n", temp->runing_status);
                    log_print("free_ca_mode: 0x%02x\r\n", temp->free_ca_mode);
                    
                    if(temp->desp_info)
                    {
                        log_print("service descriptors: \r\n");
                        dvb_dump_descriptor_info(temp->desp_info);

                        desp_info = temp->desp_info;
                        while(desp_info) {
                            tempChannel = malloc(sizeof(aml_dvb_channel_info_t));
                            if(tempChannel) {
                            	  service_descriptor_t* service_desp = (service_descriptor_t*)desp_info->descriptor;
                            	
                                tempChannel->freq = freq;
                                tempChannel->service_id = temp->service_id;
                                tempChannel->service_name = service_desp->service_name;
                                tempChannel->next = NULL;

                                if(*listChannels == NULL) {
                      	            *listChannels = tempChannel;
                                }
                                else {
                      	            tmpChannel = *listChannels;
                                    while(tmpChannel->next) {
                        	              tmpChannel = tmpChannel->next;
                                    }
                                    tmpChannel->next = tempChannel;
                                }
                            }
                            desp_info = desp_info->next;
                        }
                    }
                    temp = temp->next;
                }
            }
            
            tmp = tmp->next;
        }
    }

    return ;
}

void dvb_si_free_sdt_info(sdt_section_info_t *info)
{
    sdt_section_info_t *tmp = NULL;
    sdt_section_info_t *temp = NULL;
    struct dvb_parser *parser = NULL;
    
    if(info)
    {
        while((parser = info->desp_parser) != NULL)
        {
            dvb_unregister_parser(&info->desp_parser, parser);
        }

        info->desp_parser = NULL;

        tmp = info;
        while(tmp)
        {
            tmp->transport_stream_id = DVB_INVALID_ID;
            tmp->original_network_id = DVB_INVALID_ID;
            tmp->version_number = DVB_INVALID_VERSION;

            if(tmp->service_info)
            {
                free_service_info(tmp->service_info);
                tmp->service_info = NULL;
            }
            
            temp = tmp->next;
            free(tmp);
            tmp = temp;
        }
    }

    return ;
}

#if 0


#include <includes.h>

#include "am_types.h"
#include "am_memory.h"
#include "am_trace.h"

#include "dvb/dvb_si.h"
#include "dvb/dvb_parser.h"
#include "dvb/dvb_descriptor.h"
#include "dvb/dvb_pmt.h"

#define MAKE_SHORT_HL(exp)		            ((INT16U)((exp##_hi<<8)|exp##_lo))

void dvb_psi_clear_pmt_info(pmt_section_info_t *info)
{
    if(info)
    {
        if(info->program_info)
        {
            dvb_free_descriptor_info(info->program_info);
        }

        if(info->stream_info)
        {
            free_stream_info(info->stream_info);
        }

        info->program_number = 0;
        info->version_number = DVB_INVALID_VERSION;
        info->pcr_pid = DVB_INVALID_PID;
        info->program_info = NULL;
        info->stream_info = NULL;
    }
    
    return ;
}

void dvb_si_clear_sdt_info(sdt_section_info_t *info)
{    
    sdt_section_info_t *tmp = NULL;
    sdt_section_info_t *temp = NULL;
    
    if(info)
    {
        tmp = info->next;
        
        while(tmp)
        {
            tmp->transport_stream_id = DVB_INVALID_ID;
            tmp->original_network_id = DVB_INVALID_ID;
            tmp->version_number = DVB_INVALID_VERSION;

            if(tmp->service_info)
            {
                free_service_info(tmp->service_info);
                tmp->service_info = NULL;
            }
            
            temp = tmp->next;
            AMMem_free(tmp);
            tmp = temp;
        }

        info->transport_stream_id = DVB_INVALID_ID;
        info->original_network_id = DVB_INVALID_ID;
        info->version_number = DVB_INVALID_VERSION;

        if(info->service_info)
        {
            free_service_info(info->service_info);
            info->service_info = NULL;
        }

        info->next = NULL;
    }

    return ;
}

void dvb_si_unregister_sdt_parser(sdt_section_info_t *info, dvb_parser_t *parser)
{
    struct dvb_parser *tmp = NULL;
    INT32S count = 0;
    
    if(info)
    {
        while((tmp = info->desp_parser) != NULL)
        {
            if(((NULL != parser) && (tmp == parser)) || (NULL == parser))
            {
                dvb_unregister_parser(&info->desp_parser, tmp);
                count++;

                if(NULL != parser)
                {
                    break;
                }
                else
                {
                    continue;
                }
            }

            tmp = tmp->next;
        }
    }
    
    return ;
}

#endif
