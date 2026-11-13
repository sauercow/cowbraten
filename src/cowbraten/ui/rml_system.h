#ifndef COWBRATEN_UI_RML_SYSTEM_H
#define COWBRATEN_UI_RML_SYSTEM_H

#include "admin_api.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml { class Context; class Event; }

namespace cowui
{
    class RmlActionRouter;

    class RmlSystem final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void activate(int section = -1);
        void update();

    private:
        struct ClientRow
        {
            int client_num = -1;
            Rml::String name, team, role, version;
            bool spectator = false, master = false, local = false;

            bool operator==(const ClientRow &other) const
            {
                return client_num == other.client_num && name == other.name &&
                    team == other.team && role == other.role && version == other.version &&
                    spectator == other.spectator && master == other.master &&
                    local == other.local;
            }
        };

        void refresh();
        void setsection(int value);
        void setmastermode(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void kick(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void togglespectator(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void switchteam(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);
        void togglemaster(Rml::DataModelHandle, Rml::Event &,
            const Rml::VariantList &);

        static void adminsection(Rml::Event &);
        static void aboutsection(Rml::Event &);
        static void claimmaster(Rml::Event &);
        static void relinquishmaster(Rml::Event &);
        static RmlSystem *instance;

        Rml::DataModelHandle model;
        std::vector<ClientRow> clients;
        bool section_admin = true, section_about = false;
        bool connected = false, local_master = false, team_mode = false;
        int master_mode = 0, update_frame = 0, client_count = 0;
        Rml::String server_name, status;
    };
}

#endif
