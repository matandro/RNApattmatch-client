/*
 * HairpinSearch.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#include <HairpinSearch.h>
#include <algorithm>
#include <SuffixSearch.h>
#include <stack>
#include <utility>
#include <Log.h>
#include <climits>
#include <boost/thread.hpp>

//#define DEBUG

#ifndef DEBUG
static bool DEBUG_PRINT = false;
#else
static bool DEBUG_PRINT = true;
#endif
/**
 * debugging function
 */
void HairpinSearch::printInfoTask(const char * premsg, BiDirTask & printTask,
		bool result = false) {
	if (DEBUG_PRINT) {
		bool badgap = false;
		bool baddist = false;
		static boost::mutex writerMutex_;
		boost::mutex::scoped_lock lock(writerMutex_);
		char direction = printTask.getIsForward() ? 'F' : 'R';
		if (printTask.interval_.end - printTask.interval_.start < 5) {
			std::cout << premsg << "indexes " << direction << " [";
			for (int i = printTask.interval_.start;
					i <= printTask.interval_.end; ++i) {
				std::cout
						<< affixArray_.getIntervalsIndex(i,
								printTask.getIsForward()) << ",";
			}
			std::cout << "]" << std::endl;
		}
		std::cout << premsg << " - TASK: [" << printTask.interval_.start << ","
				<< printTask.interval_.end << "] " << direction
				<< " Query Indexes: (" << printTask.getQueryIndex(false) << ","
				<< printTask.getQueryIndex(true) << ") Target: "
				<< printTask.getTargetIndex() << " Unchecked: "
				<< printTask.getRevUnChecked() << " Gaps[";
		int total = 0;
		for (int l = printTask.getGapNo(false) - 1; l >= 0; --l) {
			total += printTask.getGaps(false)[l];
			std::cout << (int) (printTask.getGaps(false)[l]);
			std::cout << ",";
			if (printTask.getGaps(false)[l] == -1)
				badgap = true;
		}
		for (int l = 0; l < printTask.getGapNo(true); ++l) {
			total += printTask.getGaps(true)[l];
			std::cout << (int) (printTask.getGaps(true)[l]);
			std::cout << ",";
			if (printTask.getGaps(true)[l] == -1)
				baddist = true;
		}
		std::cout << "]" << std::endl;
		if ((printTask.getQueryIndex(false) + printTask.getQueryIndex(true)
				+ total)
				!= (printTask.getTargetIndex() - printTask.getRevUnChecked()))
			baddist = true;
		if ((baddist || badgap) && result) {
			if (badgap)
				std::cout << "ERROR GAP RESULT =-1" << std::endl;
			if (baddist)
				std::cout << "ERROR DISTANCE " << total << "+"
						<< (printTask.getQueryIndex(false)
								+ printTask.getQueryIndex(true)) << " != "
						<< (printTask.getTargetIndex()
								- printTask.getRevUnChecked()) << std::endl;
			exit(-1);
		}
	}
}

HairpinSearch::HairpinSearch(Input & input, AffixArray & affixArray,
		HairpinQuery & query) :
		input_(input), affixArray_(affixArray), hairpin_(query), results_(), queueMutex_(), resultMutex_(), queueWaiter_(), keepGoing_(
				true) {
}

HairpinSearch::~HairpinSearch() {
	for (unsigned int i = 0; i < results_.size(); ++i) {
		delete results_[i];
	}
}

/**
 * search for all occurrences of the hairpin
 */
bool HairpinSearch::search() {
	LcpInterval initialInterval(0, affixArray_.getTargetLength());
	BiDirTask * searchTask = new BiDirTask(initialInterval, 0, 0, 0, 0,
			hairpin_, true);

	std::queue<BiDirTask *> workQueue;

	int workingNumber = input_.getThreadNo();

	boost::thread_group threadPool;
	std::vector<boost::thread *> threads;
	workQueue.push(searchTask);
	for (int i = 0; i < input_.getThreadNo(); ++i) {
		threads.push_back(
				new boost::thread(&HairpinSearch::taskRunner, this, &workQueue,
						&workingNumber, i + 1));
		threadPool.add_thread(threads[i]);
	}

	threadPool.join_all();

	for (int i = 0; i < input_.getThreadNo(); ++i) {
		threadPool.remove_thread(threads[i]);
		delete threads[i];
	}

	// in case of too many results exit
	while (workQueue.empty() == false) {
		delete workQueue.front();
		workQueue.pop();
	}

	return !results_.empty();
}

void HairpinSearch::taskRunner(std::queue<BiDirTask *> * taskQueue,
		int * workingThreads, int id) {
	BiDirTask * currentTask = NULL;
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
			currentTask = performSearchTask(currentTask, *taskQueue);
			++taskCounter;
		}
	}
	lock.lock();
	std::cout << "Thread " << id << " finished, solved " << taskCounter
			<< " tasks!" << std::endl;
}

