/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (C) 2012-21 Miranda NG team (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "netlib.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

static bool bSslInitDone;

enum SocketState
{
	sockOpen,
	sockClosed,
	sockError
};

struct SslHandle : public MZeroedObject
{
	~SslHandle()
	{
		if (session)
			SSL_free(session);
		if (ctx)
			SSL_CTX_free(ctx);
	}

	SOCKET s;
	SSL_CTX *ctx;
	SSL *session;
	SocketState state;
};

static void SSL_library_unload(void)
{
	/* Load Library Pointers */
	if (!bSslInitDone)
		return;

	bSslInitDone = false;
}

static bool SSL_library_load(void)
{
	/* Load Library Pointers */
	if (bSslInitDone)
		return true;

	if (!bSslInitDone) { // init OpenSSL
		SSL_library_init();
		SSL_load_error_strings();
		// FIXME check errors

		bSslInitDone = true;
	}

	return bSslInitDone;
}

static void dump_error(SSL *session, int err)
{
	err = SSL_get_error(session, err);

	char buf[100];
	ERR_error_string_n(err, buf, sizeof(buf));
	Netlib_Logf(nullptr, "SSL negotiation failure: %s (%d)", buf, err);
}

const char* SSL_GetCipherName(SslHandle *ssl)
{
	if (!ssl || !ssl->session)
		return nullptr;

	return SSL_CIPHER_get_name(SSL_get_current_cipher(ssl->session));
}

static void ReportSslError(SECURITY_STATUS scRet, int line, bool = false)
{
	CMStringW tszMsg(FORMAT, L"SSL connection failure(%x %u) :", scRet, line);

	switch (scRet) {
	case 0:
	case ERROR_NOT_READY:
		return;

	case SEC_E_INVALID_TOKEN:
		tszMsg += TranslateW_LP(L"Client cannot decode host message. Possible causes: host does not support SSL or requires not existing security package");
		break;

	case CERT_E_CN_NO_MATCH:
	case SEC_E_WRONG_PRINCIPAL:
		tszMsg += TranslateW_LP(L"Host we are connecting to is not the one certificate was issued for");
		break;

	default:
		wchar_t szMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, scRet, LANG_USER_DEFAULT, szMsgBuf, _countof(szMsgBuf), nullptr);
		tszMsg += szMsgBuf;
	}

	Netlib_LogfW(nullptr, tszMsg);

	SetLastError(scRet);
	PUShowMessageW(tszMsg.GetBuffer(), SM_WARNING);
}

static bool ClientConnect(SslHandle *ssl, const char*)
{
	SSL_METHOD *meth = (SSL_METHOD*)SSLv23_client_method();

	// contrary to what it's named, SSLv23 announces all supported ciphers/versions,
	// generally TLS1.2 in a TLS1.0 Client Hello
	if (!meth) {
		Netlib_Logf(nullptr, "SSL setup failure: client method");
		return false;
	}
	ssl->ctx = SSL_CTX_new(meth);
	if (!ssl->ctx) {
		Netlib_Logf(nullptr, "SSL setup failure: context");
		return false;
	}

	// SSL_read/write should transparently handle renegotiations
	SSL_CTX_ctrl(ssl->ctx, SSL_CTRL_MODE, SSL_MODE_AUTO_RETRY, nullptr);

	RAND_screen();
	ssl->session = SSL_new(ssl->ctx);
	if (!ssl->session) {
		Netlib_Logf(nullptr, "SSL setup failure: session");
		return false;
	}
	SSL_set_fd(ssl->session, ssl->s);

	int err = SSL_connect(ssl->session);
	if (err != 1) {
		dump_error(ssl->session, err);
		return false;
	}

	return true;
}

static PCCERT_CONTEXT SSL_X509ToCryptCert(X509 * x509)
{
	unsigned char *buf = nullptr;
	PCCERT_CONTEXT pCertContext = nullptr;

	int len = i2d_X509(x509, &buf);
	if ((len >= 0) && buf) {
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, buf, len);

		CRYPTO_free(buf, __FILE__, __LINE__);
	}
	return pCertContext;
}

