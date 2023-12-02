#include <windows.h>
#include "test_utils.hpp"
#include "../../shared_memory/src/shared_memory.hpp"

static HANDLE bkg_sim_process_handle_ = nullptr;

uint32_t run_executable(const char* app, const char* args, bool background)
{
    DWORD result = 0;
    if (background)
    {
        if (bkg_sim_process_handle_)
        {
            return 1;
        }
    }
    SHELLEXECUTEINFO sei = {};
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
        bkg_sim_process_handle_ = sei.hProcess;
        Sleep(1000);
    }
    return result;
}

bool terminate_external_process_simulator(void)
{
    if (!bkg_sim_process_handle_)
    {
        return false;
    }
    TerminateProcess(bkg_sim_process_handle_, 9);
    bkg_sim_process_handle_ = nullptr;
    return true;
}

const SharedData get_sim_info(void)
{
    SharedMemory<SharedData> shared_data("ExternalProcessSimulatorSharedData");
    return *shared_data.get();
}
