/*  u-logs.cpp

    Copyright (c) Glosav 2021
    Author: Kachkovsky Sergey V.
    kasv@list.ru

    Запись в лог
*/



#include <u-logs.hpp>
#include <u-time.hpp>
#include <u-files.hpp>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <mutex>


#ifdef _WIN32
    #include <process.h>
    #define getpid  _getpid
    static char* strsignal(int sig) { return (char*)""; }
#else
    #include <sys/types.h>
    #include <unistd.h>
#endif


#define printf_stderr(...)	    if (g_logErr.m_ena_stderr) { fprintf(stderr, __VA_ARGS__); }
#define printf_stderr_ln(...)	    if (g_logErr.m_ena_stderr) { fprintf(stderr, __VA_ARGS__); fflush(stderr); }

#define DebugPointsMax	16


// ************************************************************************************************
class LogDebugPoint {
    public:
	LogDebugPoint(void);

	int	open(const char* loopName, int idx = -1);
	void	close(int idx);

	void	set(int idx, const char* file, int line);
	void	clear(int idx);

	void	print(FILE* f);

    private:
	const char* m_loopName[DebugPointsMax];
	const char* m_fileName[DebugPointsMax];
	int	    m_fileLine[DebugPointsMax];

	int	findFreeIndex(void);

	// блокировка на изменение данных
	std::mutex  m_lockIdx;
};

#define LockIdx()	m_lockIdx.lock()
#define unLockIdx()	m_lockIdx.unlock() 


// ------------------------------------------------------------------------------------------------
LogDebugPoint::LogDebugPoint(void)
{
    memset(m_loopName, 0, sizeof(m_loopName));
    memset(m_fileName, 0, sizeof(m_fileName));
    memset(m_fileLine, 0, sizeof(m_fileLine));
}


// ------------------------------------------------------------------------------------------------
int	LogDebugPoint::open(const char* loopName, int idx)
{
    if (idx < 0) {
	idx = findFreeIndex();
    }

    if (idx >= DebugPointsMax) {
	// все ошибочные индексы переходят на нулевой элемент, название цикла не изменяется
	LOG_ERR("Error! Указан недействительный индекс для создания контрольной точки");
	return 0;
    }

    m_loopName[idx] = loopName;
    return idx;
}

// ------------------------------------------------------------------------------------------------
void	LogDebugPoint::close(int idx)
{
    if (idx < 0 || idx >= DebugPointsMax) {
	LOG_ERR("Error! Указан недействительный индекс для закрытия контрольной точки");
	return;
    }

    m_fileLine[idx] = 0;
    m_fileName[idx] = nullptr;
    m_loopName[idx] = nullptr;
}

// ------------------------------------------------------------------------------------------------
void	LogDebugPoint::set(int idx, const char* file, int line)
{
    if (idx < 0 || idx >= DebugPointsMax) {
	return;
    }

    m_fileName[idx] = file;
    m_fileLine[idx] = line;
}

// ------------------------------------------------------------------------------------------------
void	LogDebugPoint::clear(int idx)
{
    if (idx < 0 || idx >= DebugPointsMax) {
	return;
    }

    m_fileName[idx] = nullptr;
    m_fileLine[idx] = 0;
}

// ------------------------------------------------------------------------------------------------
int	LogDebugPoint::findFreeIndex(void)
{
    int idx = -1;

    LockIdx();
    for (int i = DebugPointsMax - 1; i >= 0; i--) {
	if (!m_loopName[i] && !m_fileName[i] && !m_fileLine[i]) {
	    idx = i;
	    break;
	}
    }
    unLockIdx();

    if (idx < 0) {
	LOG_ERR("Error! Использованы все индексы контрольных точек");
	return 0;
    }

    return idx;
}

// ------------------------------------------------------------------------------------------------
void	LogDebugPoint::print(FILE* f)
{
    bool first = 1;
    for (int idx = 0; idx < DebugPointsMax; idx++) {
	if (m_fileName[idx] && m_fileLine[idx] > 0) {
	    if (first) {
		first = 0;
		fprintf(f, "Debug points: ");
	    }
	    else {
		fprintf(f, ", ");
	    }
	    fprintf(f, "{loop=%s, file=%s, line=%d}", m_loopName[idx], m_fileName[idx], m_fileLine[idx]);
	}
    }
    if (!first)
	fprintf(f, "; ");
}


// ************************************************************************************************
class Log {
    public:
	~Log();

