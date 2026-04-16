#ifndef XAQ_REQ_H
#define XAQ_REQ_H

#include <windows.h>


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _XAQ_REQ_HEAD
{
	char **Names;
	char **Values;
	int Count;
	int Capacity;
} XAQ_REQ_HEAD, *PXAQ_REQ_HEAD;

typedef struct _XAQ_HTTP_RESP
{
	int StatusCode;
	char *Body;
	SIZE_T BodyLength;
	char *ContentType;
} XAQ_HTTP_RESP, *PXAQ_HTTP_RESP;


BOOL XaqInit(void);
void XaqClean(void);


PXAQ_HTTP_RESP XaqHttpGet(const char *Host, const char *Path);
PXAQ_HTTP_RESP XaqHttpGetSec(const char *Host, const char *Path);
PXAQ_HTTP_RESP XaqHttpPost(const char *Host, const char *Path, const char *Data, SIZE_T DataLength);
PXAQ_HTTP_RESP XaqHttpPostSec(const char *Host, const char *Path, const char *Data, SIZE_T DataLength);
PXAQ_HTTP_RESP XaqHttpPostJson(const char *Host, const char *Path, const char *Json, SIZE_T DataLength);
PXAQ_HTTP_RESP XaqHttpPostJsonSec(const char *Host, const char *Path, const char *Json, SIZE_T DataLength);
PXAQ_HTTP_RESP XaqHttpPutSec(const char *Host, const char *Path, const char *Data, SIZE_T DataLength);
PXAQ_HTTP_RESP XaqHttpDeleteSec(const char *Host, const char *Path);

PXAQ_HTTP_RESP XaqHttpGen(const char *Method, const char *Host, WORD Port, const char *Path, const char *Data, SIZE_T DataLength, const PXAQ_REQ_HEAD Headers, DWORD Flags);
void XaqHttpDestroy(PXAQ_HTTP_RESP Response);


#ifdef __cplusplus
}
#endif


#ifdef XAQ_REQ_IMP


static CRITICAL_SECTION g_InitCs;
static volatile LONG g_InitCsReady = 0;
static volatile LONG g_InitDone = 0;
static volatile LONG g_WininetLoaded = 0;


typedef HANDLE (__stdcall *GetProcessHeap_t)(void);
typedef LPVOID (__stdcall *HeapAlloc_t)(HANDLE, DWORD, SIZE_T);
typedef LPVOID (__stdcall *HeapReAlloc_t)(HANDLE, DWORD, LPVOID, SIZE_T);
typedef BOOL (__stdcall *HeapFree_t)(HANDLE, DWORD, LPVOID);

static GetProcessHeap_t pGetProcessHeap = NULL;
static HeapAlloc_t pHeapAlloc = NULL;
static HeapReAlloc_t pHeapReAlloc = NULL;
static HeapFree_t pHeapFree = NULL;

static HANDLE g_Heap = NULL;


typedef void* HINTERNET;

typedef HINTERNET (__stdcall *InternetOpenA_t)(const char*, DWORD, const char*, const char*, DWORD);
typedef HINTERNET (__stdcall *InternetConnectA_t)(HINTERNET, const char*, WORD, const char*, const char*, DWORD, DWORD, DWORD);
typedef HINTERNET (__stdcall *HttpOpenRequestA_t)(HINTERNET, const char*, const char*, const char*, const char*, const char**, DWORD, DWORD);
typedef BOOL (__stdcall *HttpSendRequestA_t)(HINTERNET, const char*, DWORD, LPVOID, DWORD);
typedef BOOL (__stdcall *InternetReadFile_t)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL (__stdcall *InternetCloseHandle_t)(HINTERNET);
typedef BOOL (__stdcall *HttpQueryInfoA_t)(HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);
typedef DWORD (__stdcall *InternetSetOptionA_t)(HINTERNET, DWORD, LPVOID, DWORD);


static HMODULE g_Wininet = NULL;
static HMODULE g_Kernel32 = NULL;

static InternetOpenA_t pInternetOpenA = NULL;
static InternetConnectA_t pInternetConnectA = NULL;
static HttpOpenRequestA_t pHttpOpenRequestA = NULL;
static HttpSendRequestA_t pHttpSendRequestA = NULL;
static InternetReadFile_t pInternetReadFile = NULL;
static InternetCloseHandle_t pInternetCloseHandle = NULL;
static HttpQueryInfoA_t pHttpQueryInfoA = NULL;
static InternetSetOptionA_t pInternetSetOptionA = NULL;

