/*
 * DC3Algorithm.cpp
 *
 *  Created on: Sep 17, 2014
 *      Author: matan
 */

#include <DC3Algorithm.h>

#include <ctime>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stack>
#include <Log.h>
#include <fstream>

const unsigned char DC3Algorithm::mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04,
		0x02, 0x01 };

// find the start or end of each bucket
void DC3Algorithm::getBuckets(unsigned char *s, int *bkt, int n, int K, int cs,
		bool end) {
	int i, sum = 0;
	for (i = 0; i <= K; i++)
		bkt[i] = 0; // clear all buckets
	for (i = 0; i < n; i++)
		bkt[chr(i)]++; // compute the size of each bucket
	for (i = 0; i <= K; i++) {
		sum += bkt[i];
		bkt[i] = end ? sum : sum - bkt[i];
	}
}

// compute SAl
void DC3Algorithm::induceSAl(unsigned char *t, int *SA, unsigned char *s,
		int *bkt, int n, int K, int cs, bool end) {
	int i, j;
	getBuckets(s, bkt, n, K, cs, end); // find starts of buckets
	for (i = 0; i < n; i++) {
		j = SA[i] - 1;
		if (j >= 0 && !tget(j))
			SA[bkt[chr(j)]++] = j;
	}
}

// compute SAs
void DC3Algorithm::induceSAs(unsigned char *t, int *SA, unsigned char *s,
		int *bkt, int n, int K, int cs, bool end) {
	int i, j;
	getBuckets(s, bkt, n, K, cs, end); // find ends of buckets
	for (i = n - 1; i >= 0; i--) {
		j = SA[i] - 1;
		if (j >= 0 && tget(j))
			SA[--bkt[chr(j)]] = j;
	}
}

// find the suffix array SA of s[0..n-1] in {1..K}^n
// K = number of buckets, equal to the number of options in a char (256)
// cs = character size in bytes (1 enough for our alphabet)
// require s[n-1]=0 (the sentinel!), n>=2
// use a working space (excluding s and SA) of at most 2.25n+O(1) for a constant alphabet
void DC3Algorithm::SA_IS(unsigned char *s, int *SA, int n, int K, int cs) {
	int i, j;
	unsigned char *t = (unsigned char *) malloc(n / 8 + 1); // LS-type array in bits
	// Classify the type of each character
	tset(n - 2, 0);
	tset(n - 1, 1); // the sentinel must be in s1, important!!!
	for (i = n - 3; i >= 0; i--)
		tset(i, (chr(i)<chr(i+1) || (chr(i)==chr(i+1) && tget(i+1)==1))?1:0);
	// stage 1: reduce the problem by at least 1/2
	// sort all the S-substrings
	int *bkt = (int *) malloc(sizeof(int) * (K + 1)); // bucket array
	getBuckets(s, bkt, n, K, cs, true); // find ends of buckets
	for (i = 0; i < n; i++)
		SA[i] = -1;
	for (i = 1; i < n; i++)
		if (isLMS(i))
			SA[--bkt[chr(i)]] = i;
	induceSAl(t, SA, s, bkt, n, K, cs, false);
	induceSAs(t, SA, s, bkt, n, K, cs, true);
	free(bkt);
	// compact all the sorted substrings into the first n1 items of SA
	// 2*n1 must be not larger than n (provable)
	int n1 = 0;
	for (i = 0; i < n; i++)
		if (isLMS(SA[i]))
			SA[n1++] = SA[i];
	// find the lexicographic names of all substrings
	for (i = n1; i < n; i++)
		SA[i] = -1; // init the name array buffer
	int name = 0, prev = -1;
	for (i = 0; i < n1; i++) {
		int pos = SA[i];
		bool diff = false;
		for (int d = 0; d < n; d++)
			if (prev
					== -1|| chr(pos+d) != chr(prev+d) || tget(pos+d) != tget(prev+d)) {
				diff = true;
				break;
			} else if (d > 0 && (isLMS(pos+d) || isLMS(prev + d)))
				break;
		if (diff) {
			name++;
			prev = pos;
		}
		pos = (pos % 2 == 0) ? pos / 2 : (pos - 1) / 2;
		SA[n1 + pos] = name - 1;
	}
	for (i = n - 1, j = n - 1; i >= n1; i--)
		if (SA[i] >= 0)
			SA[j--] = SA[i];
	// stage 2: solve the reduced problem
	// recurse if names are not yet unique
	int *SA1 = SA, *s1 = SA + n - n1;
	if (name < n1)
		SA_IS((unsigned char*) s1, SA1, n1, name - 1, sizeof(int));
	else
		// generate the suffix array of s1 directly
		for (i = 0; i < n1; i++)
			SA1[s1[i]] = i;
	// stage 3: induce the result for the original problem
	bkt = (int *) malloc(sizeof(int) * (K + 1)); // bucket array
	// put all left-most S characters into their buckets
	getBuckets(s, bkt, n, K, cs, true); // find ends of buckets
	for (i = 1, j = 0; i < n; i++)
		if (isLMS(i))
			s1[j++] = i; // get p1
	for (i = 0; i < n1; i++)
		SA1[i] = s1[SA1[i]]; // get index in s
	for (i = n1; i < n; i++)
		SA[i] = -1; // init SA[n1..n-1]
	for (i = n1 - 1; i >= 0; i--) {
		j = SA[i];
		SA[i] = -1;
		SA[--bkt[chr(j)]] = j;
	}
	induceSAl(t, SA, s, bkt, n, K, cs, false);
	induceSAs(t, SA, s, bkt, n, K, cs, true);
	free(bkt);
	free(t);
}

