//knpreader.hpp
//Copyright 2015 Shuhei Kurita
//s.kurita.kyoto@gmail.com

#pragma once

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <initializer_list>
#include "stdutil.h"
#include "fileutil.hpp"
//#include "mtutil.h"
//#include "split.h"
//#define __cplusplus
//#include "engine.h" //for matlab
//#include <windows.h>
using namespace std;

//係り受け構造に着目して再帰的に文を格納する構造体
//rootに中心となる用言を格納し、leafにそれにかかる文要素を格納していく
struct wordstree {
	string word;
	vector<wordstree> leaf;
};
//wordstreeと似ているが、
//word idを格納する。
//メモリ節約および高速化のために存在する
//struct wordsidtree {
//	long long word;
//	vector<wordsidtree> leaf;
//};

//printing wordstree for debug
string showwordstree(wordstree &wt,string start="<", string end=">") {
	ostringstream ss;
	ss<<"'"<<wt.word<<"'";
	if(!wt.leaf.empty()) {
		ss<<start;
		for(int i=0;i<wt.leaf.size();i++) ss<<showwordstree(wt.leaf[i]);
		ss<<end;
	}
	return ss.str();
}

//knp parser
vector<wordstree> knpreader(string filename);

wordstree createwordstree(int id,vecstr &clauses,vecint &rootids) {
	wordstree wt;
	wt.word=clauses[id];
	//自分に係る単語を再帰的に探す
	//構文木にループ構造を持ってはならない
	for(int i=0;i<rootids.size();i++) {
		if(rootids[i]==id)
			wt.leaf.push_back(createwordstree(i,clauses,rootids));
	}
	return wt;
}

vector<wordstree> knpreader(string filename, vector<wordstree> &corpus) {
	ifstream ifs(filename);
	string str;
	if (ifs.fail()) {
		cerr << "Can't find: " << filename << endl;
		exit(-1);
	}

	vecstr sssp,ss1,ss2;
	//ostream ofs;
	//ofs=cout;

	bool bos=true;		//begining of sentence
	int eosnum=0;
//	vector<wordstree> lcorpus;
	string clause="";
	vecstr clauses;
	vecint rootids;

	//Read data
	for(int l=1;!ifs.eof();l++) {
		getline(ifs,str);
		if(str.empty()) continue;
		//cerr << str << endl;
//		cout << l<<'\t';
		switch(str[0]) {
			case '#':
				//cout<<endl;
				break;
			case '*':
				sssp = split(str,' ');
				//sssp[1]:24D,5Pとか。ほとんどD。Pはpara-phraseのことか?
//				cout<<"clause: "<<sssp[1]<<'\t'<<clause<<endl;
				if(bos)
					bos=false;
				else {
					clauses.push_back(clause);
					clause="";
				}
				//係り受け先をDやPを削除し数字として格納
				//cout<<atoi(sssp[1].substr(0,sssp[1].length()-1).c_str())<<endl;
				rootids.push_back(atoi(sssp[1].substr(0,sssp[1].length()-1).c_str()));
				break;
			case '+':
//				sssp = split(str,' ');
//				ss1 = split(sssp[2],':');
//				ss2 = split(ss1[1],'/');
//				cerr<<sssp[0]<<' '<<sssp[1]<<'\t'<<ss2[0]<<endl;
//				cout<<endl;
				//ss2[0]:単語(原形に変換)
				break;
			case 'E':	//EOS
				//TODO:あとでもう少し真面目にEOS判定
//				cout<<"clause: "<<clause<<endl;
				if(clause.size()<2) cout<<clause<<"****************** KNP PARSE ERROR *************** at line "<<l<<"\n";
				clauses.push_back(clause);
				clause="";
//				cout<<str<<endl;
				bos=true;
				eosnum++;
				break;
			default:	//JUMAN
				ss1 = split(str,' ');
				//ss1[0]:単語
				//ss1[1]:単語(かなに変換)
				//ss1[2]:単語(原形に変換)
				//ss1[3]:品詞タグ
//				cout<<ss1[3]<<endl;
//				cout<<'\t'<<ss1[0]<<'\t'<<ss1[3]<<endl;
//				if(ss1[3]!="特殊")
					clause+=ss1[0];
//				if(ss1[3]=="特殊")
//					cout<<'\t'<<ss1[0]<<'\t'<<ss1[3]<<endl;
				break;
		}

		if(str[0]=='E') {
			/*** printing section ***/
			//cout<<sshow(clauses)<<endl;
			
			//cout<<sshow(rootids)<<endl;
			////print
			//for(int i=0;i<rootids.size();i++) {
			//	cout<<clauses[i]<<"\tto\t";
			//	if(rootids[i]>=0)	cout<<clauses[rootids[i]]<<endl;
			//	else			cout<<"EOS"<<endl;
			//}
			//cout<<endl;

			//EOS search
			//rootを探す
			int i=0;
			while(rootids[i]>=0)	i=rootids[i];	//自分自身にかかる単語は存在しないと仮定
			corpus.push_back(createwordstree(i,clauses,rootids));
			clauses.clear();
			rootids.clear();
		}
		//add.animal = names[0];
		//add.glob = names[1];
		//add.detail = names[2];
		//while(!ifs.eof()) {
		//	getline(ifs,str);
		//	if(str.empty()) break;
		//	if(str[0]!='\t'||str.size()<2) break;
		//	//if(add.glob=="Visual")	continue;
		//	ifstream ifsdata(str.substr(1));
		//	if(ifsdata.fail()) cerr << "Load error!"<< endl;
		//	vector<double> isi;
		//	isi.reserve(2000);
		//	double temp;
		//	for(int i=0; i<2000; i++) {
		//		ifsdata >> temp;
		//		isi.push_back(temp);
		//	}
		//	//cerr << isi.size() << endl;
		//	add.isis.push_back(isi);
		//}
		//cerr << add.isis.size() << endl;
		//if(0!=add.isis.size())
		//	data.push_back(add);
		if(ifs.eof())	break;
//		if(l>1000)	break;
//		if(eosnum>24)	break;
	}

	cout<<endl<<"knpreader end:"<<eosnum<<endl;
	return corpus;
}