static HINTERNET g_Session = NULL;

#define FG_SEC 0x00800000
#define HTTP_SC 19
#define HTTP_CT 1

#define HEAP_ZERO_MEM 0x00000008

static BOOL PerfActInit(void)
{
    g_Kernel32 = GetModuleHandleA("kernel32.dll");
	
    if (!g_Kernel32) g_Kernel32 = LoadLibraryA("kernel32.dll");
	
    if (g_Kernel32) {
        pGetProcessHeap = (GetProcessHeap_t)GetProcAddress(g_Kernel32, "GetProcessHeap");
        pHeapAlloc = (HeapAlloc_t)GetProcAddress(g_Kernel32, "HeapAlloc");
        pHeapReAlloc = (HeapReAlloc_t)GetProcAddress(g_Kernel32, "HeapReAlloc");
        pHeapFree = (HeapFree_t)GetProcAddress(g_Kernel32, "HeapFree");
    }

    g_Wininet = LoadLibraryA("wininet.dll");
	
    if (!g_Wininet) return FALSE;

    pInternetOpenA = (InternetOpenA_t)GetProcAddress(g_Wininet, "InternetOpenA");
    pInternetConnectA = (InternetConnectA_t)GetProcAddress(g_Wininet, "InternetConnectA");
    pHttpOpenRequestA = (HttpOpenRequestA_t)GetProcAddress(g_Wininet, "HttpOpenRequestA");
    pHttpSendRequestA = (HttpSendRequestA_t)GetProcAddress(g_Wininet, "HttpSendRequestA");
    pInternetReadFile = (InternetReadFile_t)GetProcAddress(g_Wininet, "InternetReadFile");
    pInternetCloseHandle = (InternetCloseHandle_t)GetProcAddress(g_Wininet, "InternetCloseHandle");
    pHttpQueryInfoA = (HttpQueryInfoA_t)GetProcAddress(g_Wininet, "HttpQueryInfoA");
    pInternetSetOptionA = (InternetSetOptionA_t)GetProcAddress(g_Wininet, "InternetSetOptionA");

    if (!pInternetOpenA || !pInternetConnectA || !pHttpOpenRequestA ||
        !pHttpSendRequestA || !pInternetReadFile || !pInternetCloseHandle ||
        !pHttpQueryInfoA || !pInternetSetOptionA) {
        FreeLibrary(g_Wininet);
        g_Wininet = NULL;
        return FALSE;
    }
	
	g_Session = pInternetOpenA("xaquake/1.0", 0, NULL, NULL, 0);
	if (!g_Session) return FALSE;

    return TRUE;
}

static BOOL XaqReqInit(void)
{
    if (g_InitDone) return TRUE;

    if (InterlockedCompareExchange(&g_InitCsReady, 1, 0) == 0) {
        InitializeCriticalSection(&g_InitCs);
    } else {
        while (!g_InitCsReady) {
            Sleep(0);
        }
    }

    EnterCriticalSection(&g_InitCs);

    if (g_InitDone) {
        LeaveCriticalSection(&g_InitCs);
        return TRUE;
    }

    BOOL success = PerfActInit();
	
    if (success) {
        InterlockedExchange(&g_WininetLoaded, 1);
        InterlockedExchange(&g_InitDone, 1);
    }

    LeaveCriticalSection(&g_InitCs);
    return success;
}

BOOL XaqInit(void)
{
	return XaqReqInit();
}

void XaqClean(void)
{
	if (g_Session)
	{
		pInternetCloseHandle(g_Session);
		g_Session = NULL;
	}
	
	if (g_Wininet)
	{
		FreeLibrary(g_Wininet);
		g_Wininet = NULL;
	}
	
	InterlockedExchange(&g_WininetLoaded, 0);
	InterlockedExchange(&g_InitDone, 0);
	
	if (g_InitCsReady)
	{
		DeleteCriticalSection(&g_InitCs);
		InterlockedExchange(&g_InitCsReady, 0);
	}
	
	g_Kernel32 = NULL;
	pGetProcessHeap = NULL;
	pHeapAlloc = NULL;
	pHeapReAlloc = NULL;
	pHeapFree = NULL;
	g_Heap = NULL;
}

