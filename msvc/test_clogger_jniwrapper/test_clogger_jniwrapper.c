/**
 * test_clogger_jniwrapper.c
 *   A test app using clogger_jniwrapper.
 *   https://blog.csdn.net/qq_32583189/article/details/53172316
 */
#include <common/mscrtdbg.h>
#include <common/cstrbuf.h>
#include <common/emerglog.h>

#include <com_github_jni_JNIWrapper.h>


static const char THIS_FILE[] = "test_clogger_jniwrapper.c";


HINSTANCE jvmdll = NULL;

JavaVM * jvm = NULL;
void   * jenv = NULL;


static void appexit_cleanup(void)
{
    if (jvm) {
        (*jvm)->DestroyJavaVM(jvm);
    }

    if (jvmdll) {
        FreeLibrary(jvmdll);
    }
}


static void get_env_var(const char *var, const char *prefix, char *value, size_t valuesz)
{
    errno_t err;
    size_t bsize;

    int prelen = 0;

    if (prefix) {
        prelen = snprintf_chkd_V1(value, valuesz, prefix);
        valuesz -= prelen;
    }

    err = getenv_s(&bsize, NULL, 0, var);
    if (err || !bsize) {
        fprintf(stderr, "failed to get env: %s\n", var);
        exit(-1);
    }

    if ((int)bsize >= (int)(valuesz)) {
        fprintf(stderr, "insufficent value buffer for: JAVA_HOME\n");
        exit(-1);
    }

    err = getenv_s(&bsize, &value[prelen], valuesz, var);
    if (err || !bsize || (int)bsize >= (int)valuesz) {
        fprintf(stderr, "error to get env: %s\n", var);
        exit(-1);
    }

    printf("${env:%s}=%.*s\n", var, (int)bsize, &value[prelen]);
}


void createJVM()
{
    typedef jint(JNICALL *ProcCreateJavaVM)(JavaVM **, void**, void *);

    jint res;

    ProcCreateJavaVM jvm_CreateJavaVM;

    char javahome[200];
    char jvmdllpath[260];
    char classpath[1200];

    JavaVMInitArgs vm_args;
    JavaVMOption options[30];

    memset(&vm_args, 0, sizeof(vm_args));
    memset(&options, 0, sizeof(options));

    get_env_var("JAVA_HOME", NULL, javahome, sizeof(javahome));
    get_env_var("CLASSPATH", "-Djava.class.path=", classpath, sizeof(classpath));

    snprintf_chkd_V1(jvmdllpath, sizeof(jvmdllpath), "%s\\jre\\bin\\server\\jvm.dll", javahome);

    // 注意: 务必使用动态载入 jvm.dll 方式调用 JNI_CreateJavaVM
    jvmdll = LoadLibraryA(jvmdllpath);
    if (!jvmdll) {
        printf("failed to load: %s\n", jvmdllpath);
        exit(-1);
    }
    printf("success load: %s\n", jvmdllpath);

    jvm_CreateJavaVM = (ProcCreateJavaVM) GetProcAddress(jvmdll, "JNI_CreateJavaVM");
    if (!jvm_CreateJavaVM) {
        printf("failed to GetProcAddress: JNI_CreateJavaVM\n");
        exit(-1);
    }

	options[0].optionString = "-Djava.compiler=NONE";
    options[1].optionString = classpath;
	options[2].optionString = "-verbose:jni";

    vm_args.version = JNI_VERSION_1_8;
    vm_args.ignoreUnrecognized = JNI_TRUE;
    vm_args.options = options;
    vm_args.nOptions = 3;

    // 务必关闭异常: (VS2015: Ctrl+Alt+E -> Win32 Exceptioins -> 0xc0000005 Access violation)
    //   https://stackoverflow.com/questions/36250235/exception-0xc0000005-from-jni-createjavavm-jvm-dll
	res = jvm_CreateJavaVM(&jvm, &jenv, &vm_args);
	if (res == 0) {
		printf("successfully created JVM.\n");
	} else {
		printf("failed to create JVM.\n");
        exit(-1);
	}
}


int main(int argc, char *argv[])
{
    WINDOWS_CRTDBG_ON

    createJVM();

    jobject obj = NULL;

    atexit(appexit_cleanup);

    Java_com_github_jni_JNIWrapper_JNI_1clogger_1lib_1version(jenv, obj);

    return 0;
}