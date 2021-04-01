//
// Created by utente on 31/03/2021.
//

#ifndef MAJORDOMOPATTERN0_1_MDP_COMMON_H
#define MAJORDOMOPATTERN0_1_MDP_COMMON_H
#define MDPC_CLIENT         "MDPC0X"

//  MDP/Client commands, as strings
#define MDPC_REQUEST        "\001"
#define MDPC_REPORT         "\002"
#define MDPC_NAK            "\003"

static char *mdpc_commands [] = {
        NULL, "REQUEST", "REPORT", "NAK",
};

//  This is the version of MDP/Worker we implement
#define MDPW_WORKER         "MDPW0X"

//  MDP/Worker commands, as strings
#define MDPW_READY          "\001"
#define MDPW_REQUEST        "\002"
#define MDPW_REPORT         "\003"
#define MDPW_HEARTBEAT      "\004"
#define MDPW_DISCONNECT     "\005"

static char *mdpw_commands [] = {
        NULL, "READY", "REQUEST", "REPORT", "HEARTBEAT", "DISCONNECT"
};


#endif //MAJORDOMOPATTERN0_1_MDP_COMMON_H
