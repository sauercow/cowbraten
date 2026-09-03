#include "game.h"
#include "serverbrowser.h"
#include "sauer_server_bridge.h"

extern vector<serverinfo *> servers;
extern void refreshservers();
extern void initservers();
extern void updatefrommaster();

namespace cowui
{
    namespace sauerservers
    {
        static vector<Entry> snapshot;
        static unsigned long long snapshot_revision = 0;
        static int last_snapshot_check = -1;

        static void makeid(
            char *destination,
            size_t capacity,
            const char *host,
            int port)
        {
            snprintf(
                destination,
                capacity,
                "%s:%d",
                host,
                port
            );
        }

        static void buildentry(
            const serverinfo &server_info,
            Entry &entry)
        {
            memset(&entry, 0, sizeof(entry));

            makeid(
                entry.id,
                sizeof(entry.id),
                server_info.name,
                server_info.port
            );

            copystring(entry.host, server_info.name);
            copystring(
                entry.description,
                server_info.sdesc[0]
                    ? server_info.sdesc
                    : server_info.name
            );
            copystring(entry.map, server_info.map);

            entry.port = server_info.port;
            entry.players = server_info.numplayers;
            entry.capacity = server_info.attr.inrange(3)
                ? server_info.attr[3]
                : -1;
            entry.ping = server_info.ping == serverinfo::WAITING
                ? -1
                : server_info.ping;

            const bool resolved =
                server_info.address.host != ENET_HOST_ANY;
            const bool responded =
                resolved &&
                server_info.ping != serverinfo::WAITING &&
                !server_info.attr.empty();
            const bool compatible =
                responded &&
                server_info.attr[0] == PROTOCOL_VERSION;

            entry.selectable = compatible;
            entry.full = compatible &&
                entry.capacity >= 0 &&
                entry.players >= entry.capacity;
            entry.passworded = compatible &&
                server_info.attr.inrange(4) &&
                server_info.attr[4] == MM_PASSWORD;
            entry.waiting = !responded;
            entry.incompatible = responded && !compatible;

            if(server_info.attr.inrange(1))
            {
                copystring(
                    entry.mode,
                    server::modename(server_info.attr[1], "")
                );
            }

            if(!resolved)
            {
                copystring(entry.status, "Unknown host");
            }
            else if(!responded)
            {
                copystring(entry.status, "Waiting");
            }
            else if(!compatible)
            {
                copystring(
                    entry.status,
                    server_info.attr[0] < PROTOCOL_VERSION
                        ? "Older protocol"
                        : "Newer protocol"
                );
            }
            else if(entry.full)
            {
                copystring(entry.status, "Full");
            }
            else if(server_info.attr.inrange(4))
            {
                copystring(
                    entry.status,
                    server::mastermodename(
                        server_info.attr[4],
                        "Available"
                    )
                );
            }
            else
            {
                copystring(entry.status, "Available");
            }
        }

        static unsigned long long hashbytes(
            unsigned long long hash,
            const void *data,
            size_t size)
        {
            const uchar *bytes = static_cast<const uchar *>(data);

            loopi(size)
            {
                hash ^= bytes[i];
                hash *= 1099511628211ULL;
            }

            return hash;
        }

        static unsigned long long currentfingerprint()
        {
            unsigned long long hash = 1469598103934665603ULL;

            loopv(servers)
            {
                Entry entry;
                buildentry(*servers[i], entry);
                hash = hashbytes(hash, &entry, sizeof(entry));
            }

            const int count = servers.length();
            return hashbytes(hash, &count, sizeof(count));
        }

        static void rebuildsnapshot()
        {
            snapshot.shrink(0);

            loopv(servers)
            {
                Entry &entry = snapshot.add();
                buildentry(*servers[i], entry);
            }

            if(++snapshot_revision == 0)
                snapshot_revision = 1;
        }

        static unsigned long long synchronise(bool force)
        {
            if(!force &&
               last_snapshot_check >= 0 &&
               totalmillis >= last_snapshot_check &&
               totalmillis - last_snapshot_check < 100)
            {
                return snapshot_revision;
            }

            last_snapshot_check = totalmillis;

            const unsigned long long fingerprint =
                currentfingerprint();

            static unsigned long long previous_fingerprint = 0;

            if(force ||
               !snapshot_revision ||
               fingerprint != previous_fingerprint)
            {
                rebuildsnapshot();
                previous_fingerprint = fingerprint;
            }

            return snapshot_revision;
        }

        void initialise()
        {
            initservers();
            last_snapshot_check = -1;
            synchronise(true);
        }

        unsigned long long update()
        {
            refreshservers();
            return synchronise(false);
        }

        void visit(EntryVisitor visitor, void *user)
        {
            if(!visitor)
                return;

            loopv(snapshot)
                visitor(snapshot[i], user);
        }

        void refreshfrommaster()
        {
            updatefrommaster();
            last_snapshot_check = -1;
            synchronise(true);
        }

        bool connect(const char *id)
        {
            if(!id || !id[0])
                return false;

            loopv(servers)
            {
                serverinfo &server_info = *servers[i];
                char current_id[ID_CAPACITY];

                makeid(
                    current_id,
                    sizeof(current_id),
                    server_info.name,
                    server_info.port
                );

                if(strcmp(id, current_id))
                    continue;

                string host;
                string password;
                copystring(host, server_info.name);
                copystring(
                    password,
                    server_info.password
                        ? server_info.password
                        : ""
                );

                connectserv(
                    host,
                    server_info.port,
                    password[0] ? password : nullptr
                );
                return true;
            }

            return false;
        }
    }
}
