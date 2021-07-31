/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"explode.h"
#include	"func_break.h"

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )

int gRobocopGibModel = 0;

#define ROBOCOP_EYE_SPRITE_NAME			"sprites/gargeye1.spr"
#define ROBOCOP_EYE_BEAM_NAME			"sprites/laserbeam.spr"
#define ROBOCOP_EYE_SPOT_NAME			"sprites/glow02.spr"
#define ROBOCOP_EYE_MAX_ALPHA			255
#define ROBOCOP_EYE_FREQUENCY_IDLE		18
#define ROBOCOP_EYE_FREQUENCY_WARMUP	60

#define ROBOCOP_MAX_SHOCKWAVE_RADIUS	384
#define ROBOCOP_MAX_MORTAR_RADIUS		256

#define ROBOCOP_MORTAR_CHARGE_TIME		2.0f

#define ROBOCOP_MORTAR_ATTACK_DELAY		1.2f

#define ROBOCOP_MELEE_ATTACK_DIST		128
#define ROBOCOP_RANGE_ATTACK_DIST		512

#define ROBOCOP_RANGE_ATTACK_FOV		0.5f
#define ROBOCOP_MELEE_IMPULSE			175.0f

#define ROBOCOP_DEATH_DURATION			5.0f

#define ROBOCOP_GIB_MODEL				"models/metalplategibs.mdl"
#define ROBOCOP_HEAD_ATTACHMENT			0
#define ROBOCOP_HEAD_YAW_RANGE			60
#define ROBOCOP_HEAD_PITCH_RANGE		25

// Robocop is immune to any damage but this
#define ROBOCOP_DAMAGE					(DMG_POISON|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ROBOCOP_AE_RIGHT_FOOT		0x03
#define ROBOCOP_AE_LEFT_FOOT		0x04
#define ROBOCOP_AE_FIST				0x05


class CRoboCop : public CBaseMonster
{
public:

	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	int  ISoundMask(void);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector(-40, -40, 0);
		pev->absmax = pev->origin + Vector(40, 40, 200);
	}

	void MonsterThink(void);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }

	void PrescheduleThink(void);
	void ScheduleChange(void);
	BOOL ShouldGibMonster(int iGib) { return FALSE; }
	void Killed(entvars_t *pevAttacker, int iGib);

	Schedule_t *GetScheduleOfType(int Type);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	CUSTOM_SCHEDULES;

	void CreateEffects(void);
	void DestroyEffects(void);

	void EyeOff(void);
	void EyeOn(int level);
	void EyeUpdate(float speed);
	void SetEyeAlpha(int alpha);

	void BeamOff(void);
	void BeamOn(int level);
	void BeamUpdate(float speed);

	void SonicAttack(void);
	void MortarAttack(Vector vecSrc);

	void StartMortarAttack(void);
	void StopMortarAttack(void);

	void FinishRangeAttack();
	void HeadLookAt(const Vector& position);
	void ResetHeadBoneControllers();

	void TraceDeathPosition(TraceResult& tr);

	int			m_iSpriteTexture;
	int			m_iMortarBeamTexture;

	EHANDLE		m_hEyeGlow;			// Glow around the eyes
	int			m_eyeBrightness;	// Brightness target

	EHANDLE		m_hBeam;
	EHANDLE		m_hBeamSpot;
	int			m_beamBrightness;

	float		m_flMortarAttackStart;
	BOOL		m_bAimLocked;
	Vector		m_vecMortarPos;
	float		m_flNextMortarAttack;
	BOOL		m_fMortarAttackEvent;

	float		m_flNextSparkTime;
	float		m_flEyeFrequency;

	static const char* pSparkSounds[];
	static const char* pStepSounds[];
};

LINK_ENTITY_TO_CLASS(monster_robocop, CRoboCop);

