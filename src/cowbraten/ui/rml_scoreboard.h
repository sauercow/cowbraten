#ifndef COWBRATEN_UI_RML_SCOREBOARD_H
#define COWBRATEN_UI_RML_SCOREBOARD_H

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <vector>

namespace Rml { class Context; class ElementDocument; }

namespace cowui
{
    class RmlActionRouter;

    class RmlScoreboard final
    {
    public:
        bool init(Rml::Context *context, RmlActionRouter &router);
        void shutdown(Rml::Context *context);
        void request(bool value);
        void update(bool menu_active);
        void scroll(float wheel_delta);
        bool visible() const { return document_visible; }

    private:
        struct Row
        {
            int group_index = 0;
            bool group_first = false, local = false, spectator = false;
            bool dead = false, privileged = false;
            bool good_team = false, evil_team = false;
            Rml::String group_title, name, role;
            Rml::String flags, frags, deaths, suicides, kpd, accuracy;
            Rml::String damage, damage_received, ping, pj, client_num;

            bool operator==(const Row &other) const;
        };

        bool loaddocument();
        void setvisible(bool value);
        void sync();

        Rml::Context *context = nullptr;
        Rml::ElementDocument *document = nullptr;
        Rml::DataModelHandle model;
        std::vector<Row> rows, evil_rows, good_rows, spectator_rows;
        Rml::String server_name, match_line, time_text;
        Rml::String evil_title = "EVIL", good_title = "GOOD";
        bool team_mode = false, has_spectators = false;
        bool requested = false, document_visible = false, load_attempted = false;
        bool show_flags = false, show_frags = true, show_deaths = false;
        bool show_suicides = false, show_kpd = false, show_accuracy = false;
        bool show_damage = false, show_damage_received = false;
        bool show_ping = true, show_pj = false, show_client_num = false;
    };
}

#endif
