#include <jni.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <utils/TextOutput.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/RefBase.h>
#include <utils/Log.h>
#include <utils/TextOutput.h>
#include <utils/ashmem.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/Binder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#undef LOG_TAG
#define LOG_TAG "NativeIPC"


#define CHUNK 2048

using namespace android;

// The binder message ids. Always starting from FIRST_CALL_TRANSACTION and adding an offset of 1
enum {
    SHARE_FD= IBinder::FIRST_CALL_TRANSACTION
};


status_t ashmem_local_fd = -1;
char * data_from_client;

#define INFO(...) \
    do { \
        printf(__VA_ARGS__); \
        printf("\n"); \
        ALOGD(__VA_ARGS__); \
    } while(0)


void assert_fail(const char *file, int line, const char *func, const char *expr) {
    INFO("assertion failed at file %s, line %d, function %s:",file, line, func);
    INFO("%s", expr);
    abort();
}

#define ASSERT(e) \
    do { \
        if (!(e)) \
            assert_fail(__FILE__, __LINE__, __func__, #e); \
    } while(0)


// The interface, binder client and server should both implement it
//this interface extends IInterface defined in <binder/IInterface.h>
class ICommonInterface : public IInterface {
    public:
        //this method allows to share with a client process a file descriptor associated with a shared memory region.
        virtual status_t shareFD(uint32_t fd) = 0;


        //this is a MACRO defined in <binder/IInterface.h>
        DECLARE_META_INTERFACE(CommonInterface);
        //executing this equal to execute the following 5 lines
        //static const android::String16 descriptor;
        //static android::sp<IDemo> asInterface(const android::sp<android::IBinder>& obj);
        //virtual const android::String16& getInterfaceDescriptor() const;
        //CommonInterface();->constructor
        //virtual ~CommonInterface();->destructor
};

// Client Interface. Obviously it implements the previously defined interface
// THis class extends BpInterface (defined in <binder/IInterface.h>) with a reference to
// common interface
class BpCommonInterface : public BpInterface<ICommonInterface> {
    public:
        //this is the constructor for the client interface
        BpCommonInterface(const sp<IBinder>& impl) : BpInterface<ICommonInterface>(impl) {
            ALOGD("Constructor for the Client Iface has been Invoked");
        }

        //implementation for the client version of the method
        //this will prepare a parcel and transact to the server through
        // binder
        virtual status_t shareFD(uint32_t fd){
        ALOGD("Sharing File Descriptor from Client (%u)", fd);
        //prepare the parcels for the data(to be sent) and the reply(to be filled by the server)
        Parcel data, reply;
        //set the interface token i.e the name of the service that should deal with the request
        data.writeInterfaceToken(ICommonInterface::getInterfaceDescriptor());
        //write the file descriptor
        data.writeFileDescriptor(fd);
        //in this case, we only attach the file descriptor. However we can add more data if needed
        //using methods such as data.writeByteArray(size_t len, const uint8_t *val)

        //when all the parameters in the parcer are ready, we launch the transaction
        remote()->transact(SHARE_FD, data, &reply);
        //the server puts the answer in reply, finally we return the result
        return reply.readInt32();
    }
};

    //IMPLEMENT_META_INTERFACE(CommonInterface, "com.alex.nativeipcoc");
    // Macro above expands to code below. Doing it by hand so we can log ctor and destructor calls.
    //short said, here we implement the methods defined by DECLARE_META_INTERFACE
    const android::String16 ICommonInterface::descriptor("com.alex.nativeipcoc");
    const android::String16& ICommonInterface::getInterfaceDescriptor() const {
        return ICommonInterface::descriptor;
    }
    android::sp<ICommonInterface> ICommonInterface::asInterface(const android::sp<android::IBinder>& obj) {
        android::sp<ICommonInterface> intr;
        if (obj != NULL) {
            intr = static_cast<ICommonInterface*>(obj->queryLocalInterface(ICommonInterface::descriptor).get());
            if (intr == NULL) {
                intr = new BpCommonInterface(obj);
            }
        }
        return intr;
    }
    ICommonInterface::ICommonInterface() { ALOGD("CommonInterface Constructed"); }
    ICommonInterface::~ICommonInterface() { ALOGD("CommonInterface destroyed"); }
    // End of macro expansion


// The server interface. Likewise the client interface, it also implements CommonInterface
// THis class extends BpInterface (defined in <binder/IInterface.h>) with a reference to
// common interface
class ServerInterface : public BnInterface<ICommonInterface> {
    //this callback deals with the requests from the clients. In fact, everytime a transact() call is performed
    // this method is executed in the server
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0)
    {
        ALOGD("Server is dealing with a transact with code (%u)", code);
        //this checks if the interface exists and if the calle has propper permissions
        //for more info check Parcel::checkInterface(IBinder* binder) in Parcel.cpp
        CHECK_INTERFACE(ICommonInterface, data, reply);
        //now we proceed depending on the requested method
        switch(code) {
            case SHARE_FD: {
                //in this case wwe send the result of dealing with the request
                reply->writeInt32(shareFD(data.readFileDescriptor()));
                return NO_ERROR;
            } break;
            //case OTHER_CODE:{
                //blabla
            // }
            default: {
                //if the code in the request is unknown, the server ansers resending the transaction
                return BBinder::onTransact(code, data, reply, flags);
            } break;
        }
    }
};