TYPEDESCRIPTION	CRoboCop::m_SaveData[] =
{
	DEFINE_FIELD(CRoboCop, m_hEyeGlow, FIELD_EHANDLE),
	DEFINE_FIELD(CRoboCop, m_eyeBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_hBeam, FIELD_EHANDLE),
	DEFINE_FIELD(CRoboCop, m_hBeamSpot, FIELD_EHANDLE),
	DEFINE_FIELD(CRoboCop, m_beamBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CRoboCop, m_flMortarAttackStart, FIELD_TIME),
	DEFINE_FIELD(CRoboCop, m_bAimLocked, FIELD_BOOLEAN),
	DEFINE_FIELD(CRoboCop, m_vecMortarPos, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CRoboCop, m_flNextMortarAttack, FIELD_TIME),
	DEFINE_FIELD(CRoboCop, m_fMortarAttackEvent, FIELD_BOOLEAN),
	DEFINE_FIELD(CRoboCop, m_flNextSparkTime, FIELD_TIME),
	DEFINE_FIELD(CRoboCop, m_flEyeFrequency, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRoboCop, CBaseMonster);

const char* CRoboCop::pSparkSounds[] =
{
	"buttons/spark1.wav",
	"buttons/spark2.wav",
	"buttons/spark3.wav",
	"buttons/spark4.wav",
	"buttons/spark5.wav",
	"buttons/spark6.wav",
};

const char* CRoboCop::pStepSounds[] =
{
	"robocop/rc_step1.wav",
	"robocop/rc_step2.wav"
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlRoboCopRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
};

Schedule_t	slRoboCopRangeAttack1[] =
{
	{
		tlRoboCopRangeAttack1,
		ARRAYSIZE(tlRoboCopRangeAttack1),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"RoboCopRangeAttack1"
	},
};

DEFINE_CUSTOM_SCHEDULES(CRoboCop)
{
	slRoboCopRangeAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES(CRoboCop, CBaseMonster);

//=========================================================
// ISoundMask
//=========================================================
int CRoboCop::ISoundMask(void)
{
	return 0;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRoboCop::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoboCop::SetYawSpeed(void)
{
	int ys;

	ys = 70;

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRoboCop::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ROBOCOP_AE_RIGHT_FOOT:
	case ROBOCOP_AE_LEFT_FOOT:
		UTIL_ScreenShake(pev->origin, RANDOM_FLOAT(2.0f, 4.0f), 3.0, 1.0, 300);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, RANDOM_SOUND_ARRAY(pStepSounds), 1, ATTN_NORM, 0, 70);
		break;

	case ROBOCOP_AE_FIST:
		SonicAttack();
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CRoboCop::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( !IsAlive() )
	{
		CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
		return;
	}

	bitsDamageType &= ROBOCOP_DAMAGE;

	if ( bitsDamageType == 0)
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(0.5,1.5) );
			pev->dmgtime = gpGlobals->time;
		}
		flDamage = 0;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CRoboCop::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( IsAlive() )
	{
		if ( !(bitsDamageType & ROBOCOP_DAMAGE) )
			flDamage *= 0.01;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Spawn
//=========================================================
void CRoboCop::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/robocop.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 128));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->health = gSkillData.robocopHealth;
	pev->view_ofs = Vector(0, 0, 128);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD;

	CreateEffects();
	SetEyeAlpha(ROBOCOP_EYE_MAX_ALPHA);
	EyeOn(ROBOCOP_EYE_MAX_ALPHA);
	BeamOff();

	m_bAimLocked = FALSE;
	m_fMortarAttackEvent = FALSE;
	m_flMortarAttackStart = 0;
	m_flNextMortarAttack = gpGlobals->time;
	m_vecMortarPos = Vector(0, 0, 0);

	MonsterInit();

	m_flNextSparkTime = 0;
	m_flEyeFrequency = ROBOCOP_EYE_FREQUENCY_IDLE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoboCop::Precache()
{
	PRECACHE_MODEL("models/robocop.mdl");

	PRECACHE_SOUND("robocop/rc_charge.wav");
	PRECACHE_SOUND("robocop/rc_fist.wav");
	PRECACHE_SOUND("robocop/rc_laser.wav");

	PRECACHE_SOUND_ARRAY(pSparkSounds);
	PRECACHE_SOUND_ARRAY(pStepSounds);

	PRECACHE_MODEL(ROBOCOP_EYE_SPRITE_NAME);
	PRECACHE_MODEL(ROBOCOP_EYE_BEAM_NAME);
	PRECACHE_MODEL(ROBOCOP_EYE_SPOT_NAME);

	m_iSpriteTexture = PRECACHE_MODEL("sprites/xbeam3.spr");
	m_iMortarBeamTexture = PRECACHE_MODEL("sprites/lgtning.spr");
	gRobocopGibModel = PRECACHE_MODEL(ROBOCOP_GIB_MODEL);

	UTIL_PrecacheOther("spark_shower");
	UTIL_PrecacheOther("fire_trail");
}

