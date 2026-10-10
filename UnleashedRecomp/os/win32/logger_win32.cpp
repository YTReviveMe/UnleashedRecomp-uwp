#include <os/logger.h>
#include <os/process.h>
#ifdef UNLEASHED_RECOMP_UWP
#include <user/paths.h>
#endif

#define FOREGROUND_WHITE  (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)

static HANDLE g_hStandardOutput;
#ifdef UNLEASHED_RECOMP_UWP
static std::ofstream g_logFile;
static std::mutex g_logMutex;
static std::filesystem::path g_logPath;
#endif

void os::logger::Init()
{
    g_hStandardOutput = GetStdHandle(STD_OUTPUT_HANDLE);
}

void os::logger::Log(const std::string_view str, ELogType type, const char* func)
{
#ifdef UNLEASHED_RECOMP_UWP
    std::scoped_lock logLock(g_logMutex);
    if (IsUserPathReady())
    {
        const auto logPath = GetLogPath() / "UnleashedRecompiled.log";
        if (logPath != g_logPath)
        {
            g_logFile.close();
            g_logFile.open(logPath, std::ios::app);
            g_logPath = logPath;
        }
    }

    if (g_logFile.is_open())
    {
        if (func)
            g_logFile << '[' << func << "] " << str << '\n';
        else
            g_logFile << str << '\n';
        g_logFile.flush();
    }
#endif

    if (!os::process::g_consoleVisible)
        return;

    switch (type)
    {
        case ELogType::Utility:
            SetConsoleTextAttribute(g_hStandardOutput, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            break;

        case ELogType::Warning:
            SetConsoleTextAttribute(g_hStandardOutput, FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
            break;

        case ELogType::Error:
            SetConsoleTextAttribute(g_hStandardOutput, FOREGROUND_RED | FOREGROUND_INTENSITY);
            break;

        default:
            SetConsoleTextAttribute(g_hStandardOutput, FOREGROUND_WHITE);
            break;
    }

    if (func)
    {
        fmt::println("[{}] {}", func, str);
    }
    else
    {
        fmt::println("{}", str);
    }

    SetConsoleTextAttribute(g_hStandardOutput, FOREGROUND_WHITE);
}