// The server implementation
class Server : public ServerInterface {
public:



    status_t shareFD(uint32_t fd)
    {


        ALOGD("Server executing shareFD (%u)", fd);
        pid_t pid=getpid();
        ALOGD("Server's PID: %d",pid);

        ashmem_local_fd=fd;
        int result;
        result=fcntl(ashmem_local_fd, F_GETFL);
        if(result== -1){
            ALOGD("THe file descriptor is not longer available[Server]");
        }else if (result==O_RDWR){
            ALOGD("File descriptor is ready[Server] ");
        }else{
            ALOGD("File Descriptor status[Server]: %0X",result);
        }
        //map shared memory region
        char *shm = ( char *)mmap(NULL, CHUNK, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

        //UPDATE THE SHARED POINTER
        data_from_client=shm;

        return NO_ERROR;
    }




    static status_t registerService()
    {
        //add the service to system's service the list
        status_t result = defaultServiceManager()->addService(String16("myNative_service"), new Server());
        return result;
    }

    static sp<ICommonInterface> getService()
    {
        sp<IBinder> binder = defaultServiceManager()->getService(String16("myNative_service"));
        return interface_cast<ICommonInterface>(binder);
    }
};


// Helper function to get a hold of the "Demo" service.
sp<ICommonInterface> getDemoServ() {
    sp<IServiceManager> sm = defaultServiceManager();
    ASSERT(sm != 0);
    sp<IBinder> binder = sm->getService(String16("myNative_service"));
    // TODO: If the "Demo" service is not running, getService times out and binder == 0.
    ASSERT(binder != 0);
    sp<ICommonInterface> demo = interface_cast<ICommonInterface>(binder);
    ASSERT(demo != 0);
    return demo;
}



//entry point
int main(int argc, char **argv) {
    int fd;
    char *buff;



    //get the reference to the server
    sp <ICommonInterface> server = getDemoServ();

    //do all the stuf related with ashmem
    fd = open("/dev/ashmem", O_RDWR);
    ALOGD("ASHMEM FD is %d", fd);
    ioctl(fd, ASHMEM_SET_NAME, "ashmem");
    ioctl(fd, ASHMEM_SET_SIZE, CHUNK);
    buff =(char *) mmap(NULL, CHUNK, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ALOGD("Pointer to shared region %p",buff);
    if (buff == MAP_FAILED) {
    ALOGD("Error allocating memory");
    return -1;
    }

    //populate allocated memory
    for (int i=0;i<CHUNK;i++){
            buff[i] = 'a';
    }
    server->shareFD(fd);
    return 0;
}





static jbyteArray jni_getBuffer(JNIEnv* env, jclass clz){



    pid_t pid=getpid();
    ALOGD("Native app PID: %d",pid);
    if (data_from_client!=NULL){

        ALOGD("the SHARED pointer is not null");
        /*for(int i=0;i<CHUNK;i++){
            ALOGD("DATA: %c",data_from_client[i]);
        }*/
        //reserve a new array of 2KB
        ALOGD("Allocating jbytearray....");
        jbyteArray result = env->NewByteArray(CHUNK);
        if (result!=NULL) {
            ALOGD("Succesfully allocated jbytearray");
            jbyte *jBuf = (jbyte *) calloc(sizeof(jbyte), CHUNK);
            ALOGD("Succesfully allocated memory for copying");

            env->SetByteArrayRegion(result, 0, CHUNK, (jbyte *)data_from_client);

            //ALOGD("succesfully copied data. Freeing");
            //free(data_from_client);

            //THIS LEADS TO A SEGFAULT DKW :(
            ALOGD("succesfully copied data. Returning to java");
            return result;

        }else{
            ALOGD("Error allocating jbytearray");
        }
    }else{
        ALOGD("shared pointer is null");
    }
    return NULL;

}

static jboolean jni_startService(JNIEnv* env, jclass clz){

    status_t err = Server::registerService();
    if (err==NO_ERROR) {
        ALOGI("The service is started and ready!");
        return true;
    }else{
        ALOGI("Error starting the server");
    }

}

JNINativeMethod jniMethods[2] = {
        { "startService", "()Z", (void*)jni_startService},{ "getBuffer", "()[B", (void*)jni_getBuffer},
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    jclass clz = env->FindClass("com/alex/nativeipcpoc/NativeMethods");
    env->RegisterNatives(clz, jniMethods, sizeof(jniMethods) / sizeof(JNINativeMethod));
    env->DeleteLocalRef(clz);

    return JNI_VERSION_1_6;
}