bool HairpinSearch::checkFinish(BiDirTask & searchTask) {
	return hairpin_.getQuerySequence(searchTask.getIsForward()).length()
			== (unsigned) searchTask.getQueryIndex(searchTask.getIsForward())
			&& hairpin_.getQuerySequence(!searchTask.getIsForward()).length()
					== (unsigned) searchTask.getQueryIndex(
							!searchTask.getIsForward());
}

/**
 * Search children of a single interval intervals
 */
BiDirTask * HairpinSearch::performSearchTask(BiDirTask * searchTask,
		std::queue<BiDirTask *> & workQueue) {
	// check if this task is a result
	printInfoTask("performSearchTask start", *searchTask);
	if (checkFinish(*searchTask)) {
		printInfoTask("performSearchTask good", *searchTask, true);
		boost::mutex::scoped_lock lock(resultMutex_);
		results_.push_back(searchTask);
		lock.unlock();
		searchTask = NULL;
		return searchTask;
	}
	// Some tasks should continue straight to search (not children search)
	if (searchTask->interval_.isSingle()
			|| (affixArray_.getIntervalLcp(searchTask->interval_,
					searchTask->getIsForward()) > searchTask->getTargetIndex())
			|| (searchTask->revJob_ && searchTask->getRevUnChecked() > 0)) {
		searchTask->revJob_ = false;
		return searchIntervalTask(searchTask, workQueue);
	}
	// check every child interval
	std::vector<LcpInterval> childIntervals;
	BiDirTask * keepTask = NULL;
	affixArray_.getChildIntervals(childIntervals, searchTask->interval_,
			searchTask->getIsForward());
	LcpInterval currentInterval = searchTask->interval_;
	BiDirTask * currentTask;
	for (std::vector<LcpInterval>::iterator it = childIntervals.begin();
			it != childIntervals.end(); ++it) {
		/*std::cout << "INTERVAL: [" << (*it).start << "," << (*it).end << "]"
		 << std::endl;*/
		currentTask = new BiDirTask(*searchTask);
		currentTask->interval_ = *it;
		printInfoTask("performSearchTask child before CT", *currentTask);
		currentTask = searchIntervalTask(currentTask, workQueue);
#ifdef DEBUG
		if (currentTask != NULL)
			printInfoTask("performSearchTask child after CT", *currentTask);
		else
			std::cout << "performSearchTask child after CT - NULL" << std::endl;
#endif
		if (currentTask != NULL) {
			if (keepTask == NULL) {
				keepTask = currentTask;
			} else {
				boost::mutex::scoped_lock lock(queueMutex_);
				workQueue.push(currentTask);
				queueWaiter_.notify_one();
			}
		}
	}

	delete searchTask;
	searchTask = keepTask;

	return searchTask;
}

