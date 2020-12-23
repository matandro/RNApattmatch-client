/*
 * SuffixSearch.cpp
 *
 *  Created on: Sep 18, 2014
 *      Author: matan
 */

#include <SuffixSearch.h>
#include <Log.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <stack>
#include <climits>
#include <boost/thread.hpp>

SuffixSearch::SuffixSearch(DC3Algorithm * dc3Algorithm, Input * input,
		int targetNo) :
		dc3Algorithm_(dc3Algorithm), input_(input), target_(
				input->getTargetSequence(targetNo)), targetNo_(targetNo), results_(), queueMutex_(), resultMutex_(), queueWaiter_(), keepGoing_(
				true) {

}

SuffixSearch::~SuffixSearch() {
	for (unsigned int i = 0; i < results_.size(); ++i) {
		delete results_[i];
	}
}

double SuffixSearch::performSearch() {
	std::clock_t begin = clock();

	std::queue<UniDirTask *> taskQueue;
	LcpInterval initial(0, target_.length());
	taskQueue.push(new UniDirTask(initial, 0, 0, input_->getGapNo()));
	int workingNumber = input_->getThreadNo();

	boost::thread_group threadPool;
	std::vector<boost::thread *> threads;

	for (int i = 0; i < input_->getThreadNo(); ++i) {
		threads.push_back(
				new boost::thread(&SuffixSearch::taskRunner, this, &taskQueue,
						&workingNumber, i + 1));
		threadPool.add_thread(threads[i]);
	}

	threadPool.join_all();

	for (int i = 0; i < input_->getThreadNo(); ++i) {
		threadPool.remove_thread(threads[i]);
		delete threads[i];
	}
	// in case of too many results exit
	while (taskQueue.empty() == false) {
		delete taskQueue.front();
		taskQueue.pop();
	}

	return double(clock() - begin) / CLOCKS_PER_SEC;
}

void SuffixSearch::taskRunner(std::queue<UniDirTask *> * taskQueue,
		int * workingThreads, int id) {
	UniDirTask * currentTask = NULL;
	int taskCounter = 0;
	boost::mutex::scoped_lock lock(queueMutex_, boost::defer_lock);
	while (keepGoing_ == true) {
		lock.lock();
		while (taskQueue->empty() == true) {
			(*workingThreads)--;
			if ((*workingThreads) == 0 || keepGoing_ == false) {
				queueWaiter_.notify_all();
				std::cout << "Thread " << id << " finished, solved "
						<< taskCounter << " tasks!" << std::endl;
				return;
			}
			queueWaiter_.wait(lock);
			(*workingThreads)++;
		}
		// if we got here, queue isn't empty, get job from queue
		currentTask = taskQueue->front();
		taskQueue->pop();
		lock.unlock();
		while (currentTask != NULL && keepGoing_ == true) {
			currentTask = performTask(*taskQueue, currentTask);
			++taskCounter;
		}
	}
	lock.lock();
	std::cout << "Thread " << id << " finished, solved " << taskCounter
			<< " tasks!" << std::endl;
}

