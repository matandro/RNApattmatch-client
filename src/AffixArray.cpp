/*
 * AffixArray.cpp
 *
 *  Created on: Jan 25, 2015
 *      Author: matan
 */

#include <AffixArray.h>
#include <ctime>
#include <vector>
#include <queue>
#include <iostream>
#include <stack>
#include <fstream>
#include <boost/thread.hpp>
#include <Log.h>

AffixArray::AffixArray(const char * word, Input & input, int targetNo) :
		suffixArray_(word, false, input, targetNo), revPrefixArray_(word, true,
				input, targetNo), input_(input), targetNo_(targetNo), arrayUpdated_(false) {
	aflkF_ = new std::int64_t[suffixArray_.getWordLength()];
	aflkR_ = new std::int64_t[suffixArray_.getWordLength()];
	for (std::int64_t i = 0; i < suffixArray_.getWordLength(); ++i) {
		aflkF_[i] = -2;
		aflkR_[i] = -2;
	}
}

AffixArray::~AffixArray() {
	if (aflkF_ != NULL) {
		delete[] aflkF_;
		aflkF_ = NULL;
	}
	if (aflkR_ != NULL) {
		delete[] aflkR_;
		aflkR_ = NULL;
	}
}

double AffixArray::RunAlgorithm() {
	std::clock_t begin = clock();

	boost::thread suffixRunner(&DC3Algorithm::RunAlgorithm, &suffixArray_);
	if (input_.getThreadNo() <= 1) {
		suffixRunner.join();
	}
	boost::thread affixRunner(&DC3Algorithm::RunAlgorithm, &revPrefixArray_);
	affixRunner.join();
	if (input_.getThreadNo() > 1) {
		suffixRunner.join();
	}

	if (input_.getCacheMode() == Input::CacheMode::SAVE) {
		// Currently removed. Calculating all is very time consuming. still saving after run to allow cached info
		// calculateAflk();
		Log::getInstance() << "Writing Affix information to "
				<< getAfklFileName() << std::endl;
		saveAflk();
	} else if (input_.getCacheMode() == Input::CacheMode::LOAD) {
		loadAflk();
	}

	return double(clock() - begin) / CLOCKS_PER_SEC;
}

std::string AffixArray::getAfklFileName() {
	std::ostringstream fileName;
	fileName << input_.getCachePrefix() << "_" << targetNo_ << ".AFLK";
	return fileName.str();
}

void AffixArray::saveAflk() {
	if (!arrayUpdated_)
		return;
	std::ofstream aflkFile;
	aflkFile.open(getAfklFileName(),
			std::ios::out | std::ios::trunc | std::ios::binary);
	for (std::int64_t i = 0; i < suffixArray_.getWordLength(); ++i) {
		aflkFile.write((const char *) (&(aflkR_[i])), sizeof(std::int64_t));
		aflkFile.write((const char *) (&(aflkF_[i])), sizeof(std::int64_t));
	}
	aflkFile.close();
}

void AffixArray::loadAflk() {
	std::ifstream aflkFile;
	aflkFile.open(getAfklFileName(), std::ios::binary);
	if (aflkFile.fail())
		return;
	std::string line;
	std::int64_t i = 0;
	while (!aflkFile.eof()) {
		aflkFile.read((char *) (&(aflkR_[i])), sizeof(std::int64_t));
		aflkFile.read((char *) (&(aflkF_[i])), sizeof(std::int64_t));
		++i;
	}
	aflkFile.close();
}

/*
 * might not work incase of a target DB with only 1 letter, not an interesting case.
 */
void AffixArray::calculateAflk() {
	boost::thread frontRunner(&AffixArray::singleAflk, this, &suffixArray_,
			&revPrefixArray_, aflkF_, aflkR_);
	if (input_.getThreadNo() <= 1) {
		frontRunner.join();
	}
	boost::thread revRunner(&AffixArray::singleAflk, this, &revPrefixArray_,
			&suffixArray_, aflkR_, aflkF_);
	revRunner.join();
	if (input_.getThreadNo() > 1) {
		frontRunner.join();
	}/*
	singleAflk(&suffixArray_,
				&revPrefixArray_, aflkF_, aflkR_);
	singleAflk(&revPrefixArray_,
				&suffixArray_, aflkR_, aflkF_);*/
}