//=========================================================
// Monster Think - calls out to core AI functions and handles this
// monster's specific animation events
//=========================================================
void CRoboCop::MonsterThink(void)
{
	CBaseMonster::MonsterThink();

	// Override ground speed.
	if (m_Activity == ACT_WALK)
		m_flGroundSpeed = 200;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CRoboCop::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= gSkillData.robocopSWRadius &&
		flDot >= 0.3 &&
		!HasConditions(bits_COND_ENEMY_OCCLUDED))
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CRoboCop::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextMortarAttack > gpGlobals->time)
		return FALSE;

	if (flDist > gSkillData.robocopSWRadius &&
		flDot >= ROBOCOP_RANGE_ATTACK_FOV &&
		!HasConditions(bits_COND_ENEMY_OCCLUDED) && 
		!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ScheduleChange
//=========================================================
void CRoboCop::ScheduleChange(void)
{
	StopMortarAttack();

	CBaseMonster::ScheduleChange();
}

void CRoboCop::Killed(entvars_t *pevAttacker, int iGib)
{
	DestroyEffects();

	FCheckAITrigger();

	CBaseMonster::Killed(pevAttacker, GIB_NEVER);
}

void CRoboCop::PrescheduleThink(void)
{
	CBaseMonster::PrescheduleThink();

	EyeUpdate(m_flEyeFrequency);

	BeamUpdate(255);
}

Schedule_t *CRoboCop::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return slRoboCopRangeAttack1;

	default:
		return CBaseMonster::GetScheduleOfType(Type);
	}
}