UniDirTask * SuffixSearch::performTask(std::queue<UniDirTask *> & queue,
		UniDirTask * searchTask) {
	std::int64_t inTargetIndex = searchTask->targetIndex_;
	unsigned int inQueryIndex = searchTask->queryIndex_;
	char * inGaps = new char[input_->getGapNo()];
	for (int i = 0; i < input_->getGapNo(); ++i)
		inGaps[i] = searchTask->gaps_[i];
	const std::int64_t * sa = dc3Algorithm_->getSA();

// Retrieving all child intervals
	std::vector<LcpInterval> intervals;
	getInterval(*searchTask, intervals);
	searchTask->interval_.setEmpty();
	for (std::vector<LcpInterval>::iterator intervalIt = intervals.begin();
			intervalIt != intervals.end(); ++intervalIt) {

		unsigned int newQueryIndex = inQueryIndex;
		std::int64_t newTargetIndex = inTargetIndex;

		// if the interval has more then 1 sequence, get the size of the similar part
		if (intervalIt->start != intervalIt->end) {
			std::int64_t lcp = dc3Algorithm_->getLCP(intervalIt->start, intervalIt->end);
			newTargetIndex = lcp;

			// best case, no gaps yet: location will be fixed by compareStr
			newQueryIndex = std::min(inQueryIndex + (lcp - inTargetIndex),
					(std::int64_t)input_->getTotalLength());
		} // Only one sequence in interval, does it fit?
		else {
			newQueryIndex = input_->getTotalLength();
			//  Target gets maximum, worst case we reach end of query before
			newTargetIndex = input_->getTotalLength() + input_->getMaxGap();
		}

		std::vector<Gap *> gapsInSection;
		input_->getGapsInIndexes(gapsInSection, inQueryIndex, newQueryIndex,
				Input::INC_START);

		// Find not set gaps in section that were not initialized (value = -1)
		for (std::vector<Gap *>::iterator gapIt = gapsInSection.begin();
				gapIt != gapsInSection.end();) {
			if (inGaps[(*gapIt)->gapNo] != -1) {
				gapIt = gapsInSection.erase(gapIt);
			} else {
				break;
			}
		}

		std::vector<unsigned int> gapUsageVector;
		int currentGapIndex = -1;
		do {
			unsigned int currentQueryIndex = inQueryIndex;
			std::int64_t currentTargetIndex = inTargetIndex;

			CompareState compareState = CompareState::KEEP_COMPARING;
			while (compareState == CompareState::KEEP_COMPARING) {
				compareState = compareStr(inGaps, sa[intervalIt->start],
						currentTargetIndex, newTargetIndex, currentQueryIndex,
						newQueryIndex);
				if (compareState == CompareState::KEEP_COMPARING) {
					gapUsageVector.push_back(0);
					currentGapIndex++;
					inGaps[gapsInSection[currentGapIndex]->gapNo] = 0;
				}
			}

			if (compareState == CompareState::MATCH) {
				// matched section create new target or register result if matched to end
				handleMatch(queue, *searchTask, inGaps, currentTargetIndex,
						currentQueryIndex, *intervalIt);
			}

			while (gapUsageVector.empty() == false
					&& gapUsageVector[currentGapIndex]
							>= (unsigned) gapsInSection[currentGapIndex]->maxLength) {
				// Checked all values for current gap, go back one gap
				inGaps[gapsInSection[currentGapIndex]->gapNo] = -1;
				currentGapIndex--;
				gapUsageVector.pop_back();
			}
			if (gapUsageVector.empty() == false) {
				// We have more lengths for the current gap to test
				gapUsageVector[currentGapIndex]++;
				inGaps[gapsInSection[currentGapIndex]->gapNo]++;
			}
		} while (gapUsageVector.empty() == false && keepGoing_ == true);
	}
	delete[] inGaps;
	if (searchTask->interval_.isEmpty()) {
		delete searchTask;
		searchTask = NULL;
	}

	return searchTask;
}

/**
 * Adds a task to work queue, if more then max results return false
 */
void SuffixSearch::handleMatch(std::queue<UniDirTask *> & queue,
		UniDirTask & searchTask, char * inGaps, std::int64_t targetIndex,
		unsigned int queryIndex, LcpInterval & interval) {
	// if we found something we can copy it to he results
	if (queryIndex == input_->getTotalLength()) {
		UniDirTask * newTask = new UniDirTask(interval, queryIndex, targetIndex,
				input_->getGapNo(), inGaps);
		boost::mutex::scoped_lock lock(resultMutex_);
		results_.push_back(newTask);
		if (results_.size() >= (unsigned int) input_->getMaxResults()) {
			stopSearch();
		}
	} // If we don't have continuation, make one
	else if (searchTask.interval_.isEmpty()) {
		searchTask.queryIndex_ = queryIndex;
		searchTask.targetIndex_ = targetIndex;
		searchTask.interval_.start = interval.start;
		searchTask.interval_.end = interval.end;
		for (int i = 0; i < input_->getGapNo(); ++i) {
			searchTask.gaps_[i] = inGaps[i];
		}
	} // Create new task, we have more to do without accessing queue
	else {
		UniDirTask * newTask = new UniDirTask(interval, queryIndex, targetIndex,
				input_->getGapNo(), inGaps);
		boost::mutex::scoped_lock lock(queueMutex_);
		queue.push(newTask);
		queueWaiter_.notify_one();
	}
}

/**
 * Compare the Target[tStart, tEnd] to Query[qStart, qEnd] with the given gaps
 */
