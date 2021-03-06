#include "commonheaders.h"

int __cdecl onContactSettingChanged(WPARAM hContact, LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;
	if (!hContact || strcmp(cws->szSetting, "Status")) return 0;

	pUinKey ptr = getUinKey(hContact);
	int stat = getContactStatus(hContact);
	if (!ptr || stat == -1) return 0;

	if (stat == ID_STATUS_OFFLINE) { // go offline
		if (ptr->mode == MODE_NATIVE && cpp_keyx(ptr->cntx)) { // have active context
			cpp_delete_context(ptr->cntx); ptr->cntx = nullptr; // reset context
			showPopupDC(hContact);	// show popup "Disabled"
			ShowStatusIconNotify(hContact); // change icon in CL
		}
		else if (ptr->mode == MODE_RSAAES && mir_exp->rsa_get_state(ptr->cntx) == 7) {
			deleteRSAcntx(ptr);
			showPopupDC(hContact);	// show popup "Disabled"
			ShowStatusIconNotify(hContact); // change icon in CL
		}
	}
	else { // go not offline
		if (ptr->offlineKey) {
			cpp_reset_context(ptr->cntx);
			ptr->offlineKey = false;
		}
		ShowStatusIconNotify(hContact); // change icon in CL
	}
	return 0;
}

//  wParam=(MCONTACT)hContact
//  lParam=0
int __cdecl onContactAdded(WPARAM wParam, LPARAM)
{
	addContact(wParam);
	return 0;
}


//  wParam=(MCONTACT)hContact
//  lParam=0
int __cdecl onContactDeleted(WPARAM wParam, LPARAM)
{
	delContact(wParam);
	return 0;
}


int __cdecl onExtraImageListRebuilding(WPARAM, LPARAM)
{
	RefreshContactListIcons();
	return 0;
}

int __cdecl onExtraImageApplying(WPARAM wParam, LPARAM)
{
	if (isSecureProtocol(wParam))
		ExtraIcon_SetIcon(g_hCLIcon, wParam, mode2clicon(isContactSecured(wParam), 1));

	return 0;
}

int __cdecl onRebuildContactMenu(WPARAM hContact, LPARAM)
{
	BOOL bMC = db_mc_isMeta(hContact);
	if (bMC)
		hContact = db_mc_getMostOnline(hContact); // возьмем тот, через который пойдет сообщение
	pUinKey ptr = getUinKey(hContact);

	ShowStatusIconNotify(hContact);

	// check offline/online
	if (!ptr) {
		// hide menu bars
		for (int i = 0; i < _countof(g_hMenu); i++)
			Menu_ShowItem(g_hMenu[i], false);
		return 0;
	}

	bool isSecureProto = isSecureProtocol(hContact);
	bool isPGP = isContactPGP(hContact);
	bool isGPG = isContactGPG(hContact);
	bool isSecured = (isContactSecured(hContact)&SECURED) != 0;
	bool isChat = isChatRoom(hContact);
	bool isMiranda = isClientMiranda(hContact);

	// hide all menu bars
	for (int i = 0; i < _countof(g_hMenu); i++)
		Menu_ShowItem(g_hMenu[i], false);

	if (isSecureProto && !isChat && isMiranda &&
		 (ptr->mode == MODE_NATIVE || ptr->mode == MODE_RSAAES)) {
		// Native/RSAAES

		if (!isSecured) // create secureim connection
			Menu_ModifyItem(g_hMenu[0], nullptr, mode2icon(ptr->mode | SECURED, 2), 0);
		else // disable secureim connection
			Menu_ModifyItem(g_hMenu[1], nullptr, mode2icon(ptr->mode, 2), 0);

		// set status menu
		if (bSCM && !bMC && (!isSecured || ptr->mode == MODE_PGP || ptr->mode == MODE_GPG)) {
			extern LPCWSTR sim312[];
			Menu_ModifyItem(g_hMenu[2], sim312[ptr->status], g_hICO[ICO_ST_DIS + ptr->status], 0);

			for (int i = 0; i <= (ptr->mode == MODE_RSAAES ? 1 : 2); i++)
				Menu_ModifyItem(g_hMenu[3 + i], nullptr, (i == ptr->status) ? g_hICO[ICO_ST_DIS + ptr->status] : nullptr, 0);
		}
	}
	else if (isSecureProto && !isChat && (ptr->mode == MODE_PGP || ptr->mode == MODE_GPG)) {
		// PGP, GPG
		if (ptr->mode == MODE_PGP && bPGPloaded)
			if ((bPGPkeyrings || bPGPprivkey) && !isGPG)
				Menu_ShowItem(g_hMenu[isPGP + 6], true);

		if (ptr->mode == MODE_GPG && bGPGloaded)
			if (bGPGkeyrings && !isPGP)
				Menu_ShowItem(g_hMenu[isGPG + 8], true);
	}
	if (isSecureProto && !isChat && isMiranda) {
		// set mode menu
		if (bMCM && !bMC && (!isSecured || ptr->mode == MODE_PGP || ptr->mode == MODE_GPG)) {
			extern LPCWSTR sim311[];
			Menu_ModifyItem(g_hMenu[10], sim311[ptr->mode], g_hICO[ICO_OV_NAT + ptr->mode], 0);

			for (int i = 0; i < MODE_CNT; i++) {
				if (i == MODE_PGP && ptr->mode != MODE_PGP && !bPGP) continue;
				if (i == MODE_GPG && ptr->mode != MODE_GPG && !bGPG) continue;
				Menu_ModifyItem(g_hMenu[11 + i], nullptr, (i == ptr->mode) ? g_hICO[ICO_ST_ENA] : nullptr, 0);
			}
		}
	}

	return 0;
}
