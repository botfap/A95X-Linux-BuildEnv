#include <stdio.h>
#include <string.h>
#include <log_print.h>
#include "amutils_msg.h"

//just help function
int get_amutils_msg(player_cmd_t* cmd)
{
    const int buflen = 64;
    char buf[buflen];
    int ret = -1;
    memset(buf, 0, buflen);
    ret = get_amutils_cmd(buf);
    if (ret == 0) {
        if (memcmp(buf, "switchaid:", strlen("switchaid:")) == 0 ||
            (buf[0] == 'a' && buf[1] == 'i' && buf[2] == 'd')) {
            char *p;
            int value = -1;
            p = strstr(buf, ":");
            if (p != NULL) {
                p++;
                sscanf(p, "%d", &value);
                cmd->ctrl_cmd = CMD_SWITCH_AID;
                cmd->param = value;
                return 0;
            } else {
                //log_print("error command for audio id(fmt:\"switchaid:value\")\n");
                return -1;
            }
        } else if (memcmp(buf, "forward:", strlen("forward:")) == 0 ||
                   (buf[0] == 'f' && buf[1] == ':') ||
                   (buf[0] == 'F' && buf[1] == ':')) {
            int r = -1;
            char *p;
            p = strstr(buf, ":");
            if (p != NULL) {
                p++;
                sscanf(p, "%d", &r);
                cmd->ctrl_cmd = CMD_FF;
                cmd->param = r;
                return 0;
            } else {
                //log_print("error command for forward(fmt:\"F:value\")\n");
                return -1;
            }
        } else if (memcmp(buf, "backward:", strlen("backward:")) == 0 ||
                   (buf[0] == 'b' && buf[1] == ':') ||
                   (buf[0] == 'B' && buf[1] == ':'))   {
            int r = -1;
            char *p;
            p = strstr(buf, ":");
            if (p != NULL) {
                p++;
                sscanf(p, "%d", &r);
                cmd->ctrl_cmd = CMD_FB;
                cmd->param = r;
                return 0;

            } else {
                //log_print("error command for backward(fmt:\"B:value\")\n");
                return -1;
            }
        }


        else if (memcmp(buf, "lmono", strlen("lmono")) == 0) {
            cmd->set_mode = CMD_LEFT_MONO;
            return 0;
        } else if (memcmp(buf, "rmono", strlen("rmono")) == 0) {
            cmd->set_mode = CMD_RIGHT_MONO;
            return 0;
        } else if (memcmp(buf, "stereo", strlen("stereo")) == 0) {
            cmd->set_mode = CMD_SET_STEREO;
            return 0;
        }

    }
    return -1;
}