void CRoboCop::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		StartMortarAttack();
		CBaseMonster::StartTask(pTask);
		break;

	case TASK_DIE:
		DestroyEffects(); // Destroy effects.

		m_flWaitFinished = gpGlobals->time + ROBOCOP_DEATH_DURATION;
		m_flNextSparkTime = gpGlobals->time + RANDOM_FLOAT(0, 0.5f);
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector(64, 64, 255);
		CBaseMonster::StartTask(pTask);
		break;
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void CRoboCop::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		if (!m_hEnemy)
		{
			FinishRangeAttack();
			TaskComplete();
			return;
		}

		if (m_fSequenceFinished)
		{
			FinishRangeAttack();
			TaskComplete();
		}
		else
		{
			if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
			{
				TraceResult tr;
				UTIL_TraceLine(m_hEnemy->Center(), m_hEnemy->Center() - Vector(0, 0, 2048), ignore_monsters, ENT(pev), &tr);

				int contents = UTIL_PointContents(tr.vecEndPos);

				// Enemy will hurt itself, stop task and return.
				if (contents == CONTENTS_SKY || contents == CONTENTS_LAVA || contents == CONTENTS_SLIME)
				{
					FinishRangeAttack();
					TaskComplete();
					return;
				}
			}

			float elapsedTime = gpGlobals->time - m_flMortarAttackStart;

			if (elapsedTime > ROBOCOP_MORTAR_CHARGE_TIME)
			{
				if (!m_bAimLocked)
				{
					// Do not calculate the aim position more than once.
					m_bAimLocked = TRUE;

					TraceResult tr;

					Vector vecSrc;
					if (m_hEnemy->IsPlayer())
						vecSrc = m_hEnemy->pev->origin - Vector(0, 0, 35); // Player's feet.
					else
						vecSrc = m_hEnemy->pev->origin + Vector(0, 0, 1);
					Vector vecPredictedPos = vecSrc + m_hEnemy->pev->velocity * ROBOCOP_MORTAR_ATTACK_DELAY;

					// Trace where the enemy is expected to be soon.
					UTIL_TraceLine(vecSrc, vecPredictedPos, ignore_monsters, ENT(pev), &tr);

					// Trace to the ground position of the expected enemy position.
					UTIL_TraceLine(tr.vecEndPos, tr.vecEndPos - Vector(0, 0, 2048), ignore_monsters, ENT(pev), &tr);

					// Trace from robocop eye to ground position.
					Vector vecEyePos, vecAngles;
					GetAttachment(ROBOCOP_HEAD_ATTACHMENT, vecEyePos, vecAngles);
					UTIL_TraceLine(vecEyePos, tr.vecEndPos, ignore_monsters, ENT(pev), &tr);

					// Check if ground position is occluded.
					if (tr.flFraction < 1.0)
					{
						FinishRangeAttack();
						TaskComplete();
						return;
					}

					// Check if the position we are aiming at is within the max head angle.
					UTIL_MakeVectors(pev->angles);
					Vector2D vec2LOS = (tr.vecEndPos - vecEyePos).Make2D();
					vec2LOS = vec2LOS.Normalize();
					float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

					if (flDot < ROBOCOP_RANGE_ATTACK_FOV)
					{
						FinishRangeAttack();
						TaskComplete();
						return;
					}

					// Set aim position.
					m_vecMortarPos = tr.vecEndPos;

					// Play laser sound.
					EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "robocop/rc_laser.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

					BeamOn(160);

					// Update positions of laser beam and spot.
					if (m_hBeam)
					{
						CBeam* pBeam = (CBeam*)((CBaseEntity*)m_hBeam);
						if (pBeam)
						{
							pBeam->SetEndAttachment(1);
							pBeam->SetStartPos(m_vecMortarPos);
						}
					}

					if (m_hBeamSpot)
					{
						m_hBeamSpot->pev->origin = m_vecMortarPos;
					}

					// Look at aim position.
					HeadLookAt(m_vecMortarPos);

					// Set eye full and stop ocilliation.
					SetEyeAlpha(ROBOCOP_EYE_MAX_ALPHA);
					m_flEyeFrequency = 0;
				}
				else 
				{
					if (elapsedTime > (ROBOCOP_MORTAR_CHARGE_TIME + ROBOCOP_MORTAR_ATTACK_DELAY) && !m_fMortarAttackEvent)
					{
						// Spawn the explosion.
						MortarAttack(m_vecMortarPos);
						m_fMortarAttackEvent = TRUE;
					}
				}
			}
			else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
			{
				// Allow melee attack condition to cancel range attack as long as aiming is not locked.
				FinishRangeAttack();
				TaskComplete();
			}
		}
	}
	break;

	case TASK_DIE:

		if (m_flWaitFinished > gpGlobals->time)
		{
			if (gpGlobals->time > m_flNextSparkTime)
			{
				// Spark shower.
				TraceResult tr;
				TraceDeathPosition(tr);
				Create("spark_shower", tr.vecEndPos, tr.vecPlaneNormal, NULL);

				EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, RANDOM_SOUND_ARRAY(pSparkSounds), 1.0, 0.6, 0, RANDOM_LONG(95, 105));

				m_flNextSparkTime = gpGlobals->time + RANDOM_FLOAT(0.3f, 0.5f);
			}
		}
		else
		{
			// No head.
			SetBodygroup(0, 1);

			// Stop glowing.
			pev->renderfx = kRenderFxNone;
			pev->rendercolor.x =
			pev->rendercolor.y =
			pev->rendercolor.z = 255;

			m_flNextSparkTime = 0;


			TraceResult tr;
			TraceDeathPosition(tr);
			Vector vecSrc = tr.vecEndPos;

			// Explosion
			MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
				WRITE_BYTE( TE_EXPLOSION );
				WRITE_COORD(vecSrc.x);
				WRITE_COORD(vecSrc.y);
				WRITE_COORD(vecSrc.z);
				WRITE_SHORT(g_sModelIndexFireball);
				WRITE_BYTE(50); // scale * 10
				WRITE_BYTE(15); // framerate
				WRITE_BYTE(TE_EXPLFLAG_NONE | TE_EXPLFLAG_NODLIGHTS);
			MESSAGE_END();

			// fire trail effect.
			Vector vecEyePos, vecAngles;
			GetAttachment(ROBOCOP_HEAD_ATTACHMENT, vecEyePos, vecAngles);

			for (int i = 0; i < RANDOM_LONG(2,3); i++)
				Create("fire_trail", vecEyePos, tr.vecPlaneNormal, NULL);

			// Throw metal gibs.
			int parts = MODEL_FRAMES( gRobocopGibModel );
			for (int i = 0; i < 10; i++ )
			{
				CGib *pGib = GetClassPtr( (CGib *)NULL );

				pGib->Spawn( ROBOCOP_GIB_MODEL );
				
				int bodyPart = 0;
				if ( parts > 1 )
					bodyPart = RANDOM_LONG( 0, parts -1 );

				pGib->pev->body = bodyPart;
				pGib->m_bloodColor = DONT_BLEED;
				pGib->m_material = matNone;
				pGib->pev->origin = vecSrc;
				pGib->pev->velocity = UTIL_RandomBloodVector() * RANDOM_FLOAT( 800, 1000 );
				pGib->pev->nextthink = gpGlobals->time + 1.25;
				pGib->SetThink( &CGib::SUB_FadeOut );
			}

			CBaseMonster::RunTask(pTask);
		}
		break;
	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}