	const char*	m_logDir = nullptr;
	const char*	m_scriptName = nullptr;

	// задание контрольных точек
	LogDebugPoint	m_DbgPoint;

	// сколько дней хранить логи
	int	m_days = 0;
	int	m_countLastClean = 0;

	bool	m_ena_stderr = 0;

	FILE*	logErrorOpenFile(const char* file, int line);
	FILE*	logDebugOpenFile(const char* file, int line);

    private:
	FILE*	m_fDebug = nullptr;
	int	m_currDay = 0;
	std::string m_fnDbg;
};

Log::~Log()
{
    if (m_fDebug) {
	fclose(m_fDebug);
	m_fDebug = nullptr;
    }
}

static Log g_logErr;


// ------------------------------------------------------------------------------------------------
FILE* Log::logErrorOpenFile(const char* file, int line)
{
    time_t t;
    int us;
    getCurrTime_us(t, us);

    struct tm T;
    localtime_r(&t, &T);

    char* fn = (char*)alloca((m_logDir ? strlen(m_logDir) : 2) + 20);
    if (!fn) return nullptr;

    sprintf(fn, "%s%02d%02d%02d_error.log", m_logDir ? m_logDir : "./", T.tm_year % 100, T.tm_mon + 1, T.tm_mday);

    FILE* f = fopen(fn, "a");
    if (!f) return nullptr;

    fprintf(f, "%02d:%02d:%02d.%06d\t", T.tm_hour, T.tm_min, T.tm_sec, us);
    printf_stderr("%02d:%02d:%02d.%06d\t", T.tm_hour, T.tm_min, T.tm_sec, us);
    if (m_scriptName && *m_scriptName)
	fprintf(f, "%s; ", m_scriptName);
    fprintf(f, "pid=%d; ", getpid());

    if (file && *file && line > 0) {
	fprintf(f, "file=%s, line=%d; ", file, line);
    }

    if (errno) {
        fprintf(f, "errno=%d (%s); ", errno, strerror(errno));
	printf_stderr("errno=%d (%s); ", errno, strerror(errno));
	errno = 0;
    }

    return f;
}


// ------------------------------------------------------------------------------------------------
// Добавляет запись в лог отладки
FILE* Log::logDebugOpenFile(const char* file, int line)
{
    time_t t;
    int us;
    getCurrTime_us(t, us);

    struct tm T;
    localtime_r(&t, &T);

    int day = T.tm_year % 100;
    day *= 100;
    day += T.tm_mon + 1;
    day *= 100;
    day += T.tm_mday;

    if (m_fDebug) {
	if (m_currDay != day || (m_fnDbg.size() > 0 && !fileCheck(m_fnDbg.c_str()))) {
	    fclose(m_fDebug);
	    m_fDebug = NULL;
	}
    }

    if (!m_fDebug) {
	char* fn = (char*)alloca((m_logDir ? strlen(m_logDir) : 2) + 20);
	if (!fn) return nullptr;

	sprintf(fn, "%s%02d%02d%02d_debug.log", m_logDir ? m_logDir : "./", T.tm_year % 100, T.tm_mon + 1, T.tm_mday);
	m_fnDbg = fn;

	m_fDebug = fopen(fn, "a");
	if (!m_fDebug) return nullptr;

	m_currDay = day;
    }

    fprintf(m_fDebug, "%02d:%02d:%02d.%06d\t", T.tm_hour, T.tm_min, T.tm_sec, us);
    printf_stderr("%02d:%02d:%02d.%06d\t", T.tm_hour, T.tm_min, T.tm_sec, us);
    if (m_scriptName && *m_scriptName)
	fprintf(m_fDebug, "%s; ", m_scriptName);
    fprintf(m_fDebug, "pid=%d; ", getpid());

    if (file && *file && line > 0) {
	fprintf(m_fDebug, "file=%s, line=%d; ", file, line);
    }

    if (errno) {
	fprintf(m_fDebug, "errno=%d (%s); ", errno, strerror(errno));
	printf_stderr("errno=%d (%s); ", errno, strerror(errno));
	errno = 0;
    }

    return m_fDebug;
}


// ************************************************************************************************
// Задать путь, где создавать лог ошибок (по умолчанию будет использован текущий путь)
void    logSetDir(const char* logPath, int m_days)
{
    if (logPath && *logPath && dirCheck_or_Create(logPath)) {
        g_logErr.m_logDir = logPath;
    }
    else {
	g_logErr.m_logDir = nullptr;
    }
    g_logErr.m_days = m_days;
    logClean();
    errno = 0;
}


