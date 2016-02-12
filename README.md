# AndroidNativeService-knows-ASHMEM

This is a very simple project dealing with native binder communication and ashmem (Android'shared memory mechanism)

In particular, this project consists on two elements. A Java application including a couple of native methods and a native android binary program.

When the application is executed, a native service (myNative_service) starts listening for requests. The service supports a single call 'shareFD(int fd)'. The purpose of this call is share a file descriptor associated with a shared memory region created in the clients. Once the shareFD is called and the file descriptor is available in the service 

When the binary program (native_examples) is executed, it allocates a region in ashmem, writes 2048 times the a character on it, and shares its file descriptor with the server using the shareFD call. Then the service reads the region pointed by the file descriptor and passes it to upper java layer using the `getBuffer()` method.  

Most of the IPC code (and the idea of sharing the same cpp file for library and executable) is taken from vecio's and gburca examples:

https://github.com/vecio/AndroidIPC/blob/master/jni/BinderSHM.cpp

https://github.com/gburca/BinderDemo/blob/master/binder.cpp


#How to compile and run the example

First of all you should compile the code (no shit!) using android studio (AS). You will need to modify the nkdbuild label in build.gradle in order to make it point the location of Android NDK in your system, and then build/make the project. Compiling apps using native components with AS can be a little bit tricky, so google is your friend.  

Once the app is compiled, install it on a rooted device (sadly root is needed for adding native services). Then upload the binary in $(PROJECT)/app/src/main/java/libs/armeabi-v7a/native_client to your device using `adb push <path 2 binary> /data/local/tmp`.

Now, if you run the app, it will start the native service. You can check if the service is running by executing  `adb shell service list|grep myNative_service`. If you tap the button labeled "CONSUME BUFFER" nothing will happen since no client has communicated with the listening service.

First you need to run the executable you have uploaded in previous step.   for doing so, open an adb shell cd to /data/local/tmp then just run the file with `./native_examples`. This will lead the client to allocate a region in ashmem and share the proper file descriptor with the server. 

Finally, if you tap the button in main activity, the log will show a 2048 bytes string. This string has been written by the native executable in the ashmem region and shared using native IPC. 
