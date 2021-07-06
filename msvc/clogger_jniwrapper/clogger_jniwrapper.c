/**
 * com_github_jni_JNIWrapper.c
 */
#include "com_github_jni_JNIWrapper.h"

#include <clogger/clogger_api.h>


void JNICALL Java_com_github_jni_JNIWrapper_JNI_1clogger_1lib_1version(JNIEnv *env, jobject obj)
{
    const char *libname;
    const char * libversion = clogger_lib_version(&libname);

    printf("Java_com_github_jni_JNIWrapper_JNI_1clogger_1lib_1version: %s-%s\n", libname, libversion);
}