static void* XaqAlloc(SIZE_T Size)
{
	if (!g_Heap) g_Heap = pGetProcessHeap ? pGetProcessHeap() : GetProcessHeap();
	return pHeapAlloc ? pHeapAlloc(g_Heap, 0, Size) : HeapAlloc(g_Heap, 0, Size);
}

static void* XaqAllocZero(SIZE_T Size)
{
	if (!g_Heap) g_Heap = pGetProcessHeap ? pGetProcessHeap() : GetProcessHeap();
	return pHeapAlloc ? pHeapAlloc(g_Heap, HEAP_ZERO_MEM, Size) : HeapAlloc(g_Heap, HEAP_ZERO_MEM, Size);
}

static void* XaqRealloc(void* Ptr, SIZE_T Size)
{
    if (!g_Heap) g_Heap = pGetProcessHeap ? pGetProcessHeap() : GetProcessHeap();
    return pHeapReAlloc ? pHeapReAlloc(g_Heap, 0, Ptr, Size) : HeapReAlloc(g_Heap, 0, Ptr, Size);
}

static void XaqFree(void* Ptr)
{
	if (!Ptr) return;
	if (!g_Heap) g_Heap = pGetProcessHeap ? pGetProcessHeap() : GetProcessHeap();
	if (pHeapFree) pHeapFree(g_Heap, 0, Ptr);
	else HeapFree(g_Heap, 0, Ptr);
}

static SIZE_T XaqStrLen(const char* Str)
{
	const char* p = Str;
	while (*p) p++;
	return p - Str;
}

static void XaqMemCpy(void* Dest, const void* Src, SIZE_T Len)
{
	char* d = (char*)Dest;
	const char* s = (const char*)Src;
	while (Len--) *d++ = *s++;
}

static void XaqFillMem(void* Dest, int Value, SIZE_T Len)
{
	char* d = (char*)Dest;
	while (Len--) *d++ = (char)Value;
}

static void XaqCpyStr(char* Dest, const char* Src)
{
	while((*Dest++ = *Src++));
}

static char* XaqDupStr(const char* Str)
{
	SIZE_T Len = XaqStrLen(Str) + 1;
	char *Result = (char *)XaqAlloc(Len);
	
	if (Result) XaqMemCpy(Result, Str, Len);
	
	return Result;
}

static int XaqStrToInt(const char* Str)
{
	int Result = 0;
	
	while (*Str >= '0' && *Str <= '9') Result = Result * 10 + (*Str++ - '0');
	
	return Result;
}


PXAQ_REQ_HEAD XaqHttpHeadersBuilder(void)
{
	PXAQ_REQ_HEAD Headers = (PXAQ_REQ_HEAD)XaqAllocZero(sizeof(struct _XAQ_REQ_HEAD));
	
	if (!Headers) return NULL;
	
	Headers->Capacity = 8;
	Headers->Names = (char **)XaqAllocZero(Headers->Capacity * sizeof(char *));
	Headers->Values = (char **)XaqAllocZero(Headers->Capacity * sizeof(char *));
	
	if (!Headers->Names || !Headers->Values)
	{
		XaqFree(Headers->Names);
		XaqFree(Headers->Values);
		XaqFree(Headers);
		return NULL;
	}
	return Headers;
}

void XaqHttpHeadersDestroy(PXAQ_REQ_HEAD Headers)
{
	if (!Headers) return;
	
	for (int i=0; i < Headers->Count; i++)
	{
		XaqFree(Headers->Names[i]);
		XaqFree(Headers->Values[i]);
	}
	
	XaqFree(Headers->Names);
	XaqFree(Headers->Values);
	XaqFree(Headers);
}

