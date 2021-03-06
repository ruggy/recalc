/* 
    sound.cpp: basic positional sound using sdl_mixer 

*/

//#include "engine.h"
#include "recalc.h"

#include "SDL_mixer.h"
#define MAXVOL MIX_MAX_VOLUME

bool nosound = false ;
bool nomusic = false ;
bool nosfx   = false ;
//bool nosound = true ;

extern Camera camera ;

struct soundsample
{
    char *name;
    Mix_Chunk *chunk;

    soundsample() : name(NULL) {}
    ~soundsample() { DELETEA(name); }
};

struct soundslot
{
    soundsample *sample;
    int volume, maxuses;
};

struct soundchannel
{ 
    int id;
    bool inuse;
    vec loc; 
    soundslot *slot; 
//  extentity *ent; 
    int radius, volume, pan;
    bool dirty;

    soundchannel(int id) : id(id) { reset(); }

    bool hasloc() const { return loc.x >= -1e15f; }
    void clearloc() { loc = vec(-1e16f, -1e16f, -1e16f); }

    void reset()
    {
        inuse = false;
        clearloc();
        slot = NULL;
//        ent = NULL;
        radius = 0;
        volume = -1;
        pan = -1;
        dirty = false;
    }
};
vector<soundchannel> channels;
int maxchannels = 0;

soundchannel &newchannel(int n, soundslot *slot, const vec *loc = NULL, /*extentity *ent = NULL,*/ int radius = 0)
{
    while(!channels.inrange(n)) channels.add(channels.length());
    soundchannel &chan = channels[n];
    chan.reset();
    chan.inuse = true;
    if(loc) chan.loc = *loc;
    chan.slot = slot;
//    chan.ent = ent;   FIXME: associate sound to entity causing/emitting it. 
    chan.radius = radius;
    return chan;
}

void freechannel(int n)
{
    if (nosound) return ;
    // Note that this can potentially be called from the SDL_mixer audio thread.
    // Be careful of race conditions when checking chan.inuse without locking audio.
    // Can't use Mix_Playing() checks due to bug with looping sounds in SDL_mixer.
    if(!channels.inrange(n) || !channels[n].inuse) return;
    soundchannel &chan = channels[n];
    chan.inuse = false;
//    if(chan.ent) chan.ent->visible = false;
}

void syncchannel(soundchannel &chan)
{
    if (nosound) return ;
    if(!chan.dirty) return;
    if(!Mix_FadingChannel(chan.id)) Mix_Volume(chan.id, chan.volume);
    Mix_SetPanning(chan.id, 255-chan.pan, chan.pan);
    chan.dirty = false;
}

void stopchannels()
{
    if (nosound) return ;
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(!chan.inuse) continue;
        Mix_HaltChannel(i);
        freechannel(i);
    }
}

void setmusicvol(int musicvol);
void closemumble(); 
void updatemumble() ;
//VARFP(soundvol, 0, 255, 255, if(!soundvol) { stopchannels(); setmusicvol(0); });
int soundvol = 255 ;
//VARFP(musicvol, 0, 128, 255, setmusicvol(soundvol ? musicvol : 0));
int musicvol = 128 ;

char *musicfile = NULL, *musicdonecmd = NULL;

Mix_Music *music = NULL;
SDL_RWops *musicrw = NULL;
stream *musicstream = NULL;

void setmusicvol(int musicvol)
{
    if(nosound) return;
    if(music) Mix_VolumeMusic((musicvol*MAXVOL)/255);
}

void stopmusic()
{
    if(nosound) return;
    DELETEA(musicfile);
    DELETEA(musicdonecmd);
    if(music)
    {
        Mix_HaltMusic();
        Mix_FreeMusic(music);
        music = NULL;
    }
    if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
    DELETEP(musicstream);
}

