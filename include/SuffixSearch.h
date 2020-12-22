/*
 * SuffixSearch.h
 *
 *  Created on: Sep 18, 2014
 *      Author: matan
 */

#ifndef SUFFIXSEARCH_H_
#define SUFFIXSEARCH_H_

#include <string>
#include <queue>
#include <vector>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <Input.h>
#include <DC3Algorithm.h>
#include <UniDirTask.h>

class SuffixSearch {
private:
	DC3Algorithm * dc3Algorithm_;
	Input * input_;
	const std::string & target_;
	const int targetNo_;
	std::vector<UniDirTask *> results_;
	// Resource ordering resultMutex_ -> queueMutex (if both needed)
	boost::mutex queueMutex_;
	boost::mutex resultMutex_;
	boost::condition_variable queueWaiter_;
	bool keepGoing_;

	enum CompareState {
		KEEP_COMPARING, MISMATCH, MATCH
	};

	UniDirTask * performTask(std::queue<UniDirTask *> &, UniDirTask *);
	void getInterval(UniDirTask & searchTask,
			std::vector<LcpInterval> & childIntervals);
	CompareState compareStr(const char *, std::int64_t, std::int64_t &,
			std::int64_t, unsigned int &, unsigned int);
	char getQueryChar(const char *, unsigned int, std::int64_t, std::int64_t);
	int targetToQuery(int);
	void stopSearch();
	void handleMatch(std::queue<UniDirTask *> &, UniDirTask &, char *,
			std::int64_t, unsigned int, LcpInterval &);
	// sending pointer, thread function
	void taskRunner(std::queue<UniDirTask *> *, int *, int);
public:
	SuffixSearch(DC3Algorithm *, Input *, int);
	virtual ~SuffixSearch();
	double performSearch();
	static bool isFASTAEqual(char, char);
	void testPrintResults();
	void printResults();
};

#endif /* SUFFIXSEARCH_H_ */