//=========================================================
// SonicAttack
//=========================================================
void CRoboCop::SonicAttack(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "robocop/rc_fist.wav", 1, ATTN_NORM);

	UTIL_MakeVectors(pev->angles);

	// Position shockwave attack where fist is.
	Vector vecSrc = pev->origin + Vector(0, 0, 1);
	vecSrc = vecSrc + gpGlobals->v_forward * 100;
	vecSrc = vecSrc + gpGlobals->v_right * 25;

	// blast circles
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16 + gSkillData.robocopSWRadius / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(2); // life
	WRITE_BYTE(32);  // width
	WRITE_BYTE(0);   // noise
	WRITE_BYTE(101);  // r
	WRITE_BYTE(133);  // g
	WRITE_BYTE(221); // b
	WRITE_BYTE(255); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16 + (gSkillData.robocopSWRadius / 2) / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(3); // life
	WRITE_BYTE(32);  // width
	WRITE_BYTE(0);   // noise
	WRITE_BYTE(67);  // r
	WRITE_BYTE(85);  // g
	WRITE_BYTE(255); // b
	WRITE_BYTE(255); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16 + (gSkillData.robocopSWRadius / 3) / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(4); // life
	WRITE_BYTE(32);  // width
	WRITE_BYTE(0);   // noise
	WRITE_BYTE(62);  // r
	WRITE_BYTE(33);  // g
	WRITE_BYTE(211); // b
	WRITE_BYTE(255); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();

	// Shake the screen.
	UTIL_ScreenShake(vecSrc, 12.0, 100.0, 2.0, 1000);

	// Do not try to apply damage if shockwave radius is null.
	if (gSkillData.robocopSWRadius == 0)
		return;

	::RadiusDamage(vecSrc, pev, pev, gSkillData.robocopDmgFist, gSkillData.robocopSWRadius, Classify(), DMG_SONIC | DMG_ALWAYSGIB);

	CBaseEntity* pPlayer = UTIL_FindEntityByClassname(NULL, "player");
	if (pPlayer)
	{
		float flDist = (pPlayer->Center() - vecSrc).Length();
		if (flDist <= gSkillData.robocopSWRadius)
		{
			// Apply impulse to the player if close enough.
			float flImpulse = ROBOCOP_MELEE_IMPULSE * (gSkillData.robocopSWRadius - flDist) / gSkillData.robocopSWRadius;

			pPlayer->pev->velocity = pPlayer->pev->velocity + (pPlayer->Center() - vecSrc).Normalize() * 2.5 * flImpulse;
			pPlayer->pev->velocity = pPlayer->pev->velocity + gpGlobals->v_up * flImpulse;
		}
	}
}