void HairpinSearch::compareSingletonInterval(BiDirTask & singeltonTask) {
	// Get non initialized gaps
	std::vector<Gap *> uninitGapsFor;
	hairpin_.getGapsInIndexes(uninitGapsFor, singeltonTask.getForQueryIndex(),
			hairpin_.getQuerySequence(singeltonTask.getIsForward()).length(),
			Input::INC_BOTH, singeltonTask.getIsForward());
	for (std::vector<Gap *>::iterator it = uninitGapsFor.begin();
			it != uninitGapsFor.end();) {
		if (singeltonTask.getGaps(singeltonTask.getIsForward())[(*it)->gapNo]
				!= -1) {
			uninitGapsFor.erase(it);
		} else {
			break;
		}
	}
	std::vector<Gap *> uninitGapsRev;
	hairpin_.getGapsInIndexes(uninitGapsRev, singeltonTask.getRevQueryIndex(),
			hairpin_.getQuerySequence(!singeltonTask.getIsForward()).length(),
			Input::INC_BOTH, !singeltonTask.getIsForward());
	for (std::vector<Gap *>::iterator it = uninitGapsRev.begin();
			it != uninitGapsRev.end();) {
		if (singeltonTask.getGaps(singeltonTask.getIsForward())[(*it)->gapNo]
				!= -1) {
			uninitGapsRev.erase(it);
		} else {
			break;
		}
	}
	// compare all
	bool targetDirection = singeltonTask.getIsForward();
	std::stack<GapBackTrack> usedGap;
	int countFor = -1;
	int countRev = -1;
	do {
		CompareState compareState = CompareState::KEEP_COMPARING;
		while (compareState == CompareState::KEEP_COMPARING) {
#ifdef DEBUG
			std::cout << "compareSingletonInterval loop T"
					<< singeltonTask.getTargetIndex() << " R"
					<< singeltonTask.getRevUnChecked() << " | if?"
					<< singeltonTask.getIsForward() << " Qf"
					<< singeltonTask.getForQueryIndex() << " Qr"
					<< singeltonTask.getRevQueryIndex() << std::endl;
#endif
			// check if we are finished or reached forward end
			if ((unsigned) singeltonTask.getForQueryIndex()
					== hairpin_.getQuerySequence(singeltonTask.getIsForward()).length()) {
				if ((unsigned) singeltonTask.getRevQueryIndex()
						== hairpin_.getQuerySequence(
								!singeltonTask.getIsForward()).length()) {
					compareState = CompareState::MATCH;
					break;
				} else {
					singeltonTask.changeDirection();
				}
			}
			// check gap and increase
			Gap * currentGap = hairpin_.getGapByIndex(
					singeltonTask.getForQueryIndex(),
					singeltonTask.getIsForward());
			if (currentGap != NULL) {
				if (singeltonTask.getGaps(singeltonTask.getIsForward())[currentGap->gapNo]
						== -1) {

					printInfoTask("before Added ", singeltonTask);
					singeltonTask.getGaps(singeltonTask.getIsForward())[currentGap->gapNo] =
							0;
					GapBackTrack bt;
					bt.amount = 0;
					bt.isForward = singeltonTask.getIsForward();
					bt.queryIndex = singeltonTask.getForQueryIndex();
					bt.queryRevIndex = singeltonTask.getRevQueryIndex();
					bt.targetIndex = singeltonTask.getTargetIndex();
					bt.revUnchked = singeltonTask.getRevUnChecked();
					usedGap.push(bt);
					if (singeltonTask.getIsForward() == targetDirection)
						countFor++;
					else
						countRev++;
					printInfoTask("after Added", singeltonTask);
#ifdef DEBUG
					std::cout << "ADDED dir(" << bt.isForward << ") countFor:"
							<< countFor << " countRev:" << countRev
							<< " amount:" << (int) bt.amount << " originaldir"
							<< targetDirection << std::endl;
#endif
				}
				if (singeltonTask.getIsForward() == targetDirection) {
					singeltonTask.setTargetIndex(
							singeltonTask.getTargetIndex()
									+ getGapRemaining(singeltonTask,
											singeltonTask.getIsForward()));
				} else {
					singeltonTask.setRevUnChecked(
							singeltonTask.getRevUnChecked()
									- getGapRemaining(singeltonTask,
											singeltonTask.getIsForward()));
				}

				if (checkOutofBounds(singeltonTask, targetDirection, 0)) {
					compareState = CompareState::MISMATCH;
					break;
				}
			}
			char afStruct =
					hairpin_.getQueryStructure(singeltonTask.getIsForward())[singeltonTask.getForQueryIndex()];
			if (afStruct == '.') {
				// check if we can extend forward
				if (checkOutofBounds(singeltonTask, targetDirection, 1)) {
					compareState = CompareState::MISMATCH;
					break;
				}
				// Compare 1 forward
				char forwardTargetChar = affixArray_.getTargetChar(
						getRelativeTarget(singeltonTask, targetDirection),
						singeltonTask.interval_.start, targetDirection);

				char afSeq =
						hairpin_.getQuerySequence(singeltonTask.getIsForward())[singeltonTask.getForQueryIndex()];
#ifdef DEBUG
				std::cout << "compareSingletonInterval compare - "
						<< forwardTargetChar << "," << afSeq << " at "
						<< getRelativeTarget(singeltonTask, targetDirection)
						<< std::endl;
#endif
				if (!SuffixSearch::isFASTAEqual(forwardTargetChar, afSeq)) {
					compareState = CompareState::MISMATCH;
					break;
				}
				singeltonTask.setForQueryIndex(
						singeltonTask.getForQueryIndex() + 1);
				if (targetDirection == singeltonTask.getIsForward()) {
					singeltonTask.setTargetIndex(
							singeltonTask.getTargetIndex() + 1);
				} else {
					singeltonTask.setRevUnChecked(
							singeltonTask.getRevUnChecked() - 1);
				}
			} else {
				char arStruct =
						hairpin_.getQueryStructure(
								!singeltonTask.getIsForward())[singeltonTask.getRevQueryIndex()];
				Gap * reverseGap = hairpin_.getGapByIndex(
						singeltonTask.getRevQueryIndex(),
						!singeltonTask.getIsForward());
				if ((reverseGap != NULL
						&& singeltonTask.getGaps(!singeltonTask.getIsForward())[reverseGap->gapNo]
								== -1) || arStruct == '.') {
					singeltonTask.changeDirection();
				} else {
					// check if we can extend forward
					if (checkOutofBounds(singeltonTask, targetDirection, 1)) {
						compareState = CompareState::MISMATCH;
						break;
					}
					// Compare 1 forward
					char forwardTargetChar = affixArray_.getTargetChar(
							getRelativeTarget(singeltonTask, targetDirection),
							singeltonTask.interval_.start, targetDirection);

					char afSeq =
							hairpin_.getQuerySequence(
									singeltonTask.getIsForward())[singeltonTask.getForQueryIndex()];
#ifdef DEBUG
					std::cout << "compareSingletonInterval compare - "
							<< forwardTargetChar << "," << afSeq << " at "
							<< getRelativeTarget(singeltonTask, targetDirection)
							<< std::endl;
#endif
					if (!SuffixSearch::isFASTAEqual(forwardTargetChar, afSeq)) {
						compareState = CompareState::MISMATCH;
						break;
					}
					singeltonTask.setForQueryIndex(
							singeltonTask.getForQueryIndex() + 1);
					if (targetDirection == singeltonTask.getIsForward()) {
						singeltonTask.setTargetIndex(
								singeltonTask.getTargetIndex() + 1);
					} else {
						singeltonTask.setRevUnChecked(
								singeltonTask.getRevUnChecked() - 1);
					}
					if (checkOutofBounds(singeltonTask, !targetDirection, 1)) {
						compareState = CompareState::MISMATCH;
						break;
					}
					char reverseQueryChar =
							hairpin_.getQuerySequence(
									!singeltonTask.getIsForward())[singeltonTask.getRevQueryIndex()];
					char postMatchChar = input_.getMatrixChar(forwardTargetChar,
							reverseQueryChar);
					char reverseTargetChar = affixArray_.getTargetChar(
							getRelativeTarget(singeltonTask, !targetDirection),
							singeltonTask.interval_.start, targetDirection);
#ifdef DEBUG
					std::cout << "compareSingletonInterval compare reverse - "
							<< reverseTargetChar << "," << postMatchChar << "->"
							<< postMatchChar << " at "
							<< getRelativeTarget(singeltonTask,
									!targetDirection) << std::endl;
#endif
					if (postMatchChar == 0
							|| !SuffixSearch::isFASTAEqual(reverseTargetChar,
									postMatchChar)) {
						compareState = CompareState::MISMATCH;
						break;
					}
					singeltonTask.setRevQueryIndex(
							singeltonTask.getRevQueryIndex() + 1);
					if (targetDirection == singeltonTask.getIsForward()) {
						singeltonTask.setRevUnChecked(
								singeltonTask.getRevUnChecked() - 1);
					} else {
						singeltonTask.setTargetIndex(
								singeltonTask.getTargetIndex() + 1);
					}
				}
			}
		}
		// if match insert result
		if (compareState == CompareState::MATCH) {
			singeltonTask.setDirection(targetDirection);
			BiDirTask * result = new BiDirTask(singeltonTask);
			printInfoTask("compareSingletonInterval new result OriginalDir:",
					singeltonTask, true);
			boost::mutex::scoped_lock lock(resultMutex_);
			results_.push_back(result);
		}

		// revert to last gap attempt
#ifdef DEBUG
		if (!usedGap.empty()) {
			std::cout << "TEST dir(" << usedGap.top().isForward
					<< ") currentFor" << countFor << " currentRev " << countRev
					<< " amount " << (int) usedGap.top().amount
					<< " original dir:" << targetDirection << std::endl;
			if (usedGap.top().isForward == targetDirection)
				std::cout << (int) uninitGapsFor[countFor]->maxLength;
			else
				std::cout << (int) uninitGapsRev[countRev]->maxLength;
			std::cout << " IS BAD ("
					<< ((usedGap.top().isForward == targetDirection
							&& (int) usedGap.top().amount
									>= (int) uninitGapsFor[countFor]->maxLength)
							|| (usedGap.top().isForward != targetDirection
									&& (int) usedGap.top().amount
											>= (int) uninitGapsRev[countRev]->maxLength))
					<< ")" << std::endl;
		}
#endif
		while (!usedGap.empty()
				&& ((usedGap.top().isForward == targetDirection
						&& (int) usedGap.top().amount
								>= (int) uninitGapsFor[countFor]->maxLength)
						|| (usedGap.top().isForward != targetDirection
								&& (int) usedGap.top().amount
										>= (int) uninitGapsRev[countRev]->maxLength))) {
			if (usedGap.top().isForward == targetDirection) {
				singeltonTask.getGaps(usedGap.top().isForward)[uninitGapsFor[countFor--]->gapNo] =
						-1;
			} else {
				singeltonTask.getGaps(usedGap.top().isForward)[uninitGapsRev[countRev--]->gapNo] =
						-1;
			}
			usedGap.pop();
		}
		if (!usedGap.empty()) {
			GapBackTrack & gbt = usedGap.top();
			gbt.amount++;
			if (usedGap.top().isForward == targetDirection) {
				singeltonTask.getGaps(usedGap.top().isForward)[uninitGapsFor[countFor]->gapNo] =
						gbt.amount;
			} else {
				singeltonTask.getGaps(usedGap.top().isForward)[uninitGapsRev[countRev]->gapNo] =
						gbt.amount;
			}
			updateGapBack(singeltonTask, gbt);
		}

	} while (!usedGap.empty());
}