// ------------------------------------------------------------------------------------------------
void    logClean_scanDirLogs(const char* dest, int days);

void    logClean(void)
{
    if (g_logErr.m_countLastClean > 0) {
	g_logErr.m_countLastClean--;
	return;
    }
    logClean_scanDirLogs(g_logErr.m_logDir, g_logErr.m_days);
    g_logErr.m_countLastClean = 24 * 60 * 60;
}


// ------------------------------------------------------------------------------------------------
// Управление выдачей ошибок в stdout
void    logStderr(bool enaStderr)
{
    g_logErr.m_ena_stderr = enaStderr;
}


// ------------------------------------------------------------------------------------------------
// Добавляет запись в лог ошибок
void    logError(const char* file, int line, const char* fmt, ...)
{
    FILE* f = g_logErr.logErrorOpenFile(file, line);
    if (f) {
	if (fmt && *fmt) {
	    va_list argp;
	    va_start(argp, fmt);
	    vfprintf(f, fmt, argp);
	    va_end(argp);
	    if (g_logErr.m_ena_stderr) {
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	    }
	}
	fprintf(f, "\n");
	printf_stderr_ln("\n");
	fclose(f);
    }
}


// ------------------------------------------------------------------------------------------------
static bool g_LogSignalEna = 1;

void    logSignalBlk(void)
{
    g_LogSignalEna = 0;
}

void    logSignal(const char* file, int line, int sig, bool wrDbgPoints, const char* fmt, ...)
{
    if (!g_LogSignalEna) return;

    FILE* f = g_logErr.logErrorOpenFile(file, line);
    if (f) {
	if (sig > 0) {
	    fprintf(f, "Signal %d (%s); ", sig, strsignal(sig));
	    printf_stderr("Signal %d (%s); ", sig, strsignal(sig));
	}

	if (wrDbgPoints)
	    g_logErr.m_DbgPoint.print(f);

	if (fmt && *fmt) {
	    va_list argp;
	    va_start(argp, fmt);
	    vfprintf(f, fmt, argp);
	    va_end(argp);
	    if (g_logErr.m_ena_stderr) {
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	    }
	}
	fprintf(f, "\n");
	printf_stderr_ln("\n");
	fclose(f);
    }
}


// ------------------------------------------------------------------------------------------------
void    logException(const char* file, int line, const char* fmt, ...)
{
    FILE* f = g_logErr.logErrorOpenFile(file, line);
    if (f) {
	fprintf(f, "Exeption!; ");
	printf_stderr("Exeption!; ");

	g_logErr.m_DbgPoint.print(f);

	if (fmt && *fmt) {
	    va_list argp;
	    va_start(argp, fmt);
	    vfprintf(f, fmt, argp);
	    va_end(argp);
	    if (g_logErr.m_ena_stderr) {
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	    }
	}
	fprintf(f, "\n");
	printf_stderr_ln("\n");
	fclose(f);
    }
}


// ------------------------------------------------------------------------------------------------
void    logDebug(const char* file, int line, const char* fmt, ...)
{
    FILE* f = g_logErr.logDebugOpenFile(file, line);
    if (f) {
	if (fmt && *fmt) {
	    va_list argp;
	    va_start(argp, fmt);
	    vfprintf(f, fmt, argp);
	    va_end(argp);
	    if (g_logErr.m_ena_stderr) {
		va_list argp;
		va_start(argp, fmt);
		vprintf(fmt, argp);
		va_end(argp);
	    }
	}
	fprintf(f, "\n");
	fflush(f);
	printf_stderr_ln("\n");
    }
}


void    logSetScriptName(const char* name)
{
    g_logErr.m_scriptName = name;
}



// ------------------------------------------------------------------------------------------------
// Создать контрольную точку
int    logDbgPoint_New(const char* loopName, int idx)
{
    return g_logErr.m_DbgPoint.open(loopName, idx);
}

// Удалить контрольную точку
void    logDbgPoint_Delete(int idx)
{
    g_logErr.m_DbgPoint.close(idx);
}

// Задать контрольную точку (например, перед вызовом функции или метода)
void    logDbgPoint_Set(int idx, const char* file, int line)
{
    g_logErr.m_DbgPoint.set(idx, file, line);
}

// Очистить контрольную точку (например, когда поток переходит в ожидание)
void    logDbgPoint_Clear(int idx)
{
    g_logErr.m_DbgPoint.clear(idx);
}
