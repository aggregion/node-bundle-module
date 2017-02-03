// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once
#include "CppUnitTest.h"
#include "ppltasks.h"

namespace HiloTests
{
    class TestHelper
    {
    public:
        static void RunUISynced(std::function<void ()>&& action);

        template <typename T>
        static T RunSynced(concurrency::task<T> t, concurrency::task_status& status, bool pump = true)
        {
            Windows::UI::Core::CoreWindow^ wnd = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
            Windows::UI::Core::CoreDispatcher^ dispatcher = wnd->Dispatcher;

            HANDLE hEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if (hEvent == NULL)
            {
                throw std::bad_alloc();
            }

            T result;
            status = concurrency::not_complete;
            t.then([&hEvent, &status, &result](concurrency::task<T> previousTask){
                try
                {
                    result = previousTask.get();
                    status = concurrency::completed;
                } 
                catch(...)
                {
                    status = concurrency::canceled;
                }
                SetEvent(hEvent);
            }, concurrency::task_continuation_context::use_arbitrary());

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

            return result;
        }

        template<>
        static void RunSynced<void>(concurrency::task<void> t, concurrency::task_status& status, bool pump)
        {
            Windows::UI::Core::CoreWindow^ wnd = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
            Windows::UI::Core::CoreDispatcher^ dispatcher = wnd->Dispatcher;

            HANDLE hEvent = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
            if (hEvent == NULL)
            {
                throw std::bad_alloc();
            }

            status = concurrency::not_complete;
            t.then([&hEvent, &status](concurrency::task<void> previousTask){
                try
                {
                    previousTask.get();
                    status = concurrency::completed;
                } 
                catch(...)
                {
                    status = concurrency::canceled;
                }
                SetEvent(hEvent);
            }, concurrency::task_continuation_context::use_arbitrary());

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

            return;
        }

    private:
        template <typename T>
        static concurrency::task_status TaskWait(concurrency::task<T>& t, bool pump = true);
    };
};