bool HairpinSearch::checkOutofBounds(BiDirTask & singeltonTask,
		bool originalForward, int offset) {
	int targetIndex = affixArray_.getIntervalsIndex(
			singeltonTask.interval_.start, originalForward);
	if (originalForward == singeltonTask.getIsForward()) {
		return ((unsigned) targetIndex
				+ getRelativeTarget(singeltonTask, originalForward) + offset
				>= affixArray_.getTargetLength());
	} else {
		return (targetIndex + getRelativeTarget(singeltonTask, !originalForward)
				- offset < 0);
	}
}

void HairpinSearch::updateGapBack(BiDirTask & singeltonTask,
		GapBackTrack & gbt) {
	singeltonTask.setDirection(gbt.isForward);
	singeltonTask.setTargetIndex(gbt.targetIndex);
	singeltonTask.setRevUnChecked(gbt.revUnchked);
	singeltonTask.setForQueryIndex(gbt.queryIndex);
	singeltonTask.setRevQueryIndex(gbt.queryRevIndex);
}

int HairpinSearch::getRelativeTarget(BiDirTask & singletonTask,
		bool originalDirection) {
	if (singletonTask.getIsForward() == originalDirection) {
		return singletonTask.getTargetIndex();
	} else {
		// fix 1 for reverse
		return singletonTask.getRevUnChecked() - 1;
	}
}