BOOL XaqHttpHeadersAdd(PXAQ_REQ_HEAD Headers, const char *Name, const char *Value)
{
	if (!Headers || !Name || !Value) return FALSE;
	
	if (Headers->Count >= Headers->Capacity)
	{
		int NewCapacity = Headers->Capacity * 2;
		char **NewNames = (char **)XaqRealloc(Headers->Names, NewCapacity * sizeof(char *));
		char **NewValues = (char **)XaqRealloc(Headers->Values, NewCapacity * sizeof(char *));
		
		if (!NewNames || !NewValues) return FALSE;
		
		XaqFillMem((char *)NewNames + Headers->Capacity * sizeof(char *), 0, (NewCapacity - Headers->Capacity) * sizeof(char *));
		XaqFillMem((char *)NewValues + Headers->Capacity * sizeof(char *), 0, (NewCapacity - Headers->Capacity) * sizeof(char *));
		Headers->Names = NewNames;
		Headers->Values = NewValues;
		Headers->Capacity = NewCapacity;
	}
	
	Headers->Names[Headers->Count] = XaqDupStr(Name);
	Headers->Values[Headers->Count] = XaqDupStr(Value);
	
	if (!Headers->Names[Headers->Count] || !Headers->Values[Headers->Count]) return FALSE;
	
	Headers->Count++;
	return TRUE;
}

static char* XaqHeadersBuildStr(const PXAQ_REQ_HEAD Headers)
{
	if (!Headers || Headers->Count == 0) return NULL;
	
	SIZE_T AllTot = 1;
	
	for (int i = 0; i < Headers->Count; i++) AllTot += XaqStrLen(Headers->Names[i]) + 2 + XaqStrLen(Headers->Values[i]) + 2;

	char *Result = (char *)XaqAlloc(AllTot);
	
	if (!Result) return NULL;
	
	Result[0] = '\0';
	char *p = Result;
	
	for (int i = 0; i < Headers->Count; i++)
	{
		XaqCpyStr(p, Headers->Names[i]);
		p += XaqStrLen(Headers->Names[i]);
		*p++ = ':';
		*p++ = ' ';
		XaqCpyStr(p, Headers->Values[i]);
		p += XaqStrLen(Headers->Values[i]);
		*p++ = '\r';
		*p++ = '\n';
	}
	
	*p = '\0';
	return Result;
}

void XaqHttpDestroy(PXAQ_HTTP_RESP Response)
{
	if (!Response) return;
	XaqFree(Response->Body);
	XaqFree(Response->ContentType);
	XaqFree(Response);
}


PXAQ_HTTP_RESP XaqHttpGen(const char *Method, const char *Host, WORD Port, const char *Path, const char *Data, SIZE_T DataLength, const PXAQ_REQ_HEAD Headers, DWORD Flags)
{
	if (!g_Session)
	{
		if (!XaqInit()) return NULL;
		if (!g_Session)
		{
			g_Session = pInternetOpenA("xaquake/1.0", 0, NULL, NULL, 0);
			if (!g_Session) return NULL;
		}
	}
	
	if (!Method || !Host || !Path) return NULL;
	
	HINTERNET hConn = pInternetConnectA(g_Session, Host, Port, NULL, NULL, 3, 0, 0);
	
	if (!hConn) return NULL;
	
	HINTERNET hReq = pHttpOpenRequestA(hConn, Method, Path, NULL, NULL, NULL, Flags, 0);
	
	if (!hReq)
	{
		pInternetCloseHandle(hConn);
		return NULL;
	}
	
	char *HeadersStr = XaqHeadersBuildStr(Headers);
	DWORD HeadersLen = HeadersStr ? (DWORD)XaqStrLen(HeadersStr) : 0;
	
	BOOL Sent = pHttpSendRequestA(hReq, HeadersStr, HeadersLen, (LPVOID)Data, (DWORD)DataLength);
	XaqFree(HeadersStr);
	
	if (!Sent)
	{
		pInternetCloseHandle(hReq);
		pInternetCloseHandle(hConn);
		return NULL;
	}
	
	PXAQ_HTTP_RESP Resp = (PXAQ_HTTP_RESP)XaqAllocZero(sizeof(struct _XAQ_HTTP_RESP));
	
	if (!Resp)
	{
		pInternetCloseHandle(hReq);
		pInternetCloseHandle(hConn);
		return NULL;
	}
	
	char Buf_sc[32];
	DWORD BufLen = sizeof(Buf_sc);
	DWORD Index = 0;
	
	if (pHttpQueryInfoA(hReq, HTTP_SC, Buf_sc, &BufLen, &Index)) Resp->StatusCode = XaqStrToInt(Buf_sc);
	
	char Buf_ct[256];
	DWORD Ct_Len = sizeof(Buf_ct);
	Index = 0;
	
	if (pHttpQueryInfoA(hReq, HTTP_CT, Buf_ct, &Ct_Len, &Index)) Resp->ContentType = XaqDupStr(Buf_ct);
	
	SIZE_T Capacity = 4096;
	Resp->Body = (char*)XaqAlloc(Capacity);
	
	if(!Resp->Body)
	{
		XaqHttpDestroy(Resp);
		pInternetCloseHandle(hReq);
		pInternetCloseHandle(hConn);
		return NULL;
	}
	
	DWORD Read;
	char XaqTemp[4096];
	
	while(pInternetReadFile(hReq, XaqTemp, sizeof(XaqTemp), &Read) && Read > 0)
	{
		if (Resp->BodyLength + Read >= Capacity)
		{
			Capacity *= 2;
			char *NewBody = (char*)XaqRealloc(Resp->Body, Capacity);
			
			if (!NewBody) break;
			
			Resp->Body = NewBody;
		}
		XaqMemCpy(Resp->Body + Resp->BodyLength, XaqTemp, Read);
		Resp->BodyLength += Read;
	}
	
	Resp->Body[Resp->BodyLength] = '\0';
	pInternetCloseHandle(hReq);
	pInternetCloseHandle(hConn);
	return Resp;
}

