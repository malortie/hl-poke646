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

#ifndef XENSPIT_H
#define XENSPIT_H

#ifdef _WIN32
#pragma once
#endif

#define XENSPIT_MAX_PROJECTILES	4

class CXSBeam : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);

	static CXSBeam* ShootSmall(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage, CBaseEntity* pParent = NULL);
	static CXSBeam* ShootBig(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage, int iNumBeams, float flCycle = 0.0f);
	void Touch(CBaseEntity* pOther);
	void EXPORT FlySineThink(void);
	void EXPORT BigFlyThink(void);

	void SpawnTrail();

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int     m_iGlow;
	int		m_iTrail;
	float	m_flCycle;
	Vector  m_vecOldVelocity;

	EHANDLE m_hParent;

	EHANDLE m_hChildren[XENSPIT_MAX_PROJECTILES];
	int m_iChildCount;
};



#endif // XENSPIT_H