void DC3Algorithm::CalcEnhancments(int * isa) {
	// Calculate LCP and BW
	for (int i = 0, h = 0; i < wordLength_; ++i) {
#ifdef BW_ARRAY
		if (sa_[i] != 0) {
			bwt_[i] = word_[sa_[i] - 1];
		}
#endif
		if (isa[i] < wordLength_ - 1) {
			for (int j = sa_[isa[i] + 1]; word_[i + h] == word_[j + h]; ++h)
				;
			lcp_[isa[i] + 1] = h;
			if (h > 0)
				--h;
		}
	}
	lcp_[0] = 0;
	// construct child table of lcp intervals
	std::stack<int> supportQ = std::stack<int>();
	supportQ.push(0);
	int lastIndex = -1;
	for (int i = 1; i < wordLength_; ++i) {
		while (lcp_[i] < lcp_[supportQ.top()]) {
			lastIndex = supportQ.top();
			supportQ.pop();
			if ((lcp_[i] <= lcp_[supportQ.top()])
					&& (lcp_[supportQ.top()] != lcp_[lastIndex])) {
				cld_[supportQ.top()].down = lastIndex;
			}
		}
		if (lastIndex != -1) {
			cld_[i].up = lastIndex;
			lastIndex = -1;
		}
		supportQ.push(i);
	}
	supportQ = std::stack<int>();
	supportQ.push(0);
	lastIndex = -1;
	for (int i = 1; i < wordLength_; ++i) {
		while (lcp_[i] < lcp_[supportQ.top()]) {
			supportQ.pop();
		}
		if (lcp_[i] == lcp_[supportQ.top()]) {
			lastIndex = supportQ.top();
			supportQ.pop();
			cld_[lastIndex].next = i;
		}
		supportQ.push(i);
	}
}

std::string DC3Algorithm::getSfaFileName() {
	std::ostringstream fileName;
	fileName << input_.getCachePrefix() << "_" << targetNo_ << "_";
	if (reverse_) {
		fileName << "REV";
	} else {
		fileName << "FOR";
	}
	fileName << ".SFA";
	return fileName.str();
}

void DC3Algorithm::loadSfa() {
	std::ifstream sfaFile;
	sfaFile.open(getSfaFileName(), std::ifstream::binary);
	std::string line;
	int i = 0;
	while (!sfaFile.eof()) {
		sfaFile.read((char *) (&(sa_[i])), sizeof(int));
		sfaFile.read((char *) (&(lcp_[i])), sizeof(int));
		sfaFile.read((char *) (&(cld_[i])), sizeof(LcpChl));
		++i;
	}
	sfaFile.close();
}

void DC3Algorithm::saveSfa() {
	std::ofstream sfaFile;
	sfaFile.open(getSfaFileName(),
			std::ios::out | std::ios::trunc | std::ios::binary);
	for (int i = 0; i < getWordLength(); ++i) {
		sfaFile.write((const char *) (&(sa_[i])), sizeof(int));
		sfaFile.write((const char *) (&(lcp_[i])), sizeof(int));
		sfaFile.write((const char *) (&(cld_[i])), sizeof(LcpChl));
	}
	sfaFile.close();
}

