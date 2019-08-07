#include <iostream>
#include <Python.h>
#include <pythread.h>
#include <libmilk/var.h>
#include <libmilk/json.h>
#include <cavedb/slave.h>
#include <cavedb/slave_ssdb.h>
#include <cavedb/slave_redis.h>
#include <pthread.h>

#if PY_MAJOR_VERSION == 3
	#define PyString_FromStringAndSize PyBytes_FromStringAndSize
#endif

struct state_saver
{
	PyGILState_STATE gstate;
	state_saver(PyThreadState* threadState)
	{
		gstate = PyGILState_Ensure();
	}
	~state_saver()
	{
		PyGILState_Release(gstate);
	}
};

class python_cavedb_store:public lyramilk::cave::slave
{
  protected:
	bool bnotify_psync;
	bool bnotify_command;
	bool bnotify_idle;

	virtual bool notify_psync(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		if(!bnotify_psync) return true;
		state_saver _(threadState);

		PyObject *meth =  PyObject_GetAttrString(self, "notify_psync");
		if(meth && PyMethod_Check(meth)){
			PyObject* python_args = PyTuple_New(2);

			{
				PyObject* sobj = PyString_FromStringAndSize(replid.c_str(),replid.size());
				PyTuple_SetItem(python_args,0,sobj);
			}
			{

				PyObject* sobj = PyLong_FromUnsignedLongLong(offset);
				PyTuple_SetItem(python_args,1,sobj);
			}
			PyObject* result = PyEval_CallObject(meth,python_args);
			Py_DECREF(python_args);
			Py_DECREF(meth);

			bool have_error = PyErr_Occurred() ? true : false;
			if(have_error){
				PyErr_Print();
				return false;
			}

			if(result){
				bool ret = false;
				if(PyBool_Check(result)){
					if(Py_True == result){
						ret = true;
					}
				}
				Py_DECREF(result);
				return ret;
			}
			return false;
		}

		return true;
	}

	virtual bool notify_command(const lyramilk::data::string& replid,lyramilk::data::uint64 offset,lyramilk::data::array& args)
	{
		if(!bnotify_command) return false;
		state_saver _(threadState);

		PyObject *meth =  PyObject_GetAttrString(self, "notify_command");
		if(meth && PyMethod_Check(meth)){
			PyObject* python_args = PyTuple_New(3);

			{
				PyObject* sobj = PyString_FromStringAndSize(replid.c_str(),replid.size());
				PyTuple_SetItem(python_args,0,sobj);
			}

			{

				PyObject* sobj = PyLong_FromUnsignedLongLong(offset);
				PyTuple_SetItem(python_args,1,sobj);
			}


			PyObject* python_args_3 = PyList_New(args.size());
			for(Py_ssize_t i = 0;i < (Py_ssize_t)args.size();++i){
				lyramilk::data::string s = args[i].str();
				PyObject* sobj = PyString_FromStringAndSize(s.c_str(),s.size());
				PyList_SetItem(python_args_3,i,sobj);
			}

			PyTuple_SetItem(python_args,2,python_args_3);
			PyObject* result = PyEval_CallObject(meth,python_args);
			Py_DECREF(python_args);
			Py_DECREF(meth);

			bool have_error = PyErr_Occurred() ? true : false;
			if(have_error){
				PyErr_Print();
				return false;
			}

			if(result){
				bool ret = false;
				if(PyBool_Check(result)){
					if(Py_True == result){
						ret = true;
					}
				}
				Py_DECREF(result);
				return ret;
			}
		}
		return false;
	}

	virtual bool notify_idle(const lyramilk::data::string& replid,lyramilk::data::uint64 offset)
	{
		if(!bnotify_idle) return true;
		state_saver _(threadState);

		PyObject *meth =  PyObject_GetAttrString(self, "notify_idle");
		if(meth && PyMethod_Check(meth)){
			PyObject* python_args = PyTuple_New(2);

			{
				PyObject* sobj = PyString_FromStringAndSize(replid.c_str(),replid.size());
				PyTuple_SetItem(python_args,0,sobj);
			}
			{

				PyObject* sobj = PyLong_FromUnsignedLongLong(offset);
				PyTuple_SetItem(python_args,1,sobj);
			}
			PyObject* result = PyEval_CallObject(meth,python_args);
			Py_DECREF(python_args);
			Py_DECREF(meth);

			bool have_error = PyErr_Occurred() ? true : false;
			if(have_error){
				PyErr_Print();
				return false;
			}

			if(result){
				bool ret = false;
				if(PyBool_Check(result)){
					if(Py_True == result){
						ret = true;
					}
				}
				Py_DECREF(result);
				return ret;
			}
			return false;
		}
		return true;
	}

  public:
	PyObject *self;
	PyThreadState* threadState;
	python_cavedb_store(PyObject *call_back,PyThreadState* pts)
	{
		self = call_back;
		threadState = pts;
		bnotify_psync = PyObject_HasAttrString(self, "notify_psync") == 1;
		bnotify_command = PyObject_HasAttrString(self, "notify_command") == 1;
		bnotify_idle = PyObject_HasAttrString(self, "notify_idle") == 1;
	}

	virtual ~python_cavedb_store()
	{
	}
};

template<typename T>
struct thread_params
{
	T datasource;
	python_cavedb_store* store;
	PyThreadState* mainThreadState;
};