SuffixSearch::CompareState SuffixSearch::compareStr(const char * gaps,
		std::int64_t tLocation, std::int64_t & targetIndex, std::int64_t tEnd,
		unsigned int & queryIndex, unsigned int qEnd) {
	CompareState result = CompareState::MATCH;
	for (;
			targetIndex < tEnd && queryIndex < qEnd
					&& result == CompareState::MATCH;
			++queryIndex, ++targetIndex) {
		const Gap * gap = input_->getGapByIndex(queryIndex);
		if (gap != NULL) {
			if (gaps[gap->gapNo] == -1) {
				// gap not initialized, initialize and keep searching
				result = CompareState::KEEP_COMPARING;
				break;
			}

			// we are in a prepared gap, progress to end of gap or tEnd, whatever is closer
			int gapsBefore = 0;
			for (unsigned int i = 0; i < gap->gapNo; ++i) {
				gapsBefore += gaps[i];
			}
			targetIndex += std::min(
					gaps[gap->gapNo]
							- (targetIndex - (gapsBefore + queryIndex)),
					tEnd - targetIndex);

			// if tEnd in middle of gap and inside target, exit without increasing queryIndex
			if (targetIndex == tEnd
					&& (targetIndex + tLocation) < (std::int64_t)target_.length()) {
				break;
			}
		}

		// not in gap compare character
		char postMatchChar;
		std::int64_t targetLocation = targetIndex + tLocation;
		if (targetLocation
				>= (std::int64_t)target_.length()
				|| (postMatchChar = getQueryChar(gaps, queryIndex, targetIndex,
						tLocation)) == 0
				|| !isFASTAEqual(target_[targetLocation],
						postMatchChar)) {
			result = CompareState::MISMATCH;
		}
	}

	return result;
}

/**
 * Retrieve the query char (if complementary check with matrix)
 * assume we are not in a gap
 */
char SuffixSearch::getQueryChar(const char * gaps, unsigned int queryIndex,
		std::int64_t targetIndex, std::int64_t targetLocation) {
	char queryChar = input_->getQuerySeq()[queryIndex];
	int openBracketQueryIndex = input_->getComplamentryIndex(queryIndex, true);
	if (openBracketQueryIndex != -1) {
// remove used gaps to get the open bracket target index
		std::vector<Gap *> gapsBefore;
		input_->getGapsInIndexes(gapsBefore, openBracketQueryIndex, queryIndex,
				Input::INC_END);
		for (unsigned int i = 0; i < gapsBefore.size(); ++i) {
			targetIndex -= gaps[gapsBefore[i]->gapNo];
		}
		queryChar = input_->getMatrixChar(
				target_[targetLocation + targetIndex
						- (queryIndex - openBracketQueryIndex)], queryChar);
	}

	return queryChar;
}

/**
 * Get the appropriate child intervals
 */
void SuffixSearch::getInterval(UniDirTask & searchTask,
		std::vector<LcpInterval> & childIntervals) {
	childIntervals.clear();
	dc3Algorithm_->getChildIntervals(childIntervals, searchTask.interval_.start,
			searchTask.interval_.end);

	for (std::vector<LcpInterval>::iterator it = childIntervals.begin();
			it != childIntervals.end();) {
		const Gap * gap = input_->getGapByIndex(searchTask.queryIndex_);
		int gapRemaining = 0;
		if (gap != NULL) {
			// we are in a gap, calculate how far are we from the end of the gap
			if (searchTask.gaps_[gap->gapNo] == -1) {
				// if gap wasn't setup yet, assume max
				gapRemaining = gap->maxLength;
			} else {
				int gapsBefore = 0;
				for (unsigned int i = 0; i < gap->gapNo; ++i) {
					gapsBefore += searchTask.gaps_[i];
				}
				gapRemaining = (searchTask.gaps_[gap->gapNo]
						- (searchTask.targetIndex_
								- (gapsBefore + searchTask.queryIndex_)));
			}
		}
		if (gapRemaining == 0) {
			// no gap left, compare characters
			std::int64_t targetLocation = dc3Algorithm_->getSA()[it->start]
					+ searchTask.targetIndex_;
			char postMatchChar;
			if (targetLocation >= (std::int64_t)target_.length()
					|| (postMatchChar = getQueryChar(searchTask.gaps_,
							searchTask.queryIndex_,
							searchTask.targetIndex_,
							dc3Algorithm_->getSA()[it->start])) == 0
					||!isFASTAEqual(target_[targetLocation],
							postMatchChar)) {
				it = childIntervals.erase(it);
			} else {
				++it;
			}
		} else {
			// in gap, same as (N,.), always OK
			++it;
		}
	}
}

