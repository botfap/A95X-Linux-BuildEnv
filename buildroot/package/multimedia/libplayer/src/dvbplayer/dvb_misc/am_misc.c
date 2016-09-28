/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Misc functions
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-05: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 1

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <am_types.h>
#include <am_debug.h>
#include <am_mem.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <assert.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Static functions
 ***************************************************************************/

static AM_ErrorCode_t try_write(int fd, const char *buf, int len)
{
	const char *ptr = buf;
	int left = len;
	int ret;
	
	while(left)
	{
		ret = write(fd, ptr, left);
		if(ret==-1)
		{
			if(errno!=EINTR)
				return AM_FAILURE;
			ret = 0;
		}
		
		ptr += ret;
		left-= ret;
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t try_read(int fd, char *buf, int len)
{
	char *ptr = buf;
	int left = len;
	int ret;
	
	while(left)
	{
		ret = read(fd, ptr, left);
		if(ret==-1)
		{
			if(errno!=EINTR)
				return AM_FAILURE;
			ret = 0;
		}
		
		ptr += ret;
		left-= ret;
	}
	
	return AM_SUCCESS;
}

/****************************************************************************
 * API functions
 ***************************************************************************/

/**\brief 向一个文件打印字符串
 * \param[in] name 文件名
 * \param[in] cmd 向文件打印的字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_FileEcho(const char *name, const char *cmd)
{
	int fd, len, ret;
	
	assert(name && cmd);
	
	fd = open(name, O_WRONLY);
	if(fd==-1)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", name);
		return AM_FAILURE;
	}
	
	len = strlen(cmd);
	
	ret = write(fd, cmd, len);
	if(ret!=len)
	{
		AM_DEBUG(1, "write failed \"%s\"", strerror(errno));
		return AM_FAILURE;
	}
	
	return AM_SUCCESS;
}

/**\brief 读取一个文件中的字符串
 * \param[in] name 文件名
 * \param[out] buf 存放字符串的缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_FileRead(const char *name, char *buf, int len)
{
	FILE *fp;
	char *ret;
	
	assert(name && buf);
	
	fp = fopen(name, "r");
	if(!fp)
	{
		AM_DEBUG(1, "cannot open file \"%s\"", name);
		return AM_FAILURE;
	}
	
	ret = fgets(buf, len, fp);
	if(!ret)
	{
		AM_DEBUG(1, "read the file \"%s\" failed", name);
	}
	
	fclose(fp);
	
	return ret ? AM_SUCCESS : AM_FAILURE;
}

/**\brief 创建本地socket服务
 * \param[in] name 服务名称
 * \param[out] fd 返回服务器socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalServer(const char *name, int *fd)
{
	struct sockaddr_un addr;
	int s, ret;
#if 0	
	assert(name && fd);
	
	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(s==-1)
	{
		AM_DEBUG(1, "cannot create local socket \"%s\"", strerror(errno));
		return AM_FAILURE;
	}
	
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, name, sizeof(addr.sun_path)-1);

	ret = bind(s, (struct sockaddr*)&addr, SUN_LEN(&addr));
	if(ret==-1)
	{
		AM_DEBUG(1, "bind to \"%s\" failed \"%s\"", name, strerror(errno));
		close(s);
		return AM_FAILURE;
	}

	ret = listen(s, 5);
	if(ret==-1)
	{
		AM_DEBUG(1, "listen failed \"%s\" (%s)", name, strerror(errno));
		close(s);
		return AM_FAILURE;
	}

	*fd = s;
#endif
	return AM_SUCCESS;
        
}

/**\brief 通过本地socket连接到服务
 * \param[in] name 服务名称
 * \param[out] fd 返回socket
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalConnect(const char *name, int *fd)
{
	struct sockaddr_un addr;
	int s, ret;
#if 0	
	assert(name && fd);
	
	s = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(s==-1)
	{
		AM_DEBUG(1, "cannot create local socket \"%s\"", strerror(errno));
		return AM_FAILURE;
	}
	
	addr.sun_family = AF_LOCAL;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path)-1);
        
        ret = connect(s, (struct sockaddr*)&addr, SUN_LEN(&addr));
        if(ret==-1)
        {
        	AM_DEBUG(1, "connect to \"%s\" failed \"%s\"", name, strerror(errno));
        	close(s);
		return AM_FAILURE;
        }
        
        *fd = s;
#endif
        return AM_SUCCESS;
}

/**\brief 通过本地socket发送命令
 * \param fd socket
 * \param[in] cmd 命令字符串
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalSendCmd(int fd, const char *cmd)
{
	AM_ErrorCode_t ret;
	int len;
	
	assert(cmd);
	
	len = strlen(cmd)+1;
	
	ret = try_write(fd, (const char*)&len, sizeof(int));
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "write local socket failed");
		return ret;
	}
	
	ret = try_write(fd, cmd, len);
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "write local socket failed");
		return ret;
	}
	
	AM_DEBUG(2, "write cmd: %s", cmd);
	
	return AM_SUCCESS;
}

/**\brief 通过本地socket接收响应字符串
 * \param fd socket
 * \param[out] buf 缓冲区
 * \param len 缓冲区大小
 * \return
 *   - AM_SUCCESS 成功
 *   - 其他值 错误代码
 */
AM_ErrorCode_t AM_LocalGetResp(int fd, char *buf, int len)
{
	AM_ErrorCode_t ret;
	int bytes;
	
	assert(buf);
	
	ret = try_read(fd, (char*)&bytes, sizeof(int));
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "read local socket failed");
		return ret;
	}
	
	if(len<bytes)
	{
		AM_DEBUG(1, "respond buffer is too small");
		return AM_FAILURE;
	}
	
	ret = try_read(fd, buf, bytes);
	if(ret!=AM_SUCCESS)
	{
		AM_DEBUG(1, "read local socket failed");
		return ret;
	}
	
	return AM_SUCCESS;
}

