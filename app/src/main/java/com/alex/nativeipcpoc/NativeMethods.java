package com.alex.nativeipcpoc;

/**
 * Created by Alex on 11/02/16.
 */
public class NativeMethods {

    public static native boolean startService();
    public static native byte[] getBuffer();

    static {
        System.loadLibrary("ipc_methods");
    }


}
