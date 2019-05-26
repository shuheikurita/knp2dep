//knp2vec.cpp
//Kurita

#include "knpreader.hpp"

//wordstreeのvector集合
//すなわちすべての文を格納する
vector<wordstree> corpus;

//Improved word2vec.c
//There are many diffrence.

//  Copyright 2013 Google Inc. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.


//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <math.h>
#include <pthread.h>

//#define MAX_STRING 100
#define MAX_FILE_LENGTH 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

//cn : word count in your corpus
//point : positions of parent nodes in *vocab array
//code : hash code
//codelen : hash length
struct vocab_word {
	long long cn;
	int *point;
	char /* *word,*/ *code, codelen;
	string word;
};

char train_file[MAX_FILE_LENGTH], output_file[MAX_FILE_LENGTH];
char save_vocab_file[MAX_FILE_LENGTH], read_vocab_file[MAX_FILE_LENGTH];
//struct vocab_word *vocab;
vector<vocab_word> vocab;
int binary = 0, cbow = 0, debug_mode = 2, /*window = 5,*/ min_count = 5, num_threads = 1, min_reduce = 1;
int *vocab_hash;
long long vocab_max_size = 1000, /*vocab_size = 0,*/ layer1_size = 100;
long long train_words = 0, /*word_count_actual = 0,trivial*/ file_size = 0, classes = 0;
real alpha = 0.025, starting_alpha, sample = 0;
real *syn0, *syn1, *syn1neg, *expTable;
size_t local_cache_size = 512 * 1024, local_cache_update = 100;
clock_t start;

int hs = 1, negative = 0;
const int table_size = 1e8;
int *table;

void InitUnigramTable() {
	int i;
	long long train_words_pow = 0;
	real d1, power = 0.75;
	table = (int *)malloc(table_size * sizeof(int));
	for (int a = 0; a < vocab.size(); a++) train_words_pow += pow(vocab[a].cn, power);
	i = 0;
	d1 = pow(vocab[i].cn, power) / (real)train_words_pow;
	for (int a = 0; a < table_size; a++) {
		table[a] = i;
		if (a / (real)table_size > d1) {
			i++;
			d1 += pow(vocab[i].cn, power) / (real)train_words_pow;
		}
		if (i >= vocab.size()) i = vocab.size() - 1;
	}
}

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
//void ReadWord(char *word, FILE *fin) {
//	int a = 0, ch;
//	while (!feof(fin)) {
//		ch = fgetc(fin);
//		if (ch == 13) continue;	//CR
//		if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
//			if (a > 0) {
//				if (ch == '\n') ungetc(ch, fin);
//				break;
//			}
//			if (ch == '\n') {
//				strcpy(word, (char *)"</s>");
//				return;
//			} else continue;
//		}
//		word[a] = ch;
//		a++;
//		if (a >= MAX_STRING - 1) a--;	 // Truncate too long words
//	}
//	word[a] = 0;
//}

