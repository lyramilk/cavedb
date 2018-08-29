package lyramilk.cave;

import java.util.List;
import java.util.HashMap;
import java.lang.annotation.Annotation;
import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;
import java.lang.reflect.*;

public class cavedb
{
	static{
		System.loadLibrary("cavedbjni");
	}

	// 定义一个注解
	@Target({ElementType.METHOD})
	@Retention(RetentionPolicy.RUNTIME)
	public @interface bind {
		String value() default "";
	}


	//	这里开始才是正文
	protected Object refobj = null;
	protected HashMap<String,Method> invoker = new HashMap<String,Method>();
	public cavedb(Object obj)
	{
		this.refobj = obj;
		Class cls = this.refobj.getClass();
		Method[] fs = cls.getMethods();
		for (Method f : fs )
		{
			bind b = f.getAnnotation(bind.class);
			if(b != null){
				Type[] params = f.getGenericParameterTypes();

				if(params.length > 2){
					Type param = params[2];
					if(param instanceof ParameterizedType){
						ParameterizedType gd = (ParameterizedType)param;
						Type[] ats = gd.getActualTypeArguments();
						if(ats.length > 0 && ats[0].toString().equals("class [B")){
							invoker.put(b.value(),f);
						}
					}
				}
			}
		}
	}

	public void notify_command(byte[] replid,long offset,List<byte[]> args)
	{
		if(args.size() < 1) return;
		String cmd = new String(args.get(0));

		Method mi = invoker.get(cmd);
		if(mi == null) return;
		try{
			mi.invoke(refobj,replid,offset,args);
		}catch(IllegalAccessException e){
		}catch(IllegalArgumentException e){
		}catch(InvocationTargetException e){
		}
	}

	public boolean notify_psync(byte[]  replid,long offset)
	{
		return true;
	}

	public boolean notify_idle(byte[]  replid,long offset)
	{
		return true;
	}

	public native boolean slaveof_ssdb(String host,int port,String password);
	public native boolean slaveof_redis(String host,int port,String password);
}
