/**
 * @file xaq_req.c
 * @brief Библиотека для выполнения HTTP/S-запросов
 * @author xaquake
 * @date 04.2026
 * @version 1.1.0
 *
 * В частности пока предназначена для этого, в будущем будет расширена!
 *
 * @code
 * #define XAQ_REQ_IMP
 * #include "xaq_req.c"
 * @endcode
 */

#ifndef XAQ_REQ_H
#define XAQ_REQ_H

#ifdef __cplusplus
extern "C"
{
#endif

  typedef unsigned long SIZE_T;
  typedef unsigned long DWORD;
  typedef unsigned short WORD;
  typedef int BOOL;

  /**
   * @struct _XAQ_REQ_HEAD
   * @brief Структура для хранения HTTP-заголовков
   *
   * Используется для построения списка заголовков HTTP-запроса.
   * Имена и значения хранятся в динамических массивах строк.
   */
  typedef struct _XAQ_REQ_HEAD
  {
    char **Names;      /**< Массив имён заголовков */
    char **Values;     /**< Массив значений заголовков */
    int Count;         /**< Текущее количество заголовков */
    int Capacity;      /**< Ёмкость массивов (выделенная память) */
  } XAQ_REQ_HEAD, *PXAQ_REQ_HEAD;

  /**
   * @struct _XAQ_HTTP_RESP
   * @brief Структура ответа HTTP-запроса
   *
   * Содержит данные полученного ответа от сервера:
   * код статуса, тело ответа и тип содержимого.
   */
  typedef struct _XAQ_HTTP_RESP
  {
    int StatusCode;    /**< HTTP-код статуса (200, 404, 500 и т.д.) */
    char *Body;        /**< Тело ответа (текст) */
    SIZE_T BodyLength; /**< Длина тела ответа в байтах */
    char *ContentType; /**< Тип содержимого (Content-Type) */
  } XAQ_HTTP_RESP, *PXAQ_HTTP_RESP;

  /**
   * @brief Инициализирует библиотеку
   * @return TRUE при успехе, FALSE при ошибке
   *
   * Загружает wininet.dll и подготавливает необходимые ресурсы.
   * Должна быть вызвана перед использованием других функций.
   */
  BOOL XaqInit (void);

  /**
   * @brief Освобождает ресурсы библиотеки
   *
   * Закрывает сессию WinInet, выгружает DLL и освобождает память.
   * Должна быть вызвана перед завершением программы.
   */
  void XaqClean (void);

  /**
   * @brief Возвращает текущий User-Agent
   * @return Строка User-Agent (не освобождать!)
   *
   * Если User-Agent не установлен, возвращает "Mozilla/5.0".
   */
  static const char *XaqGetUserAgent (void);

  /**
   * @brief Устанавливает User-Agent для HTTP-запросов
   * @param UserAgent Новая строка User-Agent
   * @return TRUE при успехе, FALSE при ошибке
   */
  BOOL XaqSetUserAgent (const char *UserAgent);

  /**
   * @brief Выполняет HTTP-запрос
   * @param Method HTTP-метод ("GET", "POST", etc.)
   * @param Host Имя хоста или IP-адрес
   * @param Port Порт сервера
   * @param Path Путь запроса ("/api/endpoint")
   * @param Data Данные для POST-запроса (может быть NULL)
   * @param DataLength Длина данных в байтах
   * @param Headers Заголовки запроса (может быть NULL)
   * @param Flags Флаги WinInet (0 по умолчанию)
   * @return Указатель на структуру ответа или NULL при ошибке
   *
   * Для освобождения памяти используйте XaqHttpDestroy().
   * Таймаут соединения установлен в 10 секунд.
   *
   * @code
   * PXAQ_HTTP_RESP resp = XaqHttpGen("GET", "api.example.com", 80,
   *                                   "/data", NULL, 0, NULL, 0);
   * if (resp) {
   *     printf("Status: %d\n", resp->StatusCode);
   *     XaqHttpDestroy(resp);
   * }
   * @endcode
   */
  PXAQ_HTTP_RESP XaqHttpGen (const char *Method, const char *Host, WORD Port,
                             const char *Path, const char *Data,
                             SIZE_T DataLength, const PXAQ_REQ_HEAD Headers,
                             DWORD Flags);

  /**
   * @brief Освобождает структуру ответа HTTP
   * @param Response Указатель на структуру ответа
   *
   * Освобождает память, выделенную для тела ответа,
   * Content-Type и самой структуры.
   */
  void XaqHttpDestroy (PXAQ_HTTP_RESP Response);

  /**
   * @brief Возвращает код последней ошибки
   * @return Код ошибки Win32 (GetLastError)
   *
   * Вызывайте после неудачной операции для диагностики.
   */
  DWORD XaqGetLastErr (void);

  static BOOL EnsInit (void);

#ifdef __cplusplus
}
#endif

