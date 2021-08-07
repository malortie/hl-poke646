/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef __MUSIC_H__
#define __MUSIC_H__

typedef int BOOL;

struct soundtrack_t
{
    char mapname[128];
    char musicpath[128];
    BOOL repeat;
};

#define MAX_SOUNDTRACKS 32

class Music
{
public:
    Music();
    void LoadMapSoundtracks(const char* soundtrack_file_path);

    soundtrack_t* GetSountrackByMapName(const char* mapname);
    BOOL PlayMapMusic(const char* mapname);

protected:
    soundtrack_t m_Soundtracks[MAX_SOUNDTRACKS];
    int m_NumSoundtracks;
};

extern Music g_Music;

#endif // __MUSIC_H__