void AffixArray::singleAflk(DC3Algorithm * forword, DC3Algorithm * reverse,
		std::int64_t * fAFlk, std::int64_t * rAflk) {
	std::queue<LcpInterval> workAhead;
	workAhead.push(LcpInterval(0, forword->getWordLength()));
	while (!workAhead.empty()) {
		LcpInterval currentInterval = workAhead.front();
		workAhead.pop();

		std::vector<LcpInterval> intervals;
		forword->getChildIntervals(intervals, currentInterval.start,
				currentInterval.end);
		for (std::vector<LcpInterval>::iterator it = intervals.begin();
				it != intervals.end(); ++it) {
			// aflk only relevant for intervals larger then 1
			if (it->start == it->end)
				continue;
			workAhead.push(*it);
			searchInReverse(*forword, *reverse, *it, fAFlk, rAflk);
		}
	}
}

bool AffixArray::searchInReverse(DC3Algorithm & forword, DC3Algorithm & reverse,
		LcpInterval & forwordInterval, std::int64_t * fAFlk, std::int64_t * rAflk) {
	std::int64_t lcp = forword.getLCP(forwordInterval.start, forwordInterval.end);
	std::int64_t searchIndex = lcp;
	LcpInterval reverseInterval(0, reverse.getWordLength());
	while (searchIndex > 0 && !reverseInterval.isEmpty()) {
		std::vector<LcpInterval> intervals;
		reverse.getChildIntervals(intervals, reverseInterval.start,
				reverseInterval.end);
		reverseInterval.setEmpty();
		for (std::vector<LcpInterval>::iterator it = intervals.begin();
				it != intervals.end(); ++it) {
			std::int64_t localIndex = searchIndex;
			std::int64_t reverseLcp = reverse.getLCP(it->start, it->end);
			// compare as many as possible
			bool fit = !(it->start == it->end);
			while (localIndex > 0 && lcp - localIndex < reverseLcp && fit) {
				char fromOrigin =
						forword.getWord()[forword.getSA()[forwordInterval.start]
								+ localIndex - 1];
				char fromReverse = reverse.getWord()[reverse.getSA()[it->start]
						+ lcp - localIndex];
				if (fromOrigin != fromReverse) {
					fit = false;
					break;
				}
				localIndex--;
			}
			if (fit) {
				searchIndex = localIndex;
				if (reverseLcp < lcp) {
					reverseInterval.start = it->start;
					reverseInterval.end = it->end;
				} else {
					if (reverseLcp == lcp) {
						rAflk[home(it->start, it->end, reverse)] =
								forwordInterval.start;
					}
					fAFlk[home(forwordInterval.start, forwordInterval.end,
							forword)] = it->start;
					return true;
				}
				break;
			}
		}
	}

	return false;
}

std::int64_t AffixArray::home(std::int64_t i, std::int64_t j, DC3Algorithm & array) {
	std::int64_t homeVar;
	if (array.getLCP()[i] >= array.getLCP()[j + 1]) {
		homeVar = i;
	} else {
		homeVar = j;
	}
	return homeVar;
}

void AffixArray::printArrayTest() {
	std::cout << "i | suff | lcpf | aflkf | wSuff" << std::endl;
	for (std::int64_t i = 0; i < suffixArray_.getWordLength(); ++i) {
		std::cout << i << " | ";
		std::cout << suffixArray_.getSA()[i] << " | ";
		std::cout << suffixArray_.getLCP()[i] << " | ";
		std::cout << aflkF_[i] << " | ";
		std::cout << suffixArray_.getWord().c_str() + suffixArray_.getSA()[i];
		std::cout << std::endl;
	}
	std::cout << "i | sufr | lcpr | aflkr | wSufr" << std::endl;
	for (std::int64_t i = 0; i < revPrefixArray_.getWordLength(); ++i) {
		std::cout << i << " | ";
		std::cout << revPrefixArray_.getSA()[i] << " | ";
		std::cout << revPrefixArray_.getLCP()[i] << " | ";
		std::cout << aflkR_[i] << " | ";
		for (std::int64_t j = revPrefixArray_.getWordLength() - 2;
				j >= revPrefixArray_.getSA()[i]; --j) {
			std::cout << revPrefixArray_.getWord().c_str()[j];
		}
		std::cout << "$";
		std::cout << std::endl;
	}
}

