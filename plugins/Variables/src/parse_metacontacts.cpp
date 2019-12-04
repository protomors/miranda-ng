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

static wchar_t* parseGetParent(ARGUMENTSINFO *ai)
{
	if (ai->argc != 2)
		return nullptr;

	MCONTACT hContact = getContactFromString(ai->argv.w[1], CI_ALLFLAGS);
	if (hContact == INVALID_CONTACT_ID)
		return nullptr;

	hContact = db_mc_getMeta(hContact);
	if (hContact == NULL)
		return nullptr;

	ptrW szUniqueID;
	char* szProto = Proto_GetBaseAccountName(hContact);
	if (szProto != nullptr)
		szUniqueID = getContactInfoT(CNF_UNIQUEID, hContact);

	if (szUniqueID == NULL) {
		szProto = PROTOID_HANDLE;
		wchar_t tszID[40];
		_itow_s(hContact, tszID, 10);
		szUniqueID = mir_wstrdup(tszID);
	}

	return CMStringW(FORMAT, L"<%S:%s>", szProto, szUniqueID.get()).Detach();
}

static wchar_t* parseGetDefault(ARGUMENTSINFO *ai)
{
	if (ai->argc != 2)
		return nullptr;

	MCONTACT hContact = getContactFromString(ai->argv.w[1], CI_ALLFLAGS);
	if (hContact == INVALID_CONTACT_ID)
		return nullptr;

	hContact = db_mc_getDefault(hContact);
	if (hContact == NULL)
		return nullptr;

	ptrW szUniqueID;
	char* szProto = Proto_GetBaseAccountName(hContact);
	if (szProto != nullptr)
		szUniqueID = getContactInfoT(CNF_UNIQUEID, hContact);

	if (szUniqueID == NULL) {
		szProto = PROTOID_HANDLE;
		wchar_t tszID[40];
		_itow_s(hContact, tszID, 10);
		szUniqueID = mir_wstrdup(tszID);
	}

	return CMStringW(FORMAT, L"<%S:%s>", szProto, szUniqueID.get()).Detach();
}

static wchar_t* parseGetMostOnline(ARGUMENTSINFO *ai)
{
	if (ai->argc != 2)
		return nullptr;

	MCONTACT hContact = getContactFromString(ai->argv.w[1], CI_ALLFLAGS);
	if (hContact == INVALID_CONTACT_ID)
		return nullptr;

	hContact = db_mc_getMostOnline(hContact);
	if (hContact == NULL)
		return nullptr;

	ptrW szUniqueID;
	char *szProto = Proto_GetBaseAccountName(hContact);
	if (szProto != nullptr)
		szUniqueID = getContactInfoT(CNF_UNIQUEID, hContact);

	if (szUniqueID == NULL) {
		szProto = PROTOID_HANDLE;
		wchar_t tszID[40];
		_itow_s(hContact, tszID, 10);
		szUniqueID = mir_wstrdup(tszID);
	}

	return CMStringW(FORMAT, L"<%S:%s>", szProto, szUniqueID.get()).Detach();
}

void registerMetaContactsTokens()
{
	registerIntToken(MC_GETPARENT, parseGetParent, TRF_FUNCTION, LPGEN("Metacontacts") "\t(x)\t" LPGEN("get parent metacontact of contact x"));
	registerIntToken(MC_GETDEFAULT, parseGetDefault, TRF_FUNCTION, LPGEN("Metacontacts") "\t(x)\t" LPGEN("get default subcontact x"));
	registerIntToken(MC_GETMOSTONLINE, parseGetMostOnline, TRF_FUNCTION, LPGEN("Metacontacts") "\t(x)\t" LPGEN("get the 'most online' subcontact x"));
}
