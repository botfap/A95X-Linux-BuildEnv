#include <adec_write.h>

int init_buff(buffer_stream_t *bs,int length)
{
	//unsigned char *buffer=malloc(DEFAULT_BUFFER_SIZE);
	unsigned char *buffer=malloc(length);
	if(!buffer)
	{
		printf("Err:malloc failed \n");
		bs->data=NULL;
		return -1;
	}
	bs->data=buffer;
	//bs->buf_length=DEFAULT_BUFFER_SIZE;
	bs->buf_length=length;
	bs->buf_level=0;
	bs->rd_ptr=bs->wr_ptr=bs->data;
	bs->bInited=1;
	bs->nMutex=1;
	return 1;
}

int reset_buffer(buffer_stream_t *bs)
{
	if(bs->bInited==0)
		return -1;
	//bs->buf_length=DEFAULT_BUFFER_SIZE;
	bs->buf_level=0;
	bs->rd_ptr=bs->wr_ptr=bs->data;
	bs->nMutex=1;
	return 0;
}

int release_buffer(buffer_stream_t *bs)
{
       if(bs->data)
	free(bs->data);
	free(bs);
	bs=NULL;
	return 1;
}

//1 empty 0 not empty -1 not inited
int is_buffer_empty(buffer_stream_t *bs)
{
	if(bs->bInited==0)
		return -1;
	if(bs->buf_level==0)
		return 1;
	else
		return 0;
}
//1 full 0 not full -1 not inited
int is_buffer_full(buffer_stream_t *bs)
{
	if(bs->bInited==0)
		return -1;
	if(bs->buf_level==bs->buf_length)
		return 1;
	else
		return 0;

}

int get_buffer_length(buffer_stream_t *bs)
{
	if(bs->bInited==0)
		return -1;
	return bs->buf_level;
}

static int read_data(char * out, buffer_stream_t *bs, int size)
{
	if(bs->bInited==0)
		return -1;//read failed
	int ret=is_buffer_empty(bs);
	if(ret==1)
	{
		//printf("=====buffer empty \n");
		return 0;//buffer empty
	}
	int len= MIN(bs->buf_level,size);
	if(bs->wr_ptr>bs->rd_ptr)
	{
		memcpy(out,bs->rd_ptr,len);
		bs->rd_ptr+=len;
		bs->buf_level-=len;
		if(bs->rd_ptr==(bs->data+bs->buf_length))
		     bs->rd_ptr=bs->data;
		//printf("=====read ok: condition 1 read :%d byte \n",len);
		return len;
	}
	else if(len<(bs->data+bs->buf_length-bs->rd_ptr))
	{
		memcpy(out,bs->rd_ptr,len);
		bs->rd_ptr+=len;
		bs->buf_level-=len;
		if(bs->rd_ptr==(bs->data+bs->buf_length))
		     bs->rd_ptr=bs->data;
		//printf("=====read ok: condition 2 read :%d byte \n",len);
		return len;

	}
	else
	{
		int tail_len=(bs->data+bs->buf_length-bs->rd_ptr);
		memcpy(out,bs->rd_ptr,tail_len);
		memcpy(out+tail_len,bs->data,len-tail_len);
		bs->rd_ptr=bs->data+len-tail_len;
		bs->buf_level-=len;
		if(bs->rd_ptr==(bs->data+bs->buf_length))
		     bs->rd_ptr=bs->data;
		//printf("=====read ok: condition 3 read :%d byte \n",len);
		return len;
	}

}

int read_pcm_buffer(char * out, buffer_stream_t *bs, int size)
{
	int ret=0;
	if(bs->nMutex==1)
	{
		bs->nMutex=0;
		ret=read_data(out,bs,size);
		bs->nMutex=1;
		
	}
	return ret;
}
static int write_data(char *in, buffer_stream_t *bs, int size)
{
	if(bs->bInited==0)
		return -1;//not inited
	int ret=is_buffer_full(bs);
	if(ret==1)
	{

		printf("=====buffer full \n");
		return 0;//buffer full
	}
	//start write data
	int len= MIN(bs->buf_length-bs->buf_level,size);
	if(bs->wr_ptr<bs->rd_ptr)
	{
		memcpy(bs->wr_ptr,in,len);
		bs->wr_ptr+=len;
		bs->buf_level+=len;
		if(bs->wr_ptr==(bs->data+bs->buf_length))
		     bs->wr_ptr=bs->data;
		//printf("=====write ok: condition 1 write :%d byte \n",len);
		return len;
	}
	else if(len<(bs->data+bs->buf_length-bs->wr_ptr))
	{
		memcpy(bs->wr_ptr,in,len);
		bs->wr_ptr+=len;
		bs->buf_level+=len;
		if(bs->wr_ptr==(bs->data+bs->buf_length))
		    bs->wr_ptr=bs->data;
		//printf("=====write ok: condition 2 write :%d byte \n",len);
		return len;

	}
	else
	{
		int tail_len=(bs->data+bs->buf_length-bs->wr_ptr);
		memcpy(bs->wr_ptr,in,tail_len);
		memcpy(bs->data,in+tail_len,len-tail_len);
		bs->wr_ptr=bs->data+len-tail_len;
		bs->buf_level+=len;
		if(bs->wr_ptr==(bs->data+bs->buf_length))
		    bs->wr_ptr=bs->data;
		//printf("=====write ok: condition 3 write :%d byte \n",len);
		return len;
	}

}

int write_pcm_buffer(char * in, buffer_stream_t *bs, int size)
{
	int ret=0;
	if(bs->nMutex==1)
	{
		bs->nMutex=0;
		ret=write_data(in,bs,size);
		bs->nMutex=1;
	}
	return ret;
}


