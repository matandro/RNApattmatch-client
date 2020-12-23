/*
 * SuffixTask.cpp
 *
 *  Created on: Sep 27, 2014
 *      Author: matan
 */

#include <UniDirTask.h>

UniDirTask::UniDirTask(LcpInterval & interval, unsigned int queryIndex,
		unsigned int targetIndex, unsigned int gapNo) :
		interval_(interval), queryIndex_(queryIndex), targetIndex_(targetIndex), gapNo_(
				gapNo), gaps_(0) {
	gaps_ = new char[gapNo];
	clearGaps(gaps_, gapNo_);
}

UniDirTask::UniDirTask(unsigned int queryIndex, unsigned int targetIndex,
		unsigned int gapNo) :
		interval_(), queryIndex_(queryIndex), targetIndex_(targetIndex), gapNo_(
				gapNo), gaps_(0) {
	gaps_ = new char[gapNo];
	clearGaps(gaps_, gapNo_);
}

UniDirTask::UniDirTask(LcpInterval & interval, unsigned int queryIndex,
		unsigned int targetIndex, unsigned int gapNo, char * gaps) :
		interval_(interval), queryIndex_(queryIndex), targetIndex_(targetIndex), gapNo_(
				gapNo), gaps_(0) {
	gaps_ = new char[gapNo];
	copyGaps(gaps_, gaps, gapNo_);
}

UniDirTask::UniDirTask(const UniDirTask & other) :
		interval_(other.interval_), queryIndex_(other.queryIndex_), targetIndex_(
				other.targetIndex_), gapNo_(other.gapNo_), gaps_(0) {
	gaps_ = new char[gapNo_];
	copyGaps(gaps_, other.gaps_, gapNo_);
}

UniDirTask::~UniDirTask() {
	delete[] gaps_;
}

UniDirTask & UniDirTask::operator=(const UniDirTask & other) {
	if (this == &other) {
		return *this;
	}
	interval_ = other.interval_;
	queryIndex_ = other.queryIndex_;
	targetIndex_ = other.targetIndex_;
	gapNo_ = other.gapNo_;
	if (gaps_ != 0) {
		delete gaps_;
	}
	gaps_ = new char[gapNo_];
	copyGaps(gaps_, other.gaps_, gapNo_);
	return *this;
}