bool SuffixSearch::isFASTAEqual(char t, char q) {
	switch (t) {
	case 'A':
		switch (q) {
		case 'N':
		case 'A':
		case 'R':
		case 'M':
		case 'W':
		case 'D':
		case 'H':
		case 'V':
			return true;
		default:
			return false;
		}
		break;
	case 'G':
		switch (q) {
		case 'N':
		case 'G':
		case 'R':
		case 'K':
		case 'S':
		case 'B':
		case 'D':
		case 'V':
			return true;
		default:
			return false;
		}
		break;
	case 'C':
		switch (q) {
		case 'N':
		case 'C':
		case 'Y':
		case 'M':
		case 'S':
		case 'B':
		case 'H':
		case 'V':
			return true;
		default:
			return false;
		}
		break;
	case 'U':
		switch (q) {
		case 'N':
		case 'U':
		case 'Y':
		case 'K':
		case 'W':
		case 'B':
		case 'D':
		case 'H':
			return true;
		default:
			return false;
		}
		break;
	default:
		break;
		/* Removed so target supports ALL letters, wild card fit nothing on targret
		 *std::cout << "ERROR FASTA MATCH (Q:" << (int) q << ",T:" << (int) t
		*		<< ")" << std::endl;
		*exit(-1);
		*/
	}
	return false;
}

/**
 *  print result to log, format <index>;<gaps>;<sequence>
 */
void SuffixSearch::printResults() {
	Log & log = Log::getInstance();
	const std::int64_t * sa = dc3Algorithm_->getSA();
	int queryLength = input_->getTotalLength();

	log << "Results: " << input_->getTargetName(targetNo_) << "\n";
	for (unsigned int resultIndex = 0; resultIndex < results_.size();
			++resultIndex) {
		UniDirTask * currentResult = results_[resultIndex];
		for (std::int64_t intervalIndex = currentResult->interval_.start;
				intervalIndex <= currentResult->interval_.end;
				++intervalIndex) {
			// pushing index
			std::int64_t targetIndex = sa[intervalIndex];
			log << (sa[intervalIndex] + 1) << ";"; // start counting from 1
			// pushing gap information (for secondary structure alignment)
			int totalGaps = 0;
			for (int l = 0; l < input_->getGapNo(); ++l) {
				totalGaps += currentResult->gaps_[l];
				log << (int) (currentResult->gaps_[l]);
				if (l + 1 < input_->getGapNo())
					log << ",";
			}
			log << ";";
			// pushing sequence
			log << target_.substr(targetIndex, totalGaps + queryLength) << "\n";
		}
	}
	log << std::endl;
}

/**
 * print result for testing purposes
 */
void SuffixSearch::testPrintResults() {
	std::cout << "Results: " << std::endl;
	const std::int64_t * sa = dc3Algorithm_->getSA();
	int resultCounter = 0;
	std::set<int> test = std::set<int>();
	for (unsigned int i = 0; i < results_.size(); ++i) {
		UniDirTask * currentResult = results_[i];
		for (std::int64_t j = currentResult->interval_.start;
				j <= currentResult->interval_.end; ++j) {
			std::cout << ++resultCounter << ") "
					<< target_.substr(sa[j], input_->getQuerySeq().length())
					<< " - index: " << sa[j] << std::endl;
			std::cout << resultCounter << ") " << input_->getQueryStracture();
			std::cout << " - gaps[";
			for (int i = 0; i < input_->getGapNo(); ++i) {
				std::cout << (int) (currentResult->gaps_[i]);
				if (i + 1 < input_->getGapNo())
					std::cout << ",";
			}
			std::cout << "]" << std::endl;
			test.insert(sa[j]);

		}
	}
}

void SuffixSearch::stopSearch() {
	boost::mutex::scoped_lock lock(queueMutex_);
	keepGoing_ = false;
	queueWaiter_.notify_all();
}
