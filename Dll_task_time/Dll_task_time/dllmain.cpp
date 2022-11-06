#define _WIN32_DCOM

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <comdef.h>
#include <taskschd.h>
#include "tlhelp32.h"
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

using namespace std;

int login(HANDLE han, LPWSTR time, LPWSTR filename)
{

    HRESULT hello = CoInitializeEx(NULL, COINIT_MULTITHREADED);


    hello = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);


    string wszTaskName = "WindowsWatchDog";

    wstring time_period = L"PT" + wstring(time) + L"M";

    ITaskService* pService = NULL;
    hello = CoCreateInstance(CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void**)&pService);

    hello = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());

    ITaskFolder* pRootFolder = NULL;
    hello = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

    pRootFolder->DeleteTask(_bstr_t(wszTaskName.c_str()), 0);

    ITaskDefinition* pTask = NULL;
    hello = pService->NewTask(0, &pTask);

    pService->Release();

    IRegistrationInfo* pRegInfo = NULL;
    hello = pTask->get_RegistrationInfo(&pRegInfo);

    hello = pRegInfo->put_Author(_bstr_t(L"Administrator"));
    pRegInfo->Release();

    ITriggerCollection* pTriggerCollection = NULL;
    hello = pTask->get_Triggers(&pTriggerCollection);

    ITrigger* pTrigger = NULL;
    hello = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pTrigger);
    pTriggerCollection->Release();

    IDailyTrigger* pDailyTrigger = NULL;
    hello = pTrigger->QueryInterface(
        IID_IDailyTrigger, (void**)&pDailyTrigger);
    pTrigger->Release();

    hello = pDailyTrigger->put_Id(_bstr_t(L"Trigger1"));

    hello = pDailyTrigger->put_StartBoundary(_bstr_t(L"2005-01-01T00:00:00"));

    hello = pDailyTrigger->put_EndBoundary(_bstr_t(L"2037-05-02T12:05:00"));

    hello = pDailyTrigger->put_DaysInterval((short)1);

    IRepetitionPattern* pRepetitionPattern = NULL;
    hello = pDailyTrigger->get_Repetition(&pRepetitionPattern);
    pDailyTrigger->Release();

    hello = pRepetitionPattern->put_Duration(_bstr_t(L"PT24H"));

    hello = pRepetitionPattern->put_Interval(_bstr_t(time_period.c_str()));
    pRepetitionPattern->Release();
  
    IActionCollection* pActionCollection = NULL;

    hello = pTask->get_Actions(&pActionCollection);

    IAction* pAction = NULL;
    hello = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();

    IExecAction* pExecAction = NULL;
    hello = pAction->QueryInterface(
        IID_IExecAction, (void**)&pExecAction);
    pAction->Release();

    hello = pExecAction->put_Path(filename);
    pExecAction->Release();

    IRegisteredTask* pRegisteredTask = NULL;
    hello = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName.c_str()),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t("SYSTEM"),
        _variant_t(),
        TASK_LOGON_PASSWORD,
        _variant_t(L""),
        &pRegisteredTask);
    if (FAILED(hello))
    {
        WriteConsole(han, L"\nError saving the Task", 23, new DWORD, 0);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    WriteConsole(han, L"Success", 8, new DWORD, 0);

    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();
    return 0;
}



extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPTSTR lpCmdLine,
    int nCmdShow
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:;
        int pid;
        HANDLE h;
        PROCESSENTRY32 pe;
        HANDLE han;
        h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        pid = GetCurrentProcessId();
        pe.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(h, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    AttachConsole(pe.th32ParentProcessID);
                }
            } while (Process32Next(h, &pe));
        }
        han = GetStdHandle(STD_OUTPUT_HANDLE);
        CloseHandle(h);

        LPWSTR* szArgList;
        int argCount;

        szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
        if (argCount != 4)
        {
            WriteConsole(han, L"rundll.exe Dll_task_time,DllMain time filename", 47, new DWORD, 0);
            return 1;
        }
        login(han, szArgList[2], szArgList[3]);
        LocalFree(szArgList);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