#ifdef XAQ_REQ_IMP

typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef int BOOL;
typedef void *HANDLE;
typedef HANDLE HMODULE;
typedef void *LPVOID;
typedef DWORD *LPDWORD;
typedef unsigned long SIZE_T;
typedef const char *LPCSTR;
#define FALSE 0
#define TRUE 1
#define ERROR_INSUFFICIENT_BUFFER 122

#define STDCALL __attribute__ ((stdcall))
#define NULL ((void *)0)

extern HMODULE STDCALL GetModuleHandleA (LPCSTR lpModuleName);
extern HMODULE STDCALL LoadLibraryA (LPCSTR lpLibFileName);
extern void *STDCALL GetProcAddress (HMODULE hModule, LPCSTR lpProcName);
extern DWORD STDCALL GetLastError (void);
extern BOOL STDCALL FreeLibrary (HMODULE hModule);
extern HANDLE STDCALL GetProcessHeap (void);
extern LPVOID STDCALL HeapAlloc (HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
extern LPVOID STDCALL HeapReAlloc (HANDLE hHeap, DWORD dwFlags, LPVOID lpMem,
                                   SIZE_T dwBytes);
extern BOOL STDCALL HeapFree (HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);

static int g_InitDone = 0;
static DWORD g_LastErr = 0;
static char *g_UserAgent = NULL;
static HANDLE g_Heap = NULL;

typedef void *HINTERNET;

extern HINTERNET STDCALL InternetOpenA (const char *, DWORD, const char *,
                                        const char *, DWORD);
extern HINTERNET STDCALL InternetConnectA (HINTERNET, const char *, WORD,
                                           const char *, const char *, DWORD,
                                           DWORD, DWORD);
extern HINTERNET STDCALL HttpOpenRequestA (HINTERNET, const char *,
                                           const char *, const char *,
                                           const char *, const char **, DWORD,
                                           DWORD);
extern BOOL STDCALL HttpSendRequestA (HINTERNET, const char *, DWORD, LPVOID,
                                      DWORD);
extern BOOL STDCALL InternetReadFile (HINTERNET, LPVOID, DWORD, LPDWORD);
extern BOOL STDCALL InternetCloseHandle (HINTERNET);
extern BOOL STDCALL HttpQueryInfoA (HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD);
extern BOOL STDCALL InternetSetOptionA (HINTERNET, DWORD, LPVOID, DWORD);

#define FG_SEC 0x00800000
#define INTERNET_FLAG_RELOAD 0x80000000
#define HTTP_SC 19
#define HTTP_CT 1

#define INTERNET_OPTION_CONNECT_TIMEOUT 2
#define INTERNET_OPTION_SEND_TIMEOUT 5
#define INTERNET_OPTION_RECEIVE_TIMEOUT 6
#define INTERNET_OPTION_RESET 0x20000003

#define INT_MAX 2147483647
#define HEAP_ZERO_MEM 0x00000008

/**
 * @brief Выделяет память из кучи процесса
 * @param Size Размер памяти в байтах
 * @return Указатель на выделенную память или NULL при ошибке
 */
static void *
XaqAlloc (SIZE_T Size)
{
  if (!g_Heap)
    g_Heap = GetProcessHeap ();
  return HeapAlloc (g_Heap, 0, Size);
}

/**
 * @brief Выделяет память из кучи и обнуляет её
 * @param Size Размер памяти в байтах
 * @return Указатель на выделенную и обнулённую память или NULL при ошибке
 */
static void *
XaqAllocZero (SIZE_T Size)
{
  if (!g_Heap)
    g_Heap = GetProcessHeap ();
  return HeapAlloc (g_Heap, HEAP_ZERO_MEM, Size);
}

/**
 * @brief Перевыделяет память с изменением размера
 * @param Ptr Указатель на ранее выделенную память
 * @param Size Новый размер в байтах
 * @return Указатель на перевыделенную память или NULL при ошибке
 */
static void *
XaqRealloc (void *Ptr, SIZE_T Size)
{
  if (!g_Heap)
    g_Heap = GetProcessHeap ();
  return HeapReAlloc (g_Heap, 0, Ptr, Size);
}

/**
 * @brief Освобождает память, выделенную из кучи
 * @param Ptr Указатель на память для освобождения
 */
static void
XaqFree (void *Ptr)
{
  if (Ptr)
    HeapFree (g_Heap ? g_Heap : GetProcessHeap (), 0, Ptr);
}

/**
 * @brief Вычисляет длину строки
 * @param Str Указатель на строку
 * @return Количество символов в строке (без '\0')
 */
static SIZE_T
XaqStrLen (const char *Str)
{
  const char *p = Str;
  while (*p)
    p++;
  return p - Str;
}

/**
 * @brief Копирует блок памяти
 * @param Dest Указатель на приёмник
 * @param Src Указатель на источник
 * @param Len Количество байт для копирования
 */
static void
XaqMemCpy (void *Dest, const void *Src, SIZE_T Len)
{
  char *d = (char *)Dest;
  const char *s = (const char *)Src;
  while (Len--)
    *d++ = *s++;
}

/**
 * @brief Заполняет блок памяти значением
 * @param Dest Указатель на блок памяти
 * @param Value Значение для заполнения (будет приведено к char)
 * @param Len Количество байт для заполнения
 */
static void
XaqFillMem (void *Dest, int Value, SIZE_T Len)
{
  char *d = (char *)Dest;
  while (Len--)
    *d++ = (char)Value;
}

/**
 * @brief Копирует строку включая завершающий '\0'
 * @param Dest Указатель на приёмник (должен быть достаточного размера)
 * @param Src Указатель на исходную строку
 */
static void
XaqCpyStr (char *Dest, const char *Src)
{
  while ((*Dest++ = *Src++))
    ;
}

/**
 * @brief Создаёт копию строки в динамической памяти
 * @param Str Исходная строка
 * @return Указатель на новую копию строки или NULL при ошибке
 *
 * Выделенную память нужно освободить через XaqFree().
 */
static char *
XaqDupStr (const char *Str)
{
  SIZE_T Len = XaqStrLen (Str) + 1;
  char *Result = (char *)XaqAlloc (Len);

  if (Result)
    XaqMemCpy (Result, Str, Len);

  return Result;
}

/**
 * @brief Преобразует строку в целое число
 * @param Str Строка, содержащая цифры
 * @param Out Указатель для записи результата (может быть NULL)
 * @return Количество обработанных цифр или 0 при ошибке
 */
static int
XaqStrToInt (const char *Str, int *Out)
{
  int Result = 0;
  int Digits = 0;
  while (*Str >= '0' && *Str <= '9')
    {
      Result = Result * 10 + (*Str++ - '0');
      Digits++;
    }
  if (Out)
    *Out = Result;
  return Digits > 0;
}

/**
 * @brief Сравнивает две строки
 * @param FirstString Первая строка
 * @param SecondString Вторая строка
 * @return TRUE если строки равны, FALSE если различаются
 */
static BOOL
XaqStrEqual (const char *FirstString, const char *SecondString)
{
  if (FirstString == SecondString)
    return TRUE;

  if (!FirstString || !SecondString)
    return FALSE;

  while (*FirstString && *FirstString == *SecondString)
    {
      FirstString++;
      SecondString++;
    }

  return *FirstString == *SecondString;
}

BOOL
XaqSetUserAgent (const char *UserAgent)
{
  if (!UserAgent)
    return FALSE;

  if (!XaqInit ())
    return FALSE;

  char *UA = XaqDupStr (UserAgent);

  if (!UA)
    return FALSE;

  if (g_UserAgent)
    XaqFree (g_UserAgent);

  g_UserAgent = UA;
  return TRUE;
}

/**
 * @brief Возвращает текущий User-Agent (реализация)
 * @return Строка User-Agent или "Mozilla/5.0" по умолчанию
 */
const char *
XaqGetUserAgent (void)
{
  return g_UserAgent ? g_UserAgent : "Mozilla/5.0";
}

/**
 * @brief Возвращает код последней ошибки (реализация)
 * @return Код ошибки Win32
 */
DWORD
XaqGetLastErr (void)
{
  return g_LastErr;
}

static HMODULE g_Wininet = NULL;
static HINTERNET g_Session = NULL;
static char *g_PrevUserAgent = NULL;

extern HANDLE STDCALL GetStdHandle (DWORD nStdHandle);
extern BOOL STDCALL WriteConsoleA (HANDLE hConsoleOutput, const void *lpBuffer,
                                   DWORD nNumberOfCharsToWrite,
                                   LPDWORD lpNumberOfCharsWritten,
                                   LPVOID lpReserved);
#define STD_OUTPUT_HANDLE -11

/**
 * @brief Выводит строку в консоль
 * @param Str Строка для вывода
 */
void
XaqPrint (const char *Str)
{
  HANDLE hOut = GetStdHandle (STD_OUTPUT_HANDLE);
  DWORD Written;
  WriteConsoleA (hOut, Str, XaqStrLen (Str), &Written, NULL);
}

/**
 * @brief Обеспечивает создание или повторное использование сессии WinInet
 * @return Дескриптор сессии или NULL при ошибке
 *
 * При смене User-Agent пересоздаёт сессию.
 */
static HINTERNET
EnsSession (void)
{
  const char *UA = XaqGetUserAgent ();

  if (g_Session)
    {
      if (!XaqStrEqual (g_PrevUserAgent, UA))
        {
          DWORD Flags_ = 0;
          InternetSetOptionA (g_Session, INTERNET_OPTION_RESET, &Flags_,
                              sizeof (Flags_));
          InternetCloseHandle (g_Session);
          g_Session = NULL;
          if (g_PrevUserAgent)
            {
              XaqFree (g_PrevUserAgent);
              g_PrevUserAgent = NULL;
            }
        }
      else
        return g_Session;
    }

  g_Session = InternetOpenA (UA, 0, NULL, NULL, 0);
  if (g_Session)
    {
      if (g_PrevUserAgent)
        XaqFree (g_PrevUserAgent);
      g_PrevUserAgent = XaqDupStr (UA);
    }
  return g_Session;
}

/**
 * @brief Внутренняя инициализация библиотеки
 * @return TRUE при успехе, FALSE при ошибке
 *
 * Инициализирует кучу процесса и загружает wininet.dll.
 */
static BOOL
PerfActInit (void)
{
  if (!g_Heap)
    g_Heap = GetProcessHeap ();
  if (!g_Wininet)
    g_Wininet = LoadLibraryA ("wininet.dll");
  return g_Wininet != NULL;
}

/**
 * @brief Инициализирует библиотеку (реализация)
 * @return TRUE при успехе, FALSE при ошибке
 *
 * Потокобезопасная инициализация с отслеживанием состояния.
 */
BOOL
XaqInit (void)
{
  if (g_InitDone)
    return TRUE;
  g_InitDone = PerfActInit ();
  return g_InitDone;
}

/**
 * @brief Освобождает ресурсы библиотеки (реализация)
 *
 * Закрывает сессию, выгружает wininet.dll, освобождает память.
 */
void
XaqClean (void)
{
  if (g_Session)
    {
      InternetCloseHandle (g_Session);
      g_Session = NULL;
    }
  if (g_Wininet)
    {
      FreeLibrary (g_Wininet);
      g_Wininet = NULL;
    }
  g_InitDone = 0;
  if (g_UserAgent)
    {
      XaqFree (g_UserAgent);
      g_UserAgent = NULL;
    }
  if (g_PrevUserAgent)
    {
      XaqFree (g_PrevUserAgent);
      g_PrevUserAgent = NULL;
    }
}

/**
 * @brief Создаёт строитель заголовков HTTP
 * @return Указатель на структуру заголовков или NULL при ошибке
 *
 * Создаёт пустую структуру с начальной ёмкостью 8 элементов.
 * Для освобождения используйте XaqHttpHeadersDestroy().
 */
PXAQ_REQ_HEAD
XaqHttpHeadersBuilder (void)
{
  PXAQ_REQ_HEAD Headers
      = (PXAQ_REQ_HEAD)XaqAllocZero (sizeof (struct _XAQ_REQ_HEAD));

  if (!Headers)
    return NULL;

  Headers->Capacity = 8;
  Headers->Names = (char **)XaqAllocZero (Headers->Capacity * sizeof (char *));
  Headers->Values = (char **)XaqAllocZero (Headers->Capacity * sizeof (char *));

  if (!Headers->Names || !Headers->Values)
    {
      XaqFree (Headers->Names);
      XaqFree (Headers->Values);
      XaqFree (Headers);
      return NULL;
    }
  return Headers;
}

/**
 * @brief Уничтожает структуру заголовков HTTP
 * @param Headers Указатель на структуру заголовков
 *
 * Освобождает память для всех имён, значений и самой структуры.
 */
void
XaqHttpHeadersDestroy (PXAQ_REQ_HEAD Headers)
{
  if (!Headers)
    return;

  for (int i = 0; i < Headers->Count; i++)
    {
      XaqFree (Headers->Names[i]);
      XaqFree (Headers->Values[i]);
    }

  XaqFree (Headers->Names);
  XaqFree (Headers->Values);
  XaqFree (Headers);
}

/**
 * @brief Добавляет заголовок в список
 * @param Headers Структура заголовков
 * @param Name Имя заголовка
 * @param Value Значение заголовка
 * @return TRUE при успехе, FALSE при ошибке
 *
 * При необходимости увеличивает ёмкость массивов.
 * Имя и значение копируются во внутреннюю память.
 */
BOOL
XaqHttpHeadersAdd (PXAQ_REQ_HEAD Headers, const char *Name, const char *Value)
{
  if (!Headers || !Name || !Value)
    return FALSE;

  if (Headers->Count >= Headers->Capacity)
    {
      if (Headers->Capacity > INT_MAX / 2)
        return FALSE;
      int NewCapacity = Headers->Capacity * 2;
      char **NewNames
          = (char **)XaqRealloc (Headers->Names, NewCapacity * sizeof (char *));
      char **NewValues = (char **)XaqRealloc (Headers->Values,
                                              NewCapacity * sizeof (char *));

      if (!NewNames || !NewValues)
        return FALSE;

      XaqFillMem ((char *)NewNames + Headers->Capacity * sizeof (char *), 0,
                  (NewCapacity - Headers->Capacity) * sizeof (char *));
      XaqFillMem ((char *)NewValues + Headers->Capacity * sizeof (char *), 0,
                  (NewCapacity - Headers->Capacity) * sizeof (char *));
      Headers->Names = NewNames;
      Headers->Values = NewValues;
      Headers->Capacity = NewCapacity;
    }

  Headers->Names[Headers->Count] = XaqDupStr (Name);
  Headers->Values[Headers->Count] = XaqDupStr (Value);

  if (!Headers->Names[Headers->Count] || !Headers->Values[Headers->Count])
    return FALSE;

  Headers->Count++;
  return TRUE;
}

/**
 * @brief Формирует строку заголовков для HTTP-запроса
 * @param Headers Структура заголовков
 * @return Строка заголовков в формате HTTP или NULL
 *
 * Формат: "Name: Value\r\nName2: Value2\r\n"
 * Память нужно освободить через XaqFree().
 */
static char *
XaqHeadersBuildStr (const PXAQ_REQ_HEAD Headers)
{
  if (!Headers || Headers->Count == 0)
    return NULL;

  SIZE_T AllTot = 1;

  for (int i = 0; i < Headers->Count; i++)
    AllTot += XaqStrLen (Headers->Names[i]) + 2 + XaqStrLen (Headers->Values[i])
              + 2;

  char *Result = (char *)XaqAlloc (AllTot);

  if (!Result)
    return NULL;

  Result[0] = '\0';
  char *p = Result;

  for (int i = 0; i < Headers->Count; i++)
    {
      XaqCpyStr (p, Headers->Names[i]);
      p += XaqStrLen (Headers->Names[i]);
      *p++ = ':';
      *p++ = ' ';
      XaqCpyStr (p, Headers->Values[i]);
      p += XaqStrLen (Headers->Values[i]);
      *p++ = '\r';
      *p++ = '\n';
    }

  *p = '\0';
  return Result;
}

/**
 * @brief Освобождает структуру ответа HTTP (реализация)
 * @param Response Указатель на структуру ответа
 */
void
XaqHttpDestroy (PXAQ_HTTP_RESP Response)
{
  if (!Response)
    return;
  XaqFree (Response->Body);
  XaqFree (Response->ContentType);
  XaqFree (Response);
}

/**
 * @brief Выполняет HTTP-запрос (реализация)
 * @param Method HTTP-метод ("GET", "POST", "PUT", "DELETE" и т.д.)
 * @param Host Имя хоста или IP-адрес сервера
 * @param Port Номер порта (80 для HTTP, 443 для HTTPS)
 * @param Path Путь ресурса на сервере
 * @param Data Данные для отправки в теле запроса
 * @param DataLength Длина данных в байтах
 * @param Headers Дополнительные HTTP-заголовки
 * @param Flags Флаги WinInet (например, FG_SEC для HTTPS)
 * @return Указатель на структуру ответа или NULL при ошибке
 *
 * Устанавливает таймауты соединения, отправки и получения в 10 секунд.
 * При ошибке устанавливает код ошибки, доступный через XaqGetLastErr().
 */
PXAQ_HTTP_RESP
XaqHttpGen (const char *Method, const char *Host, WORD Port, const char *Path,
            const char *Data, SIZE_T DataLength, const PXAQ_REQ_HEAD Headers,
            DWORD Flags)
{
  if (!XaqInit ())
    {
      g_LastErr = GetLastError ();
      return NULL;
    }

  HINTERNET hSession = EnsSession ();

  if (!hSession)
    {
      g_LastErr = GetLastError ();
      return NULL;
    }

  if (!Method || !Host || !Path)
    {
      g_LastErr = GetLastError ();
      return NULL;
    }

  HINTERNET hConn
      = InternetConnectA (hSession, Host, Port, NULL, NULL, 3, 0, 0);

  if (!hConn)
    {
      g_LastErr = GetLastError ();
      return NULL;
    }

  HINTERNET hReq = HttpOpenRequestA (hConn, Method, Path, NULL, NULL, NULL,
                                     Flags | INTERNET_FLAG_RELOAD, 0);

  DWORD Timeout = 10000;
  InternetSetOptionA (hReq, INTERNET_OPTION_CONNECT_TIMEOUT, &Timeout,
                      sizeof (Timeout));
  InternetSetOptionA (hReq, INTERNET_OPTION_SEND_TIMEOUT, &Timeout,
                      sizeof (Timeout));
  InternetSetOptionA (hReq, INTERNET_OPTION_RECEIVE_TIMEOUT, &Timeout,
                      sizeof (Timeout));

  if (!hReq)
    {
      InternetCloseHandle (hConn);
      g_LastErr = GetLastError ();
      return NULL;
    }

  char *HeadersStr = XaqHeadersBuildStr (Headers);
  DWORD HeadersLen = HeadersStr ? (DWORD)XaqStrLen (HeadersStr) : 0;

  BOOL Sent = HttpSendRequestA (hReq, HeadersStr, HeadersLen, (LPVOID)Data,
                                (DWORD)DataLength);
  XaqFree (HeadersStr);

  if (!Sent)
    {
      InternetCloseHandle (hReq);
      InternetCloseHandle (hConn);
      g_LastErr = GetLastError ();
      return NULL;
    }

  PXAQ_HTTP_RESP Resp
      = (PXAQ_HTTP_RESP)XaqAllocZero (sizeof (struct _XAQ_HTTP_RESP));

  if (!Resp)
    {
      InternetCloseHandle (hReq);
      InternetCloseHandle (hConn);
      g_LastErr = GetLastError ();
      return NULL;
    }

  char Buf_sc[32];
  DWORD BufLen = sizeof (Buf_sc);
  DWORD Index = 0;

  if (HttpQueryInfoA (hReq, HTTP_SC, Buf_sc, &BufLen, &Index))
    {
      int SCode;
      if (XaqStrToInt (Buf_sc, &SCode))
        {
          Resp->StatusCode = SCode;
        }
      else
        {
          Resp->StatusCode = -1;
        }
    }
  else
    {
      Resp->StatusCode = 0;
    }

  DWORD Ct_Len = 0;
  Index = 0;

  if (!HttpQueryInfoA (hReq, HTTP_CT, NULL, &Ct_Len, &Index)
      && GetLastError () == ERROR_INSUFFICIENT_BUFFER)
    {
      char *Buf_ct = (char *)XaqAlloc (Ct_Len + 1);
      if (Buf_ct)
        {
          if (HttpQueryInfoA (hReq, HTTP_CT, Buf_ct, &Ct_Len, &Index))
            {
              Buf_ct[Ct_Len] = '\0';
              Resp->ContentType = Buf_ct;
            }
          else
            {
              XaqFree (Buf_ct);
            }
        }
    }

  SIZE_T Capacity = 1024;
  Resp->Body = (char *)XaqAlloc (Capacity);

  if (!Resp->Body)
    {
      XaqHttpDestroy (Resp);
      InternetCloseHandle (hReq);
      InternetCloseHandle (hConn);
      g_LastErr = GetLastError ();
      return NULL;
    }

  DWORD Read;
  char XaqTemp[1024];

  while (InternetReadFile (hReq, XaqTemp, sizeof (XaqTemp), &Read) && Read > 0)
    {
      if (Resp->BodyLength + Read >= Capacity)
        {
          Capacity *= 2;
          char *NewBody = (char *)XaqRealloc (Resp->Body, Capacity);

          if (!NewBody)
            break;

          Resp->Body = NewBody;
        }
      XaqMemCpy (Resp->Body + Resp->BodyLength, XaqTemp, Read);
      Resp->BodyLength += Read;
    }

  Resp->Body[Resp->BodyLength] = '\0';
  InternetCloseHandle (hReq);
  InternetCloseHandle (hConn);
  return Resp;
}

#endif /* XAQ_REQ_IMP */

#endif /* XAQ_REQ_H */
#define XAQ_REQ_IMP
