/* 
Copyright (C) 2012 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "stdafx.h"

void CreateAuthString(char *auth, MCONTACT hContact, CFeedEditor *pDlg)
{
	wchar_t *tlogin = nullptr, *tpass = nullptr;
	if (hContact && db_get_b(hContact, MODULE, "UseAuth", 0)) {
		tlogin = db_get_wsa(hContact, MODULE, "Login");
		tpass = db_get_wsa(hContact, MODULE, "Password");
	}
	else if (pDlg && pDlg->m_useauth.IsChecked()) {
		tlogin = pDlg->m_login.GetText();
		tpass = pDlg->m_password.GetText();
	}
	char *user = mir_u2a(tlogin), *pass = mir_u2a(tpass);

	char str[MAX_PATH];
	int len = mir_snprintf(str, "%s:%s", user, pass);
	mir_free(user);
	mir_free(pass);
	mir_free(tlogin);
	mir_free(tpass);

	mir_snprintf(auth, 250, "Basic %s", ptrA(mir_base64_encode(str, len)));
}

CAuthRequest::CAuthRequest(CFeedEditor *pDlg, MCONTACT hContact)
	: CSuper(g_hInstance, IDD_AUTHENTICATION),
	m_feedname(this, IDC_FEEDNAME), m_username(this, IDC_FEEDUSERNAME),
	m_password(this, IDC_FEEDPASSWORD), m_ok(this, IDOK)
{
	m_pDlg = pDlg;
	m_hContact = hContact;
	m_ok.OnClick = Callback(this, &CAuthRequest::OnOk);
}

void CAuthRequest::OnInitDialog()
{
	if (m_pDlg) {
		ptrW strfeedtitle(m_pDlg->m_feedtitle.GetText());
		
		if (strfeedtitle)
			m_feedname.SetText(strfeedtitle);
		else {
			ptrW strfeedurl(m_pDlg->m_feedurl.GetText());
			m_feedname.SetText(strfeedurl);
		}
	}
	else if (m_hContact) {
		wchar_t *ptszNick = db_get_wsa(m_hContact, MODULE, "Nick");
		if (ptszNick) {
			m_feedname.SetText(ptszNick);
			mir_free(ptszNick);
		}
		else {
			wchar_t *ptszURL = db_get_wsa(m_hContact, MODULE, "URL");
			if (ptszURL) {
				m_feedname.SetText(ptszURL);
				mir_free(ptszURL);
			}
		}
	}
}

void CAuthRequest::OnOk(CCtrlBase*)
{
	ptrW strfeedusername(m_username.GetText());
	if (!strfeedusername || mir_wstrcmp(strfeedusername, L"") == 0) {
		MessageBox(m_hwnd, TranslateT("Enter your username"), TranslateT("Error"), MB_OK | MB_ICONERROR);
		return;
	}
	ptrA strfeedpassword(m_password.GetTextA());
	if (!strfeedpassword || mir_strcmp(strfeedpassword, "") == 0) {
		MessageBox(m_hwnd, TranslateT("Enter your password"), TranslateT("Error"), MB_OK | MB_ICONERROR);
		return;
	}
	if (m_pDlg) {
		m_pDlg->m_useauth.SetState(1);
		m_pDlg->m_login.Enable(1);
		m_pDlg->m_password.Enable(1);
		m_pDlg->m_login.SetText(strfeedusername);
		m_pDlg->m_password.SetTextA(strfeedpassword);
	}
	else if (m_hContact) {
		db_set_b(m_hContact, MODULE, "UseAuth", 1);
		db_set_ws(m_hContact, MODULE, "Login", strfeedusername);
		db_set_s(m_hContact, MODULE, "Password", strfeedpassword);

	}
}
