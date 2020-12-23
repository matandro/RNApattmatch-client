/*
 * DC3Algorithm.h
 *
 *  Created on: Sep 17, 2014
 *      Author: matan
 *
 *      Creates an enhanced suffix / reverse prefix array
 *      only supports sequences under 2 GB, please break your sequence to fit the size.
 *      To add higher support all int should be changed to long or any such representation
 *      which allows for -1. notice that the actual algorithm might not like the new representation since it works with buckets.
 */

#ifndef DC3ALGORITHM_H_
#define DC3ALGORITHM_H_

#include <string>
#include <vector>
#include <Input.h>
#include <LcpInterval.h>

struct LcpChl {
	std::int64_t down;
	std::int64_t up;
	std::int64_t next;
};

class DC3Algorithm {
private:
	std::string word_;
	std::int64_t wordLength_;
	std::int64_t * sa_;
	std::int64_t * lcp_;
#ifdef BW_ARRAY
	char * bwt_;
#endif
	LcpChl * cld_;
	bool reverse_;
	bool saPushed_;
	Input & input_;
	int targetNo_;

	const static unsigned char mask[];

#define tget(i) ( (t[(i)/8]&mask[(i)%8]) ? 1 : 0 )
#define tset(i, b) t[(i)/8]=(b) ? (mask[(i)%8]|t[(i)/8]) : ((~mask[(i)%8])&t[(i)/8])
#define chr(i) (cs==sizeof(std::int64_t)?((std::int64_t*)s)[i]:((unsigned char *)s)[i])
#define isLMS(i) (i>0 && tget(i) && !tget(i-1))

	void getBuckets(unsigned char *, std::int64_t *, std::int64_t, std::int64_t, std::int64_t, bool);
	void induceSAl(unsigned char *, std::int64_t *, unsigned char *, std::int64_t *, std::int64_t, std::int64_t,
			std::int64_t, bool);
	void induceSAs(unsigned char *, std::int64_t *, unsigned char *, std::int64_t *, std::int64_t, std::int64_t,
			std::int64_t, bool);
	void SA_IS(unsigned char *, std::int64_t *, std::int64_t, std::int64_t, std::int64_t);
	void CalcEnhancments(std::int64_t *);
	void loadSfa();
	void saveSfa();
	std::string getSfaFileName();
public:
	DC3Algorithm(const char *, bool reverse, Input & input, int targetNo);
	double RunAlgorithm();
	virtual ~DC3Algorithm();
	const std::int64_t * getSA();
	const std::int64_t * getLCP();
	std::int64_t getLCP(std::int64_t, std::int64_t);
	void getChildIntervals(std::vector<LcpInterval> &, std::int64_t, std::int64_t);
	std::int64_t getWordLength();
	void testPrintDC3();
	const std::string & getWord();
};

#endif /* DC3ALGORITHM_H_ */
