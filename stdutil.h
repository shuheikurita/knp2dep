//C++ Libray for general usage
//Copyright Shuhei KURITA
//last modified : 2015/06/29
//This header file is required by almost all programs I created.

#ifndef _STDUTIL_H_
#define _STDUTIL_H_

#include <fstream>
#include <cmath>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <regex>
//#include <codecvt>
//using namespace std;

#define REP(i,n) for(int i=0; i<(int)(n); i++)

inline int d(int i) { return i<<1; }	//i*2
//template<typename T>inline T p(T i) { return i*i; }
template<typename T>inline T p2(T i) { return i*i; }
//template<typename T>inline T min(T i,T j) { return i<j?i:j; }

typedef std::vector<double> vecdbl;
typedef std::vector<int> vecint;
typedef std::vector<std::string> vecstr;

//http://eternuement.blogspot.jp/2011/04/c-string-double-int.html
double str2d(const std::string& str){
	double rt;
	std::stringstream ss;
	ss << str;
	ss >> rt;
	return rt;
}
template<typename T> std::string str(T d){
	std::string rt;
	std::stringstream ss;
	ss << d;
	ss >> rt;
	return rt;
}

//template<typename T> void p(const T &v) {
//	std::cout<<v<<std::endl;
#define p(v) std::cout<<v<<std::endl
void plf() {
	std::cout<<std::endl;
}

template<typename T> std::string sshow(const T *p, int size, const std::string &delim=" ") {
	std::ostringstream ss;
	for(int i=0;i<size;i++) ss<<p[i]<<delim;
	return ss.str();
}
template<typename T> std::string sshow(const std::vector<T> &v, const std::string &delim=" ") {
	std::ostringstream ss;
	for(typename std::vector<T>::const_iterator it=v.begin(); it!=v.end(); it++) ss<<*it<<delim;
	return ss.str();
}
template<typename T> std::string sshows(const std::vector<T> &v, const std::string &delim=" ") {
	return sshow(v,delim)+"size("+str(v.size())+")";
}

//n進数で数を数える
//1の位から順に返す
//-2進数で数を数えることも可能
int dev(int x, int y) {	return x%y==0?x/y:(x<0?(y<0?x/y+1:x/y-1):x/y); }
std::vector<int> ndecimal(std::vector<int> &v, int i, int n) {
	if(i>=0&&i<abs(n)) {
		v.push_back(i);
		return v;
	}
	int t = dev(i,n);
	v.push_back(i-t*n);	//c.f.最小非負剰余
	return ndecimal(v,t,n);
}
std::vector<int> ndecimal(int i, int n) {
	std::vector<int> v;
	return abs(n)<2||(i<0&&n>0)?v:ndecimal(v,i,n);
}

//http://shnya.jp/blog/?p=195
std::vector<std::string> split(const std::string &str, char delim){
	std::vector<std::string> res;
	size_t current = 0, found;
	while((found = str.find_first_of(delim, current)) != std::string::npos){
	res.push_back(std::string(str, current, found - current));
	current = found + 1;
	}
	res.push_back(std::string(str, current, str.size() - current));
	return res;
}

//vector<string>の中から正規表現patternに一致するindex(0...size-1)番目のstringを取り出す
std::string matchss(vecstr args,const char *pattern,int index=0) {
	std::regex re(pattern);
	int match=0;
	for(int i=0;i<args.size();i++) {
		//regex_match(L"ワイド文字列",re);
		//const wchar_t *wc=L"ワイド文字列";
		//string narrow = converter.to_bytes(wide_utf16_source_string);
		if(regex_match(args[i],re)) {
			if(match==index) return args[i];
			match++;
		}
	}
	return "";
}
/*
 * recureing #include <codecvt>
string matchss(vecstr args,const wchar_t *pattern,int index=0) {
	wregex re(pattern);
	int match=0;
	wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
	for(int i=0;i<args.size();i++) {
		//regex_match(L"ワイド文字列",re);
		//const wchar_t *wc=L"ワイド文字列";
		//string narrow = converter.to_bytes(wide_utf16_source_string);
		wstring wide = converter.from_bytes(args[i]);
		if(regex_match(wide,re)) {
			if(match==index) return args[i];
			match++;
		}
	}
	return "";
}
 */
//string matchss(vecstr args,string pattern,int index=0) {
//	int match=0;
//	for(int i=0;i<args.size();i++) {
//		if(args[i].substr(0,pattern.size())) {
//			if(match==index) return args[i];
//			match++;
//		}
//	}
//	return "";
//}

//"\n","\r","\r\n"に対応した一行読み込み(getline())
std::istream& getline_safe(std::istream& is, std::string& t)
{
	t.clear();

	// The characters in the stream are read one-by-one using a std::streambuf.
	// That is faster than reading them one-by-one using the std::istream.
	// Code that uses streambuf this way must be guarded by a sentry object.
	// The sentry object performs various tasks,
	// such as thread synchronization and updating the stream state.

	std::istream::sentry se(is, true);
	std::streambuf* sb = is.rdbuf();

	for(;;) {
		int c = sb->sbumpc();
		switch (c) {
		case '\n':
			return is;
		case '\r':
			if(sb->sgetc() == '\n')
				sb->sbumpc();
			return is;
		case EOF:
			// Also handle the case when the last line has no line ending
			if(t.empty())
				is.setstate(std::ios::eofbit);
			return is;
		default:
			t += (char)c;
		}
	}
}

std::string getword(std::istream& ifs, char sep=' ') {
	std::string ss;
	while(!ifs.eof()) {
		char c=ifs.get();
		if(c==sep) break;
		ss.push_back(c);
	}
	return ss;
}

#endif
