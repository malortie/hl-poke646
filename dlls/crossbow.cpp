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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "shake.h"
#include "gamerules.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrossbowBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

	int m_iTrail;

public:
	static CCrossbowBolt *BoltCreate( void );
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

CCrossbowBolt *CCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( &CCrossbowBolt::BoltTouch );
	SetThink( &CCrossbowBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2;
}


void CCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		if ( pOther->IsPlayer() )
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB ); 
		}
		else
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 
		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		if ( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_NEVER );
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( &CCrossbowBolt::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
		SetThink( &CCrossbowBolt::ExplodeThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CCrossbowBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

void CCrossbowBolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);		
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}
#endif

#define DRAWBACK_SEQUENCE_DURATION (41.0f / 30.0f)
#define UNDRAW_SEQUENCE_DURATION (43.0f / 30.0f)

extern int gmsgScope;

enum crossbow_e {
	CROSSBOW_IDLE1 = 0,	// drawn
	CROSSBOW_IDLE2,		// undrawn
	CROSSBOW_FIDGET1,	// drawn
	CROSSBOW_FIDGET2,	// undrawn
	CROSSBOW_FIRE1,
	CROSSBOW_RELOAD,	// drawn
	CROSSBOW_DRAWBACK,
	CROSSBOW_UNDRAW,
	CROSSBOW_DRAW1,		// drawn
	CROSSBOW_DRAW2,		// undrawn
	CROSSBOW_HOLSTER1,	// drawn
	CROSSBOW_HOLSTER2,	// undrawn
};

LINK_ENTITY_TO_CLASS( weapon_cmlwbr, CCrossbow );

void CCrossbow::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CROSSBOW;
	SET_MODEL(ENT(pev), "models/w_crossbow.mdl");

	m_iDefaultAmmo = CROSSBOW_DEFAULT_GIVE;

	SetDrawn( FALSE ); // Start undrawn.

	m_fInSpecialReload = 0;

	FallInit();// get ready to fall down.
}

int CCrossbow::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CCrossbow::Precache( void )
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_cmlwbr.mdl");
	PRECACHE_MODEL("models/p_cmlwbr.mdl");

	PRECACHE_SOUND("weapons/cmlwbr_drawback.wav");
	PRECACHE_SOUND("weapons/cmlwbr_fire.wav");
	PRECACHE_SOUND("weapons/cmlwbr_reload.wav");
	PRECACHE_SOUND("weapons/cmlwbr_undraw.wav");
	PRECACHE_SOUND("weapons/cmlwbr_zoom.wav");

	UTIL_PrecacheOther( "crossbow_bolt" );

	m_usCrossbow = PRECACHE_EVENT( 1, "events/crossbow1.sc" );
	m_usCrossbow2 = PRECACHE_EVENT( 1, "events/crossbow2.sc" );
}


int CCrossbow::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CROSSBOW_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = WEAPON_CROSSBOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CCrossbow::Deploy( )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 18.0f / 30.0f;

	if (IsDrawn())
		return DefaultDeploy( "models/v_cmlwbr.mdl", "models/p_cmlwbr.mdl", CROSSBOW_DRAW1, "bow" );
	return DefaultDeploy( "models/v_cmlwbr.mdl", "models/p_cmlwbr.mdl", CROSSBOW_DRAW2, "bow" );
}

void CCrossbow::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_fInSpecialReload = 0; // cancel pending reload.

	if ( m_fInZoom )
	{
		ZoomOut( );
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (IsDrawn())
		SendWeaponAnim( CROSSBOW_HOLSTER1 );
	else
		SendWeaponAnim( CROSSBOW_HOLSTER2 );

	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_reload.wav");
}

void CCrossbow::PrimaryAttack( void )
{
	FireBolt();
}

// this function only gets called in multiplayer
void CCrossbow::FireSniperBolt()
{
	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);

	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	TraceResult tr;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr);

#ifndef CLIENT_DLL
	if ( tr.pHit->v.takedamage )
	{
		ClearMultiDamage( );
		CBaseEntity::Instance(tr.pHit)->TraceAttack(m_pPlayer->pev, 120, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB ); 
		ApplyMultiDamage( pev, m_pPlayer->pev );
	}
#endif
}

void CCrossbow::FireBolt()
{
	TraceResult tr;

	if ( !IsDrawn() )
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.25f);
		return;
	}

	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	
	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir	 = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();
	pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
	pBolt->pev->speed = BOLT_AIR_VELOCITY;
	pBolt->pev->avelocity.z = 10;
#endif

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase(); // Allow to draw back immediately.
	m_flNextPrimaryAttack = GetNextAttackDelay(0.5f);

	SetDrawn(FALSE);
}


