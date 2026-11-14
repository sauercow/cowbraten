#include "rml_system.h"

#include "rml_action.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Event.h>

namespace cowui
{
    RmlSystem *RmlSystem::instance = nullptr;

    bool RmlSystem::init(Rml::Context *context, RmlActionRouter &router)
    {
        if(!context || instance) return false;
        Rml::DataModelConstructor constructor =
            context->CreateDataModel("system_admin");
        if(!constructor) return false;

        bool valid = true;
        if(auto row = constructor.RegisterStruct<ClientRow>())
            valid = row.RegisterMember("client_num", &ClientRow::client_num) &&
                row.RegisterMember("name", &ClientRow::name) &&
                row.RegisterMember("team", &ClientRow::team) &&
                row.RegisterMember("role", &ClientRow::role) &&
                row.RegisterMember("version", &ClientRow::version) &&
                row.RegisterMember("spectator", &ClientRow::spectator) &&
                row.RegisterMember("master", &ClientRow::master) &&
                row.RegisterMember("local", &ClientRow::local);
        else valid = false;

        valid = valid &&
            constructor.RegisterArray<std::vector<ClientRow>>() &&
            constructor.Bind("clients", &clients) &&
            constructor.Bind("section_admin", &section_admin) &&
            constructor.Bind("section_about", &section_about) &&
            constructor.Bind("connected", &connected) &&
            constructor.Bind("local_master", &local_master) &&
            constructor.Bind("team_mode", &team_mode) &&
            constructor.Bind("master_mode", &master_mode) &&
            constructor.Bind("client_count", &client_count) &&
            constructor.Bind("server_name", &server_name) &&
            constructor.Bind("cowbraten_version", &cowbraten_version) &&
            constructor.Bind("status", &status) &&
            constructor.BindEventCallback("select_master_mode",
                &RmlSystem::setmastermode, this) &&
            constructor.BindEventCallback("kick_client", &RmlSystem::kick, this) &&
            constructor.BindEventCallback("toggle_spectator",
                &RmlSystem::togglespectator, this) &&
            constructor.BindEventCallback("switch_team", &RmlSystem::switchteam, this) &&
            constructor.BindEventCallback("toggle_master", &RmlSystem::togglemaster, this) &&
            router.registeraction("system.section.admin", adminsection) &&
            router.registeraction("system.section.about", aboutsection) &&
            router.registeraction("admin.claim", claimmaster) &&
            router.registeraction("admin.relinquish", relinquishmaster);
        if(!valid)
        {
            context->RemoveDataModel("system_admin");
            return false;
        }
        model = constructor.GetModelHandle();
        cowbraten_version = admin::clientversion();
        instance = this;
        return true;
    }

    void RmlSystem::shutdown(Rml::Context *context)
    {
        if(instance == this) instance = nullptr;
        if(context && model) context->RemoveDataModel("system_admin");
        model = Rml::DataModelHandle();
        clients.clear();
    }

    void RmlSystem::activate(int section)
    {
        if(section >= 0) setsection(section);
        update_frame = 0;
        refresh();
    }

    void RmlSystem::update()
    {
        if(section_admin && ++update_frame >= 30)
        {
            update_frame = 0;
            refresh();
        }
    }

    void RmlSystem::refresh()
    {
        const Rml::String current_version = admin::clientversion();
        if(current_version != cowbraten_version)
        {
            cowbraten_version = current_version;
            model.DirtyVariable("cowbraten_version");
        }
        admin::State state;
        admin::readstate(state);
        connected = state.connected;
        local_master = state.local_master;
        team_mode = state.team_mode;
        master_mode = state.master_mode;
        server_name = state.server_name;
        std::vector<admin::Client> source;
        admin::readclients(source);
        std::vector<ClientRow> next;
        for(const admin::Client &client : source)
        {
            ClientRow row;
            row.client_num = client.client_num;
            row.name = client.name;
            row.team = client.team.empty() ? "No team" : client.team;
            row.version = client.version.empty() ? "Vanilla / unknown" : client.version;
            row.spectator = client.spectator;
            row.master = client.privilege != 0;
            row.local = client.local;
            row.role = client.privilege >= 2 ? "Admin" :
                (client.privilege ? "Master" :
                    (client.spectator ? "Spectator" : "Player"));
            next.push_back(row);
        }
        if(next != clients)
        {
            clients.swap(next);
            model.DirtyVariable("clients");
        }
        client_count = int(clients.size());
        model.DirtyVariable("client_count");
        model.DirtyVariable("connected");
        model.DirtyVariable("local_master");
        model.DirtyVariable("team_mode");
        model.DirtyVariable("master_mode");
        model.DirtyVariable("server_name");
    }

    void RmlSystem::setsection(int value)
    {
        section_admin = value == 0;
        section_about = value == 1;
        model.DirtyVariable("section_admin");
        model.DirtyVariable("section_about");
        if(section_admin) refresh();
    }

    void RmlSystem::setmastermode(Rml::DataModelHandle, Rml::Event &event,
        const Rml::VariantList &arguments)
    {
        const bool ok = arguments.size() == 1 &&
            admin::setmastermode(arguments[0].Get<int>(0));
        status = ok ? "Master mode request sent." : "Master privileges are required.";
        model.DirtyVariable("status");
        event.StopPropagation();
    }

#define ADMIN_CLIENT_ACTION(method, api_method, success_message) \
    void RmlSystem::method(Rml::DataModelHandle, Rml::Event &event, \
        const Rml::VariantList &arguments) \
    { \
        const bool ok = arguments.size() == 1 && \
            admin::api_method(arguments[0].Get<int>(-1)); \
        status = ok ? success_message : "The requested action is unavailable."; \
        model.DirtyVariable("status"); \
        refresh(); \
        event.StopPropagation(); \
    }

    ADMIN_CLIENT_ACTION(kick, kick, "Kick request sent.")
    ADMIN_CLIENT_ACTION(togglespectator, togglespectator, "Spectator request sent.")
    ADMIN_CLIENT_ACTION(switchteam, switchteam, "Team switch request sent.")
    ADMIN_CLIENT_ACTION(togglemaster, togglemaster, "Master request sent.")
#undef ADMIN_CLIENT_ACTION

    void RmlSystem::adminsection(Rml::Event &event)
    {
        if(instance) instance->setsection(0);
        event.StopPropagation();
    }

    void RmlSystem::aboutsection(Rml::Event &event)
    {
        if(instance) instance->setsection(1);
        event.StopPropagation();
    }

    void RmlSystem::claimmaster(Rml::Event &event)
    {
        if(instance)
        {
            const bool ok = admin::setlocalmaster(true);
            instance->status = ok ? "Master request sent." : "Connect to a server first.";
            instance->model.DirtyVariable("status");
            instance->refresh();
        }
        event.StopPropagation();
    }

    void RmlSystem::relinquishmaster(Rml::Event &event)
    {
        if(instance)
        {
            const bool ok = admin::setlocalmaster(false);
            instance->status = ok ? "Relinquish request sent." : "Connect to a server first.";
            instance->model.DirtyVariable("status");
            instance->refresh();
        }
        event.StopPropagation();
    }
}
