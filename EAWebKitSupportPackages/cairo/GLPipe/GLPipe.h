#pragma once

//a structure that can convey all the function pointers
//we declare them as void* to keep this interface slim by avoiding header pollution
//take care to set up the functions properly...
struct GLPipe_Procs
{
	#define GLPIPE_DO_EGL
	#define GLPIPE_DO_GLES2
	#define GLPIPE_PROCENT(ns,a,b) void* ns##b;
	#include "GLPIPE_proclist.inc"
	#undef GLPIPE_PROCENT
	#undef GLPIPE_DO_GLES2
	#undef GLPIPE_DO_EGL
};

//sets the procs from the given struct
void GLPipe_SetProcs(GLPipe_Procs* procs);
