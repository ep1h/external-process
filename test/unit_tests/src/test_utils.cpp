/**-----------------------------------------------------------------------------
; @file test_utils.cpp
;
; @author ep1h
;-----------------------------------------------------------------------------*/
#include "test_utils.hpp"
#include <windows.h>

const char test_application[] = Q(EXTERNAL_PROCESS_SIMULATOR_NAME);
static HANDLE bkg_sim_process_handle = nullptr;

static uint32_t run_executable(const char *app, const char *args,
                               bool background)
{
    // std::string command;
    // if (background)
    // {
    //     command += "start /MIN ";
    // }
    // command += app;
    // command += " ";
    // command += args;
    // return system(command.c_str());

    DWORD result = 0;

    if (background)
    {
        if (bkg_sim_process_handle)
        {
            return result;
        }
    }

    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.hwnd = NULL;
    sei.lpVerb = "open";
    sei.lpFile = app;
    sei.lpParameters = args;
    sei.lpDirectory = NULL;
    sei.nShow = SW_HIDE;
    sei.hInstApp = NULL;
    ShellExecuteEx(&sei);
    if (!background)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        GetExitCodeProcess(sei.hProcess, &result);
    }
    else
    {
        bkg_sim_process_handle = sei.hProcess;
        Sleep(50);
    }
    return result;
}

uint32_t run_external_process_simulator(void)
{
    return run_executable(test_application, "loop", true);
}

uint32_t run_external_process_simulator(const char *arg)
{
    return run_executable(test_application, arg, false);
}

void terminate_external_process_simulator(void)
{
    // std::string command = "taskkill /IM ";
    // command += test_application;
    // system(command.c_str());

    if (bkg_sim_process_handle)
    {
        TerminateProcess(bkg_sim_process_handle, 9);
        bkg_sim_process_handle = nullptr;
    }
}
