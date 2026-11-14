#ifndef COWBRATEN_UI_ADMIN_API_H
#define COWBRATEN_UI_ADMIN_API_H

#include <string>
#include <vector>

namespace cowui
{
    namespace admin
    {
        struct State
        {
            bool connected = false;
            bool local_master = false;
            bool team_mode = false;
            int local_client_num = -1;
            int master_mode = 0;
            std::string server_name;
        };

        struct Client
        {
            int client_num = -1;
            std::string name;
            std::string team;
            std::string version;
            bool spectator = false;
            int privilege = 0;
            bool local = false;
        };

        bool readstate(State &result);
        const char *clientversion();
        void readclients(std::vector<Client> &result);
        bool setlocalmaster(bool enabled);
        bool setmastermode(int mode);
        bool kick(int client_num);
        bool togglespectator(int client_num);
        bool switchteam(int client_num);
        bool togglemaster(int client_num);
    }
}

#endif
