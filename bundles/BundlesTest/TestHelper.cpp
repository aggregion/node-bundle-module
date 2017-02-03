// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
#include "TestHelper.h"
using namespace HiloTests;
using namespace Platform;
using namespace concurrency;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void TestHelper::RunUISynced(std::function<void ()>&& action)
{
    Windows::UI::Core::CoreWindow^ wnd = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;

    if (wnd != nullptr)
    {
        Platform::String^ errorMessage = nullptr;

        auto callback = ref new Windows::UI::Core::DispatchedHandler(
            [&errorMessage, action]()
        {
            try 
            {
                action();
            } 
            catch (Platform::Exception^ ex) 
            {
                errorMessage = ex->Message;
            } 
            catch (...) 
            {
                errorMessage = ref new Platform::String(L"Exception occured in code");
            }

        });

        auto uiTask = create_task(wnd->Dispatcher->RunAsync(
            Windows::UI::Core::CoreDispatcherPriority::Normal,
            callback));
        task_status status = TaskWait<void>(uiTask);
        
        if (status == not_complete)
        {
            Assert::Fail(L"Task did not complete");
        }

        if (errorMessage != nullptr) 
        {
            Assert::Fail(errorMessage->Data());
        }
    }
}

template<typename T>
task_status TestHelper::TaskWait(task<T>& t, bool pump)
{
    Windows::UI::Core::CoreWindow^ wnd = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
    Windows::UI::Core::CoreDispatcher^ dispatcher = wnd->Dispatcher;

    HANDLE hEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, WRITE_OWNER | EVENT_ALL_ACCESS);
    if (hEvent == NULL)
    {
        throw std::bad_alloc();
    }

    task_status status = not_complete;
    t.then([&hEvent, &status](task<T> result){
        try
        {
            result.get();
            status = completed;
        }
        catch(...)
        {
            status = canceled;
        }
        SetEvent(hEvent);
    });

    if (pump)
    {
        // Spin wait and exercise message pump
        DWORD waitResult = STATUS_PENDING;
        while(waitResult != WAIT_OBJECT_0)
        {
            dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
            waitResult = WaitForSingleObjectEx(hEvent, 0, true);
        }
    }
    else
    {
        WaitForSingleObjectEx(hEvent, INFINITE, true);
    }
    CloseHandle(hEvent);

    return status;
}