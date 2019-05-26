//knp2dep.cpp
//Copyright 2015 Shuhei Kurita
//s.kurita.kyoto@gmail.com

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "stdutil.h"
#include "fileutil.hpp"
using namespace std;

//係り受け先は最初の１単語に絞る
//void knpreader(std::istream &ifs, vector<wordstree> &corpus, bool printingmode=false) {
int main(int argc, char *argv[]) {
	string str;
	vecstr sssp,ss1,ss2;
	//ostream ofs;
	//ofs=cout;

	bool bos=true;		//begining of sentence
	int eosnum=0;
//	vector<wordstree> lcorpus;
	string clause="";
	vecstr clauses;
	vector<vecstr> clausesperword;
	vecstr clauseperword;
	vecint rootids;

	//const bool printingmode=true;
	const bool bunsetsu=false;
	const bool order=true;

	//Read data
	for(int l=1;!cin.eof();l++) {
		getline(cin,str);
		if(str.empty()) continue;
		if(!(l%100000)) fprintf(stderr,"%d lines\n",l);
		//cerr << str << endl;
//		cout << l<<'\t';
		switch(str[0]) {
			case '#':
				//cout<<endl;
				break;
			case '*':
//				sssp = split(str,' ');
				//sssp[1]:24D,5Pとか。ほとんどD。Pはpara-phraseのことか?
//				cout<<"clause: "<<sssp[1]<<'\t'<<clause<<endl;
//				if(bos)
//					bos=false;
//				else {
//					clauses.push_back(clause);
//					clause="";
//					clausesperword.push_back(clauseperword);
//					clauseperword.clear();
//				}
//				//係り受け先をDやPを削除し数字として格納
//				//cout<<atoi(sssp[1].substr(0,sssp[1].length()-1).c_str())<<endl;
//				rootids.push_back(atoi(sssp[1].substr(0,sssp[1].length()-1).c_str()));
				break;
			case '+':
				sssp = split(str,' ');
				//ss1 = split(sssp[2],':');
				//ss2 = split(ss1[1],'/');
				//cerr<<sssp[0]<<' '<<sssp[1]<<'\t'<<ss2[0]<<endl;
				//cout<<endl;
				//ss2[0]:単語(原形に変換)
				//
				//
				if(bos)
					bos=false;
				else {
					clauses.push_back(clause);
					clause="";
					clausesperword.push_back(clauseperword);
					clauseperword.clear();
				}
				//係り受け先をDやPを削除し数字として格納
				//cout<<atoi(sssp[1].substr(0,sssp[1].length()-1).c_str())<<endl;
				rootids.push_back(atoi(sssp[1].substr(0,sssp[1].length()-1).c_str()));
				break;
			case 'E':	//EOS
				//TODO:あとでもう少し真面目にEOS判定
//				cout<<"clause: "<<clause<<endl;
				if(clause.size()<2) {
					cerr<<clause<<"****************** KNP PARSE ERROR *************** at line "<<l<<"\n";
					clause="";
					clauses.clear();
					clauseperword.clear();
					clausesperword.clear();
					rootids.clear();
					bos=true;
					continue;
				}
				clauses.push_back(clause);
				clause="";
				clausesperword.push_back(clauseperword);
				clauseperword.clear();
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
//					cout<<'\t'<<ss1[0]<<'\t'<<ss1[3]<<endl;
//					clause+=ss1[0];
				clause+=ss1[0];
				clauseperword.push_back(string(ss1[2]));
				/*************** printing mode *****************/
				//if(printingmode) cout<<string(ss1[2])<<' ';
				break;
		}

		if(str[0]=='E') {
			/*** printing section ***/
//			cout<<sshow(rootids)<<endl;
			int oi=0; //order index
			for(int i=0;i<rootids.size();i++) {
				//cout<<clauses[i]<<" "<<rootids[i]<<endl;
				if(bunsetsu) {
					if(rootids[i]>=0) {
						if(order)	cout<<clauses[i]<<" "<<rootids[i]<<endl;
						else		cout<<clauses[i]<<" "<<rootids[i]-i<<endl;
					} else			cout<<clauses[i]<<" "<<0<<endl;		//文末（自分自身に係ると解釈）
				} else {
					if(order) {
						if(rootids[i]>=0) {
							for(int j=0;j<clausesperword[i].size()-1;j++) {
								cout<</*oi<<'\t'<<*/clausesperword[i][j]<<"\tA\t"<<oi+2<<endl;
								oi++;
							}
							cout<</*oi<<'\t'<<*/clausesperword[i][clausesperword[i].size()-1]<<"\tA\t";
							int s=1;
							//文節内の単語数を数えて係り受け先を指定
							for(int j=i+1;j<rootids[i];j++) s+=clausesperword[j].size();
							cout<<s+oi+1<<endl;
							oi++;
						}
						else {
							for(int j=0;j<clausesperword[i].size()-1;j++) {
								cout<</*oi<<'\t'<<*/clausesperword[i][j]<<"\tA\t"<<oi+2<<endl;
								oi++;
							}
							cout<</*oi<<'\t'<<*/clausesperword[i][clausesperword[i].size()-1]<<"\tA\t0"<<endl;	//文末
							oi++;
						}
					} else {
						if(rootids[i]>=0) {
							for(int j=0;j<clausesperword[i].size()-1;j++)
								cout<<clausesperword[i][j]<<" 1"<<endl;
							cout<<clausesperword[i][clausesperword[i].size()-1]<<" ";
							int s=1;
							//文節内の単語数を数えて係り受け先を指定
							for(int j=i+1;j<rootids[i];j++) s+=clausesperword[j].size();
							cout<<s<<endl;
						}
						else {
							for(int j=0;j<clausesperword[i].size()-1;j++)
								cout<<clausesperword[i][j]<<" 1"<<endl;
							cout<<clausesperword[i][clausesperword[i].size()-1]<<" 0"<<endl;	//文末（自分自身に係ると解釈）
						}
					}
				}
			}
			//cout<<"EOS 0"<<endl;
			cout<<endl;

			clauses.clear();
			clauseperword.clear();
			clausesperword.clear();
			rootids.clear();
		}
		if(cin.eof())	break;
	}

//	if(!printingmode) cout<<endl<<"knpreader end:"<<eosnum<<endl;
	return 0;
}


