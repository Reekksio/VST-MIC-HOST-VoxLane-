#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

#pragma comment(lib, "dbghelp.lib")

class CrashLogInstaller
{
public:
    CrashLogInstaller()
    {
        AddVectoredExceptionHandler(
            1,
            &CrashLogInstaller::handleException);
    }

    static LONG WINAPI handleException(
        PEXCEPTION_POINTERS exceptionPointers)
    {
        if (s_logged)
            return EXCEPTION_CONTINUE_SEARCH;

        if (exceptionPointers == nullptr)
            return EXCEPTION_CONTINUE_SEARCH;

        const EXCEPTION_RECORD* record =
            exceptionPointers->ExceptionRecord;

        if (record == nullptr)
            return EXCEPTION_CONTINUE_SEARCH;

        const DWORD code =
            record->ExceptionCode;

        if (code != 0xC0000005u && code != 0xC0000409u)
            return EXCEPTION_CONTINUE_SEARCH;

        s_logged = true;

        char appData[MAX_PATH] = {};
        GetEnvironmentVariableA(
            "APPDATA",
            appData,
            MAX_PATH);

        char dir[MAX_PATH] = {};

        snprintf(
            dir,
            sizeof(dir),
            "%s\\VSTMicHost",
            appData);

        CreateDirectoryA(
            dir,
            nullptr);

        char path[MAX_PATH] = {};
        snprintf(
            path,
            sizeof(path),
            "%s\\%s",
            dir,
            "crashlog.txt");

        auto* file = fopen(
            path,
            "w");

        if (file == nullptr)
            return EXCEPTION_CONTINUE_SEARCH;

        fprintf(
            file,
            "code=0x%08X address=0x%llX\n",
            (unsigned int)code,
            (unsigned long long)(record->ExceptionAddress
                                     ? reinterpret_cast<ULONG_PTR>(
                                         record->ExceptionAddress)
                                     : 0));

        const CONTEXT* context =
            exceptionPointers->ContextRecord;

        fprintf(
            file,
            "rip=0x%llX rcx=0x%llX rdx=0x%llX rbx=0x%llX\n",
            (unsigned long long)context->Rip,
            (unsigned long long)context->Rcx,
            (unsigned long long)context->Rdx,
            (unsigned long long)context->Rbx);

        fprintf(
            file,
            "rsi=0x%llX rdi=0x%llX rbp=0x%llX rsp=0x%llX\n",
            (unsigned long long)context->Rsi,
            (unsigned long long)context->Rdi,
            (unsigned long long)context->Rbp,
            (unsigned long long)context->Rsp);

        fprintf(
            file,
            "r8=0x%llX r9=0x%llX r10=0x%llX r11=0x%llX\n",
            (unsigned long long)context->R8,
            (unsigned long long)context->R9,
            (unsigned long long)context->R10,
            (unsigned long long)context->R11);

        fprintf(
            file,
            "r12=0x%llX r13=0x%llX r14=0x%llX r15=0x%llX\n",
            (unsigned long long)context->R12,
            (unsigned long long)context->R13,
            (unsigned long long)context->R14,
            (unsigned long long)context->R15);

        STACKFRAME64 frame = {};
        frame.AddrPC.Mode =
            AddrModeFlat;
        frame.AddrStack.Mode =
            AddrModeFlat;
        frame.AddrFrame.Mode =
            AddrModeFlat;

        frame.AddrPC.Offset =
            context->Rip;
        frame.AddrStack.Offset =
            context->Rsp;
        frame.AddrFrame.Offset =
            context->Rbp;

        const HANDLE process =
            GetCurrentProcess();
        const HANDLE thread =
            GetCurrentThread();

        DWORD symOptions =
            SymGetOptions();

        symOptions |=
            SYMOPT_DEFERRED_LOADS;

        SymSetOptions(
            symOptions);

        SymInitialize(
            process,
            nullptr,
            TRUE);

        ULONG64 moduleBase =
            SymGetModuleBase64(
                process,
                context->Rip);

        fprintf(
            file,
            "moduleBase 0x%llX\n",
            (unsigned long long)moduleBase);

        for (int i = 0; i < 64; ++i)
        {
            if (!StackWalk64(
                    IMAGE_FILE_MACHINE_AMD64,
                    process,
                    thread,
                    &frame,
                    const_cast<CONTEXT*>(context),
                    nullptr,
                    SymFunctionTableAccess64,
                    SymGetModuleBase64,
                    nullptr))
                break;

            if (frame.AddrPC.Offset == 0)
                break;

            ULONG64 pc =
                frame.AddrPC.Offset;

            ULONG64 base =
                SymGetModuleBase64(
                    process,
                    pc);

            if (base == 0)
                base = moduleBase;

            if (pc < base)
                continue;

            std::aligned_storage_t<
                sizeof(SYMBOL_INFO) + 512 * sizeof(char),
                8>
                symbolStorage;

            SYMBOL_INFO& symbolInfo =
                *reinterpret_cast<SYMBOL_INFO*>(
                    &symbolStorage);

            symbolInfo.SizeOfStruct =
                sizeof(SYMBOL_INFO);
            symbolInfo.MaxNameLen =
                511;

            DWORD64 displacement =
                0;

            char name[512] = "";

            if (SymFromAddr(
                    process,
                    pc,
                    &displacement,
                    &symbolInfo) &&
                symbolInfo.Name != nullptr)
            {
                strncpy(
                    name,
                    symbolInfo.Name,
                    511);
            }

            char line[1024] = "";

            IMAGEHLP_LINE64 lineInfo = {};
            lineInfo.SizeOfStruct =
                sizeof(IMAGEHLP_LINE64);

            DWORD lineDisplacement =
                0;

            if (SymGetLineFromAddr64(
                    process,
                    pc,
                    &lineDisplacement,
                    &lineInfo) &&
                lineInfo.FileName != nullptr)
            {
                snprintf(
                    line,
                    sizeof(line),
                    "  %s(%lu)",
                    lineInfo.FileName,
                    lineInfo.LineNumber);
            }

            fprintf(
                file,
                "frame %02d rva 0x%08llX+0x%04llX %s%s\n",
                i,
                (unsigned long long)(
                    pc - base),
                (unsigned long long)displacement,
                name,
                line);
        }

        fclose(file);

        return EXCEPTION_CONTINUE_SEARCH;
    }

private:
    static volatile LONG s_logged;
};

volatile LONG CrashLogInstaller::s_logged = 0;

static CrashLogInstaller g_crashLogInstaller;