double DC3Algorithm::RunAlgorithm() {
	std::clock_t begin = clock();

	if (input_.getCacheMode() == Input::CacheMode::LOAD) {
		loadSfa();
	} else {
		unsigned char * s = new unsigned char[wordLength_];
		strcpy((char *) s, word_.c_str());
		SA_IS(s, sa_, wordLength_ + 1, 256, 1);
		delete[] s;
		// calc ISA and push SA to ignore empty string
		sa_++;
		saPushed_ = true;
		sa_[wordLength_] = wordLength_;
		int * isa_ = new int[wordLength_];
		for (int i = 0; i < wordLength_; i++)
			isa_[sa_[i]] = i;
		CalcEnhancments(isa_);
		delete[] isa_;
	}

	if (input_.getCacheMode() == Input::CacheMode::SAVE) {
		Log::getInstance() << "Writing Suffix information to "
				<< getSfaFileName() << std::endl;
		saveSfa();
	}
	word_.replace(word_.length() - 1, 1, "$");

	return double(clock() - begin) / CLOCKS_PER_SEC;
}
DC3Algorithm::DC3Algorithm(const char * word, bool reverse, Input & input,
		int targetNo) :
		word_(), reverse_(reverse), saPushed_(false), input_(input), targetNo_(
				targetNo) {
	if (reverse_) {
		std::string temp(word);
		word_ = std::string(temp.rbegin(), temp.rend());
	} else {
		word_ = std::string(word);
	}
	word_ += 255;
	wordLength_ = word_.size();
	sa_ = new int[wordLength_ + 1];
	lcp_ = new int[wordLength_];
#ifdef BW_ARRAY
	bwt_ = new char[wordLength_];
#endif
	cld_ = new LcpChl[wordLength_];
	for (int i = 0; i < wordLength_; ++i) {
		sa_[i] = 0;
		lcp_[i] = 0;
#ifdef BW_ARRAY
		bwt_[i] = 0;
#endif
		cld_[i].down = -1;
		cld_[i].up = -1;
		cld_[i].next = -1;
	}
	sa_[wordLength_] = 0;
}

DC3Algorithm::~DC3Algorithm() {
	if (saPushed_) {
		sa_--;
	}
	delete[] sa_;
	delete[] lcp_;
	delete[] cld_;
#ifdef BW_ARRAY
	delete[] bwt_;
#endif
}

const int * DC3Algorithm::getSA() {
	return sa_;
}
const int * DC3Algorithm::getLCP() {
	return lcp_;
}

int DC3Algorithm::getWordLength() {
	// return length without $ at the end
	return wordLength_ - 1;
}

void DC3Algorithm::getChildIntervals(std::vector<LcpInterval> & childIntervals,
		int i, int j) {
	int i1, i2;
	childIntervals.clear();

	if (j == getWordLength() && i == 0) {
		int next = i;
		while (cld_[i].next != -1) {
			next = cld_[i].next;
			childIntervals.push_back(LcpInterval(i, next - 1));
			i = next;
		}
	} else {
		if (i < cld_[j + 1].up && cld_[j + 1].up <= j)
			i1 = cld_[j + 1].up;
		else
			i1 = cld_[i].down;
		childIntervals.push_back(LcpInterval(i, i1 - 1));
		while (cld_[i1].next != -1) {
			i2 = cld_[i1].next;
			childIntervals.push_back(LcpInterval(i1, i2 - 1));
			i1 = i2;
		}
		childIntervals.push_back(LcpInterval(i1, j));
	}
}

int DC3Algorithm::getLCP(int i, int j) {
	if (i == 0 && j == getWordLength())
		return 0;
	else if (i < cld_[j + 1].up && cld_[j + 1].up <= j)
		return lcp_[cld_[j + 1].up];
	else
		return lcp_[cld_[i].down];
}

void DC3Algorithm::testPrintDC3() {
	std::cout << word_ << std::endl;
	std::cout << "i |SA|LCP|up|down|next|\n";
	for (int i = 0; i < wordLength_; ++i) {
		std::cout << i << " | ";
		std::cout << sa_[i] << " | ";
		std::cout << lcp_[i] << " | ";
		std::cout << cld_[i].up << " | ";
		std::cout << cld_[i].down << " | ";
		std::cout << cld_[i].next << " | ";
		std::cout << (word_.c_str() + sa_[i]);
		std::cout << "\n";
	}
	std::cout << std::endl;
}

const std::string & DC3Algorithm::getWord() {
	return word_;
}
