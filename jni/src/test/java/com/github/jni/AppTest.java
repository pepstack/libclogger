package com.github.jni;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Unit test for simple App.
 */
public class AppTest 
    extends TestCase
{
    /**
     * Create the test case
     *
     * @param testName name of the test case
     */
    public AppTest( String testName )
    {
        super( testName );
    }

    /**
     * @return the suite of tests being tested
     */
    public static Test suite()
    {
        return new TestSuite( AppTest.class );
    }

    /**
     * Rigourous Test :-)
     */
    public void testApp()
    {
		System.out.println("java.class.path=" + System.getProperty("java.class.path"));
		System.out.println("java.library.path=" + System.getProperty("java.library.path"));

		JNIWrapper jniCall = new JNIWrapper();

		jniCall.JNI_clogger_lib_version();

        assertTrue( true );
    }
}