// Returns hash value of a word
int GetWordHash(const string &word) {
	unsigned long long a, hash = 0;
	for (a = 0; a < word.size(); a++) hash = hash * 257 + word[a];
	hash = hash % vocab_hash_size;
	return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
//int SearchVocab(const char *word) {
int SearchVocab(const string &word) {
	unsigned int hash = GetWordHash(word);
	while (1) {
		if (vocab_hash[hash] == -1) return -1;
		//if (!strcmp(word != vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
		if (word == vocab[vocab_hash[hash]].word) return vocab_hash[hash];
		hash = (hash + 1) % vocab_hash_size;
	}
	return -1;
}

// Reads a word and returns its index in the vocabulary
//int ReadWordIndex(FILE *fin) {
//	char word[MAX_STRING];
//	ReadWord(word, fin);
//	if (feof(fin)) return -1;
//	return SearchVocab(word);
//}



string showvocab(const vector<vocab_word> &v) {
	ostringstream ss;
	for(vector<vocab_word>::iterator it=vocab.begin(); it!=vocab.end(); it++) {
		ss<<(*it).cn<<'\t'<<(*it).word<<endl;
	}
	return ss.str();
}

// Used later for sorting by word counts
//int VocabCompare(const void *a, const void *b) {
//		return ((struct vocab_word *)b)->cn - ((struct vocab_word *)a)->cn;
//}
bool vocabcompare(const vocab_word &a, const vocab_word &b) {
		return (a.cn>b.cn);
}

// Sorts the vocabulary by frequency using word counts
void SortVocab() {
	// Sort the vocabulary and keep </s> at the first position
	// 降順ソート
	//qsort(&vocab[1], vocab_size - 1, sizeof(struct vocab_word), VocabCompare);
	sort(vocab.begin()+1, vocab.end(), vocabcompare);
	//cerr<<showvocab(vocab);
	for(int a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	train_words = 0;
	for(int i=vocab.size()-1; i>=0; i--) {
		// Words occuring less than min_count times will be discarded from the vocab
		if (vocab[i].cn < min_count) {
			//vocab_size--;
			//free(vocab[vocab_size].word);
			free(vocab[i].code);
			free(vocab[i].point);
			vocab.pop_back();
		} else {
			// Hash will be re-computed, as after the sorting it is not actual
			unsigned int hash=GetWordHash(vocab[i].word);
			while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
			vocab_hash[hash] = i;
			train_words += vocab[i].cn;
		}
	}
	//vocab = (struct vocab_word *)realloc(vocab, (vocab_size + 1) * sizeof(struct vocab_word));
	// Allocate memory for the binary tree construction
	//for (a = 0; a < vocab_size; a++) {
	//	vocab[a].code = (char *)calloc(MAX_CODE_LENGTH, sizeof(char));
	//	vocab[a].point = (int *)calloc(MAX_CODE_LENGTH, sizeof(int));
	//}
}

// Create binary Huffman tree using the word counts
// Frequent words will have short uniqe binary codes
void CreateBinaryTree() {
	long long a, b, i, min1i, min2i, pos1, pos2, point[MAX_CODE_LENGTH];
	char code[MAX_CODE_LENGTH];
	long long *count       = (long long *)calloc(vocab.size() * 2 + 1, sizeof(long long));
	long long *binary      = (long long *)calloc(vocab.size() * 2 + 1, sizeof(long long));
	long long *parent_node = (long long *)calloc(vocab.size() * 2 + 1, sizeof(long long));
	for (a = 0; a < vocab.size(); a++) count[a] = vocab[a].cn;
	for (a = vocab.size(); a < vocab.size() * 2; a++) count[a] = 1e15;
	pos1 = vocab.size() - 1;
	pos2 = vocab.size();
	// Following algorithm constructs the Huffman tree by adding one node at a time
	for (a = 0; a < vocab.size() - 1; a++) {
		// First, find two smallest nodes 'min1, min2'
		if (pos1 >= 0) {
			if (count[pos1] < count[pos2]) {
				min1i = pos1;
				pos1--;
			} else {
				min1i = pos2;
				pos2++;
			}
		} else {
			min1i = pos2;
			pos2++;
		}
		if (pos1 >= 0) {
			if (count[pos1] < count[pos2]) {
				min2i = pos1;
				pos1--;
			} else {
				min2i = pos2;
				pos2++;
			}
		} else {
			min2i = pos2;
			pos2++;
		}
		count[vocab.size() + a] = count[min1i] + count[min2i];
		parent_node[min1i] = vocab.size() + a;
		parent_node[min2i] = vocab.size() + a;
		binary[min2i] = 1;
	}
	// Now assign binary code to each vocabulary word
	for (a = 0; a < vocab.size(); a++) {
		b = a;
		i = 0;
		while (1) {
			code[i] = binary[b];
			point[i] = b;
			i++;
			b = parent_node[b];
			if (b == vocab.size() * 2 - 2) break;
		}
		vocab[a].codelen = i;
		vocab[a].point[0] = 0;
		for (b = 0; b < i; b++) {
			vocab[a].code[i - b - 1] = code[b];
			vocab[a].point[i - b] = vocab.size() * 2 - 2 - point[b];
		}
	}
	free(count);
	free(binary);
	free(parent_node);
}

// Adds a word to the vocabulary
int AddWordToVocab(const string &word) {
	//unsigned int hash, length = strlen(word) + 1;
	unsigned int hash/*, length = word.size() + 1*/;
	//if (length > MAX_STRING) length = MAX_STRING;
//	fprintf(stderr,"<calloc>");
	//vocab[vocab_size].word = (char *)malloc(length*sizeof(char));
	//strcpy(vocab[vocab_size].word, word);
	vocab_word vw;
	vw.word=word;
	vw.cn=1;
	vw.code = (char *)calloc(MAX_CODE_LENGTH, sizeof(char));
	vw.point = (int *)calloc(MAX_CODE_LENGTH, sizeof(int));
	vocab.push_back(vw);
	//vocab[vocab_size].word = word;
	//vocab[vocab_size].cn = 0;
	//vocab_size++;
//	fprintf(stderr,"</calloc>");
//	fflush(stderr);
	// Reallocate memory if needed
	//if (vocab.size() + 2 >= vocab_max_size) {
	//	vocab_max_size += 1000;
	//	fprintf(stderr,"<realloc>");
	//	//vocab = (struct vocab_word *)realloc(vocab, vocab_max_size * sizeof(struct vocab_word));
	//	vocab.reserve(vocab_max_size);
	//	fprintf(stderr,"</realloc>\n");
	//}
	hash = GetWordHash(word);
	while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
	vocab_hash[hash] = vocab.size() - 1;
	//return vocab_size - 1;
	return vocab.size();
}

// Reduces the vocabulary by removing infrequent tokens	
void ReduceVocab() {
	//int b = 0;
	unsigned int hash;
	//for(int a = 0; a < vocab.size(); a++) if (vocab[a].cn > min_reduce) {
	//	vocab[b].cn = vocab[a].cn;
	//	vocab[b].word = vocab[a].word;
	//	b++;
	//} else /*free(vocab[a].word)*/;
	for(vector<vocab_word>::iterator it=vocab.begin(); it!=vocab.end();) if((*it).cn <= min_reduce) {
		free((*it).code);
		free((*it).point);
		it=vocab.erase(it);
	} else it++;
	//vocab_size = b;
	for(int a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	for(int a = 0; a < vocab.size(); a++) {
		// Hash will be re-computed, as it is not actual
		hash = GetWordHash(vocab[a].word);
		while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
		vocab_hash[hash] = a;
	}
	fflush(stdout);
	min_reduce++;
}

//Kurita
void getvocabfromwordstree(wordstree &wt) {
		//ReadWord(word, fin);
		//strcpy(word,corpus[i].word.c_str());
//		if (feof(fin)) break;
		train_words++;
//		fprintf(stderr,"%d ",train_words);
		if ((debug_mode > 1) && (train_words % 100000 == 0)) {	//???
			printf("%lldK%c", train_words / 1000, 13);
			fflush(stdout);
		}
		//fprintf(stderr,"<SV>",train_words);
		int i = SearchVocab(wt.word.c_str());
		//fprintf(stderr,"<AV i=%d word=%s>",i,wt.word.c_str());
		//if (i == -1) {
		//	int a = AddWordToVocab(wt.word);
		//	vocab[a].cn = 1;
		//} else vocab[i].cn++;
		if (i == -1) AddWordToVocab(wt.word);
		else vocab[i].cn++;
		//fprintf(stderr,"<RV>\n");
		if (vocab.size() > vocab_hash_size * 0.7) ReduceVocab();
		//fprintf(stderr,"</SRVR> ");
		
		//if(wt.leaf.empty()) return;
		for(int i=0;i<wt.leaf.size();i++)
			getvocabfromwordstree(wt.leaf[i]);
		return;
}

void LearnVocabFromTrainFile() {
	//vector<wordstree> corpus = knpreader("000000.knp");
	corpus = knpreader(train_file);
	//char word[MAX_STRING];
	//FILE *fin;
	//long long a, i;
	for (int a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
	//fin = fopen(train_file, "rb");
	//if (fin == NULL) {
	//	printf("ERROR: training data file not found!\n");
	//	exit(1);
	//}
	//vocab_size = 0;
	AddWordToVocab((char *)"</s>");
	//while (1) {
	for(int i=0;i<corpus.size();i++) {
		//if(!(i%100)) printf("%f%%\n",((double)i)/corpus.size());
		getvocabfromwordstree(corpus[i]);
	}
	SortVocab();
	//if (debug_mode > 0) {
	if(1) {
		printf("Vocab size: %lld\n", vocab.size());
		printf("Words in train file: %lld\n", train_words);
	}
	//file_size = ftell(fin);
	//fclose(fin);

}

void SaveVocab() {
	long long i;
	FILE *fo = fopen(save_vocab_file, "wb");
	for (i = 0; i < vocab.size(); i++) fprintf(fo, "%s %lld\n", vocab[i].word.c_str(), vocab[i].cn);
	fclose(fo);
}

//void ReadVocab() {
//	long long a, i = 0;
//	char c;
//	char word[MAX_STRING];
//	FILE *fin = fopen(read_vocab_file, "rb");
//	if (fin == NULL) {
//		printf("Vocabulary file not found\n");
//		exit(1);
//	}
//	for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
//	vocab_size = 0;
//	while (1) {
//		ReadWord(word, fin);
//		if (feof(fin)) break;
//		a = AddWordToVocab(word);
//		fscanf(fin, "%lld%c", &vocab[a].cn, &c);
//		i++;
//	}
//	SortVocab();
//	if (debug_mode > 0) {
//		printf("Vocab size: %lld\n", vocab_size);
//		printf("Words in train file: %lld\n", train_words);
//	}
//	fin = fopen(train_file, "rb");
//	if (fin == NULL) {
//		printf("ERROR: training data file not found!\n");
//		exit(1);
//	}
//	fseek(fin, 0, SEEK_END);
//	file_size = ftell(fin);////////////////////////////TODO
//	fclose(fin);
//}

void InitNet() {
	long long a, b;
	a = posix_memalign((void **)&syn0, 128, (long long)vocab.size() * layer1_size * sizeof(real));
	if (syn0 == NULL) {printf("Memory allocation failed\n"); exit(1);}
	if (hs) {
		a = posix_memalign((void **)&syn1, 128, (long long)vocab.size() * layer1_size * sizeof(real));
		if (syn1 == NULL) {printf("Memory allocation failed\n"); exit(1);}
		for (b = 0; b < layer1_size; b++) for (a = 0; a < vocab.size(); a++)
		 syn1[a * layer1_size + b] = 0;
	}
	if (negative>0) {
		a = posix_memalign((void **)&syn1neg, 128, (long long)vocab.size() * layer1_size * sizeof(real));
		if (syn1neg == NULL) {printf("Memory allocation failed\n"); exit(1);}
		for (b = 0; b < layer1_size; b++) for (a = 0; a < vocab.size(); a++)
		 syn1neg[a * layer1_size + b] = 0;
	}
	for (b = 0; b < layer1_size; b++) for (a = 0; a < vocab.size(); a++)
	 syn0[a * layer1_size + b] = (rand() / (real)RAND_MAX - 0.5) / layer1_size;
	//printf("Kurita:CBT\n");
	CreateBinaryTree();
	//printf("Kurita:CBT end\n");
}

//文中の最大ノード分岐数
#define MAX_NODE 40
//ノードあたりの最大葉数+2
#define MAX_LEAF 22

/* 
The structure of 2-dimensional array, sen(knp2vec) is

num_of_leaves+2 wordid leaf_1_id leaf_2_id ...
num_of_leaves+2 wordid leaf_1_id leaf_2_id ...
...

where num_of_leaves must be in {1,...,MAX_LEAF-2}.
*/

//wordstreeから転写
//recursive function
long long copywordstree(wordstree &wt, long long (*sen)[MAX_LEAF], long long node) {
//printf("Entering copywordstree...\"%s\"\n",showwordstree(wt).c_str());
	//word = ReadWordIndex(fi);
	long long i=0,j=2;
	long long root = SearchVocab(wt.word.c_str());
	if (root != -1) {	//not out of vocabulary
		sen[node][1] = root;
		for(i=0; i<wt.leaf.size(); i++) {
			long long leaf = SearchVocab(wt.leaf[i].word.c_str());
			if(leaf != -1) {
				sen[node][j++] = leaf;
				if(j==MAX_LEAF) break;
			}
		}
		if(j>2) { //vocabulary内で完結している係り受け関係がひとつもない場合は無視
			sen[node][i+1] = 0;
			sen[node][0]   = j;
			node++;
		}
	}
	for(i=0; i<wt.leaf.size(); i++) {
//printf("recursive\n");
		node = copywordstree(wt.leaf[i],sen,node);
		if(node==MAX_NODE) break;
	}
	return node;

	// The subsampling randomly discards frequent words while keeping the ranking same
	//if (sample > 0) {
	//	real ran = (sqrt(vocab[word].cn / (sample * train_words)) + 1) * (sample * train_words) / vocab[word].cn;
	//	next_random = next_random * (unsigned long long)25214903917 + 11;
	//	if (ran < (next_random & 0xFFFF) / (real)65536) continue;
	//}

}

void *TrainModelThread(void *tid) {
	printf("Entering TrainModelThread(%d)...\n",tid);
	//sen[] : 文ごとに単語のIDを格納するための配列
	//sentence_position : 単語数で数えた単語の位置
	// >> sentence_num : 文の位置
	//b >> winrand
	//last_word >> rootid
	//long long /*a, b,*/winrand, /*d, word, last_word,*/rootid, sentence_length = 0/*, sentence_position = 0*/;
	//long long /*word_count = 0, last_word_count = 0, Theses are trivial.*/ sen[MAX_SENTENCE_LENGTH + 1];
	long long sen[MAX_NODE][MAX_LEAF];
	long long l1, l2, /*c,*/ target, label;
	unsigned long long next_random = (long long)tid;
	//real f, g;
	clock_t now;
	//
	real *neu1 = (real *)calloc(layer1_size, sizeof(real));
	real *neu1e = (real *)calloc(layer1_size, sizeof(real));
	size_t num_updates = 0;
	size_t local_syn1_size = (size_t)(local_cache_size / (real)(layer1_size * sizeof(real)));
	real *syn1_local = NULL, *syn1_diff = NULL, *syn1neg_local = NULL, *syn1neg_diff = NULL;
	//FILE *fi = fopen(train_file, "rb");
	//fseek(fi, file_size / (long long)num_threads * (long long)id, SEEK_SET);

	if (0 < local_syn1_size) {
		long long a;
		if (hs) {
			a = posix_memalign((void **)&syn1_local, 128, (long long)local_syn1_size * layer1_size * sizeof(real));
			if (syn1_local == NULL) {printf("Memory allocation failed\n"); exit(1);}
			for (a = 0; a < local_syn1_size * layer1_size; a++)
				syn1_local[a] = syn1[a];
			a = posix_memalign((void **)&syn1_diff, 128, (long long)local_syn1_size * layer1_size * sizeof(real));
			if (syn1_diff == NULL) {printf("Memory allocation failed\n"); exit(1);}
			for (a = 0; a < local_syn1_size * layer1_size; a++)
				syn1_diff[a] = 0;
		}
		if (negative>0) {
			a = posix_memalign((void **)&syn1neg_local, 128, (long long)local_syn1_size * layer1_size * sizeof(real));
			if (syn1neg_local == NULL) {printf("Memory allocation failed\n"); exit(1);}
			for (a = 0; a < local_syn1_size * layer1_size; a++)
				syn1neg_local[a] = syn1neg[a];
			a = posix_memalign((void **)&syn1neg_diff, 128, (long long)local_syn1_size * layer1_size * sizeof(real));
			if (syn1neg_diff == NULL) {printf("Memory allocation failed\n"); exit(1);}
			for (a = 0; a < local_syn1_size * layer1_size; a++)
				syn1neg_diff[a] = 0;
		}
	}
	//scount : sentence count
	//コーパスイテレータ
	for(long long scount=corpus.size()/(long long)num_threads*(long long)tid;
			scount<corpus.size()/(long long)num_threads*((long long)tid+1); scount++) {	//scount means "sentence count".
//		if (word_count - last_word_count > 10000) {						//学習係数alphaの更新
		if(!(scount%10000)) {						//学習係数alphaの更新
//			word_count_actual += word_count - last_word_count;
//			last_word_count = word_count;
			if ((debug_mode > 1)) {
				now=clock();
				printf("%cAlpha: %f	Progress: %.2f%%	Words/thread/sec: %.2fk	", 13, alpha,
				 scount / (real)(corpus.size() + 1) * 100,
				 scount / ((real)(now - start + 1) / (real)CLOCKS_PER_SEC * 1000));
				fflush(stdout);
			}
			//alpha = starting_alpha * (1 - word_count_actual / (real)(train_words + 1));	//alphaは学習係数
			alpha   = starting_alpha * (1 - scount       / (real)(corpus.size() + 1));	//alphaは学習係数
			if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;		//alphaの下限
		}
		//word2vec:文中の単語をsenに書き出す
		//knp2vec :wordstreeからwordsidtreeに転写する
		long long maxdeps = copywordstree(corpus[scount],sen,0);
		//文中イテレータ
		// >> knp2vecでは構文木イテレータに変更
		for(long long dep=0; dep<maxdeps; dep++) {
			//if (feof(fi)) break;
			//if (word_count > train_words / num_threads) break;	//>>for loop condition
			long long word = sen[dep][1];
			//if (word == -1) continue;
			for (long long c = 0; c < layer1_size; c++) neu1[c] = 0;
			for (long long c = 0; c < layer1_size; c++) neu1e[c] = 0;
			//next_random = next_random * (unsigned long long)25214903917 + 11;
			//winrand = next_random % window;	//winrand \in {0,..,window-1}
			if (cbow) {	//train the cbow architecture
				printf("Kurita : Sorry, we can't use CBOW yet.\nInstead, use \"-cbow 0\" for skip-gram.\n");
				exit(1);
//				// in -> hidden
//				for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
//					c = sentence_position - window + a;
//					if (c < 0) continue;
//					if (c >= sentence_length) continue;
//					last_word = sen[c];
//					if (last_word == -1) continue;
//					for (c = 0; c < layer1_size; c++) neu1[c] += syn0[c + last_word * layer1_size];
//				}
//				if (hs) for (d = 0; d < vocab[word].codelen; d++) {
//					real *_syn1 = (vocab[word].point[d] < local_syn1_size) ? syn1_local : syn1;
//					f = 0;
//					l2 = vocab[word].point[d] * layer1_size;
//					// Propagate hidden -> output
//					for (c = 0; c < layer1_size; c++) f += neu1[c] * _syn1[c + l2];
//					if (f <= -MAX_EXP) continue;
//					else if (f >= MAX_EXP) continue;
//					else f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
//					// 'g' is the gradient multiplied by the learning rate
//					g = (1 - vocab[word].code[d] - f) * alpha;
//					// Propagate errors output -> hidden
//					for (c = 0; c < layer1_size; c++) neu1e[c] += g * _syn1[c + l2];
//					// Learn weights hidden -> output
//					for (c = 0; c < layer1_size; c++) _syn1[c + l2] += g * neu1[c];
//					if (vocab[word].point[d] < local_syn1_size)
//						for (c = 0; c < layer1_size; c++) syn1_diff[c + l2] += g * neu1[c];
//				}
//				// NEGATIVE SAMPLING
//				if (negative > 0) for (d = 0; d < negative + 1; d++) {
//					real *_syn1neg = NULL;
//					if (d == 0) {
//						target = word;
//						label = 1;
//					} else {
//						next_random = next_random * (unsigned long long)25214903917 + 11;
//						target = table[(next_random >> 16) % table_size];
//						if (target == 0) target = next_random % (vocab.size() - 1) + 1;
//						if (target == word) continue;
//						label = 0;
//					}
//					_syn1neg = (target < local_syn1_size) ? syn1neg_local : syn1neg;
//					l2 = target * layer1_size;
//					f = 0;
//					for (c = 0; c < layer1_size; c++) f += neu1[c] * _syn1neg[c + l2];
//					if (f > MAX_EXP) g = (label - 1) * alpha;
//					else if (f < -MAX_EXP) g = (label - 0) * alpha;
//					else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;
//					for (c = 0; c < layer1_size; c++) neu1e[c] += g * _syn1neg[c + l2];
//					for (c = 0; c < layer1_size; c++) _syn1neg[c + l2] += g * neu1[c];
//					if (target < local_syn1_size)
//						for (c = 0; c < layer1_size; c++) syn1neg_diff[c + l2] += g * neu1[c];
//				}
//				// hidden -> in
//				for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
//					c = sentence_position - window + a;
//					if (c < 0) continue;
//					if (c >= sentence_length) continue;
//					last_word = sen[c];
//					if (last_word == -1) continue;
//					for (c = 0; c < layer1_size; c++) syn0[c + last_word * layer1_size] += neu1e[c];
//				}
			} else {	//train skip-gram
				//for(long long a = winrand; a < window * 2 + 1 - winrand; a++) if (a != window) { //a : ???
				for(long long leaf=2; leaf<sen[dep][0]; leaf++) { //a : ???
					//c = sentence_position - window + a;
					//if (c < 0) continue;
					//if (c >= sentence_length) continue;
					//rootid = sen[c];
					//if (rootid == -1) continue;
					l1 = sen[dep][leaf] * layer1_size;	//???
					for(long long i=0; i<layer1_size; i++) neu1e[i] = 0;
					// HIERARCHICAL SOFTMAX
					if (hs) {
						for (long long i = 0; i < vocab[word].codelen; i++) {
							real *_syn1 = (vocab[word].point[i] < local_syn1_size) ? syn1_local : syn1;
							real f = 0;
							l2 = vocab[word].point[i] * layer1_size;
							// Propagate hidden -> output
							for(long long j = 0; j < layer1_size; j++) f += syn0[j + l1] * _syn1[j + l2];
							if (f <= -MAX_EXP) continue;
							else if (f >= MAX_EXP) continue;
							else f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
							// 'g' is the gradient multiplied by the learning rate
							real g = (1 - vocab[word].code[i] - f) * alpha;
							// Propagate errors output -> hidden
							for(long long j = 0; j < layer1_size; j++) neu1e[j] += g * _syn1[j + l2];
							// Learn weights hidden -> output
							for(long long j = 0; j < layer1_size; j++) _syn1[j + l2] += g * syn0[j + l1];
							if (vocab[word].point[i] < local_syn1_size)
								for(long long j = 0; j < layer1_size; j++) syn1_diff[j + l2] += g * syn0[j + l1];
						}
					}
					// NEGATIVE SAMPLING
					if (negative > 0) {
						for (long long i = 0; i < negative + 1; i++) {
							real *_syn1neg = NULL;
							if(i == 0) {
								target = word;
								label = 1;
							} else {
								next_random = next_random * (unsigned long long)25214903917 + 11;
								// >>16 is /(2^16)
								target = table[(next_random >> 16) % table_size];
								if (target == 0) target = next_random % (vocab.size() - 1) + 1;
								if (target == word) continue;
								label = 0;
							}
							_syn1neg = (target < local_syn1_size) ? syn1neg_local : syn1neg;
							l2 = target * layer1_size;
							real f = 0, g;
							for(long long c = 0; c < layer1_size; c++) f += syn0[c + l1] * _syn1neg[c + l2];
							if(f > MAX_EXP) g = (label - 1) * alpha;
							else if (f < -MAX_EXP) g = (label - 0) * alpha;
							else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;
							for(long long c = 0; c < layer1_size; c++) neu1e[c] += g * _syn1neg[c + l2];
							for(long long c = 0; c < layer1_size; c++) _syn1neg[c + l2] += g * syn0[c + l1];
							if(target < local_syn1_size)
								for(long long c = 0; c < layer1_size; c++) syn1neg_diff[c + l2] += g * syn0[c + l1];
						}
					}
					// Learn weights input -> hidden
					for(int i=0; i<layer1_size; i++) syn0[i+l1] += neu1e[i];
				}
			}
			//sentence_position++;
			//if (sentence_position >= sentence_length) {
			//	//sentence_length = 0;
			//	//continue;
			//	break;
			//}
			num_updates++;
			if (local_cache_update <= num_updates) {
				if (hs) {
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1[a] += syn1_diff[a];
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1_local[a] = syn1[a];
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1_diff[a] = 0;
				}
				if (negative > 0) {
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1neg[a] += syn1neg_diff[a];
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1neg_local[a] = syn1neg[a];
					for(long long a = 0; a < local_syn1_size * layer1_size; a++)
						syn1neg_diff[a] = 0;
				}
				num_updates = 0;
			}
		}//end_for sentence_position
	}
	//fclose(fi);
	free(syn1neg_diff);
	free(syn1neg_local);
	free(syn1_diff);
	free(syn1_local);
	free(neu1);
	free(neu1e);
	pthread_exit(NULL);
}

void TrainModel() {
	time_t begin;
	long a, b, c, d;
	FILE *fo;
	pthread_t *pt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
	printf("Starting training using file %s\n", train_file);
	starting_alpha = alpha;
	if (read_vocab_file[0] != 0) {
		//ReadVocab();
		printf("Kurita : ReadVocab() can't be used!\n");
		exit(1);
	} else LearnVocabFromTrainFile();
	if (save_vocab_file[0] != 0) SaveVocab();
	if (output_file[0] == 0) return;

	//Kurita:NN
//	printf("Kurita : InitNet()\n");
	InitNet();
	if (negative > 0) InitUnigramTable();
	start = clock();
	printf("creating %d threads ...\n",num_threads);
	for (a = 0; a < num_threads; a++) pthread_create(&pt[a], NULL, TrainModelThread, (void *)a);
	for (a = 0; a < num_threads; a++) pthread_join(pt[a], NULL);

//	printf("Kurita : train end\n");
	//Kurita:after
	fo = fopen(output_file, "wb");
	if (classes == 0) {
		// Save the word vectors
		fprintf(fo, "%lld %lld\n", vocab.size(), layer1_size);
		for (a = 0; a < vocab.size(); a++) {
			fprintf(fo, "%s ", vocab[a].word.c_str());
			if (binary) for (b = 0; b < layer1_size; b++) fwrite(&syn0[a * layer1_size + b], sizeof(real), 1, fo);
			else for (b = 0; b < layer1_size; b++) fprintf(fo, "%lf ", syn0[a * layer1_size + b]);
			fprintf(fo, "\n");
		}
	} else {
		// Run K-means on the word vectors
		int clcn = classes, iter = 10, closeid;
		int *centcn = (int *)malloc(classes * sizeof(int));
		int *cl = (int *)calloc(vocab.size(), sizeof(int));
		real closev, x;
		real *cent = (real *)calloc(classes * layer1_size, sizeof(real));
		for (a = 0; a < vocab.size(); a++) cl[a] = a % clcn;
		for (a = 0; a < iter; a++) {
			for (b = 0; b < clcn * layer1_size; b++) cent[b] = 0;
			for (b = 0; b < clcn; b++) centcn[b] = 1;
			for (c = 0; c < vocab.size(); c++) {
				for (d = 0; d < layer1_size; d++) cent[layer1_size * cl[c] + d] += syn0[c * layer1_size + d];
				centcn[cl[c]]++;
			}
			for (b = 0; b < clcn; b++) {
				closev = 0;
				for (c = 0; c < layer1_size; c++) {
					cent[layer1_size * b + c] /= centcn[b];
					closev += cent[layer1_size * b + c] * cent[layer1_size * b + c];
				}
				closev = sqrt(closev);
				for (c = 0; c < layer1_size; c++) cent[layer1_size * b + c] /= closev;
			}
			for (c = 0; c < vocab.size(); c++) {
				closev = -10;
				closeid = 0;
				for (d = 0; d < clcn; d++) {
					x = 0;
					for (b = 0; b < layer1_size; b++) x += cent[layer1_size * d + b] * syn0[c * layer1_size + b];
					if (x > closev) {
						closev = x;
						closeid = d;
					}
				}
				cl[c] = closeid;
			}
		}
		// Save the K-means classes
		for (a = 0; a < vocab.size(); a++) fprintf(fo, "%s %d\n", vocab[a].word.c_str(), cl[a]);
		free(centcn);
		free(cent);
		free(cl);
	}
	fclose(fo);
}

int ArgPos(char *str, int argc, char **argv) {
	int a;
	for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
		if (a == argc - 1) {
			printf("Argument missing for %s\n", str);
			exit(1);
		}
		return a;
	}
	return -1;
}

#define LEAFMAX 10
struct wordidtree {
	int root;
	int leaf[LEAFMAX];
};




int main(int argc, char **argv) {
//	knpreader("000000.knp");return 0;
	int i;
	if (argc == 1) {
		printf("WORD VECTOR estimation toolkit v 0.1b\n\n");
		printf("Options:\n");
		printf("Parameters for training:\n");
		printf("\t-train <file>\n");
		printf("\t\tUse text data from <file> to train the model\n");
		printf("\t-output <file>\n");
		printf("\t\tUse <file> to save the resulting word vectors / word clusters\n");
		printf("\t-size <int>\n");
		printf("\t\tSet size of word vectors; default is 100\n");
		printf("\t-window <int>\n");
		printf("\t\tSet max skip length between words; default is 5\n");
		printf("\t-sample <float>\n");
		printf("\t\tSet threshold for occurrence of words. Those that appear with higher frequency");
		printf(" in the training data will be randomly down-sampled; default is 0 (off), useful value is 1e-5\n");
		printf("\t-hs <int>\n");
		printf("\t\tUse Hierarchical Softmax; default is 1 (0 = not used)\n");
		printf("\t-negative <int>\n");
		printf("\t\tNumber of negative examples; default is 0, common values are 5 - 10 (0 = not used)\n");
		printf("\t-threads <int>\n");
		printf("\t\tUse <int> threads (default 1)\n");
		printf("\t-cache-update <int>\n");
		printf("\t\tSynchronize after <int> updates (default 100)\n");
		printf("\t-min-count <int>\n");
		printf("\t\tThis will discard words that appear less than <int> times; default is 5\n");
		printf("\t-alpha <float>\n");
		printf("\t\tSet the starting learning rate; default is 0.025\n");
		printf("\t-classes <int>\n");
		printf("\t\tOutput word classes rather than word vectors; default number of classes is 0 (vectors are written)\n");
		printf("\t-debug <int>\n");
		printf("\t\tSet the debug mode (default = 2 = more info during training)\n");
		printf("\t-binary <int>\n");
		printf("\t\tSave the resulting vectors in binary moded; default is 0 (off)\n");
		printf("\t-save-vocab <file>\n");
		printf("\t\tThe vocabulary will be saved to <file>\n");
		printf("\t-read-vocab <file>\n");
		printf("\t\tThe vocabulary will be read from <file>, not constructed from the training data\n");
		printf("\t-cbow <int>\n");
		printf("\t\tUse the continuous bag of words model; default is 0 (skip-gram model)\n");
		printf("\nExamples:\n");
		printf("./word2vec -train data.txt -output vec.txt -debug 2 -size 200 -window 5 -sample 1e-4 -negative 5 -hs 0 -binary 0 -cbow 1\n\n");
		return 0;
	}
	output_file[0] = 0;
	save_vocab_file[0] = 0;
	read_vocab_file[0] = 0;
	if ((i = ArgPos((char *)"-size", argc, argv)) > 0) layer1_size = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-train", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0) strcpy(save_vocab_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0) strcpy(read_vocab_file, argv[i + 1]);
	if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-cbow", argc, argv)) > 0) cbow = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0) alpha = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
//	if ((i = ArgPos((char *)"-window", argc, argv)) > 0) window = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-sample", argc, argv)) > 0) sample = atof(argv[i + 1]);
	if ((i = ArgPos((char *)"-hs", argc, argv)) > 0) hs = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-negative", argc, argv)) > 0) negative = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-threads", argc, argv)) > 0) num_threads = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-cache-update", argc, argv)) > 0) local_cache_update = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0) min_count = atoi(argv[i + 1]);
	if ((i = ArgPos((char *)"-classes", argc, argv)) > 0) classes = atoi(argv[i + 1]);
	//vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
	vocab.reserve(vocab_max_size);
	vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
	expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
	for (i = 0; i < EXP_TABLE_SIZE; i++) {
		expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
		expTable[i] = expTable[i] / (expTable[i] + 1);									 // Precompute f(x) = x / (x + 1)
	}
//	printf("Entering training mode...\n");
	TrainModel();
	return 0;
}