void CCrossbow::SecondaryAttack()
{
	ToggleZoom();

	if (m_pPlayer->pev->fov == 0)
		m_flNextSecondaryAttack = GetNextAttackDelay(0.7f);
	else
		m_flNextSecondaryAttack = GetNextAttackDelay(0.5f);
}


void CCrossbow::Reload( void )
{
	// Already full.
	if (m_iClip == CROSSBOW_MAX_CLIP)
		return;

	if ( m_pPlayer->ammo_bolts <= 0 )
		return;

	if ( m_fInZoom )
	{
		ZoomOut();
	}
	
	// Do not allow reload if the bolt is drawn.
	if (IsDrawn())
	{
		Undraw();

		// Tell there is a pending reload.
		m_fInSpecialReload = 1;
		// Wait until the undraw sequence has fully completed before starting reload.
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + UNDRAW_SEQUENCE_DURATION;
		return;
	}

	BOOL bResult = DefaultReload( CROSSBOW_MAX_CLIP, CROSSBOW_RELOAD, 141.0f / 30.0f );
	if ( bResult )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_reload.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));
	}
}


void CCrossbow::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if (m_fInSpecialReload != 0)
		{
			m_fInSpecialReload = 0;
			Reload();
		}
		else if (m_iClip != 0 && !IsDrawn())
		{
			DrawBack();
		}
		else 
		{
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.75)
			{
				if ( IsDrawn() )
				{
					SendWeaponAnim( CROSSBOW_IDLE1 );
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 93.0f / 30.0f;
				}
				else
				{
					SendWeaponAnim( CROSSBOW_IDLE2 );
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 93.0f / 30.0f;
				}
			}
			else
			{
				if ( IsDrawn() )
				{
					SendWeaponAnim( CROSSBOW_FIDGET1 );
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 83.0f / 30.0f;
				}
				else
				{
					SendWeaponAnim( CROSSBOW_FIDGET2 );
					m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 83.0f / 30.0f;
				}
			}
		}
	}
}

BOOL CCrossbow::ShouldWeaponIdle(void)
{
	// Idle if reload is pending.
	return m_fInSpecialReload != 0;
}

BOOL CCrossbow::IsDrawn(void)
{
	return (m_fInAttack != 0);
}

void CCrossbow::SetDrawn(BOOL bDrawn)
{
	m_fInAttack = (bDrawn) ? TRUE : FALSE;
}

void CCrossbow::ToggleZoom(void)
{
	if (m_pPlayer->pev->fov == 0)
	{
		ZoomIn(42);
	}
	else if (m_pPlayer->pev->fov == 42)
	{
		ZoomIn(22);
	}
	else
	{
		ZoomOut();
	}
}

void CCrossbow::ZoomIn(int iFOV)
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = iFOV;

	m_fInZoom = TRUE;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_AUTO, "weapons/cmlwbr_zoom.wav", VOL_NORM, ATTN_NORM);

#ifndef CLIENT_DLL
	MESSAGE_BEGIN(MSG_ONE, gmsgScope, NULL, m_pPlayer->pev);
		WRITE_BYTE(1);
	MESSAGE_END();

	UTIL_ScreenFade(m_pPlayer, Vector(0, 0, 0), 0.1, 0.1, 255, FFADE_IN);
#endif
}

void CCrossbow::ZoomOut(void)
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;

	m_fInZoom = FALSE;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_AUTO, "weapons/cmlwbr_zoom.wav", VOL_NORM, ATTN_NORM);

#ifndef CLIENT_DLL
	MESSAGE_BEGIN(MSG_ONE, gmsgScope, NULL, m_pPlayer->pev);
		WRITE_BYTE(0);
	MESSAGE_END();

	UTIL_ScreenFade(m_pPlayer, Vector(0, 0, 0), 0.1, 0.1, 255, FFADE_IN);
#endif
}

void CCrossbow::DrawBack(void)
{
	SendWeaponAnim(CROSSBOW_DRAWBACK);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_AUTO, "weapons/cmlwbr_drawback.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	SetDrawn(TRUE);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + DRAWBACK_SEQUENCE_DURATION;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(DRAWBACK_SEQUENCE_DURATION);
}

void CCrossbow::Undraw()
{
	SendWeaponAnim(CROSSBOW_UNDRAW);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_AUTO, "weapons/cmlwbr_undraw.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	SetDrawn(FALSE);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UNDRAW_SEQUENCE_DURATION;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(UNDRAW_SEQUENCE_DURATION);
}

class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_CROSSBOWCLIP_GIVE, "bolts", BOLT_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_bolts, CCrossbowAmmo );

#endif
