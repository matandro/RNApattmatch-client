/*
 * AffixArray.h
 *
 *  Created on: Jan 25, 2015
 *      Author: matan
 */

#ifndef AFFIXARRAY_H_
#define AFFIXARRAY_H_

#include <DC3Algorithm.h>
#include <string>
#include <Input.h>

class AffixArray {
private:
	DC3Algorithm suffixArray_;
	DC3Algorithm revPrefixArray_;
	int * aflkF_;
	int * aflkR_;
	Input & input_;
	int targetNo_;

	void calculateAflk();
	void loadAflk();
	int home(int, int, DC3Algorithm &);
	bool searchInReverse(DC3Algorithm &, DC3Algorithm &, LcpInterval &, int *,
			int *);
	std::string getAfklFileName();
	void singleAflk(DC3Algorithm * forword, DC3Algorithm * reverse,
			int * fAFlk, int * rAflk);
public:
	AffixArray(const char *, Input &, int targetNo);
	double RunAlgorithm();
	virtual ~AffixArray();
	void printArrayTest();
	unsigned int getTargetLength();
	void getChildIntervals(std::vector<LcpInterval> &,
			LcpInterval & parentInterval, bool);
	int getIntervalLcp(LcpInterval &, bool);
	bool isOutOfBound(int targetIndex, int suffixIndex, bool isForward);
	char getTargetChar(int targetIndex, int arrayIndex, bool isForward);
	int getIntervalsIndex(int arrayIndex, bool isForward);
	LcpInterval getReversedInterval(LcpInterval & forwordInterval,
			bool isForward);
	const std::string & getWord() {
		return suffixArray_.getWord();
	}
	void saveAflk();
};

#endif /* AFFIXARRAY_H_ */
