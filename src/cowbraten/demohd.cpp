// Optional .dmohd sidecar recording for Cowbraten client demos.
// Wire format is compatible with the L4ZY HD demo v1 extension
// (L4ZY-source, Lazy-Apathy and contributors, zlib license).
// The existing .dmo recorder and its HiFi N_P1X_DEMO_AIM messages are
// deliberately untouched. Playback and network HD will be separate steps.

#include "game.h"
#include <cmath>

VARP(demohdrecord, 0, 0, 1);
VARP(demohdplay, 0, 1, 1);
VARP(hdcurve, 0, 1, 1);

namespace game
{
    extern int demostartmillis;

    // Playback data is separate from the existing HiFi .dmo decoder.
    // Each client number has its own monotonic track for O(log n) seeking.
    struct hdframe
    {
        int t, cn;
        uchar flags, gun, state;
        float yaw, pitch, roll;
        vec o;
    };

    static vector<hdframe> hdtracks[256];
    static bool hdloaded = false;
    static int hdrecordedcn = -1;
    static int hdexactcount = 0;
    static string hdloadedname = "";
    static bool hdpendingzoom = false, hdforcedzoom = false;
    static float hdpendingzoomvalue = 0;
    static const int HDMAXBYTES = 96 << 20;
    static const int HDHEADERSIZE = 220; // L4ZY v1: 28-byte prefix + 3*64 strings
    static const int HDFRAMESIZE = 32;
    static const int HDOWN = 1;
    static bool hdvalidfloat(float x);
    static vector<hdframe> hdoutgoing;
    static vector<hdframe> hdlive[256];
    static int hdlastnetstamp = -1;
    static int hdlastservertime[256];
    static int hdtimeoffset[256];
    static bool hdclockready[256] = { false };
    static const int HDLIVEKEEP = 2000;

    void hdclearstreams()
    {
        hdoutgoing.setsize(0);
        loopi(256)
        {
            hdlive[i].setsize(0);
            hdlastservertime[i] = hdtimeoffset[i] = 0;
            hdclockready[i] = false;
        }
        hdlastnetstamp = -1;
    }

    static void hdappendnet(const hdframe &s)
    {
        // At most 24 pending camera frames (≈96 ms). Drop oldest on congestion.
        if(hdoutgoing.length() >= 24) hdoutgoing.remove(0);
        hdoutgoing.add(s);
    }

    void hdcaptureliveframe()
    {
        if(!hdserverready() || !camera1 || !player1 || demoplayback ||
           player1->clientnum < 0 || player1->clientnum >= 255 ||
           player1->state == CS_SPECTATOR) return;
        if(hdlastnetstamp >= 0 && lastmillis - hdlastnetstamp < 4) return;
        hdlastnetstamp = lastmillis;
        hdframe s;
        s.t = lastmillis;
        s.cn = player1->clientnum;
        s.flags = 4 | HDOWN | (clamp(int(getzoomprogress()*31.0f + 0.5f), 0, 31) << 3);
        s.gun = clamp(player1->gunselect, 0, 255);
        s.state = player1->state;
        s.yaw = camera1->yaw;
        s.pitch = camera1->pitch;
        s.roll = camera1->roll;
        s.o = camera1->o;
        hdappendnet(s);
    }

