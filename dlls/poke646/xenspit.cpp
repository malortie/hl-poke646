/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"xenspit.h"

LINK_ENTITY_TO_CLASS(xs_beam, CXSBeam);

TYPEDESCRIPTION	CXSBeam::m_SaveData[] =
{
	DEFINE_FIELD(CXSBeam, m_flCycle, FIELD_FLOAT),
	DEFINE_FIELD(CXSBeam, m_vecOldVelocity, FIELD_VECTOR),
	DEFINE_FIELD(CXSBeam, m_hParent, FIELD_EHANDLE),
	DEFINE_ARRAY(CXSBeam, m_hChildren, FIELD_EHANDLE, XENSPIT_MAX_PROJECTILES),
	DEFINE_FIELD(CXSBeam, m_iChildCount, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CXSBeam, CBaseEntity);


CXSBeam* CXSBeam::ShootSmall(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage, CBaseEntity* pParent)
{
	CXSBeam* pSpit = (CXSBeam*)Create("xs_beam", vecStart, g_vecZero, ENT(pevOwner));
	if (pSpit)
	{
		pSpit->SpawnTrail();

		SET_MODEL(ENT(pSpit->pev), "sprites/hotglow.spr");
		UTIL_SetSize(pSpit->pev, Vector(0, 0, 0), Vector(0, 0, 0));
		pSpit->pev->rendermode = kRenderTransAdd;
		pSpit->pev->renderamt = 255;
		pSpit->pev->scale = 0.5;
		pSpit->pev->dmg = flDamage;

		UTIL_SetOrigin(pSpit->pev, vecStart);
		pSpit->pev->velocity = vecVelocity;

		pSpit->m_vecOldVelocity = vecVelocity;
		pSpit->m_hParent = pParent;

		pSpit->SetThink(&CXSBeam::FlySineThink);
		pSpit->pev->nextthink = gpGlobals->time + 0.01;
	}

	return pSpit;
}

CXSBeam* CXSBeam::ShootBig(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage, int iNumBeams, float flCycle)
{
	CXSBeam* pSpit = (CXSBeam*)Create("xs_beam", vecStart, g_vecZero, ENT(pevOwner));
	if (pSpit)
	{
		SET_MODEL(ENT(pSpit->pev), "sprites/glow02.spr");
		UTIL_SetSize(pSpit->pev, Vector(0, 0, 0), Vector(0, 0, 0));
		pSpit->pev->rendermode = kRenderTransAdd;
		pSpit->pev->renderamt = 0;
		pSpit->pev->scale = 0.1;
		pSpit->pev->dmg = flDamage;

		UTIL_SetOrigin(pSpit->pev, vecStart);
		pSpit->pev->velocity = vecVelocity;

		pSpit->m_flCycle = flCycle;
		pSpit->m_vecOldVelocity = vecVelocity;
		pSpit->m_iChildCount = std::min(iNumBeams, XENSPIT_MAX_PROJECTILES);

		// Initialize children here...
		for (int i = 0; i < pSpit->m_iChildCount; i++)
		{
			CBaseEntity* pSmallSpit = CXSBeam::ShootSmall(pevOwner, pSpit->pev->origin, vecVelocity, 0, pSpit);
			if (pSmallSpit)
			{
				pSmallSpit->pev->scale = 0.25f;
				pSmallSpit->SetThink(NULL); // Do not think while part of a bigger spit.

				pSpit->m_hChildren[i] = pSmallSpit;
			}
		}

		pSpit->SetThink(&CXSBeam::BigFlyThink);
		pSpit->pev->nextthink = gpGlobals->time + 0.01;
	}

	return pSpit;
}

void CXSBeam::SpawnTrail(void)
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );		// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 4 );  // width
		WRITE_BYTE( 161 );   // r, g, b
		WRITE_BYTE( 188 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 128 );	// brightness
	MESSAGE_END();
}

void CXSBeam::Spawn(void)
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->frame = 0;
	pev->gravity = 0;
	pev->dmg = gSkillData.plrDmgGauss;

	// Initialize variables.
	m_flCycle = 0.0f;
	m_vecOldVelocity = g_vecZero;
	m_hParent = NULL;
	m_iChildCount = 0;
	for (int i = 0; i < XENSPIT_MAX_PROJECTILES; ++i)
		m_hChildren[i] = NULL;
}