//=========================================================
// SonicAttack
//=========================================================
void CRoboCop::MortarAttack(Vector vecSrc)
{
	// mortar beam
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z + 1024);
		WRITE_SHORT(m_iMortarBeamTexture);
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(1); // life
		WRITE_BYTE(40);  // width
		WRITE_BYTE(0);   // noise
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(160);   // r, g, b
		WRITE_BYTE(100);   // r, g, b
		WRITE_BYTE(128);	// brightness
		WRITE_BYTE(0);		// speed
	MESSAGE_END();

	// blast circles
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z + 16 + ROBOCOP_MAX_MORTAR_RADIUS / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(2); // life
	WRITE_BYTE(32);  // width
	WRITE_BYTE(0);   // noise
	WRITE_BYTE(255);   // r
	WRITE_BYTE(160);  // g
	WRITE_BYTE(100);   // b
	WRITE_BYTE(255); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();

	// Explosion
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(vecSrc.x);	// Send to PAS because of the sound
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 50 ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NONE | TE_EXPLFLAG_NODLIGHTS);
	MESSAGE_END();

	RadiusDamage(vecSrc, pev, pev, gSkillData.robocopDmgMortar, Classify(), DMG_BLAST | DMG_MORTAR);

	UTIL_ScreenShake(vecSrc, 25.0, 150.0, 1.0, 750);
}

void CRoboCop::StartMortarAttack(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "robocop/rc_charge.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	m_bAimLocked = FALSE;
	m_fMortarAttackEvent = FALSE;
	m_flMortarAttackStart = gpGlobals->time;

	// Set eye full and ocilliate faster.
	SetEyeAlpha(ROBOCOP_EYE_MAX_ALPHA);
	m_flEyeFrequency = ROBOCOP_EYE_FREQUENCY_WARMUP;
}

void CRoboCop::StopMortarAttack(void)
{
	BeamOff();

	ResetHeadBoneControllers();

	STOP_SOUND( ENT(pev), CHAN_BODY, "robocop/rc_charge.wav" );

	m_bAimLocked = FALSE;
	m_fMortarAttackEvent = FALSE;
	m_flMortarAttackStart = 0;

	// Set eye full and reset idle frequency.
	SetEyeAlpha(ROBOCOP_EYE_MAX_ALPHA);
	m_flEyeFrequency = ROBOCOP_EYE_FREQUENCY_IDLE;
}

void CRoboCop::FinishRangeAttack()
{
	m_IdealActivity = ACT_IDLE;

	StopMortarAttack();

	// Wait 2 seconds before next mortar attack.
	m_flNextMortarAttack = gpGlobals->time + 2.0f;
}

void CRoboCop::HeadLookAt(const Vector& position)
{
	Vector vecEyePos, vecAngles;
	GetAttachment(ROBOCOP_HEAD_ATTACHMENT, vecEyePos, vecAngles);

	Vector lookAtAngles = UTIL_VecToAngles(position - vecEyePos);

	float flHeadYaw = lookAtAngles.y - pev->angles.y;
	float flHeadPitch = lookAtAngles.x - pev->angles.x;
	flHeadPitch = -flHeadPitch;

	if (flHeadPitch < -180)
		flHeadPitch += 360;
	else if (flHeadPitch > 180)
		flHeadPitch -= 360;

	if (flHeadYaw < -180)
		flHeadYaw += 360;
	else if (flHeadYaw > 180)
		flHeadYaw -= 360;

	flHeadYaw = clamp(flHeadYaw, -ROBOCOP_HEAD_YAW_RANGE, ROBOCOP_HEAD_YAW_RANGE);
	flHeadPitch = clamp(flHeadPitch, -ROBOCOP_HEAD_PITCH_RANGE, ROBOCOP_HEAD_PITCH_RANGE);

	//ALERT(at_console, "flHeadYaw: %.2f, flHeadPitch: %.2f\n", flHeadYaw, flHeadPitch);

	// Update head bone controllers.
	SetBoneController(0, flHeadYaw);
	SetBoneController(1, flHeadPitch);
}

void CRoboCop::ResetHeadBoneControllers()
{
	SetBoneController(1, 0); // Only reset head pitch. The base code will update the yaw controller as needed.
}

void CRoboCop::TraceDeathPosition(TraceResult& tr)
{
	UTIL_MakeVectors(pev->angles);

	Vector vecSrc = pev->origin + Vector(0, 0, 1) + gpGlobals->v_forward * 100;
	UTIL_TraceLine(vecSrc, vecSrc - Vector(0, 0, 40), ignore_monsters, ENT(pev), &tr);
}

