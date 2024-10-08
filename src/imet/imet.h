/* Copyright (c) 2017, DF9DQ
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials provided with the
 * distribution.
 * Neither the name of the author nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __IMET_H
#define __IMET_H

#include "lpclib.h"


typedef struct IMET_Context *IMET_Handle;


LPCLIB_Result IMET_open (IMET_Handle *pHandle);
LPCLIB_Result IMET_processBlock (
        IMET_Handle handle,
        void *buffer,
        float rxSetFrequencyHz,
        float rxOffset,
        float rssi,
        uint64_t realTime);
void IMET_handleAudioCallback (int32_t *samples, int nSamples);
LPCLIB_Result IMET_resendLastPositions (IMET_Handle handle);
LPCLIB_Result IMET_removeFromList (IMET_Handle handle, uint32_t id, float *frequency);
LPCLIB_Result IMET_pauseResume (IMET_Handle handle, bool pause);
void IMET_selectDebugAudio (int debugAudioChannel);

#endif
