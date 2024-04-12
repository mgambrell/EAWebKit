//hmmm.. problem.. the function pointer declarations will be mixed up for different GL versions (maybe?)
//I will end up having to make my own function pointer typedefs
//for example.. typedef _GLES2_PFNGLGETERRORPROC
//Then again, maybe the point of these old statically linked apis is that they're always the same... well, I guess we'll find out
//if I ever put more than one API in here

#include "GLPipe.h"

#include "EGL/egl.h"
#include "GLES2/gl2.h"

#define GLPIPE_DO_EGL
#define GLPIPE_DO_GLES2

//we don't use the 'ns' prefix in here.. because..... the #includes of gl2.h etc above have #defines which mess up the proclist
//bleh.. i can fix that by putting all the gl2.h #defines in a separate section and disabling it in this file
//but I dont think I need to

//declare the function pointers
#define GLPIPE_PROCENT(ns,fntype,name,rettype,...) fntype name;
#include "GLPIPE_proclist.inc"
#undef GLPIPE_PROCENT

void GLPipe_SetProcs(GLPipe_Procs* procs)
{
	//we could have set things up to use a global 'procs' pointer and have all the #defines in the headers use it,
	//e.g. #define glEnable _glpipe_._gles2_glEnable
	//but.. I didn't.

	#define GLPIPE_PROCENT(ns,fntype,name,rettype,...) name = (fntype)procs->name;
	#include "GLPIPE_proclist.inc"
	#undef GLPIPE_PROCENT
}

////REMINDER OF HOW TO GENERATE LIST ON CLIENT SIDE
//#define GLPIPE_DO_GLES2
//#define GLPIPE_PROCENT(ns,fntype,name,rettype,...) GLFUNC_EXTERN rettype name __VA_ARGS__;
//#include "GLPIPE_proclist.inc"
//#undef GLPIPE_PROCENT
