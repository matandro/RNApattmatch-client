/*
 * HairpinSearch.h
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#ifndef INCLUDE_HAIRPINSEARCH_H_
#define INCLUDE_HAIRPINSEARCH_H_

#include <Input.h>
#include <BiDirTask.h>
#include <vector>
#include <queue>
#include <HairpinQuery.h>
#include <AffixArray.h>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

struct GapBackTrack {
	int queryIndex;
	int queryRevIndex;
	int targetIndex;
	int revUnchked;
	char amount;
	bool isForward;
};

class HairpinSearch {
private:
	Input & input_;
	AffixArray & affixArray_;
	HairpinQuery & hairpin_;
	std::vector<BiDirTask *> results_;
	// Resource ordering resultMutex_ -> queueMutex (if both needed)
	boost::mutex queueMutex_;
	boost::mutex resultMutex_;
	boost::condition_variable queueWaiter_;
	bool keepGoing_;

	enum CompareState {
		KEEP_COMPARING, MISMATCH, MATCH, MATCH_REV
	};

	BiDirTask * performSearchTask(BiDirTask *, std::queue<BiDirTask *> &);
	BiDirTask * searchIntervalTask(BiDirTask *, std::queue<BiDirTask*> &);
	BiDirTask * handleGap(BiDirTask *, std::queue<BiDirTask *> &, bool);
	BiDirTask * compareInterval(BiDirTask *, std::queue<BiDirTask *> &, int);
	int getGapRemaining(BiDirTask &, bool);
	bool compareLetters(BiDirTask &, bool);
	bool compareCompletionLetter(BiDirTask &, LcpInterval &, char, bool, int);
	int getRelativeTarget(BiDirTask &, bool);
	void compareSingletonInterval(BiDirTask&);
	void updateGapBack(BiDirTask &, GapBackTrack &);
	bool checkFinish(BiDirTask &);
	void taskRunner(std::queue<BiDirTask *> *, int *, int);
	bool checkOutofBounds(BiDirTask &, bool, int);
	void printInfoTask(const char * , BiDirTask & , bool );
public:
	HairpinSearch(Input &, AffixArray &, HairpinQuery &);
	virtual ~HairpinSearch();
	bool search();
	void printResults();
	const std::vector<BiDirTask *> & getResults();
};

#endif /* INCLUDE_HAIRPINSEARCH_H_ */