//VARF(soundchans, 1, 32, 128, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
int soundchans = 8 ;
//VARF(soundfreq, 0, MIX_DEFAULT_FREQUENCY, 44100, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
int soundfreq = MIX_DEFAULT_FREQUENCY ;
//VARF(soundbufferlen, 128, 1024, 4096, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
//int soundbufferlen = 1024 ;
//int soundbufferlen = 4096 ;
//int soundbufferlen = 512 ;

// Yes, this is small, but for whatever reason larger buffer sizes result in crazy latencies! 
//int soundbufferlen = 256 ;
int soundbufferlen = 1024 ;

void initsound()
{
    if ( nosound ) return ;
    
    printf("\n[SOUND::initsound] called. ") ;
    
    if(Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, 2, soundbufferlen)<0)
    {
        nosound = true;
//        conoutf(CON_ERROR, "sound init failed (SDL_mixer): %s", (size_t)Mix_GetError());
        //printf("sound init failed (SDL_mixer): %s", (size_t)Mix_GetError());
        printf("sound init failed (SDL_mixer): %s", Mix_GetError());
        return;
    }
    else
    {
        printf("\nSound properly initialized. \n") ;
    }
    //return ;
	Mix_AllocateChannels(soundchans);	
    Mix_ChannelFinished(freechannel);
    
    printf("initsound: soundchans == %d", soundchans) ;
    maxchannels = soundchans;
    nosound = false;
}

void musicdone()
{
    if (nosound) return ;
    if(music) { Mix_HaltMusic(); Mix_FreeMusic(music); music = NULL; }
    if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
    DELETEP(musicstream);
    DELETEA(musicfile);


    if(!musicdonecmd) return;

/*
    FIXME: do we need a musicdonecmd? 
    char *cmd = musicdonecmd;
    musicdonecmd = NULL;
    execute(cmd);
    delete[] cmd;
*/
}

Mix_Music *loadmusic(const char *name)
{
    if(!musicstream) musicstream = openzipfile(name, "rb");
    if(musicstream)
    {
        if(!musicrw) musicrw = musicstream->rwops();
        if(!musicrw) DELETEP(musicstream);
    }
    if(musicrw) 
    {
        music = Mix_LoadMUS_RW(musicrw);
    }
    else 
    {
        music = Mix_LoadMUS(findfile(name, "rb")); 
    }
    if(!music)
    {
        if(musicrw) { SDL_FreeRW(musicrw); musicrw = NULL; }
        DELETEP(musicstream);
    }
    return music;
}
 
void startmusic(const char *name, char *cmd)
{
    if(nosound) return;
    if(nomusic) return;

    stopmusic();
    if(soundvol && musicvol && *name)
    {
        //defformatstring(file)("packages/%s", name);
        defformatstring(file)("data/music/%s", name);
        path(file);

        printf("\nstarmusic now attempting to load music file: %s\n", file) ;
        if(loadmusic(file))
        {
            DELETEA(musicfile);
            DELETEA(musicdonecmd);
            musicfile = newstring(file);
    printf("\n9 \n") ;
       //     if(cmd[0]) musicdonecmd = newstring(cmd);
            //Mix_PlayMusic(music, cmd[0] ? 0 : -1);
            Mix_PlayMusic(music, -1);
    printf("\n10 \n") ;
            Mix_VolumeMusic((musicvol*MAXVOL)/255);
//            intret(1);
        }
        else
        {
            //conoutf(CON_ERROR, "could not play music: %s", file);
            printf("could not play music: %s", file);
            fprintf(stderr, "BAD MUSIC: %s\n", Mix_GetError());
        }
    }
}

//COMMANDN(music, startmusic, "ss");

hashtable<const char *, soundsample> samples;
vector<soundslot> gamesounds, mapsounds;

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
    if (nosound) return -1 ;
    loopv(sounds)
    {
        if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].volume==vol)) return i;
    }
    return -1;
}