/**
 * search in a single interval interval
 */
BiDirTask * HairpinSearch::searchIntervalTask(BiDirTask * intervalTask,
		std::queue<BiDirTask*> & workQueue) {
	printInfoTask("searchIntervalTask start", *intervalTask);
	if (intervalTask->interval_.isSingle()) {
		printInfoTask("call compareSingletonInterval", *intervalTask);
		compareSingletonInterval(*intervalTask);
		delete intervalTask;
		intervalTask = 0;
	} else {
		// Compare 1 and create continue tasks
		int lcp = affixArray_.getIntervalLcp(intervalTask->interval_,
				intervalTask->getIsForward());
		bool isForwardBefore = intervalTask->getIsForward();
		while (intervalTask != NULL
				&& isForwardBefore == intervalTask->getIsForward()
				&& affixArray_.getIntervalLcp(intervalTask->interval_,
						intervalTask->getIsForward())
						> intervalTask->getTargetIndex()
				&& hairpin_.getQueryStructure(intervalTask->getIsForward()).length()
						> (unsigned) intervalTask->getForQueryIndex()) {
			printInfoTask("call compareInterval", *intervalTask);
			intervalTask = compareInterval(intervalTask, workQueue, lcp);
		}
	}
	return intervalTask;
}

/**
 * compares a single interval for 1 charecter
 */
