#include "game.h"

namespace server {

    static bool parseipv4(const char *text, uint &ip)
    {
        if(!text || !text[0]) return false;
        uchar bytes[4];
        const char *cursor = text;
        loopi(4)
        {
            if(*cursor < '0' || *cursor > '9') return false;
            int value = 0, digits = 0;
            while(*cursor >= '0' && *cursor <= '9')
            {
                value = value*10 + (*cursor++ - '0');
                if(++digits > 3 || value > 255) return false;
            }
            bytes[i] = uchar(value);
            if(i < 3)
            {
                if(*cursor++ != '.') return false;
            }
            else if(*cursor) return false;
        }
        memcpy(&ip, bytes, sizeof(ip));
        return true;
    }

    static const char *formatipv4(uint ip, char *buffer, size_t size)
    {
        const uchar *bytes = reinterpret_cast<const uchar *>(&ip);
        if(!buffer || size < 16) return nullptr;
        snprintf(buffer, size, "%u.%u.%u.%u",
            bytes[0], bytes[1], bytes[2], bytes[3]);
        return buffer;
    }

    void getip(clientinfo *sender, int cn)
    {
        if(sender->privilege<PRIV_ADMIN) return;
        clientinfo *ci = (clientinfo *)getclientinfo(cn);
        if(!ci) { sendf(sender->clientnum, 1, "ris", N_SERVMSG, "no such client"); return; }
        const char *ip = getclienthostname(cn);
        defformatstring(reply, "%s (%d) has IP %s", ci->name, ci->clientnum, ip);
        sendf(sender->clientnum, 1, "ris", N_SERVMSG, reply);
    }

    extern vector<ban> bannedips;
    extern void addban(uint ip, int expire);

    bool addban(const char *ipstring, int expire)
    {
        uint ip;
        if(!expire || !parseipv4(ipstring, ip)) return false;
        addban(ip, expire);
        return true;
    }

    ICOMMAND(ban, "si", (char *ip, int *minutes), addban(ip, *minutes * 60*1000));

    void addban(clientinfo *sender, const char *ipstring, int minutes)
    {
        if(sender->privilege<PRIV_ADMIN) return;
        if (!addban(ipstring, minutes * 60*1000)) return;
        defformatstring(confirmation, "banned %s for %dh %dm", ipstring, minutes/60, minutes%60);
        sendf(sender->clientnum, 1, "ris", N_SERVMSG, confirmation);
    }

    void delban(const char *ipstring)
    {
        uint ip;
        if(!parseipv4(ipstring, ip)) return;
        loopv(bannedips) if(bannedips[i].ip==ip) { bannedips.remove(i); }
    }

    ICOMMAND(unban, "s", (char *ip), delban(ip));

    void delban(clientinfo *sender, const char *ipstring)
    {
        if(sender->privilege<PRIV_ADMIN) return;
        delban(ipstring);
        defformatstring(confirmation, "unbanned %s", ipstring);
        sendf(sender->clientnum, 1, "ris", N_SERVMSG, confirmation);
    }

    const char *listbans()
    {
        if(!bannedips.length()) { return "no bans"; }
        vector<char> buf;
        loopv(bannedips)
        {
            static char ip[16];
            if(!formatipv4(bannedips[i].ip, ip, sizeof(ip))) continue;
            int expsecs = (bannedips[i].expire - totalmillis) / 1000;
            defformatstring(line, "%s (%dh %dm %ds)", ip, expsecs/(60*60), (expsecs%(60*60))/60, (expsecs%(60*60))%60);
            if(i) buf.put('\n');
            buf.put(line, strlen(line));
        };
        buf.put('\0');
        return newstring(buf.getbuf());
    }

    ICOMMAND(listbans, "", (), result(listbans()));

    void listbans(clientinfo *sender)
    {
        if(sender->privilege<PRIV_ADMIN) return;
        sendf(sender->clientnum, 1, "ris", N_SERVMSG, listbans());
    }

}