int addsound(const char *name, int vol, int maxuses, vector<soundslot> &sounds)
{
    if (nosound) return -1 ;
    printf("\nadding sound 1.0") ;
    soundsample *s = samples.access(name);
    if(!s)
    {
        char *n = newstring(name);
        s = &samples[n];
        s->name = n;
        s->chunk = NULL;
    }
    printf("\nadding sound 1.1") ;
    soundslot *oldsounds = sounds.getbuf();
    int oldlen = sounds.length();
    soundslot &slot = sounds.add();
    // sounds.add() may relocate slot pointers
    if(sounds.getbuf() != oldsounds) loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse && chan.slot >= oldsounds && chan.slot < &oldsounds[oldlen])
            chan.slot = &sounds[chan.slot - oldsounds];
    }
    slot.sample = s;
    slot.volume = vol ? vol : 100;
    slot.maxuses = maxuses;
    printf("\nadding sound 1.3 (id %d)", oldlen) ;
    return oldlen;
}

int registersound(const char *name, int *vol)
{
    if (nosound) return -1 ;
// intret(
    addsound(name, *vol, 0, gamesounds) ;
    //); 
}
//COMMAND(registersound, "si");

void mapsound(char *name, int *vol, int *maxuses) 
{
    if (nosound) return ;
// intret(
    addsound(name, *vol, *maxuses < 0 ? 0 : max(1, *maxuses), mapsounds) ;
    //); 
}
//COMMAND(mapsound, "sii");

void resetchannels()
{
    if (nosound) return ;
    loopv(channels) if(channels[i].inuse) freechannel(i);
    channels.shrink(0);
}

void clear_sound()
{
    if(nosound) return;
    closemumble();
    stopmusic();
    Mix_Quit() ;
    Mix_CloseAudio();
    resetchannels();
    gamesounds.setsize(0);
    mapsounds.setsize(0);
    samples.clear();
}

void clearmapsounds()
{
    if (nosound) return ;
    loopv(channels) if(channels[i].inuse/* && channels[i].ent*/)
    {
        Mix_HaltChannel(i);
        freechannel(i);
    }
    mapsounds.setsize(0);
}

void stopmapsound(/*extentity *e*/)
{
    if (nosound) return ;
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse /*&& chan.ent == e*/)
        {
            Mix_HaltChannel(i);
            freechannel(i);
        }
    }
}

/*
    This function plays every sound that is in audible range and which 
    apparently is visible as well. 

    Disabled for now. 
*/
void checkmapsounds()
{
    /*
    const vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type!=ET_SOUND) continue;
        if(camera1->o.dist(e.o) < e.attr2)
        {
            if(!e.visible) playsound(e.attr1, NULL, &e, -1);
        }
        else if(e.visible) stopmapsound(&e);
    }
    */
}

//VAR(stereo, 0, 1, 1);
bool stereo = true ;

//VARP(maxsoundradius, 0, 340, 10000);
int maxsoundradius = 340 ;

bool updatechannel(soundchannel &chan)
{
    if(nosound) return false ;
    if(!chan.slot) return false;
    int vol = soundvol, pan = 255/2;
    if(chan.hasloc())
    {
        vec v;
        float dist = chan.loc.dist(camera.pos, v);
        int rad = maxsoundradius;
        /*
        if(chan.ent)
        {
            rad = chan.ent->attr2;
            if(chan.ent->attr3)
            {
                rad -= chan.ent->attr3;
                dist -= chan.ent->attr3;
            }
        }
        else*/ if(chan.radius > 0) rad = maxsoundradius ? min(maxsoundradius, chan.radius) : chan.radius;
        if(rad > 0) vol -= int(clamp(dist/rad, 0.0f, 1.0f)*soundvol); // simple mono distance attenuation
        if(stereo && (v.x != 0 || v.y != 0) && dist>0)
        {
            v.rotate_around_z(-camera.yaw*RAD);
            pan = int(255.9f*(0.5f - 0.5f*v.x/v.magnitude2())); // range is from 0 (left) to 255 (right)
        }
    }
    vol = (vol*MAXVOL*chan.slot->volume)/255/255;
    vol = min(vol, MAXVOL);
    if(vol == chan.volume && pan == chan.pan) return false;
    chan.volume = vol;
    chan.pan = pan;
    chan.dirty = true;
    return true;
}  

