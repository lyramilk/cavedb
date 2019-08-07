package lyramilk.cave;
import java.util.List;
import java.util.ArrayList;
import java.util.*;

public class cavedb_test
{

	@cavedb.bind("set")
	public void xxxxxxxxxxxxxx2(byte[] replid,long offset,List<byte[]> args)
	{
		if(args.size() != 3) return;
		String cmd = new String(args.get(0));
		String key = new String(args.get(1));
		String score = new String(args.get(2));
		System.out.println(cmd + " " + key + " " + score);
	}


	public static void main(String[] args)
	{
		new cavedb(new cavedb_test()).slaveof_ssdb("192.168.226.168",7016,"CNVXlB8CmSZgDCB4yI8mqbquQmAr4XKt",new byte[0],0);
		System.out.println("exit");
	}
}
