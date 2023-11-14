#include "cavedb_key.h"



namespace lyramilk{ namespace cave
{
	inline bool cavedb_key_put_double(std::string* r,double i)
	{
		r->append((char*)&i,sizeof(double));
		return true;
	}

	inline double cavedb_key_get_double(const char* p,const char**n)
	{
		double i = *(double*)p;
		if(n)*n = p + sizeof(double);
		return i;
	}



	inline bool cavedb_key_put_int(std::string* r,lyramilk::data::uint64 i)
	{
		r->append((char*)&i,sizeof(lyramilk::data::uint64));
		return true;
	}

	inline lyramilk::data::uint64 cavedb_key_get_int(const char* p,const char**n)
	{
		lyramilk::data::uint64 i = *(lyramilk::data::uint64*)p;
		if(n)*n = p + sizeof(lyramilk::data::uint64);
		return i;
	}




	inline bool cavedb_key_put_str(std::string* r,const cavedb::Slice2& s)
	{
		if(s.size() < 0x80){
			unsigned char c = (unsigned char)s.size();
			r->push_back(c);
			r->append(s.data(),c);
			return true;
		}else if(s.size() < 0x8000000000000000ull){
			lyramilk::data::uint64 i;
			i = 0x8000000000000000ull | s.size();
			r->append((char*)&i,sizeof(lyramilk::data::uint64));
			r->append(s.data(),s.size());
			return true;
		}
		return false;
	}

	inline cavedb::Slice2 cavedb_key_get_str(const char* p,const char**n)
	{
		char c = *p;
		if((c&0x80) == 0){
			lyramilk::data::uint64 z = (unsigned int)c&0x7f;
			if(n){
				*n = p + 1;
				*n += z;
			}
			return cavedb::Slice(p+1,z);
		}else{
			lyramilk::data::uint64 i = *(lyramilk::data::uint64*)p;
			lyramilk::data::uint64 z = i&0x7fffffffffffffffull;
			if(n){
				*n = p + 8;
				*n += z;
			}
			return cavedb::Slice(p+8,z);
		}
	}







	bool cavedb_key::parse(leveldb::Slice key,cavedb_key* s)
	{
		if(key.size() < 2) return false;

		const char* n;
		const char* p = key.data();
		const char* e = p + key.size();
		if(p[0] != magic) return false;

		s->key = cavedb_key_get_str(p+1,&n);
		p = n;

		if(p[0] != magic) return false;
		++p;

		if(p == e){
			s->type = s_any;
			return true;
		}

		int type = (int)(unsigned char)p[0];
		++p;

		s->type = (cavedb_key_type)type;
		switch(s->type){
			case s_any:{
				return true;
			}break;
			case s_native:{
				return true;
			}break;
			case s_string:{
				return true;
			}break;
			case s_hash:{
				if(p[0] != magic) return false;
				++p;
				s->hash.field = cavedb_key_get_str(p,&n);
				return true;
			}break;
			case s_list:{
				if(p[0] != magic) return false;
				++p;
				s->list.seq = cavedb_key_get_int(p,&n);
				return true;
			}break;
			case s_set:{
				if(p[0] != magic) return false;
				++p;
				s->set.member = cavedb_key_get_str(p,&n);
				return true;
			}break;
			case s_zset:{
				if(p[0] != magic) return false;
				++p;
				n = p;
				s->zset.score = cavedb_key_get_double(p,&n);
				p = n;
				if(p[0] != magic) return false;
				++p;
				s->zset.member = cavedb_key_get_str(p,&n);
				return true;
			}break;
			case s_zset_m2s:{
				if(p[0] != magic) return false;
				++p;
				s->zset.member = cavedb_key_get_str(p,&n);
				return true;
			}break;
			case s_eof:{
				return true;
			}break;
		}
COUT << "解EOF包失败" << s->type << std::endl;
		return false;
	}