void updatesounds()
{
    if(nosound) return;
    updatemumble();
    checkmapsounds();
    int dirty = 0;
    loopv(channels)
    {
        soundchannel &chan = channels[i];
        if(chan.inuse && chan.hasloc() && updatechannel(chan)) dirty++;
    }
    if(dirty)
    {
        SDL_LockAudio(); // workaround for race conditions inside Mix_SetPanning
        loopv(channels) 
        {
            soundchannel &chan = channels[i];
            if(chan.inuse && chan.dirty) syncchannel(chan);
        }
        SDL_UnlockAudio();
    }
    if(music)
    {
        if(!Mix_PlayingMusic()) musicdone();
        else if(Mix_PausedMusic()) Mix_ResumeMusic();
    }
}

//VARP(maxsoundsatonce, 0, 5, 100);
int maxsoundsatonce = 5 ;

//VAR(dbgsound, 0, 0, 1);
bool dbgsound = false ;

static Mix_Chunk *loadwav(const char *name)
{
    printf("\n loadwav - attempting to load %s", name) ;
    Mix_Chunk *c = NULL;
    stream *z = openzipfile(name, "rb");
    if(z)
    {
        SDL_RWops *rw = z->rwops();
        if(rw) 
        {
            c = Mix_LoadWAV_RW(rw, 0);
            SDL_FreeRW(rw);
        }
        delete z;
    }
    if(!c) c = Mix_LoadWAV(findfile(name, "rb"));

    fprintf(stderr, "trying to open: %s\n", name);

    //printf("Hello! and file %s", name) ;
    
    if (!c) fprintf(stderr, "Unable to initialize audio erooor: %s\n", Mix_GetError());
    return c;
}

