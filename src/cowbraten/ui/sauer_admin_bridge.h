#ifndef COWBRATEN_UI_SAUER_ADMIN_BRIDGE_H
#define COWBRATEN_UI_SAUER_ADMIN_BRIDGE_H

namespace cowui
{
    namespace saueradmin
    {
        struct State
        {
            bool connected;
            bool local_master;
            bool team_mode;
            int local_client_num;
            int master_mode;
            const char *server_name;
        };

        typedef void (*ClientVisitor)(int client_num, const char *name,
            const char *team, bool spectator, int privilege,
            bool local, void *data);

        bool readstate(State &result);
        void visitclients(ClientVisitor visitor, void *data);
        bool setlocalmaster(bool enabled);
        bool setmastermode(int mode);
        bool kick(int client_num);
        bool togglespectator(int client_num);
        bool switchteam(int client_num);
        bool togglemaster(int client_num);
    }
}

#endif