PXAQ_HTTP_RESP XaqHttpGet(const char *Host, const char *Path)
{
	return XaqHttpGen("GET", Host, 80, Path, NULL, 0, NULL, 0);
}

PXAQ_HTTP_RESP XaqHttpGetSec(const char *Host, const char *Path)
{
	return XaqHttpGen("GET", Host, 443, Path, NULL, 0, NULL, FG_SEC);
}

PXAQ_HTTP_RESP XaqHttpPost(const char *Host, const char *Path, const char *Data, SIZE_T DataLength)
{
	PXAQ_REQ_HEAD Headers = XaqHttpHeadersBuilder();
	if (Headers) XaqHttpHeadersAdd(Headers, "Content-Type", "application/x-www-form-urlencoded");
	PXAQ_HTTP_RESP Resp = XaqHttpGen("POST", Host, 80, Path, Data, DataLength, Headers, 0);
	XaqHttpHeadersDestroy(Headers);
	return Resp;
}

PXAQ_HTTP_RESP XaqHttpPostSec(const char *Host, const char *Path, const char *Data, SIZE_T DataLength)
{
	PXAQ_REQ_HEAD Headers = XaqHttpHeadersBuilder();
	if (Headers) XaqHttpHeadersAdd(Headers, "Content-Type", "application/x-www-form-urlencoded");
	PXAQ_HTTP_RESP Resp = XaqHttpGen("POST", Host, 443, Path, Data, DataLength, Headers, FG_SEC);
	XaqHttpHeadersDestroy(Headers);
	return Resp;
}

PXAQ_HTTP_RESP XaqHttpPostJson(const char *Host, const char *Path, const char *Json, SIZE_T DataLength)
{
	PXAQ_REQ_HEAD Headers = XaqHttpHeadersBuilder();
	if (Headers) XaqHttpHeadersAdd(Headers, "Content-Type", "application/json");
	PXAQ_HTTP_RESP Resp = XaqHttpGen("POST", Host, 80, Path, Json, DataLength, Headers, 0);
	XaqHttpHeadersDestroy(Headers);
	return Resp;
}

PXAQ_HTTP_RESP XaqHttpPostJsonSec(const char *Host, const char *Path, const char *Json, SIZE_T DataLength)
{
	PXAQ_REQ_HEAD Headers = XaqHttpHeadersBuilder();
	if (Headers) XaqHttpHeadersAdd(Headers, "Content-Type", "application/json");
	PXAQ_HTTP_RESP Resp = XaqHttpGen("POST", Host, 443, Path, Json, DataLength, Headers, FG_SEC);
	XaqHttpHeadersDestroy(Headers);
	return Resp;
}

PXAQ_HTTP_RESP XaqHttpPutSec(const char *Host, const char *Path, const char *Data, SIZE_T DataLength)
{
	return XaqHttpGen("PUT", Host, 443, Path, Data, DataLength, NULL, FG_SEC);
}

PXAQ_HTTP_RESP XaqHttpDeleteSec(const char *Host, const char *Path)
{
	return XaqHttpGen("DELETE", Host, 443, Path, NULL, 0, NULL, FG_SEC);
}

#endif /* XAQ_REQ_IMP */

#endif /* XAQ_REQ_H */
