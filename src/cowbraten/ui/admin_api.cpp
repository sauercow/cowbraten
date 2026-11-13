#include "admin_api.h"
#include "sauer_admin_bridge.h"

namespace cowui
{
    namespace admin
    {
        bool readstate(State &result)
        {
            saueradmin::State state = {};
            if(!saueradmin::readstate(state)) return false;
            result.connected = state.connected;
            result.local_master = state.local_master;
            result.team_mode = state.team_mode;
            result.local_client_num = state.local_client_num;
            result.master_mode = state.master_mode;
            result.server_name = state.server_name ? state.server_name : "";
            return true;
        }

        void readclients(std::vector<Client> &result)
        {
            result.clear();
            saueradmin::visitclients(
                [](int client_num, const char *name, const char *team,
                    const char *version, bool spectator, int privilege,
                    bool local, void *data)
                {
                    static_cast<std::vector<Client> *>(data)->push_back({
                        client_num, name ? name : "", team ? team : "",
                        version ? version : "",
                        spectator, privilege, local
                    });
                }, &result);
        }

        bool setlocalmaster(bool enabled) { return saueradmin::setlocalmaster(enabled); }
        bool setmastermode(int mode) { return saueradmin::setmastermode(mode); }
        bool kick(int client_num) { return saueradmin::kick(client_num); }
        bool togglespectator(int client_num) { return saueradmin::togglespectator(client_num); }
        bool switchteam(int client_num) { return saueradmin::switchteam(client_num); }
        bool togglemaster(int client_num) { return saueradmin::togglemaster(client_num); }
    }
}