static PCCERT_CONTEXT SSL_CertChainToCryptAnchor(SSL* session)
{
	/* convert the active certificate chain provided in the handshake of 'session' into
	the format used by CryptAPI.
	*/
	PCCERT_CONTEXT anchor = nullptr;
	// create cert store
	HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG, nullptr);

	if (store) {
		X509 *server_cert = SSL_get_peer_certificate(session);
		if (server_cert) {
			// add the server's cert first, to make sure CryptAPI builds the correct chain
			PCCERT_CONTEXT primary_cert;
			BOOL ok = CertAddCertificateContextToStore(store, SSL_X509ToCryptCert(server_cert), CERT_STORE_ADD_ALWAYS, &primary_cert);
			if (ok && primary_cert) {
				// add all remaining certs to store (note: stack needs not be freed, it is not a copy)
				STACK_OF(X509) *server_chain = SSL_get_peer_cert_chain(session);
				if (server_chain) {
					for (int i = 0; i < OPENSSL_sk_num((OPENSSL_STACK *)server_chain); i++) {
						X509 *next_cert = (X509 *)OPENSSL_sk_value((OPENSSL_STACK *)server_chain, i);
						CertAddCertificateContextToStore(store, SSL_X509ToCryptCert(next_cert), CERT_STORE_ADD_USE_EXISTING, nullptr);
					}
				}

				// return primary cert; MUST be freed by caller which will free the associated store
				anchor = primary_cert;
			}
			else {
				if (primary_cert)
					CertFreeCertificateContext(primary_cert);
			}

			X509_free(server_cert);
		}

		CertCloseStore(store, 0);
	}

	return anchor;
}

static LPSTR rgszUsages[] =
{
	szOID_PKIX_KP_SERVER_AUTH,
	szOID_SERVER_GATED_CRYPTO,
	szOID_SGC_NETSCAPE
};

static bool VerifyCertificate(SslHandle *ssl, PCSTR pszServerName, DWORD dwCertFlags)
{
	DWORD scRet;

	ptrW pwszServerName(mir_a2u(pszServerName));

	HTTPSPolicyCallbackData polHttps = {};
	CERT_CHAIN_POLICY_PARA PolicyPara = {};
	CERT_CHAIN_POLICY_STATUS PolicyStatus = {};
	CERT_CHAIN_PARA ChainPara = {};

	PCCERT_CHAIN_CONTEXT pChainContext = nullptr;
	PCCERT_CONTEXT pServerCert = SSL_CertChainToCryptAnchor(ssl->session);
	if (pServerCert == nullptr) {
		scRet = SEC_E_WRONG_PRINCIPAL;
		goto cleanup;
	}

	ChainPara.cbSize = sizeof(ChainPara);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = _countof(rgszUsages);
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;
	
	if (!CertGetCertificateChain(nullptr, pServerCert, nullptr, pServerCert->hCertStore, &ChainPara, 0, nullptr, &pChainContext)) {
		scRet = GetLastError();
		goto cleanup;
	}

	polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks = dwCertFlags;
	polHttps.pwszServerName = pwszServerName;

	PolicyPara.cbSize = sizeof(PolicyPara);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	PolicyStatus.cbSize = sizeof(PolicyStatus);

	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL, pChainContext, &PolicyPara, &PolicyStatus)) {
		scRet = GetLastError();
		goto cleanup;
	}

	if (PolicyStatus.dwError) {
		scRet = PolicyStatus.dwError;
		goto cleanup;
	}

	scRet = SEC_E_OK;

cleanup:
	if (pChainContext)
		CertFreeCertificateChain(pChainContext);
	if (pServerCert)
		CertFreeCertificateContext(pServerCert);

	ReportSslError(scRet, __LINE__, true);
	return scRet == SEC_E_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
// negotiate SSL session, verify cert, return NULL if failed