void CXSBeam::Precache()
{
	PRECACHE_MODEL("sprites/glow02.spr");
	m_iGlow = PRECACHE_MODEL("sprites/hotglow.spr");

	m_iTrail = PRECACHE_MODEL("sprites/laserbeam.spr");
}

void CXSBeam::Touch(CBaseEntity *pOther)
{
	// Do not collide if this is a child beam.
	if (m_hParent != NULL)
		return;

	// Do not collide with another beam.
	if (FClassnameIs(pOther->pev, STRING(pev->classname)))
		return;

	// ALERT(at_console, "CXSBeam::Touch START\n");

	TraceResult tr;

	RadiusDamage(pev->origin, pev, pev, pev->dmg, std::max( 100.0f, pev->dmg ), CLASS_NONE, DMG_POISON);

	if (pOther->IsBSPModel())
	{
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH3);

		if (!pOther->pev->takedamage)
		{
			// entry wall glow
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_GLOWSPRITE );
				WRITE_COORD( pev->origin.x );	// pos
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( m_iGlow );		// model
				WRITE_BYTE( 5 );				// life * 10
				WRITE_BYTE( 5 );				// size * 10
				WRITE_BYTE( 200 );			// brightness
			MESSAGE_END();
		}
	}

	// light.
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE(std::max(8, static_cast<int>(pev->dmg / 10.0f)));		// radius * 0.1
		WRITE_BYTE(161);	// r
		WRITE_BYTE(188);	// g
		WRITE_BYTE(0);		// b
		WRITE_BYTE(50);	// time * 10
		WRITE_BYTE(4);		// decay * 0.1
	MESSAGE_END();

	// Remove all children.
	for (int i = 0; i < m_iChildCount; i++)
	{
		if (m_hChildren[i])
		{
			UTIL_Remove(m_hChildren[i]);
			m_hChildren[i] = NULL;
		}
	}
	m_iChildCount = 0;

	SetThink(&CXSBeam::SUB_Remove);
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time;

	// ALERT(at_console, "CXSBeam::Touch END\n");
}

void VectorToVectors(Vector forward, Vector& right, Vector& up)
{
	forward = forward.Normalize();

	Vector horizontal = Vector(forward.x, forward.y, 0).Normalize();

	Vector cross;
	float flDot = DotProduct(forward, horizontal);
	if (flDot < 0.5)
		cross = Vector(1, 0, 0);
	else
		cross = Vector(0, 0, 1);

	right = CrossProduct(forward, cross);
	up = CrossProduct(right, forward);
}

inline float UTIL_NormalizeAnglePI(float flAngle)
{
	// Clamp between 0 and 2 PI.
	if (flAngle > 2 * M_PI)
		flAngle -= 2 * M_PI;
	else if (flAngle < -2 * M_PI)
		flAngle += 2 * M_PI;

	return flAngle;
}

void CXSBeam::FlySineThink(void)
{
	pev->nextthink = gpGlobals->time + 0.01f;

	m_flCycle += 0.2f;
	m_flCycle = UTIL_NormalizeAnglePI(m_flCycle);

	Vector right, up;
	VectorToVectors(pev->velocity.Normalize(), right, up);

	pev->velocity = m_vecOldVelocity + up * std::cos(m_flCycle) * 64;
}

void CXSBeam::BigFlyThink(void)
{
	pev->nextthink = gpGlobals->time + 0.05f;

	Vector src, forward, right, up;

	forward = pev->velocity.Normalize();

	VectorToVectors(forward, right, up);

	for (int i = 0; i < m_iChildCount; i++)
	{
		CBaseEntity* pSpit = m_hChildren[i];
		if (pSpit)
		{
			float cs, sn, dist;
			cs = std::cos(m_flCycle);
			sn = std::sin(m_flCycle);
			dist = 2.8f * m_iChildCount;

			// ALERT(at_console, "cs: %.2f. sn: %.2f\n", cs, sn);

			Vector target = (right * cs * dist) + (up * sn * dist);
			pSpit->pev->origin = pev->origin + target;
		}

		// Increment cycle to position the next children.
		m_flCycle += (2 * M_PI) / m_iChildCount;
		m_flCycle = UTIL_NormalizeAnglePI(m_flCycle);
	}

	// Increment cycle globally.
	m_flCycle += M_PI / 4.0f;
	m_flCycle = UTIL_NormalizeAnglePI(m_flCycle);
}