void CRoboCop::CreateEffects(void)
{
	if (!m_hEyeGlow)
	{
		CSprite* pEyeGlow = CSprite::SpriteCreate(ROBOCOP_EYE_SPRITE_NAME, pev->origin, FALSE);
		if (pEyeGlow)
		{
			pEyeGlow->SetTransparency(kRenderTransAdd, 255, 255, 255, 0, kRenderFxNoDissipation);
			pEyeGlow->SetAttachment(edict(), 1);
			pEyeGlow->SetScale(0.5f);
			m_hEyeGlow = pEyeGlow;
		}
	}

	if (!m_hBeam)
	{
		CBeam* pBeam = CBeam::BeamCreate(ROBOCOP_EYE_BEAM_NAME, 20);
		if (pBeam)
		{
			pBeam->PointEntInit(pev->origin, entindex());
			pBeam->SetEndAttachment(1);
			pBeam->SetBrightness(0);
			pBeam->SetColor(255, 0, 0);
			pBeam->SetScrollRate( 20 );
			m_hBeam = pBeam;
		}
	}

	if (!m_hBeamSpot)
	{
		CSprite* pBeamSpot = CSprite::SpriteCreate(ROBOCOP_EYE_SPOT_NAME, pev->origin, FALSE);
		if (pBeamSpot)
		{
			pBeamSpot->SetTransparency(kRenderTransAdd, 255, 0, 0, 0, kRenderFxNoDissipation);
			pBeamSpot->SetScale(0.15f);
			m_hBeamSpot = pBeamSpot;
		}
	}
}

void CRoboCop::DestroyEffects(void)
{
	if (m_hEyeGlow)
	{
		UTIL_Remove(m_hEyeGlow);
		m_hEyeGlow = NULL;
	}

	if (m_hBeam)
	{
		UTIL_Remove(m_hBeam);
		m_hBeam = NULL;
	}

	if (m_hBeamSpot)
	{
		UTIL_Remove(m_hBeamSpot);
		m_hBeamSpot = NULL;
	}
}

void CRoboCop::EyeOn(int level)
{
	m_eyeBrightness = level;
}

void CRoboCop::EyeOff(void)
{
	m_eyeBrightness = 0;
}

void CRoboCop::EyeUpdate(float speed)
{
	if (m_hEyeGlow)
	{
		SetEyeAlpha(UTIL_Approach(m_eyeBrightness, m_hEyeGlow->pev->renderamt, speed));

		if (m_hEyeGlow->pev->renderamt == ROBOCOP_EYE_MAX_ALPHA)
			m_eyeBrightness = 0;
		else if (m_hEyeGlow->pev->renderamt == 0)
			m_eyeBrightness = ROBOCOP_EYE_MAX_ALPHA;
	}
}

void CRoboCop::SetEyeAlpha(int alpha)
{
	if (m_hEyeGlow)
	{
		m_hEyeGlow->pev->renderamt = clamp(alpha, 0, ROBOCOP_EYE_MAX_ALPHA);

		if (m_hEyeGlow->pev->renderamt == 0)
			m_hEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_hEyeGlow->pev->effects &= ~EF_NODRAW;
	}
}

void CRoboCop::BeamOn(int level)
{
	m_beamBrightness = level;
}

void CRoboCop::BeamOff(void)
{
	m_beamBrightness = 0;
}

void CRoboCop::BeamUpdate(float speed)
{
	if (m_hBeam)
	{
		m_hBeam->pev->renderamt = UTIL_Approach(m_beamBrightness, m_hBeam->pev->renderamt, speed);
		if (m_hBeam->pev->renderamt == 0)
			m_hBeam->pev->effects |= EF_NODRAW;
		else
			m_hBeam->pev->effects &= ~EF_NODRAW;
	}

	if (m_hBeamSpot)
	{
		m_hBeamSpot->pev->renderamt = UTIL_Approach(m_beamBrightness, m_hBeamSpot->pev->renderamt, speed);
		if (m_hBeamSpot->pev->renderamt == 0)
			m_hBeamSpot->pev->effects |= EF_NODRAW;
		else
			m_hBeamSpot->pev->effects &= ~EF_NODRAW;
	}
}