void cave_ssdb_thread(void* param)
{
	thread_params<lyramilk::cave::slave_ssdb>* tp = (thread_params<lyramilk::cave::slave_ssdb>*)param;

	PyInterpreterState * mainInterpreterState = tp->mainThreadState->interp;
	PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
	tp->store->threadState = myThreadState;

	{
		tp->datasource.svc();
	}
}

void cave_redis_thread(void* param)
{
	thread_params<lyramilk::cave::slave_redis>* tp = (thread_params<lyramilk::cave::slave_redis>*)param;

	PyInterpreterState * mainInterpreterState = tp->mainThreadState->interp;
	PyThreadState * myThreadState = PyThreadState_New(mainInterpreterState);
	tp->store->threadState = myThreadState;

	{
		tp->datasource.active(1);

		while(tp->datasource.size() > 0){
			usleep(100);
		}
		tp->datasource.svc();
	}
}

static PyObject * slaveof_ssdb(PyObject *self, PyObject *args)
{
	if(self == NULL){
		return NULL;
	}

	char *host;
	unsigned short port;
	char *pwd;
	PyObject *replid;
	long long offset;

	if (!(PyArg_ParseTuple(args, "sHsO!I", &host,&port,&pwd,&PyBytes_Type,&replid,&offset))) {
		return NULL;
	}

	Py_ssize_t bytescount = PyBytes_GET_SIZE(replid);
	const char* bytesptr = PyBytes_AsString(replid);

	lyramilk::data::string bytes_replid((const char* )bytesptr,bytescount);

	thread_params<lyramilk::cave::slave_ssdb>* tp = new thread_params<lyramilk::cave::slave_ssdb>;
	tp->mainThreadState = PyThreadState_Get();
	tp->store = new python_cavedb_store(self,NULL);
	tp->datasource.init(host,port,pwd,bytes_replid,offset,tp->store);

	if(!PyEval_ThreadsInitialized()){
		Py_Initialize();
		PyEval_InitThreads();
		//PyEval_ReleaseThread(tp->mainThreadState);
	}
	/*
	*/
	PyThread_start_new_thread(cave_ssdb_thread,tp);

	Py_RETURN_TRUE;
}

static PyObject * slaveof_redis(PyObject *self, PyObject *args)
{
	if(self == NULL){
		return NULL;
	}

	char *host;
	unsigned short port;
	char *pwd;
	PyObject *replid;
	long long offset;

	if (!(PyArg_ParseTuple(args, "sHsO!I", &host,&port,&pwd,&PyBytes_Type,&replid,&offset))) {
		return NULL;
	}

	Py_ssize_t bytescount = PyBytes_GET_SIZE(replid);
	const char* bytesptr = PyBytes_AsString(replid);

	lyramilk::data::string bytes_replid((const char* )bytesptr,bytescount);

	thread_params<lyramilk::cave::slave_redis>* tp = new thread_params<lyramilk::cave::slave_redis>;
	tp->mainThreadState = PyThreadState_Get();
	tp->store = new python_cavedb_store(self,NULL);
	tp->datasource.init(host,port,pwd,bytes_replid,offset,tp->store);

	if(!PyEval_ThreadsInitialized()){
		Py_Initialize();
		PyEval_InitThreads();
		//PyEval_ReleaseThread(tp->mainThreadState);
	}
	/*
	*/
	PyThread_start_new_thread(cave_redis_thread,tp);

	Py_RETURN_TRUE;
}


static PyObject * cavedb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return type->tp_alloc(type, 0);
}

static void cavedb_dealloc(PyObject* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyMethodDef cavedbModuleMethods[] = {
	{NULL, NULL},
};

#if PY_MAJOR_VERSION == 3


#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))


static int cavedb_extension_traverse(PyObject *m, visitproc visit, void *arg) {
	return 0;
}

static int cavedb_extension_clear(PyObject *m) {
	return 0;
}

struct module_state {
	PyObject *error;
};

static struct PyModuleDef cavedbModuleDefine = {
	PyModuleDef_HEAD_INIT,//默认
	"cavedb",//模块名
	NULL,
	-1,
	cavedbModuleMethods, //上面的数组
	NULL,
	cavedb_extension_traverse,
	cavedb_extension_clear,
};
#endif

static PyMethodDef cavedbClassMethods[] = {
	{"slaveof_ssdb", slaveof_ssdb, METH_VARARGS},
	{"slaveof_redis", slaveof_redis, METH_VARARGS},
	{NULL, NULL},
};

typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
} cavedbObject;




static PyTypeObject cavedbObjectType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"cavedb.cavedb",			/*tp_name*/
	sizeof(cavedbObject),		/*tp_basicsize*/
	0, /*tp_itemsize*/
	cavedb_dealloc,
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_reserved */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"cavedb desc", /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	cavedbClassMethods, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	0, /* tp_init */
	0, /* tp_alloc */
	cavedb_new, /* tp_new */
};

#if PY_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_cavedb()
#else
extern "C" void initcavedb()
#endif
{
	if(PyType_Ready(&cavedbObjectType) < 0){
		printf("cavedb not ready");
#if PY_MAJOR_VERSION == 3
		return NULL;
#endif
	}


#if PY_MAJOR_VERSION == 3
	PyObject* m = PyModule_Create(&cavedbModuleDefine);
#else
	PyObject* m = Py_InitModule("cavedb", cavedbModuleMethods);
#endif
	if(m){
		//cavedbObjectType.tp_methods = cavedbClassMethods;
		Py_INCREF(&cavedbObjectType);
		PyModule_AddObject(m, "cavedb", (PyObject *)&cavedbObjectType);
	}
#if PY_MAJOR_VERSION == 3
	return m;
#endif
}
