#include "cube.h"
#include "embedded_cfgs.h"

namespace game {
    struct embeddedfile<17379> menus_cfg = {
        "data/cowbraten/menus.cfg.gz",
        0xa61c4d24,
        {
            #include "../../data/cowbraten/menus.cfg.gz.xxd"
        }
    };

    struct embeddedfile<1121> master_cfg = {
        "data/cowbraten/master.cfg.gz",
        0xc2ae1c43,
        {
            #include "../../data/cowbraten/master.cfg.gz.xxd"
        }
    };

    struct embeddedfile<640> gamehud_cfg = {
        "data/cowbraten/gamehud.cfg.gz",
        0x7ef763f8,
        {
            #include "../../data/cowbraten/gamehud.cfg.gz.xxd"
        }
    };

    struct embeddedfile<838> keymap_cfg = {
        "data/cowbraten/keymap.cfg.gz",
        0xfd4470e9,
        {
            #include "../../data/cowbraten/keymap.cfg.gz.xxd"
        }
    };

    template<size_t N>
    int update(embeddedfile<N> *i)
    {
        stream *f = opengzfile(path(i->name), "rb");
        if(f)
        {
            uint fcrc = f->getcrc();
            delete f;
            if(fcrc && fcrc == i->crc) return 0; // no need to write
        }

        f = openrawfile(path(i->name), "wb");
        if(!f) return 2;
        f->write(i->contents, N);
        delete f;
        conoutf("updated %s", i->name);
        return 0;
    }

    int writeembeddedcfgs()
    {
        int err = update(&menus_cfg);
        if(err) { conoutf("\f6error updating %s (error code %d)!", menus_cfg.name, err); return err; }

        err = update(&master_cfg);
        if(err) { conoutf("\f6error updating %s (error code %d)!", master_cfg.name, err); return err; }

        err = update(&gamehud_cfg);
        if(err) { conoutf("\f6error updating %s (error code %d)!", gamehud_cfg.name, err); return err; }

        err = update(&keymap_cfg);
        if(err) { conoutf("\f6error updating %s (error code %d)!", keymap_cfg.name, err); return err; }

        return 0;
    }

    MOD(VARFP, usecowbratenmenus, 0, 1, 1, {
        if(usecowbratenmenus) execfile(menus_cfg.name);
        else execfile("data/menus.cfg");
    });
}