MIR_APP_DLL(HSSL) Netlib_SslConnect(SOCKET s, const char* host, int verify)
{
	SslHandle *ssl = new SslHandle();
	ssl->s = s;
	bool res = ClientConnect(ssl, host);

	if (res && verify) {
		DWORD dwFlags = 0;
		if (!host || inet_addr(host) != INADDR_NONE)
			dwFlags |= 0x00001000;
		res = VerifyCertificate(ssl, host, dwFlags);
	}

	if (res)
		return ssl;

	delete ssl;
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
// return true if there is either unsend or buffered received data (ie. after peek)

MIR_APP_DLL(BOOL) Netlib_SslPending(HSSL ssl)
{
	return ssl && ssl->session && (SSL_pending(ssl->session) > 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// reads number of bytes, keeps in buffer if peek != 0

MIR_APP_DLL(int) Netlib_SslRead(HSSL ssl, char *buf, int num, int peek)
{
	if (!ssl || !ssl->session) return SOCKET_ERROR;
	if (num <= 0) return 0;

	int err = 0;
	if (peek)
		err = SSL_peek(ssl->session, buf, num);
	else
		err = SSL_read(ssl->session, buf, num);

	if (err <= 0) {
		int err2 = SSL_get_error(ssl->session, err);
		if (err2 == SSL_ERROR_ZERO_RETURN) {
			Netlib_Logf(nullptr, "SSL connection gracefully closed");
			ssl->state = sockClosed;
			return 0;
		}

		Netlib_Logf(nullptr, "SSL failure recieving data (%d, %d, %d)", err, err2, WSAGetLastError());
		ssl->state = sockError;
		return SOCKET_ERROR;
	}

	return err;
}

/////////////////////////////////////////////////////////////////////////////////////////
// writes data to the SSL socket

MIR_APP_DLL(int) Netlib_SslWrite(HSSL ssl, const char *buf, int num)
{
	if (!ssl || !ssl->session)
		return SOCKET_ERROR;
	if (num <= 0)
		return 0;

	int err = SSL_write(ssl->session, buf, num);
	if (err > 0)
		return err;

	int err2 = SSL_get_error(ssl->session, err);
	switch (err2) {
	case SSL_ERROR_ZERO_RETURN:
		Netlib_Logf(nullptr, "SSL connection gracefully closed");
		ssl->state = sockClosed;
		break;

	default:
		Netlib_Logf(nullptr, "SSL failure sending data (%d, %d, %d)", err, err2, WSAGetLastError());
		ssl->state = sockError;
		return SOCKET_ERROR;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// closes SSL session, but keeps socket open

MIR_APP_DLL(void) Netlib_SslShutdown(HSSL ssl)
{
	if (ssl && ssl->session)
		SSL_shutdown(ssl->session);
}

/////////////////////////////////////////////////////////////////////////////////////////
// frees all data associated with the SSL socket

MIR_APP_DLL(void) Netlib_SslFree(HSSL ssl)
{
	delete ssl;
}

/////////////////////////////////////////////////////////////////////////////////////////
// makes connection SSL
// returns 0 on failure / 1 on success

MIR_APP_DLL(int) Netlib_StartSsl(HNETLIBCONN hConnection, const char *szHost)
{
	NetlibConnection *nlc = (NetlibConnection*)hConnection;
	if (nlc == nullptr)
		return 0;

	NetlibUser *nlu = nlc->nlu;
	if (szHost == nullptr)
		szHost = nlc->nloc.szHost;
	szHost = NEWSTR_ALLOCA(szHost);

	Netlib_Logf(nlu, "(%d %s) Starting SSL negotiation", int(nlc->s), szHost);

	nlc->hSsl = Netlib_SslConnect(nlc->s, szHost, nlu->settings.validateSSL);
	if (nlc->hSsl == nullptr)
		Netlib_Logf(nlu, "(%d %s) Failure to negotiate SSL connection", int(nlc->s), szHost);
	else
		Netlib_Logf(nlu, "(%d %s) SSL negotiation successful", int(nlc->s), szHost);

	return nlc->hSsl != nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
// gets TLS channel binging data for a socket

MIR_APP_DLL(void*) Netlib_GetTlsUnique(HNETLIBCONN nlc, int &cbLen)
{
	if (nlc == nullptr || nlc->hSsl == nullptr)
		return nullptr;

	char buf[1000];
	size_t len = SSL_get_finished(nlc->hSsl->session, buf, sizeof(buf));
	if (len == 0)
		return nullptr;

	cbLen = (int)len;
	void *pBuf = mir_alloc(len);
	memcpy(pBuf, buf, len);
	return pBuf;
}