vector<wordstree> knpreader(string filename) {
	vector<wordstree> corpus;
	return knpreader(filename, corpus);
}

void knpprinter(string filename) {
	ifstream ifs(filename);
	string str;
	if (ifs.fail()) {
		cerr << "Can't find: " << filename << endl;
		exit(-1);
	}

	vecstr sssp,ss1,ss2;
	//ostream ofs;
	//ofs=cout;

	bool bos=true;		//begining of sentence
	int eosnum=0;
//	vector<wordstree> lcorpus;
	string clause="";
	vecstr clauses;
	vecint rootids;

	//Read data
	for(int l=1;!ifs.eof();l++) {
		getline(ifs,str);
		if(str.empty()) continue;
		//cerr << str << endl;
//		cout << l<<'\t';
		switch(str[0]) {
			case '#':
				//cout<<endl;
				break;
			case '*':
				sssp = split(str,' ');
				//sssp[1]:24D,5Pとか。ほとんどD。Pはpara-phraseのことか?
//				cout<<"clause: "<<sssp[1]<<'\t'<<clause<<endl;
				if(bos)
					bos=false;
				else {
					clauses.push_back(clause);
					clause="";
				}
				//係り受け先をDやPを削除し数字として格納
				//cout<<atoi(sssp[1].substr(0,sssp[1].length()-1).c_str())<<endl;
				rootids.push_back(atoi(sssp[1].substr(0,sssp[1].length()-1).c_str()));
				break;
			case '+':
//				sssp = split(str,' ');
//				ss1 = split(sssp[2],':');
//				ss2 = split(ss1[1],'/');
//				cerr<<sssp[0]<<' '<<sssp[1]<<'\t'<<ss2[0]<<endl;
//				cout<<endl;
				//ss2[0]:単語(原形に変換)
				break;
			case 'E':	//EOS
				//TODO:あとでもう少し真面目にEOS判定
//				cout<<"clause: "<<clause<<endl;
				if(clause.size()<2) cout<<clause<<"****************** KNP PARSE ERROR *************** at line "<<l<<"\n";
				clauses.push_back(clause);
				clause="";
//				cout<<str<<endl;
				bos=true;
				eosnum++;
				break;
			default:	//JUMAN
				ss1 = split(str,' ');
				//ss1[0]:単語
				//ss1[1]:単語(かなに変換)
				//ss1[2]:単語(原形に変換)
				//ss1[3]:品詞タグ
//				cout<<ss1[3]<<endl;
//				cout<<'\t'<<ss1[0]<<'\t'<<ss1[3]<<endl;
//				if(ss1[3]!="特殊")
					clause+=ss1[0];
//				if(ss1[3]=="特殊")
//					cout<<'\t'<<ss1[0]<<'\t'<<ss1[3]<<endl;
				break;
		}

		if(str[0]=='E') {
			/*** printing section ***/
//			cout<<sshow(clauses)<<endl;
			
			//cout<<sshow(rootids)<<endl;
			////print
			//for(int i=0;i<rootids.size();i++) {
			//	cout<<clauses[i]<<"\tto\t";
			//	if(rootids[i]>=0)	cout<<clauses[rootids[i]]<<endl;
			//	else			cout<<"EOS"<<endl;
			//}
			//cout<<endl;

			//EOS search
			//rootを探す
			int i=0;
			while(rootids[i]>=0)	i=rootids[i];	//自分自身にかかる単語は存在しないと仮定
			clauses.clear();
			rootids.clear();
		}
		//add.animal = names[0];
		//add.glob = names[1];
		//add.detail = names[2];
		//while(!ifs.eof()) {
		//	getline(ifs,str);
		//	if(str.empty()) break;
		//	if(str[0]!='\t'||str.size()<2) break;
		//	//if(add.glob=="Visual")	continue;
		//	ifstream ifsdata(str.substr(1));
		//	if(ifsdata.fail()) cerr << "Load error!"<< endl;
		//	vector<double> isi;
		//	isi.reserve(2000);
		//	double temp;
		//	for(int i=0; i<2000; i++) {
		//		ifsdata >> temp;
		//		isi.push_back(temp);
		//	}
		//	//cerr << isi.size() << endl;
		//	add.isis.push_back(isi);
		//}
		//cerr << add.isis.size() << endl;
		//if(0!=add.isis.size())
		//	data.push_back(add);
		if(ifs.eof())	break;
//		if(l>1000)	break;
//		if(eosnum>24)	break;
	}

	return;
}
//ディレクトリからぶっこ抜く
vector<wordstree> dirreader(string path) {
	vector<wordstree> corpus;
	vecstr ss = getfilelist(path);
	for(vecstr::iterator it=ss.begin(); it!=ss.end(); it++) {
//		if((*it).size()<7) continue;
//		if((*it).substr((*it).size()-7)==".knp.gz") {
//			system("gunzip "+(*it)+" ");
//		}
		if((*it).size()<4) continue;
		if((*it).substr((*it).size()-4)==".knp") {
			knpreader((*it),corpus);
		}
	}
	return corpus;
}