//int playsound(int n, const vec *loc, /*extentity *ent,*/ int loops, int fade, int chanid, int radius, int expire)
//int playsound(int n, const vec *loc = NULL, /*extentity *ent = NULL,*/ int loops = 0, int fade = 0, int chanid = -1, int radius = 0, int expire = -1)
int playsound(int n, const vec *loc, /*extentity *ent = NULL,*/ int loops, int fade, int chanid, int radius, int expire) 
{
    //if(nosound || !soundvol || nosfx) return -1;

//    vector<soundslot> &sounds = ent ? mapsounds : gamesounds;
    vector<soundslot> &sounds = gamesounds ;    // FIXME: reuse this gamesounds/mapsounds distinction
    if(!sounds.inrange(n)) { 
    //conoutf(CON_WARN, "unregistered sound: %d", n); return -1; 
    printf("unregistered sound: %d", n); return -1; 
    }
    else
    {
        printf("\nRECALC TRYING TO PLAY SOUND: %s\n", sounds[n].sample->name) ;
    }
    soundslot &slot = sounds[n];

    if(loc && (maxsoundradius || radius > 0))
    {
        // cull sounds that are unlikely to be heard
        int rad = radius > 0 ? (maxsoundradius ? min(maxsoundradius, radius) : radius) : maxsoundradius;
        if(camera.pos.dist(*loc) > 1.5f*rad)
        {
            if(channels.inrange(chanid) && channels[chanid].inuse && channels[chanid].slot == &slot)
            {
                Mix_HaltChannel(chanid);
                freechannel(chanid);
            }
            return -1;    
        }
    }
    
    if(chanid < 0)
    {
        if(slot.maxuses)
        {
            int uses = 0;
            loopv(channels) if(channels[i].inuse && channels[i].slot == &slot && ++uses >= slot.maxuses) return -1;
        }

        // avoid bursts of sounds with heavy packetloss and in sp
        static int soundsatonce = 0, lastsoundmillis = 0;

        /*FIXME: return to this logic */
// if(totalmillis == lastsoundmillis) soundsatonce++; else 
        soundsatonce = 1;

//lastsoundmillis = totalmillis;
        if(maxsoundsatonce && soundsatonce > maxsoundsatonce) return -1;
    }

    if(!slot.sample->chunk)
    {
        if(!slot.sample->name[0]) return -1;

        const char *exts[] = { "", ".wav", ".ogg", ".mp3" };
        string buf;
        loopi(sizeof(exts)/sizeof(exts[0]))
        {
            //formatstring(buf)("packages/sounds/%s%s", slot.sample->name, exts[i]);
            //formatstring(buf)("../data/sounds/%s%s", slot.sample->name, exts[i]);
            formatstring(buf)("data/sounds/%s%s", slot.sample->name, exts[i]);
            //formatstring(buf)("../data/sounds/%s%s", slot.sample->name, exts[i]);
            path(buf);
            printf("\nTRYING to load sound %s", buf) ;
            slot.sample->chunk = loadwav(buf);
            if(slot.sample->chunk) break;
        }

        if(!slot.sample->chunk) { 
            //conoutf(CON_ERROR, "failed to load sample: %s", buf); return -1; 
            printf("failed to load sample: %s", buf); return -1; 
        }
    }
    

    if(channels.inrange(chanid))
    {
        soundchannel &chan = channels[chanid];
        if(chan.inuse && chan.slot == &slot) 
        {
            if(loc) chan.loc = *loc;
            else if(chan.hasloc()) chan.clearloc();
            return chanid;
        }
    }
    if(fade < 0) return -1;
    
           
    if(dbgsound) 
    {
        // conoutf("sound: %s", slot.sample->name);
        printf("sound: %s", slot.sample->name);
    }
 printf("playsound: maxchannels == %d", maxchannels) ;
    chanid = -1;
    loopv(channels) if(!channels[i].inuse) { ;chanid = i; break; }
printf("playsound:  chanid 1 == %d", chanid) ;
    //printf("channels[i].inuse==%d", channels[i].inuse)
    if(chanid < 0 && channels.length() < maxchannels) chanid = channels.length();
printf("playsound:  chanid 2 == %d", chanid) ;
printf("playsound:  channels.length == %d", channels.length()) ;
    if(chanid < 0) loopv(channels) if(!channels[i].volume) { chanid = i; break; }    
printf("playsound:  chanid 3 == %d", chanid) ;
    if(chanid < 0) return -1;
    
    
    
    

    SDL_LockAudio(); // must lock here to prevent freechannel/Mix_SetPanning race conditions
    if(channels.inrange(chanid) && channels[chanid].inuse)
    {
        Mix_HaltChannel(chanid);
        freechannel(chanid);
    }

    soundchannel &chan = newchannel(chanid, &slot, loc, /*ent,*/ radius);
    updatechannel(chan);
    int playing = -1;
    if(fade) 
    {
        Mix_Volume(chanid, chan.volume);
        playing = expire >= 0 ? Mix_FadeInChannelTimed(chanid, slot.sample->chunk, loops, fade, expire) : Mix_FadeInChannel(chanid, slot.sample->chunk, loops, fade);
    }
    else playing = expire >= 0 ? Mix_PlayChannelTimed(chanid, slot.sample->chunk, loops, expire) : Mix_PlayChannel(chanid, slot.sample->chunk, loops);
    if(playing >= 0) syncchannel(chan); 
    else freechannel(chanid);
    SDL_UnlockAudio();
    
    printf("PLAYING THIS SOUNSD!") ;
    return playing;
}

void stopsounds()
{
    if (nosound) return ;
    loopv(channels) if(channels[i].inuse)
    {
        Mix_HaltChannel(i);
        freechannel(i);
    }
}

bool stopsound(int n, int chanid, int fade)
{
    if (nosound) return false ;
    
    if( !channels.inrange(chanid)   || 
        !channels[chanid].inuse     || 
        !gamesounds.inrange(n)      || 
        channels[chanid].slot != &gamesounds[n]) return false;

    if(dbgsound) 
    {
        //conoutf("stopsound: %s", channels[chanid].slot->sample->name);
        printf("stopsound: %s", channels[chanid].slot->sample->name);
    }

    if(!fade || !Mix_FadeOutChannel(chanid, fade))
    {
        Mix_HaltChannel(chanid);
        freechannel(chanid);
    }

    return true;
}

