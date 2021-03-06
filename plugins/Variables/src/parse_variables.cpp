/*
    Variables Plugin for Miranda-IM (www.miranda-im.org)
    Copyright 2003-2006 P. Boon

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "stdafx.h"

// this is for get and put(s)
static mir_cs csVarRegister;
static VARIABLEREGISTER *vr = nullptr;
static int vrCount = 0;

static int addToVariablesRegister(wchar_t *szName, wchar_t *szText)
{
	if ((szName == nullptr) || (szText == nullptr) || (mir_wstrlen(szName) <= 0))
		return -1;

	mir_cslock lck(csVarRegister);
	for (int i = 0; i < vrCount; i++) {
		if ((!mir_wstrcmp(vr[i].szName, szName))) {
			mir_free(vr[i].szText);
			vr[i].szText = mir_wstrdup(szText);
			return 0;
		}
	}
	VARIABLEREGISTER *pvr = (VARIABLEREGISTER*)mir_realloc(vr, (vrCount + 1)*sizeof(VARIABLEREGISTER));
	if (pvr == nullptr)
		return -1;

	vr = pvr;
	vr[vrCount].szName = mir_wstrdup(szName);
	vr[vrCount].szText = mir_wstrdup(szText);
	vr[vrCount++].dwOwnerThread = GetCurrentThreadId();
	return 0;
}

static wchar_t* searchVariableRegister(wchar_t *szName)
{
	if ((szName == nullptr) || (mir_wstrlen(szName) <= 0))
		return nullptr;

	mir_cslock lck(csVarRegister);
	for (int i = 0; i < vrCount; i++)
		if ((!mir_wstrcmp(vr[i].szName, szName)))
			return mir_wstrdup(vr[i].szText);

	return nullptr;
}

static wchar_t* parsePut(ARGUMENTSINFO *ai)
{
	if (ai->argc != 3)
		return nullptr;

	//	ai->flags |= AIF_DONTPARSE;
	if (addToVariablesRegister(ai->argv.w[1], ai->argv.w[2]))
		return nullptr;

	FORMATINFO fi;
	memcpy(&fi, ai->fi, sizeof(fi));
	fi.szFormat.w = ai->argv.w[2];
	fi.flags |= FIF_UNICODE;
	return formatString(&fi);
}

static wchar_t* parsePuts(ARGUMENTSINFO *ai)
{
	if (ai->argc != 3)
		return nullptr;

	if (addToVariablesRegister(ai->argv.w[1], ai->argv.w[2]))
		return nullptr;

	return mir_wstrdup(L"");
}

static wchar_t* parseGet(ARGUMENTSINFO *ai)
{
	if (ai->argc != 2)
		return nullptr;

	return searchVariableRegister(ai->argv.w[1]);
}

void registerVariablesTokens()
{
	registerIntToken(GET, parseGet, TRF_FUNCTION, LPGEN("Variables") "\t(x)\t" LPGEN("variable set by put(s) with name x"));
	registerIntToken(PUT, parsePut, TRF_FUNCTION, LPGEN("Variables") "\t(x,y)\t" LPGEN("x, and stores y as variable named x"));//TRF_UNPARSEDARGS);
	registerIntToken(PUTS, parsePuts, TRF_FUNCTION, LPGEN("Variables") "\t(x,y)\t" LPGEN("only stores y as variables x"));//TRF_UNPARSEDARGS);
}

void unregisterVariablesTokens()
{
	mir_cslock lck(csVarRegister);
	for (int i = 0; i < vrCount; i++) {
		mir_free(vr[i].szName);
		mir_free(vr[i].szText);
	}
	mir_free(vr);
	vr = nullptr;
	vrCount = 0;
}
