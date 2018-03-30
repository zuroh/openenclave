// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#if defined(__linux__)
#define __OE_NEED_TIME_CALLS
#endif

#include <assert.h>
#include <stdio.h>

#if defined(__linux__)
#include <linux/futex.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif

#include <openenclave/host.h>

#include <openenclave/bits/calls.h>
#include <openenclave/bits/utils.h>
#include "enclave.h"
#include "ocalls.h"
#include "quote.h"

void HandleMalloc(uint64_t argIn, uint64_t* argOut)
{
    if (argOut)
        *argOut = (uint64_t)malloc(argIn);
}

void HandleRealloc(uint64_t argIn, uint64_t* argOut)
{
    OE_ReallocArgs* args = (OE_ReallocArgs*)argIn;

    if (args)
    {
        if (argOut)
            *argOut = (uint64_t)realloc(args->ptr, args->size);
    }
}

void HandleFree(uint64_t arg)
{
    free((void*)arg);
}

void HandlePuts(uint64_t argIn)
{
    const char* str = (const char*)argIn;

    if (str)
        puts(str);
}

void HandlePrint(uint64_t argIn)
{
    OE_PrintArgs* args = (OE_PrintArgs*)argIn;

    if (args)
    {
        if (args->device == 0)
        {
            fprintf(stdout, "%s", args->str);
            fflush(stdout);
        }
        else if (args->device == 1)
        {
            fprintf(stderr, "%s", args->str);
            fflush(stderr);
        }
    }
}

void HandlePutchar(uint64_t argIn)
{
    int c = (int)argIn;
    putchar(c);
}

void HandleThreadWait(OE_Enclave* enclave, uint64_t argIn)
{
    const uint64_t tcs = argIn;
    EnclaveEvent* event = GetEnclaveEvent(enclave, tcs);
    assert(event);

#if defined(__linux__)

    if (__sync_fetch_and_add(&event->value, -1) == 0)
        syscall(__NR_futex, &event->value, FUTEX_WAIT, -1, NULL, NULL, 0);

#elif defined(_WIN32)

    WaitForSingleObject(event->handle, INFINITE);

#endif
}

void HandleThreadWake(OE_Enclave* enclave, uint64_t argIn)
{
    const uint64_t tcs = argIn;
    EnclaveEvent* event = GetEnclaveEvent(enclave, tcs);
    assert(event);

#if defined(__linux__)

    if (__sync_fetch_and_add(&event->value, 1) != 0)
        syscall(__NR_futex, &event->value, FUTEX_WAKE, 1, NULL, NULL, 0);

#elif defined(_WIN32)

    SetEvent(event->handle);

#endif
}

void HandleThreadWakeWait(OE_Enclave* enclave, uint64_t argIn)
{
    OE_ThreadWakeWaitArgs* args = (OE_ThreadWakeWaitArgs*)argIn;

    if (!args)
        return;

#if defined(__linux__)

    HandleThreadWake(enclave, (uint64_t)args->waiter_tcs);
    HandleThreadWait(enclave, (uint64_t)args->self_tcs);

#elif defined(_WIN32)

    HandleThreadWake(enclave, (uint64_t)args->waiter_tcs);
    HandleThreadWait(enclave, (uint64_t)args->self_tcs);

#endif
}

void HandleInitQuote(uint64_t argIn)
{
    OE_InitQuoteArgs* args = (OE_InitQuoteArgs*)argIn;

    if (!args)
        return;

    args->result = SGX_InitQuote(&args->targetInfo, &args->epidGroupID);
}

void HandleGetRemoteReport(OE_Enclave* enclave, uint64_t argIn)
{
    OE_GetRemoteReportArgs* args = (OE_GetRemoteReportArgs*)argIn;
    if (!args)
        return;

    args->result = OE_GetReport(
        enclave,
        OE_REPORT_OPTIONS_REMOTE_ATTESTATION,
        args->reportData,
        args->reportDataSize,
        NULL,
        0,
        args->reportBuffer,
        &args->reportBufferSize);
}

#if defined(__OE_NEED_TIME_CALLS)
void HandleStrftime(uint64_t argIn)
{
    OE_StrftimeArgs* args = (OE_StrftimeArgs*)argIn;

    if (!args)
        return;

    args->ret = strftime(args->str, sizeof(args->str), args->format, &args->tm);
}
#endif

#if defined(__OE_NEED_TIME_CALLS)
void HandleGettimeofday(uint64_t argIn)
{
    OE_GettimeofdayArgs* args = (OE_GettimeofdayArgs*)argIn;

    if (!args)
        return;

    args->ret = gettimeofday(args->tv, args->tz);
}
#endif

#if defined(__OE_NEED_TIME_CALLS)
void HandleClockgettime(uint64_t argIn)
{
    OE_ClockgettimeArgs* args = (OE_ClockgettimeArgs*)argIn;

    if (!args)
        return;

    args->ret = clock_gettime(args->clk_id, args->tp);
}
#endif

#if defined(__OE_NEED_TIME_CALLS)
void HandleNanosleep(uint64_t argIn)
{
    OE_NanosleepArgs* args = (OE_NanosleepArgs*)argIn;

    if (!args)
        return;

    args->ret = nanosleep(args->req, args->rem);
}
#endif
