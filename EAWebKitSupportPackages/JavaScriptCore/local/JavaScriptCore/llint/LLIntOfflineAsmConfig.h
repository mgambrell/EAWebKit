/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2015 Electronic Arts, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef LLIntOfflineAsmConfig_h
#define LLIntOfflineAsmConfig_h

#include "LLIntCommon.h"
#include <wtf/Assertions.h>
#include <wtf/InlineASM.h>

//MBG simplified this file to keep things sane. I want the cloop always
//I think thie file determines which configuration gets used... (maybe?)
//and definitely which configuration has the LLIntDesiredOffsets computed for it
//(which should be the one that's used anyway, so..)

//#if !ENABLE(JIT)
#define OFFLINE_ASM_C_LOOP 1
#define OFFLINE_ASM_X86 0
#define OFFLINE_ASM_X86_WIN 0
#define OFFLINE_ASM_ARM 0
#define OFFLINE_ASM_ARMv7 0
#define OFFLINE_ASM_ARMv7_TRADITIONAL 0
#define OFFLINE_ASM_ARM64 0
#define OFFLINE_ASM_X86_64 0
#define OFFLINE_ASM_X86_64_WIN 0
#define OFFLINE_ASM_ARMv7k 0
#define OFFLINE_ASM_ARMv7s 0
#define OFFLINE_ASM_MIPS 0
#define OFFLINE_ASM_SH4 0

#define OFFLINE_ASM_CPLOAD(reg)
#define OFFLINE_ASM_JSVALUE64 1
#define OFFLINE_ASM_BIG_ENDIAN 0
#define OFFLINE_ASM_GGC 1

#endif // LLIntOfflineAsmConfig_h