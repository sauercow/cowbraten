#include "engine.h"
#include "game.h"

#include "sauer_chat_bridge.h"

extern int confilter, miniconfilter, consize, miniconsize, miniconwidth,
    confade, miniconfade;

namespace cowui
{
    namespace sauerchat
    {
        InputMode inputmode()
        {
            switch(chatcommandmode())
            {
                case 1: return InputMode::Public;
                case 2: return InputMode::Team;
                default: return InputMode::None;
            }
        }

        const char *inputtext()
        {
            return commandlinetext();
        }

        int inputposition()
        {
            return commandlineposition();
        }

        int now()
        {
            return totalmillis;
        }

        bool classifyline(int type, bool &team)
        {
            if(type & CON_TEAMCHAT)
            {
                team = true;
                return true;
            }
            if(type & CON_CHAT)
            {
                team = false;
                return true;
            }
            return false;
        }

        bool topenabled(int type)
        {
            return (confilter & type) != 0;
        }

        bool minienabled(int type)
        {
            return miniconsize && miniconwidth &&
                (miniconfilter & type) != 0;
        }

        int topfade() { return confade; }
        int minifade() { return miniconfade; }
        int topcapacity() { return consize; }
        int minicapacity() { return miniconsize; }
        bool topfeedminimapclipped() { return game::clipconsole(100, 100) > 0; }
    }
}
