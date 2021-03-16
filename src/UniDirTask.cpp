/*
 * SuffixTask.cpp
 *
 *  Created on: Sep 27, 2014
 *      Author: matan
 */

#include <UniDirTask.h>
#include <stdlib.h>

UniDirTask::UniDirTask(LcpInterval & interval, unsigned int queryIndex,
		std::int64_t targetIndex, unsigned int gapNo) :
		interval_(interval), queryIndex_(queryIndex), targetIndex_(targetIndex), gapNo_(
				gapNo), gaps_(0) {
	gaps_ = new char[gapNo];
	clearGaps(gaps_, gapNo_);
}

UniDirTask::UniDirTask(unsigned int queryIndex, std::int64_t targetIndex,
		unsigned int gapNo) :
		interval_(), queryIndex_(queryIndex), targetIndex_(targetIndex), gapNo_(
				gapNo), gaps_(0) {
	gaps_ = new char[gapNo];
	clearGaps(gaps_, gapNo_);
}

UniDirTask::UniDirTask(LcpInterval & interval, unsigned int queryIndex,
		std::int64_t targetIndex, unsigned int gapNo, char * gaps) :
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
	if (gaps_ != NULL) {
		delete[] gaps_;
		gaps_ = NULL;
	}
}

UniDirTask & UniDirTask::operator=(const UniDirTask & other) {
	if (this == &other) {
		return *this;
	}
	interval_ = other.interval_;
	queryIndex_ = other.queryIndex_;
	targetIndex_ = other.targetIndex_;
	gapNo_ = other.gapNo_;
	if (gaps_ != NULL) {
		delete gaps_;
		gaps_ = NULL;
	}
	gaps_ = new char[gapNo_];
	copyGaps(gaps_, other.gaps_, gapNo_);
	return *this;
}
