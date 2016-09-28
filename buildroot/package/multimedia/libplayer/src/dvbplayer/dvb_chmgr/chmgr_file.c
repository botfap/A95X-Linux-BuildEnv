
#include "chmgr_file.h"
#include "stdio.h"
#include "stdlib.h"
#include "log_print.h"

#define FIX_BUFFER_SIZE (1024 * 16)

static char work_buffer[FIX_BUFFER_SIZE];

static char * find_key_value(char *buffer,int buff_size,const char*keyword)
{
	char *pTmp=buffer;
	char *pBufferEnd=(buffer+buff_size);
	char *pKeyTmp=(char *)keyword;
	int key_len=0;
	int i=0;
	int j=0;
	char bKeyFind=0;
	
	if(!pTmp)
	{
		return 0;
	}
	
	key_len=strlen(keyword);


	while(pTmp<=pBufferEnd)
	{
		if(pTmp[i]==pKeyTmp[j])	
		{

			i++;
			j++;
		}
		else
		{
			j=0;	
			i=0;
			pTmp++;
		}
		if(j>=key_len)
		{
			bKeyFind=1;
			break;	
		}
	}


	if(bKeyFind)
	{
		return pTmp+i;	
	}

	return 0;
}

int load_key_value(const char *filename,const char *keyword,int *keyvalue)
{
	FILE *fp=0;
	int filesize=0;
	char *pBuffer=0;

	char *key_ptr=0;	
	int i=0;

	if(!filename || !keyword||!keyvalue)
	{
    log_print("!filename %d || !keyword %d ||!keyvalue %d\n", (unsigned int)filename, (unsigned int)keyword, (unsigned int)keyvalue);
		return -1;
	}

	fp=fopen(filename,"rb+");
	if(!fp)
	{
    log_print("can't open file %s\n", filename);
		return -1;
	}
	
	fseek(fp,0,SEEK_END);
	filesize=ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(!filesize)
	{
    log_print("filesize zero\n");
		return -1;
	}
	if(filesize<=FIX_BUFFER_SIZE)
	{
			
	}
	else
	{
		//TODO	
		log_print("!!!!! file size exceeds 16KB !!!!!!!\n");
		filesize=FIX_BUFFER_SIZE-1;
	}

	pBuffer= work_buffer;

	if(!pBuffer)
	{
    log_print("no pBuffer\n");
		return -1;
	}
	fread(pBuffer,1,filesize,fp);	
	key_ptr=find_key_value(pBuffer,filesize,keyword);
	if(key_ptr)
	{

		while(key_ptr[i]!='\n'&&i<strlen(key_ptr))
		{
			i++;
		}
		key_ptr[i]=0;
		*keyvalue=atoi(key_ptr);
	}
	
	fclose(fp);
	return 0;	

}

int load_freq_value(const char *filename,const char *keyword,int *keyvalue, int **keylist)
{
	FILE *fp=0;
	int filesize=0, tmp_size=0;
	char *pBuffer=0;
	char *key_ptr=NULL;	
	int i=0;
	int offset = 0;
	int *list=NULL;

	if(!filename || !keyword||!keyvalue||!keylist)
	{
		return -1;
	}

	fp=fopen(filename,"rb+");
	if(!fp)
	{
		return -1;
	}
	
	fseek(fp,0,SEEK_END);
	filesize=ftell(fp);
	fseek(fp,0,SEEK_SET);
	if(!filesize)
	{
		return -1;
	}
	if(filesize<=FIX_BUFFER_SIZE)
	{
			
	}
	else
	{
		//TODO	
		log_print("!!!!! file size exceeds 16KB !!!!!!!\n");
		filesize=FIX_BUFFER_SIZE-1;
	}

	pBuffer= work_buffer;
	tmp_size = filesize;

	if(!pBuffer)
	{
		return -1;
	}
	fread(pBuffer,1,filesize,fp);

  i = 0;

  do {
  	key_ptr=find_key_value(pBuffer,tmp_size,keyword);
  	if(key_ptr) {
  		i++;
  		tmp_size -= key_ptr - pBuffer;
  		pBuffer = key_ptr;
  		offset = 0;
		  while(key_ptr[offset]!='\n'&&offset<strlen(key_ptr))
		  {
			  offset++;
			  tmp_size--;
			  pBuffer++;
		  }
  	}
  } while(key_ptr);

  *keyvalue = i;
  if(*keyvalue) {
  	list = malloc(sizeof(int) * i);
  	if(list == NULL) {
  		fclose(fp);
  		return -1;
  	}

  	pBuffer= work_buffer;
  	tmp_size = filesize;
  	i = 0;

    do {
  	  key_ptr=find_key_value(pBuffer,tmp_size,keyword);
  	  if(key_ptr) {
        tmp_size -= key_ptr - pBuffer;
  		  pBuffer = key_ptr;
    		offset = 0;
	  	  while(key_ptr[offset]!='\n'&&offset<strlen(key_ptr))
		    {
			    offset++;
			    tmp_size--;
  			  pBuffer++;
	  	  }
		    key_ptr[offset]=0;
		    list[i++]=atoi(key_ptr);
  	  }
    } while(key_ptr != NULL);
  }

  *keylist = list;
  fclose(fp);
	return 0;	

}

#if 0
int main(int argc ,char **argv)
{
	int key_value;

	load_key_value("test.txt","freq=",&key_value);	

	printf("key_value=%d\n",key_value);
	load_key_value("test.txt","symbol_rate=",&key_value);	

	printf("key_value=%d\n",key_value);
	load_key_value("test.txt","qam=",&key_value);	

	printf("key_value=%d\n",key_value);
	load_key_value("test.txt","v_pid=",&key_value);	

	printf("key_value=%d\n",key_value);
	load_key_value("test.txt","a_pid=",&key_value);	

	printf("key_value=%d\n",key_value);
	load_key_value("test.txt","v_type=",&key_value);	

	printf("key_value=%d\n",key_value);



}

#endif