	std::string cavedb_key::stringify(const cavedb_key& s)
	{
		std::string r;
		unsigned int t = s.type;
		r.push_back(magic);
		cavedb_key_put_str(&r,s.key);

		r.push_back(magic);
		if(s.type == s_any) return r;
		r.push_back(t%0xff);
		switch(s.type){
			case s_any:{
				return r;
			}break;
			case s_native:{
				return r;
			}break;
			case s_string:{
				return r;
			}break;
			case s_hash:{
				r.push_back(magic);
				if(!cavedb_key_put_str(&r,s.hash.field)) throw string_too_large();
				return r;
			}break;
			case s_list:{
				r.push_back(magic);
				if(!cavedb_key_put_int(&r,s.list.seq)) throw string_too_large();
				return r;
			}break;
			case s_set:{
				r.push_back(magic);
				if(!cavedb_key_put_str(&r,s.set.member)) throw string_too_large();
				return r;
			}break;
			case s_zset:{
				r.push_back(magic);
				cavedb_key_put_double(&r,s.zset.score);
				r.push_back(magic);
				if(!cavedb_key_put_str(&r,s.zset.member)) throw string_too_large();
				return r;
			}break;
			case s_zset_m2s:{
				r.push_back(magic);
				if(!cavedb_key_put_str(&r,s.zset.member)) throw string_too_large();
				return r;
			}break;
			case s_eof:{
				return r;
			}break;
		}

		throw type_error();
	}

	std::string cavedb_key::make_key_prefix(leveldb::Slice key)
	{
		cavedb::Slice cskey(key.data(),key.size());
		std::string r;
		r.push_back(magic);
		cavedb_key_put_str(&r,cskey);
		r.push_back(magic);
		return r;
	}

	std::string cavedb_key::make_key_eof(leveldb::Slice key)
	{
		cavedb::Slice cskey(key.data(),key.size());
		std::string r;
		r.push_back(magic);
		cavedb_key_put_str(&r,cskey);
		r.push_back(magic);
		r.push_back(s_eof%0x100);
		return r;
	}

	std::string cavedb_key::make_hashmap_prefix(leveldb::Slice key,leveldb::Slice field)
	{
		cavedb::Slice cskey(key.data(),key.size());
		std::string r;
		r.push_back(magic);
		cavedb_key_put_str(&r,cskey);
		r.push_back(magic);
		r.push_back(s_hash%0x100);
		return r;
	}

	int cavedb_key::Compare(const leveldb::Slice& a, const leveldb::Slice& b)
	{
		cavedb_key pa;
		if(!cavedb_key::parse(a,&pa)){
			return a.compare(b);
		}

		cavedb_key pb;
		if(!cavedb_key::parse(b,&pb)){
			return a.compare(b);
		}

		if(pa.key != pb.key){
			return pa.key.compare(pb.key);
		}

		if(pa.type == cavedb_key_type::s_any || pb.type == cavedb_key_type::s_any){
			return 0;
		}

		if(pa.type != pb.type){
			if(pa.type < pb.type) return -1;
			return 1;
		}

		if(pa.type == cavedb_key_type::s_string){
			return 0;
		}else if(pa.type == cavedb_key_type::s_native){
			return 0;
		}else if(pa.type == cavedb_key_type::s_hash){
			return pa.hash.field.compare(pb.hash.field);
		}else if(pa.type == cavedb_key_type::s_list){
			if(pa.list.seq == pb.list.seq){
				return 0;
			}else if(pa.list.seq < pb.list.seq){
				return -1;
			}else{
				return 1;
			}
		}else if(pa.type == cavedb_key_type::s_set){
			return pa.set.member.compare(pb.set.member);
		}else if(pa.type == cavedb_key_type::s_zset){
			if(pa.zset.score < pb.zset.score){
				return -1;
			}else if(pa.zset.score > pb.zset.score){
				return 1;
			}
			return pa.zset.member.compare(pb.zset.member);
		}else if(pa.type == cavedb_key_type::s_zset_m2s){
			return pa.zset.member.compare(pb.zset.member);
		}
TODO();
		return a.compare(b);
	}

	void cavedb_key::FindShortestSeparator(std::string* start,const leveldb::Slice& limit)
	{
	}

	void cavedb_key::FindShortSuccessor(std::string* key)
	{
	}
}}