BiDirTask * HairpinSearch::compareInterval(BiDirTask * intervalTask,
		std::queue<BiDirTask *> & workQueue, int forwardLcp) {
	// init gaps (if needed) and push over them (if reached the end, return)
	intervalTask = handleGap(intervalTask, workQueue,
			intervalTask->getIsForward());
	int gapsRemaining = getGapRemaining(*intervalTask,
			intervalTask->getIsForward());
	intervalTask->setTargetIndex(
			std::min(forwardLcp,
					intervalTask->getTargetIndex() + gapsRemaining));
	if ((unsigned) affixArray_.getIntervalsIndex(intervalTask->interval_.start,
			intervalTask->getIsForward()) + intervalTask->getTargetIndex()
			+ intervalTask->getRevUnChecked()
			>= affixArray_.getTargetLength()) {
		printInfoTask("compareInterval 1 - FAIL:", *intervalTask);
		delete intervalTask;
		intervalTask = 0;
		return 0;
	} else if (forwardLcp <= intervalTask->getTargetIndex()) {
		return intervalTask;
	}

// compare characters
	char atStruct =
			hairpin_.getQueryStructure(intervalTask->getIsForward())[intervalTask->getForQueryIndex()];
	if (atStruct == '.') {
		if (compareLetters(*intervalTask, intervalTask->getIsForward())) {
			intervalTask->setTargetIndex(intervalTask->getTargetIndex() + 1);
			intervalTask->setForQueryIndex(
					intervalTask->getForQueryIndex() + 1);
		} else {
			printInfoTask("compareInterval 3 - FAIL:", *intervalTask);
			delete intervalTask;
			intervalTask = NULL;
		}
	} else {
		intervalTask = handleGap(intervalTask, workQueue,
				!intervalTask->getIsForward());
		Gap * reverseGap = hairpin_.getGapByIndex(
				intervalTask->getRevQueryIndex(),
				!intervalTask->getIsForward());
		char reverseQueryChar = hairpin_.getQueryStructure(
				!intervalTask->getIsForward())[intervalTask->getRevQueryIndex()];
		LcpInterval reverseInterval = affixArray_.getReversedInterval(
				intervalTask->interval_, intervalTask->getIsForward());
		int reverseLcp = affixArray_.getIntervalLcp(reverseInterval,
				!intervalTask->getIsForward());
		int gapRemaining = 0;
		if (reverseGap != NULL
				&& (gapRemaining = getGapRemaining(*intervalTask,
						!intervalTask->getIsForward())) > 0) {
			if (intervalTask->getRevUnChecked() > 0) {
				int progress = std::min(intervalTask->getRevUnChecked(),
						gapRemaining);
				intervalTask->setRevUnChecked(
						intervalTask->getRevUnChecked() - progress);
				gapRemaining -= progress;
			}
			if (gapRemaining > 0) {
				intervalTask->interval_ = reverseInterval;
				intervalTask->setRevUnChecked(
						forwardLcp - intervalTask->getTargetIndex());
				intervalTask->setTargetIndex(forwardLcp);
				intervalTask->changeDirection();
				printInfoTask("compareInterval changeDir Gap", *intervalTask);
			}
		} else if (reverseQueryChar == '.') {
			// Get reverse, if gap or . return with a task for the other side
			if (intervalTask->getRevUnChecked() > 0) {
				if (compareLetters(*intervalTask,
						!intervalTask->getIsForward())) {
					intervalTask->setRevUnChecked(
							intervalTask->getRevUnChecked() - 1);
					intervalTask->setRevQueryIndex(
							intervalTask->getRevQueryIndex() + 1);
				} else {
					printInfoTask("compareInterval 2 - FAIL:", *intervalTask);
					delete intervalTask;
					intervalTask = 0;
					return intervalTask;
				}
			} else {
				intervalTask->interval_ = reverseInterval;
				intervalTask->setRevUnChecked(
						forwardLcp - intervalTask->getTargetIndex());
				intervalTask->setTargetIndex(forwardLcp);
				intervalTask->changeDirection();
				printInfoTask("compareInterval changeDir .", *intervalTask);
			}
		} else {
			char forwardTarget = affixArray_.getTargetChar(
					intervalTask->getTargetIndex(),
					intervalTask->interval_.start,
					intervalTask->getIsForward());
			if (!compareLetters(*intervalTask, intervalTask->getIsForward())) {
				printInfoTask("compareInterval 4 - FAIL:", *intervalTask);
				delete intervalTask;
				intervalTask = NULL;
			} else {
				intervalTask->setTargetIndex(
						intervalTask->getTargetIndex() + 1);
				intervalTask->setForQueryIndex(
						intervalTask->getForQueryIndex() + 1);
				// if we have uncheck at start, don't look at other side, compare start
				if (intervalTask->getRevUnChecked() > 0) {
					if (compareCompletionLetter(*intervalTask,
							intervalTask->interval_, forwardTarget,
							intervalTask->getIsForward(),
							intervalTask->getRevUnChecked() - 1)) {
						intervalTask->setRevQueryIndex(
								intervalTask->getRevQueryIndex() + 1);
						intervalTask->setRevUnChecked(
								intervalTask->getRevUnChecked() - 1);
						printInfoTask("compareInterval foundrev (",
								*intervalTask);
					} else {
						printInfoTask("compareInterval 5 - FAIL:",
								*intervalTask);
						delete intervalTask;
						intervalTask = NULL;
					}
				} else {
					if (reverseLcp > forwardLcp) {
						// reverse side LCP is larger then outs, we don't need to check child intervals
						if (compareCompletionLetter(*intervalTask,
								reverseInterval, forwardTarget,
								!intervalTask->getIsForward(), forwardLcp)) {
							intervalTask->setRevQueryIndex(
									intervalTask->getRevQueryIndex() + 1);
#ifdef DEBUG
							std::cout << "TEST: rlcp:" << reverseLcp << " flcp:"
									<< forwardLcp << " clcp:"
									<< affixArray_.getIntervalLcp(
											intervalTask->interval_,
											!intervalTask->getIsForward())
									<< " targetIndex:"
									<< intervalTask->getTargetIndex()
									<< std::endl;
#endif
							intervalTask->setRevUnChecked(
									forwardLcp
											- intervalTask->getTargetIndex());
							intervalTask->setTargetIndex(forwardLcp + 1);
							intervalTask->interval_ = reverseInterval;
							intervalTask->changeDirection();
							printInfoTask("compareInterval create origin",
									*intervalTask);
						} else {
							printInfoTask("compareInterval 6 - FAIL:",
									*intervalTask);
							delete intervalTask;
							intervalTask = NULL;
						}
					} else {
						// same lcp, need to check all children
						std::vector<LcpInterval> reverseIntervals;
						affixArray_.getChildIntervals(reverseIntervals,
								reverseInterval, !intervalTask->getIsForward());
						for (std::vector<LcpInterval>::iterator revIt =
								reverseIntervals.begin();
								revIt != reverseIntervals.end(); ++revIt) {
							if (compareCompletionLetter(*intervalTask, *revIt,
									forwardTarget,
									!intervalTask->getIsForward(),
									forwardLcp)) {
								BiDirTask * newRevTask = new BiDirTask(
										*intervalTask);
								newRevTask->setRevQueryIndex(
										newRevTask->getRevQueryIndex() + 1);
#ifdef DEBUG
								std::cout << "TEST: rlcp:" << reverseLcp
										<< " flcp:" << forwardLcp << " clcp:"
										<< affixArray_.getIntervalLcp(*revIt,
												!intervalTask->getIsForward())
										<< " targetIndex:"
										<< newRevTask->getTargetIndex()
										<< std::endl;
#endif
								newRevTask->setRevUnChecked(
										forwardLcp
												- intervalTask->getTargetIndex());
								newRevTask->setTargetIndex(forwardLcp + 1);
								newRevTask->interval_ = *revIt;
								newRevTask->changeDirection();
								printInfoTask("compareInterval create new",
										*newRevTask);
								boost::mutex::scoped_lock lock(queueMutex_);
								workQueue.push(newRevTask);
								queueWaiter_.notify_one();
							}
						}
						printInfoTask("compareInterval 7 - FAIL:",
								*intervalTask);
						delete intervalTask;
						intervalTask = NULL;
					}
				}
			}
		}

	}
	return intervalTask;
}

