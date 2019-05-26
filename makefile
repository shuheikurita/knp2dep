CC = g++
#The -Ofast might not work with older versions of gcc; in that case, use -O2
CFLAGS = -lm -pthread -Ofast -march=native -funroll-loops -Wno-unused-result -std=c++1
CLAPACK = -lrefblas -llapack -lgfortran

all: word2vec word2phrase distance word-analogy compute-accuracy

knp2dep : knp2dep.cpp
	$(CC) knp2dep.cpp -o knp2dep $(CFLAGS)
flexvec : flexvec.cpp
	$(CC) flexvec.cpp -o flexvec $(CFLAGS)
complearn : complearn.cpp classmt19937ar.cpp
	$(CC) complearn.cpp classmt19937ar.cpp -o complearn $(CFLAGS) $(CLAPACK)
complearn_origin : complearn_origin.cpp classmt19937ar.cpp
	$(CC) complearn_origin.cpp classmt19937ar.cpp -o complearn_origin $(CFLAGS) $(CLAPACK)
comptest : comptest.cpp classmt19937ar.cpp
	$(CC) comptest.cpp classmt19937ar.cpp -o comptest $(CFLAGS) $(CLAPACK)
#classmt19937.cpp : classmt19937.cpp
2gram2vec : 2gram2vec.c
	$(CC) 2gram2vec.c -o 2gram2vec $(CFLAGS)
knpprinter : knpprinter.cpp
	$(CC) knpprinter.cpp -o knpprinter $(CFLAGS)
wordsim : wordsim.cpp
	$(CC) wordsim.cpp -o wordsim $(CFLAGS)
word2vec : word2vec.c
	$(CC) word2vec.c -o word2vec $(CFLAGS)
word2vec_neg : word2vec_neg0.c
	$(CC) word2vec_neg0.c -o word2vec_neg0 $(CFLAGS)
word2vec_mixed : word2vec_mixed.c
	$(CC) word2vec_mixed.c -o word2vec_mixed $(CFLAGS)
word2vec_non_local : word2vec_non_local.c
	$(CC) word2vec_non_local.c -o word2vec_non_local $(CFLAGS)
word2phrase : word2phrase.c
	$(CC) word2phrase.c -o word2phrase $(CFLAGS)
distance : distance.c
	$(CC) distance.c -o distance $(CFLAGS)
word-analogy : word-analogy.c
	$(CC) word-analogy.c -o word-analogy $(CFLAGS)
compute-accuracy : compute-accuracy.c
	$(CC) compute-accuracy.c -o compute-accuracy $(CFLAGS)
	chmod +x *.sh

clean:
	rm -rf word2vec word2phrase distance word-analogy compute-accuracy
