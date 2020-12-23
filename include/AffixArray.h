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
	std::int64_t * aflkF_;
	std::int64_t * aflkR_;
	Input & input_;
	int targetNo_;

	void calculateAflk();
	void loadAflk();
	std::int64_t home(std::int64_t, std::int64_t, DC3Algorithm &);
	bool searchInReverse(DC3Algorithm &, DC3Algorithm &, LcpInterval &, std::int64_t *,
			std::int64_t *);
	std::string getAfklFileName();
	void singleAflk(DC3Algorithm * forword, DC3Algorithm * reverse,
			std::int64_t * fAFlk, std::int64_t * rAflk);
public:
	AffixArray(const char *, Input &, int targetNo);
	double RunAlgorithm();
	virtual ~AffixArray();
	void printArrayTest();
	std::int64_t getTargetLength();
	void getChildIntervals(std::vector<LcpInterval> &,
			LcpInterval & parentInterval, bool);
	std::int64_t getIntervalLcp(LcpInterval &, bool);
	bool isOutOfBound(std::int64_t targetIndex, std::int64_t suffixIndex, bool isForward);
	char getTargetChar(std::int64_t targetIndex, std::int64_t arrayIndex, bool isForward);
	std::int64_t getIntervalsIndex(std::int64_t arrayIndex, bool isForward);
	LcpInterval getReversedInterval(LcpInterval & forwordInterval,
			bool isForward);
	const std::string & getWord() {
		return suffixArray_.getWord();
	}
	void saveAflk();
};

#endif /* AFFIXARRAY_H_ */