bool HairpinSearch::compareCompletionLetter(BiDirTask & intervalTask,
		LcpInterval & interval, char complatorTarget, bool intervalDircetion,
		int targetOffset) {
	if (affixArray_.getIntervalsIndex(interval.start, intervalDircetion)
			+ targetOffset >= (signed) affixArray_.getTargetLength()) {
		return false;
	}

	char reverseQueryChar = hairpin_.getQuerySequence(
			!intervalTask.getIsForward())[intervalTask.getRevQueryIndex()];
	char postMatchChar = input_.getMatrixChar(complatorTarget,
			reverseQueryChar);
	char reverseTargetChar = affixArray_.getTargetChar(targetOffset,
			interval.start, intervalDircetion);
#ifdef DEBUG
	std::cout << "compareCompletionLetter: " << complatorTarget << ","
			<< reverseQueryChar << "->" << postMatchChar << "==" << "A["
			<< targetOffset << "] = " << reverseTargetChar
			<< " At reverse interval: [" << interval.start << ","
			<< interval.end << "] (" << intervalDircetion << ") [";
	for (int i = interval.start; i <= interval.end; ++i) {
		std::cout << affixArray_.getIntervalsIndex(i, intervalDircetion);
		if (i < interval.end)
			std::cout << ",";
	}
	std::cout << "]" << std::endl;
#endif
	return (postMatchChar == 0) ?
			false : SuffixSearch::isFASTAEqual(reverseTargetChar, postMatchChar);
}

BiDirTask * HairpinSearch::handleGap(BiDirTask * searchTask,
		std::queue<BiDirTask *> & workQueue, bool isForward) {
	Gap * gap = hairpin_.getGapByIndex(searchTask->getQueryIndex(isForward),
			isForward);
#ifdef DEBUG
	std::cout << "handleGap gap (" << gap << ") queryIndex: "
			<< searchTask->getQueryIndex(isForward) << " at " << isForward
			<< std::endl;
	printInfoTask("handleGap", *searchTask, false);
#endif
	if (gap != NULL && searchTask->getGaps(isForward)[gap->gapNo] == -1) {
		// generate new tasks for each capacity of this gap
		searchTask->getGaps(isForward)[gap->gapNo] = 0;
		for (int i = 1; i <= (int) gap->maxLength; ++i) {
			BiDirTask * intervalTask = new BiDirTask(*searchTask);
			intervalTask->getGaps(isForward)[gap->gapNo] = i;
			if (searchTask->getIsForward() != isForward) {
				intervalTask->revJob_ = true;
			}
			boost::mutex::scoped_lock lock(queueMutex_);
			workQueue.push(intervalTask);
			queueWaiter_.notify_one();
		}
	}
	return searchTask;
}

