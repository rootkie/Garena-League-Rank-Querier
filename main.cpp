#include<vector>
#include<Windows.h>
#include<TlHelp32.h>
#include <iostream>
#include <winternl.h>

int getProcessPID(const wchar_t *procName)
{
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32FirstW(snapshot, &entry))
    {
         while (Process32NextW(snapshot, &entry) == TRUE)
        {
            for (;;)
            {
                if (!_wcsicmp(entry.szExeFile, procName))
                {
                    CloseHandle(snapshot);
                    return entry.th32ProcessID;
                }
                if (!Process32NextW(snapshot, &entry))
                    break;
            }
        }
    }

    CloseHandle(snapshot);
    return 0;
}

wchar_t *getCMDline(DWORD pid, wchar_t *cmdline) 
{
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    PROCESS_BASIC_INFORMATION pbi;
    PEB peblk;
    RTL_USER_PROCESS_PARAMETERS procParam;
    if (!hProcess) {
        puts("Failed to obtain handle");
        return 0;
    }
    NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    printf("peb base addr: %lx, pid: %d\n", pbi.PebBaseAddress, pbi.UniqueProcessId);
    
    if (ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peblk, sizeof(PEB), NULL) == 0)
    {
        puts("read peb failed");
        return 0;
    }
    if (ReadProcessMemory(hProcess, peblk.ProcessParameters, &procParam, sizeof(RTL_USER_PROCESS_PARAMETERS), NULL) == 0)
    {
        puts("read proc param failed");
        return 0;
    };
    printf ("cli length %d\n", procParam.CommandLine.Length);
    std::vector<wchar_t> buffer;
    buffer.resize(procParam.CommandLine.Length);
    SIZE_T bytes_read = 0;
    ReadProcessMemory(hProcess, procParam.CommandLine.Buffer, &buffer[0], procParam.CommandLine.Length, &bytes_read);
    wprintf (L"bytes read %d\n", bytes_read);
    wcsncpy(cmdline, &buffer[0], bytes_read);

    return cmdline;
}

void parseTokenPort(wchar_t *cmdline, wchar_t *token, wchar_t *port)
{
    const wchar_t token_flag[] = L"--remoting-auth-token=";
    const wchar_t port_flag[] = L"--app-port=";
    wchar_t *pwc;
    // wprintf(L"%ls\n", cmdline);
    pwc = wcsstr(cmdline, token_flag);
    wcsncpy(token, pwc+sizeof(token_flag)/sizeof(wchar_t)-1, 22);
    token[22] = '\0';
    
    pwc = wcsstr(cmdline, port_flag);
    wcsncpy(port, pwc+sizeof(port_flag)/sizeof(wchar_t)-1, 5);
    port[5] = '\0';
    
    return;
}

int main()
{
    DWORD pid = getProcessPID(L"LeagueClientUx.exe");
    // DWORD pid = GetCurrentProcessId();
    std::cout << "Pid of target process is: " << pid << std::endl;

    if (!pid) {
        std::cout << "Process not running" << std::endl;
        return 0;
    }
    wchar_t cmdline[4096];
    if (!getCMDline(pid, cmdline)){
        puts("failed to get cmdline");
        return 0;
    }
    
    wchar_t token[23];
    wchar_t port[6];
    parseTokenPort(cmdline, token, port);
    wprintf(L"token %ls\nport %ls\n", token, port);

    return 0;
}