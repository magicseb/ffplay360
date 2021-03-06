/*
 * log functions
 * Copyright (c) 2003 Michel Bardiaux
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * logging functions
 */

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <stdlib.h>
#include "avutil.h"
#include "log.h"

static int av_log_level = AV_LOG_INFO;
static int flags;

#if defined(_WIN32) && !defined(__MINGW32CE__)
#ifndef _XBOX
#include <windows.h>
#else
#include <xtl.h>
#endif

static const uint8_t color[] = {12,12,12,14,7,7,7};

#define set_color(x)  fprintf(stderr, "\033[%d;3%dm", color[x]>>4, color[x]&15)
#define reset_color() fprintf(stderr, "\033[0m")

static int16_t background, attr_orig;
static HANDLE con;

#ifndef _XBOX
#define set_color(x)  SetConsoleTextAttribute(con, background | color[x])
#define reset_color() SetConsoleTextAttribute(con, attr_orig)
#endif
#else
static const uint8_t color[]={0x41,0x41,0x11,0x03,9,9,9};
#define set_color(x)  fprintf(stderr, "\033[%d;3%dm", color[x]>>4, color[x]&15)
#define reset_color() fprintf(stderr, "\033[0m")
#endif

static int use_color=-1;

#ifdef _XBOX
#undef _WIN32
#endif

#undef fprintf

#ifdef _XBOX
static void colored_fputs(int level, const char *str){
    if(use_color<0){
#if HAVE_ISATTY && !defined(_WIN32)
        use_color= getenv("TERM") && !getenv("NO_COLOR") && isatty(2);
#else
        use_color= 0;
#endif

    }

    if(use_color){
        set_color(level);
    }
    fputs(str, stderr);
    if(use_color){
        reset_color();
    }
}
#endif
const char* av_default_item_name(void* ptr){
    return (*(AVClass**)ptr)->class_name;
}

static void sanitize(uint8_t *line){
    while(*line){
        if(*line < 0x08 || (*line > 0x0D && *line < 0x20))
            *line='?';
        line++;
    }
}

void av_log_default_callback(void* ptr, int level, const char* fmt, va_list vl)
{
    static int print_prefix=1;
    static int count;
    static char prev[1024], line[1024];
    static int is_atty;
    AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
    if(level>av_log_level)
        return;
 
#undef fprintf
    if(print_prefix && avc) {
        snprintf(line, sizeof(line), "[%s @ %p]", avc->item_name(ptr), ptr);
    }else
        line[0]=0;

    vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    print_prefix= line[strlen(line)-1] == '\n';
    if(print_prefix && !strcmp(line, prev)){
        count++;
        return;
    }
    if(count>0){
        fprintf(stderr, "    Last message repeated %d times\n", count);
        count=0;
    }
    colored_fputs(color[av_clip(level>>3, 0, 6)], line);
    strcpy(prev, line);

}

static void (*av_log_callback)(void*, int, const char*, va_list) = av_log_default_callback;

void av_log(void* avcl, int level, const char *fmt, ...)
{
#if defined(_CONSOLE)
    AVClass* avc= avcl ? *(AVClass**)avcl : NULL;
    va_list vl;
    va_start(vl, fmt);
    if(avc && avc->version >= (50<<16 | 15<<8 | 2) && avc->log_level_offset_offset && level>=AV_LOG_FATAL)
        level += *(int*)(((uint8_t*)avcl) + avc->log_level_offset_offset);
    av_vlog(avcl, level, fmt, vl);
    va_end(vl);
#endif
}

void av_vlog(void* avcl, int level, const char *fmt, va_list vl)
{
    av_log_callback(avcl, level, fmt, vl);
}

int av_log_get_level(void)
{
    return av_log_level;
}

void av_log_set_level(int level)
{
    av_log_level = level;
}

void av_log_set_flags(int arg)
{
    flags= arg;
}

void av_log_set_callback(void (*callback)(void*, int, const char*, va_list))
{
    av_log_callback = callback;
}
