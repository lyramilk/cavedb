#include <jni.h>
#include <libmilk/var.h>
#include <libmilk/thread.h>
#include <cavedb/slave.h>
#include <cavedb/slave_ssdb.h>
#include <cavedb/slave_redis.h>

namespace lyramilk{ namespace cave
{
	__thread JNIEnv *env = nullptr;
	class jdk_store:public lyramilk::cave::slave
	{
		JavaVM *jvm;
		jobject jthis;
		jclass jcls_store;
		jclass jcls_arraylist;
		jmethodID jid_notify_command;
		jmethodID jid_notify_psync;
		jmethodID jid_notify_idle;

		jmethodID jid_arraylist_construct;
		jmethodID jid_arraylist_add;
	  protected:
		virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
		{
			if(env == nullptr){
				jvm->AttachCurrentThread((void**)&env,nullptr);
				if(env == nullptr) return false;
			}

			if(!jid_notify_psync){
				return false;
			}

			jbyteArray jstr = env->NewByteArray(replid.size());
			env->SetByteArrayRegion(jstr,0,replid.size(),(const jbyte*)replid.c_str());
			env->CallBooleanMethod(jthis,jid_notify_psync,jstr,(jint)offset);
			env->DeleteLocalRef(jstr);
			return true;
		}

		virtual void notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::var::array& args)
		{

			if(env == nullptr){
				jvm->AttachCurrentThread((void**)&env,nullptr);
				if(env == nullptr) return;
			}


			if(!jid_arraylist_construct){
				return ;
			}

			if(!jid_arraylist_add){
				return ;
			}

			if(!jid_notify_command){
				return ;
			}

			jobject jobj_arraylist = env->NewObject(jcls_arraylist,jid_arraylist_construct,"");

			for(std::size_t i = 0;i < args.size();++i){
				lyramilk::data::string tmp = args[i].str();
				jbyteArray arg2jbytes = env->NewByteArray(tmp.size());
				env->SetByteArrayRegion(arg2jbytes,0,tmp.size(),(const jbyte*)tmp.c_str());

				env->CallObjectMethod(jobj_arraylist,jid_arraylist_add,arg2jbytes);  
				env->DeleteLocalRef(arg2jbytes);
			}


			jbyteArray jstr = env->NewByteArray(replid.size());
			env->SetByteArrayRegion(jstr,0,replid.size(),(const jbyte*)replid.c_str());
			env->CallVoidMethod(jthis,jid_notify_command,jstr,(jint)offset,jobj_arraylist);
			env->DeleteLocalRef(jstr);
			env->DeleteLocalRef(jobj_arraylist);
			return ;
		}

		virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
		{
			if(env == nullptr){
				jvm->AttachCurrentThread((void**)&env,nullptr);
				if(env == nullptr) return false;
			}

			if(!jid_notify_idle){
				return false;
			}

			jbyteArray jstr = env->NewByteArray(replid.size());
			env->SetByteArrayRegion(jstr,0,replid.size(),(const jbyte*)replid.c_str());
			env->CallBooleanMethod(jthis,jid_notify_idle,jstr,(jint)offset);
			env->DeleteLocalRef(jstr);
			return true;
		}

	  public:
		jdk_store(JNIEnv *env,jobject jthis)
		{
			env->GetJavaVM(&jvm);
			this->jthis = env->NewGlobalRef(jthis);
			jcls_store = env->GetObjectClass(jthis);
			jid_notify_command = env->GetMethodID(jcls_store, "notify_command", "([BJLjava/util/List;)V");
			jid_notify_psync = env->GetMethodID(jcls_store, "notify_psync", "([BJ)Z");
			jid_notify_idle = env->GetMethodID(jcls_store, "notify_idle", "([BJ)Z");

			jcls_arraylist = env->FindClass("java/util/ArrayList");
			jid_arraylist_construct = env->GetMethodID(jcls_arraylist,"<init>","()V");
			jid_arraylist_add = env->GetMethodID(jcls_arraylist,"add","(Ljava/lang/Object;)Z");  

		}
		virtual ~jdk_store()
		{
			env->DeleteGlobalRef(jthis);
		}
	};
}}



lyramilk::data::string inline jstr2str(JNIEnv* env,jstring jstr) 
{ 
	lyramilk::data::string result;
	jclass clsstring = env->FindClass("java/lang/String");  
	jstring strencode = env->NewStringUTF("utf8"); 
	jmethodID mid = env->GetMethodID(clsstring,   "getBytes",   "(Ljava/lang/String;)[B");  
	jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr,mid,strencode); 
	jsize alen = env->GetArrayLength(barr); 
	jbyte* ba = env->GetByteArrayElements(barr,JNI_FALSE); 
	if(alen > 0){ 
		result.assign((const char*)ba,alen);
	}  
	env->ReleaseByteArrayElements(barr,ba,0); 
	return result;
}

lyramilk::data::string inline jbytes2str(JNIEnv* env,jbyteArray barr) 
{ 
	lyramilk::data::string result;
	jsize alen = env->GetArrayLength(barr); 
	jbyte* ba = env->GetByteArrayElements(barr,JNI_FALSE); 
	if(alen > 0){ 
		result.assign((const char*)ba,alen);
	}  
	env->ReleaseByteArrayElements(barr,ba,0); 
	return result;
}

extern "C" {

	/*
	 * Class:     lyramilk_cave_cavedb
	 * Method:    slaveof_ssdb
	 * Signature: (Ljava/lang/String;ILjava/lang/String;[BJ)Z
	 */
	JNIEXPORT jboolean JNICALL Java_lyramilk_cave_cavedb_slaveof_1ssdb(JNIEnv *env, jobject jthis, jstring jhost, jint jport, jstring jpwd,jbyteArray jreplid,jlong joffset)
	{
		lyramilk::data::string host = jstr2str(env,jhost);
		unsigned short port = jport;
		lyramilk::data::string pwd = jstr2str(env,jpwd);

		lyramilk::data::string replid = jbytes2str(env,jreplid);
		lyramilk::data::uint64 offset = joffset;

		lyramilk::cave::slave_ssdb datasource;
		datasource.slaveof(host,port,pwd,replid,offset,new lyramilk::cave::jdk_store(env,jthis));

		while(true){
			sleep(1);
		}
		return true;
	}

	/*
	 * Class:     lyramilk_cave_cavedb
	 * Method:    slaveof_redis
	 * Signature: (Ljava/lang/String;ILjava/lang/String;[BJ)Z
	 */
	JNIEXPORT jboolean JNICALL Java_lyramilk_cave_cavedb_slaveof_1redis(JNIEnv *env, jobject jthis, jstring jhost, jint jport, jstring jpwd,jbyteArray jreplid,jlong joffset)
	{
		lyramilk::data::string host = jstr2str(env,jhost);
		unsigned short port = jport;
		lyramilk::data::string pwd = jstr2str(env,jpwd);

		lyramilk::data::string replid = jbytes2str(env,jreplid);
		lyramilk::data::uint64 offset = joffset;

		lyramilk::cave::slave_redis datasource;
		datasource.slaveof(host,port,pwd,replid,offset,new lyramilk::cave::jdk_store(env,jthis));

		while(true){
			sleep(1);
		}
		return true;
	}
}