int playsoundname(const char *s, const vec *loc, int vol, int loops, int fade, int chanid, int radius, int expire) 
{ 
    if (nosound) return -1 ;
    if(!vol) vol = 100;
    int id = findsound(s, vol, gamesounds);
    if(id < 0) id = addsound(s, vol, 0, gamesounds);
    return playsound(id, loc, loops, fade, chanid, radius, expire);
}

void sound(int *n) { playsound(*n); }
//COMMAND(sound, "i");

void resetsound()
{
    if (nosound) return ;
    const SDL_version *v = Mix_Linked_Version();
    if(SDL_VERSIONNUM(v->major, v->minor, v->patch) <= SDL_VERSIONNUM(1, 2, 8))
    {
        //conoutf(CON_ERROR, "Sound reset not available in-game due to SDL_mixer-1.2.8 bug. Please restart for changes to take effect.");
        printf("Sound reset not available in-game due to SDL_mixer-1.2.8 bug. Please restart for changes to take effect.");
        return;
    }

//    clearchanges(CHANGE_SOUND);
    if(!nosound) 
    {
        enumerate(samples, soundsample, s, { Mix_FreeChunk(s.chunk); s.chunk = NULL; });
        if(music)
        {
            Mix_HaltMusic();
            Mix_FreeMusic(music);
        }
        if(musicstream) musicstream->seek(0, SEEK_SET);
        Mix_CloseAudio();
    }
    initsound();
    resetchannels();
    if(nosound)
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
        music = NULL;
        gamesounds.setsize(0);
        mapsounds.setsize(0);
        samples.clear();
        return;
    }
    if(music && loadmusic(musicfile))
    {
        Mix_PlayMusic(music, musicdonecmd ? 0 : -1);
        Mix_VolumeMusic((musicvol*MAXVOL)/255);
    }
    else
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
    }
}

//COMMAND(resetsound, "");

#ifdef WIN32

#include <wchar.h>

#else

#include <unistd.h>

#ifdef _POSIX_SHARED_MEMORY_OBJECTS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wchar.h>
#endif

#endif

#if defined(WIN32) || defined(_POSIX_SHARED_MEMORY_OBJECTS)
struct MumbleInfo
{
    int version, timestamp;
    vec pos, front, top;
    wchar_t name[256];
};
#endif

#ifdef WIN32


static HANDLE mumblelink = NULL;
static MumbleInfo *mumbleinfo = NULL;
#define VALID_MUMBLELINK (mumblelink && mumbleinfo)
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
static int mumblelink = -1;
static MumbleInfo *mumbleinfo = (MumbleInfo *)-1; 
#define VALID_MUMBLELINK (mumblelink >= 0 && mumbleinfo != (MumbleInfo *)-1)
#endif

#ifdef VALID_MUMBLELINK
//VARFP(mumble, 0, 1, 1, { if(mumble) initmumble(); else closemumble(); });
int mumble = 1 ;
#else
//VARFP(mumble, 0, 0, 1, { if(mumble) initmumble(); else closemumble(); });
#endif

void initmumble()
{
    if (nosound) return ;
    if(!mumble) return;
#ifdef VALID_MUMBLELINK
    if(VALID_MUMBLELINK) return;

    #ifdef WIN32
        mumblelink = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MumbleLink");
        if(mumblelink)
        {
            mumbleinfo = (MumbleInfo *)MapViewOfFile(mumblelink, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleInfo));
            if(mumbleinfo) wcsncpy(mumbleinfo->name, L"Sauerbraten", 256);
        }
    #elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
        defformatstring(shmname)("/MumbleLink.%d", getuid());
        mumblelink = shm_open(shmname, O_RDWR, 0);
        if(mumblelink >= 0)
        {
            mumbleinfo = (MumbleInfo *)mmap(NULL, sizeof(MumbleInfo), PROT_READ|PROT_WRITE, MAP_SHARED, mumblelink, 0);
            if(mumbleinfo != (MumbleInfo *)-1) wcsncpy(mumbleinfo->name, L"Sauerbraten", 256);
        }
    #endif
    if(!VALID_MUMBLELINK) closemumble();
