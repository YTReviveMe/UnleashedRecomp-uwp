/*
    Various helpers for UWP apps, add a new function if you need to interop between a dll and UWP calls
*/
#include "pch.h"
#include "libuwp.h"

#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Graphics.Display.Core.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "SDL.h"

static int width = 0;
static int height = 0;

using namespace winrt::Windows;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Display::Core;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

void uwp_GetBundlePath(char* buffer)
{
    sprintf_s(buffer, 256, "%s", winrt::to_string(ApplicationModel::Package::Current().InstalledPath()).c_str());
}

void uwp_GetBundleFilePath(char* buffer, const char* filename)
{
    sprintf_s(buffer, 256, "%s\\%s", winrt::to_string(ApplicationModel::Package::Current().InstalledPath()).c_str(), filename);
}

void uwp_GetScreenSize(int* x, int* y)
{
    if (width == 0) {
        HdmiDisplayInformation hdi = HdmiDisplayInformation::GetForCurrentView();
        width = hdi.GetCurrentDisplayMode().ResolutionWidthInRawPixels();
        height = hdi.GetCurrentDisplayMode().ResolutionHeightInRawPixels();
    }

    *x = width;
    *y = height;
}

float uwp_GetRefreshRate()
{
    return HdmiDisplayInformation::GetForCurrentView().GetCurrentDisplayMode().RefreshRate();
}

void* uwp_GetWindowReference()
{
    return reinterpret_cast<void*>(winrt::get_abi(CoreWindow::GetForCurrentThread()));
}

void uwp_SetupHDR(bool enabled)
{
    auto hdi = HdmiDisplayInformation::GetForCurrentView();
    auto modes = hdi.GetSupportedDisplayModes();
    for (unsigned i = 0; i < modes.Size(); i++)
    {
        HdmiDisplayMode mode = modes.GetAt(i);

        if (enabled && mode.ColorSpace() == HdmiDisplayColorSpace::BT2020 && mode.RefreshRate() >= 59)
        {
            hdi.RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::Eotf2084);
            return;
        }
        else if (!enabled && mode.ColorSpace() == HdmiDisplayColorSpace::BT709 && mode.RefreshRate() >= 59)
        {
            hdi.RequestSetCurrentDisplayModeAsync(mode, HdmiDisplayHdrOption::EotfSdr);
            return;
        }
    }
}

void uwp_ProcessEvents()
{
    if (CoreWindow::GetForCurrentThread() != NULL) {
        //CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
        SDL_PumpEvents(); // This will call dispatcher process events and flush out all joystick events which are missing for some reason in this setup
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    }
}

// Register gamepad event hooks to method inside of calling program
void uwp_RegisterGamepadCallbacks(void (*callback)(void))
{
    namespace WGI = winrt::Windows::Gaming::Input;

    try
    {
        WGI::Gamepad::GamepadAdded([callback](auto&&, const WGI::Gamepad) {
            callback();
        });

        WGI::Gamepad::GamepadRemoved([callback](auto&&, const WGI::Gamepad) {
            callback();
        });
    }
    catch (winrt::hresult_error)
    {
    }

}