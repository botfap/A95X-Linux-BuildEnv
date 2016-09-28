
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <dlfcn.h>

#include "dts_transenc_api.h"
#include "pcmenc_api.h" //xujian
#include "spdif_api.h" //xujian
#include <log-print.h>

pcm51_encoded_info_t		dts_transenc_info;
static int							dts_init_flag = 0;				
char										*stream;						//input raw pcm
unsigned char						*output;
static int write_success_flag=1;
unsigned int 						input_size;
unsigned int 						output_size;
static int nNumFrmCoded;
//sjw added ; param set by shaoshuai
typedef struct  
{
	    int (*enc_init)(pcm51_encoded_info_t dts_transenc_info, unsigned int *input_size, unsigned int *output_size);
	    int (*enc_encode)(pcm51_encoded_info_t dts_transenc_info, char *stream, unsigned char *output, unsigned int output_size);
	    int (*enc_release)();  
}ecoder_operations;

static ecoder_operations enc_ops;

int dts_transenc_init()
{
	int 		rv;
	dts_init_flag = 0;
	write_success_flag=1;
	rv = pcmenc_init();//xujian
	if(rv==-1)
	{
	    adec_print("==pcmenc_init failed \n");
	    goto err1;
	}
	rv = iec958_init();//xujian
	if(rv!=0)
	{
	    adec_print("==iec958_init failed ret:%d\n",rv);
	    goto err2;
	}
	rv = pcmenc_get_pcm_info(&dts_transenc_info);//xujian
    	if(dts_transenc_info.LFEFlag > 1)
		dts_transenc_info.LFEFlag = 1;
	int fd_dtsenc = dlopen("libdtsenc.so",RTLD_NOW);
	if (fd_dtsenc != 0)
	{
	       enc_ops.enc_init = dlsym(fd_dtsenc, "init");
		enc_ops.enc_encode = dlsym(fd_dtsenc, "encode_frame");
		enc_ops.enc_release = dlsym(fd_dtsenc, "release");
	}
	else
	 {
	    adec_print("==find libdtsenc.so  failed \n");
	    goto err3;
	    }
	
	rv = enc_ops.enc_init(dts_transenc_info, &input_size, &output_size);//encode init
       if(rv!=0)
        goto err4;
       
	stream = (char *)malloc( input_size ); //malloc input buf
	output = (unsigned char *)malloc(output_size); //malloc output buf
	
	dts_init_flag = 1;

	return dts_init_flag;
err1:
	    return -1;
err2:
    pcmenc_deinit();//xujian
    return -1;
err3:
    pcmenc_deinit();//xujian
    iec958_deinit();//xujian
    return -1;
err4:
    pcmenc_deinit();//xujian
    iec958_deinit();//xujian
    close(fd_dtsenc);
    return -1;
	    
}

int dts_transenc_process_frame()
{
	int				rv;
	if(write_success_flag)
	{
        	   rv=pcmenc_read_pcm(stream, input_size);//xujian
               if(rv==0)/* no enough pcm data in the buffer */
               {
                    //adec_print("=====read data failed :%d input_size:%d  \n",rv,input_size);
                    if(iec958_check_958buf_level() == 0){
						adec_print("transenc:insert zero pcm data \n"); 
						memset(stream,0,input_size);//insert zero pcm data when 958 hw buffer underrun
                    }
					else{
                    	usleep(1000);
                    	return -1;
					}	
                }
                #ifdef DUMP_FILE
                FILE *fp1=fopen("/mnt/sda4/a.pcm","a+");
                fwrite(stream,1,input_size,fp1);
                fclose(fp1);
                #endif
                
        	rv = enc_ops.enc_encode(dts_transenc_info, stream, output, &output_size);//encode frame
        	#ifdef DUMP_FILE
                FILE *fp2=fopen("/mnt/sda4/a.dts","a+");
                fwrite(output,1,output_size,fp2);
                fclose(fp2);
                #endif
        	rv = iec958_pack_frame(output, output_size);
	}
	rv = iec958_packed_frame_write_958buf(output, output_size);
	if(rv==-1){
	    write_success_flag=0;
	    usleep(1000);
	}	
	else
	    write_success_flag=1;
	
	//adec_print("===pack frame write 958 ret:%d size:%d  \n",rv,output_size);
	return 1;
}

int dts_transenc_deinit()
{
    iec958_deinit();//xujian
    pcmenc_deinit();//xujian
    enc_ops.enc_release();
    if(stream)
        free(stream);
    stream = NULL;
    if(output)
        free(output);
    output = NULL;
	return 1;
}
