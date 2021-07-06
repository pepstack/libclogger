/**
 * JNIWrapper.java
 *
 *   $ cd $projectdir/jni/src/main/java
 *   $ javah -classpath . -jni com.github.jni.JNIWrapper
 */
package com.github.jni;

public class JNIWrapper {

	public static String getJarPath(Class clazz) {
        String path = clazz.getProtectionDomain().getCodeSource().getLocation().getFile();
        try {
            path = java.net.URLDecoder.decode(path, "UTF-8");
            java.io.File jarFile = new java.io.File(path);
            return java.net.URLDecoder.decode(jarFile.getParentFile().getAbsolutePath(), "UTF-8");
        } catch (java.io.UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    }

    public static String getParentPath(String spath) {
        try {
            String path = java.net.URLDecoder.decode(spath, "UTF-8");
            java.io.File file = new java.io.File(path);
            return java.net.URLDecoder.decode(file.getParentFile().getAbsolutePath(), "UTF-8");
        } catch (java.io.UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return null;
    }

	public static String concatPaths(String path, String...paths) {
        final String sep = System.getProperty("file.separator");

        int i, j, found;
        StringBuffer sb = new StringBuffer(path);

        for (String p : paths) {
            if (p != null && p.length() > 0 && ! p.equals(sep)) {
                found = 1;

                if (sb.length() >= sep.length()) {
                    for (j = 0; j < sep.length(); j++) {
                        i = sb.length() - sep.length() + j;
                        if (sep.charAt(j) != sb.charAt(i)) {
                            found = 0;
                            break;
                        }
                    }
                } else {
                    found = 0;
                }

                if (found != 1) {
                    sb.append(sep);
                }

                sb.append(p);
            }
        }

        return sb.toString();
    }


	static {
		// https://blog.csdn.net/forandever/article/details/5983846
		//   java.library.path=/path/to/clogger_jniwrapper.dll
        //System.loadLibrary("clogger_jniwrapper");
		
		final String osName = System.getProperty("os.name");
		final String osVer = System.getProperty("os.version");
		final String osArch = System.getProperty("os.arch");
		final String archBits = System.getProperty("sun.arch.data.model");

		if (osName.indexOf("Windows") != -1) {
			// win86|win64, Debug|Release
			String buildConfig;
			if (archBits.equals("64")) {
				buildConfig = JNIWrapper.concatPaths("win64", "Release");
			} else {
				buildConfig = JNIWrapper.concatPaths("win86", "Release");
			}

			final String jniLibsPrefix = JNIWrapper.concatPaths(JNIWrapper.getParentPath(JNIWrapper.getJarPath(JNIWrapper.class)), "libs", buildConfig);

			System.load(JNIWrapper.concatPaths(jniLibsPrefix, "libclogger_dll.dll"));
			System.load(JNIWrapper.concatPaths(jniLibsPrefix, "clogger_jniwrapper.dll"));
		} else if (osName.indexOf("Linux") != -1) {
			// TODO:
		} else {
			// TODO:
		}
	}

    public native void JNI_clogger_lib_version();


    public static void main(String[] args) {
        JNIWrapper jniCall = new JNIWrapper();

		jniCall.JNI_clogger_lib_version();
    }
}