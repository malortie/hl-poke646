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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "effects.h"

// Fire Trail
class CFireTrail : public CBaseEntity
{
	void Spawn(void);
	void Think(void);
};

LINK_ENTITY_TO_CLASS(fire_trail, CFireTrail);

void CFireTrail::Spawn(void)
{
	pev->velocity = pev->angles; // vecPlaneNormal is stored in pev->angles.
	pev->velocity.x += RANDOM_FLOAT(-1, 1);
	pev->velocity.y += RANDOM_FLOAT(-1, 1);
	if (pev->velocity.z >= 0)
		pev->velocity.z += RANDOM_FLOAT(0.5f, 1.0f);
	else
		pev->velocity.z -= RANDOM_FLOAT(0.5f, 1.0f);
	pev->velocity = pev->velocity * RANDOM_FLOAT(200, 300);

	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL(edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT(0.5, 1.5); // Start scale of the fire trail.

	pev->angles = g_vecZero;
}

void CFireTrail::Think(void)
{
	CSprite* pSprite = CSprite::SpriteCreate("sprites/zerogxplode.spr", pev->origin, FALSE);
	pSprite->pev->spawnflags |= SF_SPRITE_ONCE; // Play sprite only once.
	pSprite->pev->framerate = RANDOM_FLOAT(10, 15);
	pSprite->TurnOn();
	pSprite->AnimateAndDie(pSprite->pev->framerate); // Kill the sprite after it is done playing.
	pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 192, kRenderFxNoDissipation );
	pSprite->SetScale(pev->speed);

	pev->speed -= 0.1; // Make smaller fire trail next think next think.
	if (pev->speed > 0)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove(this);

	pev->flags &= ~FL_ONGROUND;
}