#else
//    conoutf(CON_ERROR, "Mumble positional audio is not available on this platform.");
    printf("Mumble positional audio is not available on this platform.");
#endif
}

void closemumble()
{
    if (nosound) return ;
#ifdef WIN32
    if(mumbleinfo) { UnmapViewOfFile(mumbleinfo); mumbleinfo = NULL; }
    if(mumblelink) { CloseHandle(mumblelink); mumblelink = NULL; }
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
    if(mumbleinfo != (MumbleInfo *)-1) { munmap(mumbleinfo, sizeof(MumbleInfo)); mumbleinfo = (MumbleInfo *)-1; } 
    if(mumblelink >= 0) { close(mumblelink); mumblelink = -1; }
#endif
}

static inline vec mumblevec(const vec &v, bool pos = false)
{
    // change from X left, Z up, Y forward to X right, Y up, Z forward
    // 8 cube units = 1 meter
    vec m(-v.x, v.z, v.y);
    if(pos) m.div(8);
    return m;
}

void updatemumble()
{
    if (nosound) return ;
#ifdef VALID_MUMBLELINK
    if(!VALID_MUMBLELINK) return;

    static int timestamp = 0;

    mumbleinfo->version = 1;
    mumbleinfo->timestamp = ++timestamp;

    // FIXME: replace these positions with our stuff. 
//    mumbleinfo->pos = mumblevec(player->o, true);
    mumbleinfo->pos = mumblevec(camera.pos, true);
//    mumbleinfo->front = mumblevec(vec(RAD*player->yaw, RAD*player->pitch));
//    mumbleinfo->front = mumblevec(vec(RAD*player->yaw, RAD*player->pitch));
//    mumbleinfo->top = mumblevec(vec(RAD*player->yaw, RAD*(player->pitch+90)));
//    mumbleinfo->top = mumblevec(vec(RAD*player->yaw, RAD*(player->pitch+90)));
#endif
}


/*
    End all playing sounds and disable sound. 
*/
void soundoff() 
{
    printf("[SOUND::soundoff] called... ") ;
    
    stopsounds() ;
    stopmusic() ;
    nosound = true ;
    nomusic = true ;
    nosfx   = true ;
    
    printf("done. ") ;
}

// Stop sound effects, without stopping music. 
void musicoff() { stopmusic() ; nomusic = true ; }

// Stop sound effects, without stopping music. 
void musicon() { stopmusic() ; nomusic = false ; }

// Stop sound effects. 
void sfxoff() { nosfx = true ; }

// Enable sound effects. 
void sfxon() { nosfx = false ; }

/*
    End all playing sounds and disable sound. 
    TODO: perhaps add a way to resume whatever music was last playing. 
*/
void soundon() 
{
    nosound = false ;
    nomusic = false ;
    nosfx   = false ;
}


/*
*/
void init_sound() 
{
    printf("\n[SOUND::init_sound] called... ") ;
    if (nosound)
    {
        printf("[SOUND::init_sound] skipping because sound is set to off. ") ;
        return ;
    }

    initsound() ;
    int vol = 80 ;
    int regres = registersound("computerbeep", &vol) ;
    printf("\nSound computerbeep register gave result of %d. ", regres) ;
    if (!nosound) startmusic("cranberry-radio_edit.mp3", NULL) ;
    printf("done. ") ;
    
    printf("init_sound: channels length = %d", channels.length()) ;
    loopv(channels) {
        printf("init_sound: channel %d in use? %d", i, channels[i].inuse) ;
    }
    
}