    void hdflushlive()
    {
        if(!hdserverready() || !player1 || player1->state == CS_SPECTATOR)
        {
            hdoutgoing.setsize(0);
            return;
        }
        const int n = min(hdoutgoing.length(), 12);
        if(!n) return;
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, N_HDPOS);
        putint(p, n);
        loopi(n)
        {
            const hdframe &s = hdoutgoing[i];
            putint(p, s.cn); putint(p, s.t); putint(p, s.flags);
            putint(p, s.gun); putint(p, s.state);
            putfloat(p, s.yaw); putfloat(p, s.pitch); putfloat(p, s.roll);
            putfloat(p, s.o.x); putfloat(p, s.o.y); putfloat(p, s.o.z);
        }
        hdoutgoing.remove(0, n);
        sendclientpacket(p.finalize(), 1);
    }

    void hdreceivelive(ucharbuf &p)
    {
        int n = getint(p);
        if(n < 1 || n > 32) { p.forceoverread(); return; }
        hdframe input[32];
        loopi(n)
        {
            hdframe &s = input[i];
            s.cn = getint(p); s.t = getint(p);
            int flags = getint(p), gun = getint(p), state = getint(p);
            s.flags = uchar(flags); s.gun = uchar(gun); s.state = uchar(state);
            s.yaw = getfloat(p); s.pitch = getfloat(p); s.roll = getfloat(p);
            s.o.x = getfloat(p); s.o.y = getfloat(p); s.o.z = getfloat(p);
            if(p.overread()) return;
        }
        if(!hdserverready() || !player1 || player1->state != CS_SPECTATOR) return;
        const int receiveTime = lastmillis;
        loopi(n)
        {
            const hdframe &in = input[i];
            if(in.cn < 0 || in.cn >= 255 || in.t < 0 || !(in.flags & HDOWN) ||
               !hdvalidfloat(in.yaw) || !hdvalidfloat(in.pitch) || !hdvalidfloat(in.roll) ||
               !hdvalidfloat(in.o.x) || !hdvalidfloat(in.o.y) || !hdvalidfloat(in.o.z) ||
               in.pitch < -90 || in.pitch > 90) continue;
            const int cn = in.cn;
            bool reset = !hdclockready[cn] || in.t <= hdlastservertime[cn] ||
                         in.t - hdlastservertime[cn] > 1000;
            if(reset)
            {
                hdlive[cn].setsize(0);
                hdtimeoffset[cn] = receiveTime - input[n-1].t;
                hdclockready[cn] = true;
            }
            hdlastservertime[cn] = in.t;
            hdframe s = in;
            s.t = in.t + hdtimeoffset[cn];
            vector<hdframe> &track = hdlive[cn];
            // Track stays monotonic; discard delayed/out-of-order frames.
            if(!track.empty() && s.t <= track[track.length()-1].t) continue;
            track.add(s);
            const int cutoff = receiveTime - HDLIVEKEEP;
            while(!track.empty() && (track[0].t < cutoff || track.length() > 500)) track.remove(0);
        }
    }


    static bool hashdframe(int cn)
    {
        return cn >= 0 && cn < 255 && !hdtracks[cn].empty();
    }

    bool hdplaybackloaded() { return hdloaded; }
    int hdplaybackrecorder() { return hdloaded ? hdrecordedcn : -1; }
    const char *hdplaybackstatus()
    {
        fpsent *target = followingplayer();
        if(!demohdplay || !target) return "";
        if(!demoplayback)
        {
            const int cn = target->clientnum;
            if(!hdserverready() || !player1 || player1->state != CS_SPECTATOR || cn < 0 || cn >= 255) return "";
            const vector<hdframe> &track = hdlive[cn];
            const int t = lastmillis - hdspecdelay();
            return !track.empty() && t >= track[0].t && t <= track[track.length()-1].t ? "HD" : "";
        }
        if(!hashdframe(target->clientnum)) return "";
        const vector<hdframe> &track = hdtracks[target->clientnum];
        return server::gamemillis >= track[0].t && server::gamemillis <= track[track.length()-1].t ? "HD" : "";
    }

    void clearhdplayback()
    {
        loopi(256) hdtracks[i].setsize(0);
        hdloaded = false;
        hdrecordedcn = -1;
        hdexactcount = 0;
        hdloadedname[0] = '\0';
        hdpendingzoom = false;
        // Reset the renderer's forced zoom on its next recompute, not here:
        // the engine may currently be inside a camera update.
    }

    // Validate IEEE754 float bits explicitly: -ffast-math assumes values are
    // finite, so std::isfinite() is not reliable for untrusted demo files.
    static bool hdvalidfloat(float x)
    {
        uint bits = 0;
        memcpy(&bits, &x, sizeof(bits));
        return (bits & 0x7F800000u) != 0x7F800000u;
    }

    static void makehdname(char *output, size_t outputsize, const char *dmofile)
    {
        copystring(output, dmofile, outputsize);
        int len = strlen(output);
        if(len < 4 || strcasecmp(output + len - 4, ".dmo")) { output[0] = '\0'; return; }
        output[len - 4] = '\0';
        concatstring(output, ".dmohd", outputsize);
    }

    static bool readhdfile(const char *dmofile)
    {
        string hdpath;
        makehdname(hdpath, sizeof(hdpath), dmofile);
        if(!hdpath[0]) return false;
        stream *f = openrawfile(hdpath, "rb");
        if(!f) return false;
        const stream::offset bytes = f->size();
        if(bytes < HDHEADERSIZE || bytes > HDMAXBYTES || (bytes-HDHEADERSIZE)%HDFRAMESIZE)
        {
            conoutf(CON_WARN, "HD demo: invalid or oversized sidecar: %s", hdpath);
            delete f;
            return false;
        }
        char magic[8];
        if(f->read(magic, 8) != 8 || memcmp(magic, "SAUERHD", 8))
        {
            conoutf(CON_WARN, "HD demo: invalid file magic: %s", hdpath);
            delete f;
            return false;
        }
        const int version = f->getlil<ushort>();
        f->getlil<ushort>(); // file flags / number of tracks
        const int protocol = f->getlil<int>();
        f->getlil<int>(); // mode
        const int recorder = f->getlil<int>();
        f->getlil<uint>(); // recording/session ID
        char extra[192];
        if(f->read(extra, sizeof(extra)) != sizeof(extra) || version != 1 || protocol != PROTOCOL_VERSION)
        {
            conoutf(CON_WARN, "HD demo: unsupported header: %s", hdpath);
            delete f;
            return false;
        }
        hdrecordedcn = recorder;
        bool valid = true;
        int frames = 0;
        for(stream::offset i = HDHEADERSIZE; i < bytes; i += HDFRAMESIZE)
        {
            hdframe s;
            s.t = f->getlil<int>();
            s.cn = f->getchar();
            int flags = f->getchar(), gun = f->getchar(), state = f->getchar();
            if(s.cn < 0 || flags < 0 || gun < 0 || state < 0) { valid = false; break; }
            s.flags = uchar(flags);
            s.gun = uchar(gun);
            s.state = uchar(state);
            s.yaw = f->getlil<float>();
            s.pitch = f->getlil<float>();
            s.roll = f->getlil<float>();
            s.o.x = f->getlil<float>();
            s.o.y = f->getlil<float>();
            s.o.z = f->getlil<float>();
            if(s.cn == 255) continue; // L4ZY extension/visibility metadata
            if(!(s.flags & HDOWN)) continue; // do not degrade Cowbraten HiFi with inferred network aim
            vector<hdframe> &track = hdtracks[s.cn];
            if(s.t < 0 || (!track.empty() && s.t < track[track.length()-1].t)
                || !hdvalidfloat(s.yaw) || !hdvalidfloat(s.pitch) || !hdvalidfloat(s.roll)
                || !hdvalidfloat(s.o.x) || !hdvalidfloat(s.o.y) || !hdvalidfloat(s.o.z))
            {
                valid = false;
                break;
            }
            track.add(s);
            frames++;
        }
        delete f;
        if(!valid)
        {
            conoutf(CON_WARN, "HD demo: corrupt or unsorted samples: %s", hdpath);
            loopi(256) hdtracks[i].setsize(0);
            hdrecordedcn = -1;
            return false;
        }
        if(!frames)
        {
            hdrecordedcn = -1;
            return false;
        }
        hdloaded = true;
        loopi(255) if(hashdframe(i)) hdexactcount++;
        conoutf("HD demo: loaded %d precise samples for %d player(s), recorder CN %d",
            frames, hdexactcount, hdrecordedcn);
        return true;
    }

    void loadhdplayback(const char *file)
    {
        // Backward seeking reopens the .dmo: keep the already decoded HD track.
        if(hdloaded && !strcmp(file, hdloadedname)) return;
        clearhdplayback();
        copystring(hdloadedname, file);
        string candidate;
        candidate[0] = '\0';
        if(const char *p = server::getdemofile(file, false))
        {
            copystring(candidate, p);
            if(readhdfile(candidate)) return;
        }
        if(!candidate[0] || strcmp(candidate, file)) readhdfile(file);
    }

    static float hdanglestep(float a, float b)
    {
        float d = b - a;
        while(d > 180.0f) d -= 360.0f;
        while(d < -180.0f) d += 360.0f;
        return d;
    }

    static float hdtangent(float d0, float d1)
    {
        if(d0*d1 <= 0) return 0;
        return clamp((d0+d1)*0.5f, -3.0f*min(fabsf(d0),fabsf(d1)), 3.0f*min(fabsf(d0),fabsf(d1)));
    }

    static float hdhermite(float v0, float v1, float v2, float v3,
        int t0, int t1, int t2, int t3, float k)
    {
        const float h = float(t2-t1);
        const float slope = h > 0 ? (v2-v1)/h : 0;
        const float d0 = t1 > t0 ? hdtangent((v1-v0)/float(t1-t0), slope) : slope;
        const float d1 = t3 > t2 ? hdtangent(slope, (v3-v2)/float(t3-t2)) : slope;
        const float k2 = k*k, k3 = k2*k;
        return (2*k3-3*k2+1)*v1 + (k3-2*k2+k)*h*d0
             + (-2*k3+3*k2)*v2 + (k3-k2)*h*d1;
    }

    static float hdmix(float v0, float v1, float v2, float v3,
        int t0, int t1, int t2, int t3, float k)
    {
        return hdcurve && t2 > t1 ? hdhermite(v0,v1,v2,v3,t0,t1,t2,t3,k) : v1+(v2-v1)*k;
    }

    static bool samplehd(int cn, int t, hdframe &output, float &zoom)
    {
        if(!hdloaded || !demohdplay || !hashdframe(cn)) return false;
        const vector<hdframe> &track = hdtracks[cn];
        if(t < track[0].t || t > track[track.length()-1].t) return false;
        int lo = 0, hi = track.length()-1;
        while(lo < hi)
        {
            const int mid = lo+(hi-lo)/2;
            if(track[mid].t < t) lo=mid+1;
            else hi=mid;
        }
        const int b = lo, a = b > 0 ? b-1 : b;
        const hdframe &sa=track[a], &sb=track[b];
        const float k = (sb.t > sa.t) ? clamp(float(t-sa.t)/float(sb.t-sa.t), 0.0f, 1.0f) : 1.0f;
        const hdframe &sp=track[a>0?a-1:a], &sn=track[b+1<track.length()?b+1:b];
        output = sa;
        const float y2=hdanglestep(sa.yaw,sb.yaw);
        const float y0=-hdanglestep(sp.yaw,sa.yaw);
        const float y3=y2+hdanglestep(sb.yaw,sn.yaw);
        output.yaw = sa.yaw + hdmix(y0,0,y2,y3,sp.t,sa.t,sb.t,sn.t,k);
        while(output.yaw < 0) output.yaw += 360;
        while(output.yaw >= 360) output.yaw -= 360;
        #define HDMIX(field) output.field = hdmix(sp.field, sa.field, sb.field, sn.field, sp.t,sa.t,sb.t,sn.t,k)
        HDMIX(pitch); HDMIX(roll); HDMIX(o.x); HDMIX(o.y); HDMIX(o.z);
        #undef HDMIX
        const float za = float((sa.flags >> 3) & 31) / 31.0f;
        const float zb = float((sb.flags >> 3) & 31) / 31.0f;
        zoom = za + (zb-za)*k;
        return true;
    }

    static bool samplelivehd(int cn, int t, hdframe &out, float &zoom)
    {
        if(!hdserverready() || !demohdplay || !player1 ||
           player1->state != CS_SPECTATOR || cn < 0 || cn >= 255) return false;
        const vector<hdframe> &track = hdlive[cn];
        if(track.empty() || t < track[0].t || t > track[track.length()-1].t) return false;
        int a = 0, b = track.length()-1;
        while(a < b)
        {
            int m = a+(b-a)/2;
            if(track[m].t < t) a = m+1;
            else b = m;
        }
        b = a;
        a = max(b-1, 0);
        const hdframe &s0 = track[a], &s1 = track[b];
        const hdframe &prev = track[max(a-1, 0)], &next = track[min(b+1, track.length()-1)];
        float k = s1.t == s0.t ? 1.0f : clamp(float(t-s0.t)/float(s1.t-s0.t), 0.0f, 1.0f);
        out = s0;
        float y2=hdanglestep(s0.yaw,s1.yaw);
        float y0=-hdanglestep(prev.yaw,s0.yaw);
        float y3=y2+hdanglestep(s1.yaw,next.yaw);
        out.yaw = s0.yaw + hdmix(y0,0,y2,y3,prev.t,s0.t,s1.t,next.t,k);
        while(out.yaw < 0) out.yaw += 360;
        while(out.yaw >= 360) out.yaw -= 360;
        #define HDLIVEMIX(field) out.field = hdmix(prev.field,s0.field,s1.field,next.field,prev.t,s0.t,s1.t,next.t,k)
        HDLIVEMIX(pitch); HDLIVEMIX(roll); HDLIVEMIX(o.x); HDLIVEMIX(o.y); HDLIVEMIX(o.z);
        #undef HDLIVEMIX
        float za = float((s0.flags >> 3)&31)/31.0f;
        float zb = float((s1.flags >> 3)&31)/31.0f;
        zoom = za+(zb-za)*k;
        return true;
    }

    static void applyhd(fpsent *d, const hdframe &s)
    {
        d->yaw=s.yaw;
        d->pitch=s.pitch;
        d->roll=s.roll;
        d->o=s.o;
        d->smoothmillis=0;
        d->resetinterp();
    }

    bool hdplaybackcamera(fpsent *target)
    {
        if(!target) return false;
        hdframe s;
        float zoom=0;
        if(demoplayback)
        {
            if(!samplehd(target->clientnum, server::gamemillis, s, zoom)) return false;
        }
        else if(!samplelivehd(target->clientnum, lastmillis - hdspecdelay(), s, zoom)) return false;
        applyhd(target, s);
        player1->yaw=s.yaw;
        player1->pitch=s.pitch;
        player1->roll=s.roll;
        player1->o=s.o;
        player1->resetinterp();
        hdpendingzoom=true;
        hdpendingzoomvalue=zoom;
        return true;
    }

    void hdplaybackothers(fpsent *followed)
    {
        if(!demoplayback || !hdloaded || !demohdplay) return;
        loopv(players)
        {
            fpsent *d=players[i];
            if(!d || d==player1 || d==followed || d->state==CS_SPECTATOR || d->clientnum < 0) continue;
            hdframe s;
            float ignoredZoom=0;
            if(samplehd(d->clientnum, server::gamemillis, s, ignoredZoom)) applyhd(d, s);
        }
    }

    void hdplaybackzoom()
    {
        if(hdpendingzoom)
        {
            forcezoom(hdpendingzoomvalue);
            hdforcedzoom=true;
        }
        else if(hdforcedzoom)
        {
            disablezoom();
            hdforcedzoom=false;
        }
        hdpendingzoom=false;
    }

    static stream *hdsidecar = NULL;
    static int hdlaststamp = -1;
    static int hdrecordcn = -1;

    void stophddemorecord()
    {
        DELETEP(hdsidecar);
        hdlaststamp = -1;
        hdrecordcn = -1;
    }

    void starthddemorecord(const char *dmopath)
    {
        stophddemorecord();
        if(!demohdrecord || !dmopath || !dmopath[0] || !player1) return;
        if(player1->clientnum < 0 || player1->clientnum >= 255) return;

        string sidecar;
        copystring(sidecar, dmopath);
        int n = strlen(sidecar);
        if(n < 4 || strcasecmp(sidecar + n - 4, ".dmo")) return;
        sidecar[n - 4] = '\0';
        concatstring(sidecar, ".dmohd");

        hdsidecar = openrawfile(sidecar, "wb");
        if(!hdsidecar)
        {
            conoutf(CON_WARN, "HD demo: cannot create %s (regular demo continues)", sidecar);
            return;
        }
        hdrecordcn = player1->clientnum;

        // L4ZY .dmohd v1 header: 220 bytes, little-endian fields.
        const char magic[8] = { 'S', 'A', 'U', 'E', 'R', 'H', 'D', '\0' };
        hdsidecar->write(magic, sizeof(magic));
        hdsidecar->putlil<ushort>(1); // DEMOHD_VERSION
        hdsidecar->putlil<ushort>(1); // header flags
        hdsidecar->putlil<int>(PROTOCOL_VERSION);
        hdsidecar->putlil<int>(gamemode);
        hdsidecar->putlil<int>(hdrecordcn);
        hdsidecar->putlil<uint>(uint(time(NULL)));

        char map[64] = { 0 }, name[64] = { 0 }, reserved[64] = { 0 };
        copystring(map, getclientmap(), sizeof(map));
        const char *basename = strrchr(dmopath, '/');
        basename = basename ? basename + 1 : dmopath;
        const char *windowsbase = strrchr(basename, '\\');
        if(windowsbase) basename = windowsbase + 1;
        copystring(name, basename, sizeof(name));
        n = strlen(name);
        if(n >= 4 && !strcasecmp(name + n - 4, ".dmo")) name[n - 4] = '\0';
        hdsidecar->write(map, sizeof(map));
        hdsidecar->write(name, sizeof(name));
        hdsidecar->write(reserved, sizeof(reserved));
        conoutf("HD demo sidecar recording: %s", sidecar);
    }

    void recordhdframe()
    {
        hdcaptureliveframe();
        if(!hdsidecar || !demorecord || !camera1 || !player1 || demoplayback) return;
        int t = max(lastmillis - demostartmillis, 0);
        if(hdlaststamp >= 0 && t - hdlaststamp < 4) return; // <=250 Hz
        hdlaststamp = t;

        // One L4ZY hdsample: 32 bytes, keeping float camera geometry.
        int zoomlevel = clamp(int(getzoomprogress()*31.0f + 0.5f), 0, 31);
        int flags = 4 | (player1->state==CS_SPECTATOR ? 2 : 1) | (zoomlevel << 3);
        hdsidecar->putlil<int>(t);
        hdsidecar->putchar(hdrecordcn);
        hdsidecar->putchar(flags);
        hdsidecar->putchar(clamp(player1->gunselect, 0, 255));
        hdsidecar->putchar(clamp(player1->state, 0, 255));
        hdsidecar->putlil<float>(camera1->yaw);
        hdsidecar->putlil<float>(camera1->pitch);
        hdsidecar->putlil<float>(camera1->roll);
        hdsidecar->putlil<float>(camera1->o.x);
        hdsidecar->putlil<float>(camera1->o.y);
        hdsidecar->putlil<float>(camera1->o.z);
    }
}

// Runtime playback controls; default is on only when a matching .dmohd exists.
ICOMMAND(demohdloaded, "", (), intret(game::hdplaybackloaded() ? 1 : 0));
ICOMMAND(demohdrecorder, "", (), intret(game::hdplaybackrecorder()));
ICOMMAND(demohdstatus, "", (), result(game::hdplaybackstatus()));
ICOMMAND(toggledemohd, "", (),
{
    demohdplay = demohdplay ? 0 : 1;
    conoutf("HD demo playback %s", demohdplay ? "enabled" : "disabled (original HiFi)" );
});