int HairpinSearch::getGapRemaining(BiDirTask & searchTask, bool isForward) {
	int result = 0;
	Gap * gap = hairpin_.getGapByIndex(searchTask.getQueryIndex(isForward),
			isForward);
	if (gap != NULL) {
		// counts gaps remaining, assuming other gaps used
		int fullGap = searchTask.getGaps(isForward)[gap->gapNo];
		if (fullGap == -1) {
			std::cerr << "PANIC - getGapRemaining on an uninitialized gap"
					<< std::endl;
			exit(-1);
		}
		int gapsBefore = 0;
		for (unsigned int i = 0; i < gap->gapNo; ++i) {
			gapsBefore += (int) searchTask.getGaps(isForward)[i];
		} // on other side we assume full use of all initiated gaps
		for (int i = 0; i < (int) searchTask.getGapNo(!isForward); ++i) {
			int val = (int) searchTask.getGaps(!isForward)[i];
			if (val != -1)
				gapsBefore += val;
		}
		result = fullGap
				- ((searchTask.getTargetIndex() - searchTask.getRevUnChecked())
						- (gapsBefore + searchTask.getForQueryIndex()
								+ searchTask.getRevQueryIndex()));
	}
#ifdef DEBUG
	std::cout << "getGapRemaining gap (" << gap << ") queryIndex: "
			<< searchTask.getQueryIndex(isForward) << " at " << isForward
			<< " Result:" << result << std::endl;
#endif
	return result;
}

bool HairpinSearch::compareLetters(BiDirTask & currentTask, bool isForward) {
	char target;
	char query = hairpin_.getQuerySequence(isForward)[currentTask.getQueryIndex(
			isForward)];
	if (isForward == currentTask.getIsForward()) {
		target = affixArray_.getTargetChar(currentTask.getTargetIndex(),
				currentTask.interval_.start, currentTask.getIsForward());
	} else {
		target = affixArray_.getTargetChar(currentTask.getRevUnChecked() - 1,
				currentTask.interval_.start, currentTask.getIsForward());
	}
#ifdef DEBUG
	std::cout << "compareLetters: " << target << " == " << query << std::endl;
#endif
	return SuffixSearch::isFASTAEqual(target, query);
}

void HairpinSearch::printResults() {
	Log & log = Log::getInstance();
	int queryLength = hairpin_.getQuerySequence(true).length()
			+ hairpin_.getQuerySequence(false).length();
	log << "Results:" << "\n";
	for (unsigned int resultIndex = 0; resultIndex < results_.size();
			++resultIndex) {
		BiDirTask * currentResult = results_[resultIndex];
		for (int intervalIndex = currentResult->interval_.start;
				intervalIndex <= currentResult->interval_.end;
				++intervalIndex) {
#ifdef DEBUG
			std::string temp = (currentResult->getIsForward()) ? "FOR" : "REV";
			std::cout << "result sa[" << intervalIndex << "] = "
					<< affixArray_.getIntervalsIndex(intervalIndex,
							currentResult->getIsForward()) << " (k="
					<< currentResult->getRevUnChecked() << " t="
					<< currentResult->getTargetIndex() << ")" << temp
					<< std::endl;
#endif
			// pushing index
			int targetIndex = affixArray_.getIntervalsIndex(intervalIndex,
					currentResult->getIsForward());
			targetIndex += currentResult->getRevUnChecked();
			if (!currentResult->getIsForward()) {
				targetIndex = affixArray_.getTargetLength() - targetIndex;
				targetIndex -= (currentResult->getTargetIndex()
						- currentResult->getRevUnChecked());
			}
			log << targetIndex + 1 << ";"; // start counting from 1
			// pushing gap information (for secondary structure alignment)
			int totalGaps = 0;
			int gapNo = currentResult->getGapNo(false)
					+ currentResult->getGapNo(true);
			for (int l = currentResult->getGapNo(false) - 1; l >= 0; --l) {
				totalGaps += currentResult->getGaps(false)[l];
				log << (int) (currentResult->getGaps(false)[l]);
				if (--gapNo > 0)
					log << ",";
			}
			for (int l = 0; l < currentResult->getGapNo(true); ++l) {
				totalGaps += currentResult->getGaps(true)[l];
				log << (int) (currentResult->getGaps(true)[l]);
				if (--gapNo > 0)
					log << ",";
			}
			log << ";";
			// pushing sequence
			log
					<< affixArray_.getWord().substr(targetIndex,
							totalGaps + queryLength) << "\n";
		}
	}
	log << std::endl;
}

const std::vector<BiDirTask *> & HairpinSearch::getResults() {
	return results_;
}
