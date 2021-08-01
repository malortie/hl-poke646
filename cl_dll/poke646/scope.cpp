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

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <cstring>

#include "triangleapi.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"

extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle

#define SCOPE_BORDER_SPRITE	"sprites/scopeborder.spr"

DECLARE_MESSAGE(m_Scope, Scope)

int CHudScope::Init(void)
{
	HOOK_MESSAGE(Scope);

	Reset();

	gHUD.AddHudElem(this);
	return 1;
}

void CHudScope::Reset(void)
{
	m_iFlags &= ~HUD_ACTIVE;
}

int CHudScope::VidInit(void)
{
	m_hSprite = SPR_Load(SCOPE_BORDER_SPRITE);

	return 1;
}

int CHudScope::MsgFunc_Scope(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int fOn = READ_BYTE();

	if (fOn)
	{
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}


int CHudScope::Draw(float flTime)
{
	return 1;
}

void CHudScope::DrawScopeBorder(int frame, int x, int y, int width, int height)
{
	if (!m_hSprite)
	{
		m_hSprite = SPR_Load(SCOPE_BORDER_SPRITE);
		if (!m_hSprite)
			return;
	}

	struct model_s* hSpriteModel = (struct model_s*)gEngfuncs.GetSpritePointer(m_hSprite);
	if (!hSpriteModel)
		return;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture); //additive

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, frame);
	gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, 1.0);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3f(x, y + height, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3f(x + width, y + height, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3f(x + width, y, 0);

	gEngfuncs.pTriAPI->End();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal
}

int CHudScope::DrawScope(void)
{
	if (!(m_iFlags & HUD_ACTIVE))
		return 1;

	int halfScreenWidth = ScreenWidth / 2;
	int x = 0;
	int y = ScreenHeight / 2 - halfScreenWidth;

	// Top left scope border.
	DrawScopeBorder(0, x, y, halfScreenWidth, halfScreenWidth);

	// Top right scope border.
	DrawScopeBorder(1, x + halfScreenWidth, y, halfScreenWidth, halfScreenWidth);

	y = ScreenHeight / 2;

	// Bottom left scope border.
	DrawScopeBorder(2, x, y, halfScreenWidth, halfScreenWidth);

	// Bottom right scope border.
	DrawScopeBorder(3, x + halfScreenWidth, y, halfScreenWidth, halfScreenWidth);

	vec3_t forward;
	AngleVectors(v_angles, forward, NULL, NULL);
	VectorScale(forward, 8192, forward);
	VectorAdd(forward, v_origin, forward);
	pmtrace_t * trace = gEngfuncs.PM_TraceLine(v_origin, forward, PM_TRACELINE_PHYSENTSONLY, 2, -1);

	float dist = (trace->endpos - v_origin).Length();
	// Assuming 1 foot is roughly equal to 12 hammer units, convert to meters. 
	float meters = (dist / 12.0f) * 0.3048f;

	char szDistance[16] = {};
	if (meters < 3)
		std::strcpy(szDistance, "-,-- m");
	else
		std::snprintf(szDistance, sizeof(szDistance), "%.2f m", meters);

	x = ScreenWidth - ScreenWidth / 4;
	y = ScreenHeight / 2 - ScreenHeight / 32;

	// Draw distance.
	gEngfuncs.pfnDrawSetTextColor(1, 0.35, 0.35);
	DrawConsoleString(x, y, szDistance);

	return 1;
}