std::int64_t AffixArray::getTargetLength() {
	return suffixArray_.getWordLength();
}

void AffixArray::getChildIntervals(std::vector<LcpInterval> & childIntervals,
		LcpInterval & parentInterval, bool isForward) {
	if (isForward)
		suffixArray_.getChildIntervals(childIntervals, parentInterval.start,
				parentInterval.end);
	else
		revPrefixArray_.getChildIntervals(childIntervals, parentInterval.start,
				parentInterval.end);
	/*
	 for (unsigned int i = 0; i < childIntervals.size(); ++i) {
	 if (childIntervals[i].start > childIntervals[i].end) {
	 std::cerr << "ERROR: [" << childIntervals[i].start << ","
	 << childIntervals[i].end << "] for ["
	 << parentInterval.start << "," << parentInterval.end
	 << "] isForward:" << isForward << std::endl;
	 exit(0);
	 }
	 }
	 */
}

std::int64_t AffixArray::getIntervalLcp(LcpInterval & parentInterval, bool isForward) {
	std::int64_t lcp = -1;
	if (isForward)
		lcp = suffixArray_.getLCP(parentInterval.start, parentInterval.end);
	else
		lcp = revPrefixArray_.getLCP(parentInterval.start, parentInterval.end);
	return lcp;
}

bool AffixArray::isOutOfBound(std::int64_t targetIndex, std::int64_t arrayIndex, bool isForward) {
	std::int64_t targetLocation = targetIndex;
	if (isForward)
		targetLocation += suffixArray_.getSA()[arrayIndex];
	else
		targetLocation += revPrefixArray_.getSA()[arrayIndex];
	return (targetLocation > getTargetLength());
}

/**

 */
std::int64_t AffixArray::getIntervalsIndex(std::int64_t arrayIndex, bool isForward) {
	if (isForward)
		return suffixArray_.getSA()[arrayIndex];
	else
		return revPrefixArray_.getSA()[arrayIndex];
}

char AffixArray::getTargetChar(std::int64_t targetoffset, std::int64_t arrayIndex,
		bool isForward) {
	char result;
	if (isForward)
		result = suffixArray_.getWord()[getIntervalsIndex(arrayIndex, isForward)
				+ targetoffset];
	else
		result = revPrefixArray_.getWord()[getIntervalsIndex(arrayIndex,
				isForward) + targetoffset];
	return result;
}

LcpInterval AffixArray::getReversedInterval(LcpInterval & forwordInterval,
		bool isForward) {
	std::int64_t start;
	std::int64_t end;
	std::int64_t homeVar;
	std::int64_t size = forwordInterval.end - forwordInterval.start;
	if (isForward) {
		homeVar = home(forwordInterval.start, forwordInterval.end,
				suffixArray_);
		if (aflkF_[homeVar] == -2) {
			searchInReverse(suffixArray_, revPrefixArray_, forwordInterval,
					aflkF_, aflkR_);
		}
		start = aflkF_[homeVar];
		if (start == -1) {
			std::cerr << "WTF" << std::endl;
			exit(-2);
		}
	} else {
		homeVar = home(forwordInterval.start, forwordInterval.end,
				revPrefixArray_);
		if (aflkR_[homeVar] == -2) {
			arrayUpdated_ = true;
			searchInReverse(revPrefixArray_, suffixArray_, forwordInterval,
					aflkR_, aflkF_);
		}
		start = aflkR_[homeVar];
		if (start == -1) {
			std::cerr << "WTF" << std::endl;
			exit(-2);
		}
	}
	end = start + size;
	/*
	 std::cout << "TEST old:[" << forwordInterval.start << ","
	 << forwordInterval.end << "] new:[" << start << "," << end
	 << "] homevar:" << homeVar << " Dir:" << isForward << std::endl;
	 */
	return LcpInterval(start, end);
}
