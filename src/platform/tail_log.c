
#include "tail_log.h"

#include "util/log.h"

#ifdef LINUX_ENVRIONMENT

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char* ci_server_sock_path = "/tmp/CI_UNIX_SOCK_SRV";
static const char* ci_client_sock_path = "/tmp/CI_UNIX_SOCK_CLIENT";

static int socket_fd = 0;
static struct sockaddr_un server_address;
static struct sockaddr_un client_address;

static int32_t tail_log_file_write(void* data,int32_t len);

static CILogHandler tail_log_file_handle = {
    tail_log_file_write,
    tail_log_file_write,
    "local_log",
    CI_TRUE,
};

static int32_t tail_log_file_write(void* data, int32_t len)
{
    sendto(socket_fd, (char *)data, len, 0,
        (struct sockaddr *) &server_address,
        sizeof(struct sockaddr_un));

    return 0;
}

static int tail_log_sock_init(void)
{
    memset(&server_address, 0, sizeof(struct sockaddr_un));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, ci_server_sock_path);

    memset(&client_address, 0, sizeof(struct sockaddr_un));
    client_address.sun_family = AF_UNIX;
    strcpy(client_address.sun_path, ci_client_sock_path);

    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        CILog_Errno("初始化unix套接字失败");
        return -1;
    }

    unlink(ci_client_sock_path);

    if (bind(socket_fd, (const struct sockaddr *) &client_address,
        sizeof(struct sockaddr_un)) < 0)
    {
        CILog_Errno("TailLog:绑定套接字失败");
        return -1;
    }

    return 0;
}
/*
 功能描述    : 初始化
 返回值      : 成功为0，失败为-1
 参数        : 无
 日期        : 2015年5月27日 13:37:06
*/
int32_t CITailLog_Init(void)
{
    static CI_BOOL b_initialized = CI_FALSE;
    int ret = 0;

#ifndef CI_UNIT_TEST
    if (CI_TRUE == b_initialized)
    {
        return -1;
    }

    if (CI_FALSE == tail_log_file_handle.b_use)
    {
        return 0;
    }
#endif /*CI_UNIT_TEST*/

    ret = tail_log_sock_init();
    if (0 > ret)
    {
        return -1;
    }

    ret = CILog_RegistHandler(&tail_log_file_handle);
    if(0 > ret)
    {
        return -1;
    }
    b_initialized = CI_TRUE;

    return 0;
}

#endif /*!LINUX_ENVRIONMENT*/