/*
 * BiDirTask.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#include <BiDirTask.h>

BiDirTask::BiDirTask(unsigned int queryIndex, std::int64_t targetIndex,
		unsigned int queryRevIndex, int revUnchecked, HairpinQuery & query,
		bool forward) :
		UniDirTask(queryIndex, targetIndex, query.getForwardGapNo()), forward_(
				forward), queryRevIndex_(queryRevIndex), reverseGaps_(0), reverseGapNo_(
				query.getReverseGapNo()), revUnchecked_(revUnchecked), revJob_(
				false) {
	reverseGaps_ = new char[query.getReverseGapNo()];
	clearGaps(reverseGaps_, query.getReverseGapNo());
}

BiDirTask::BiDirTask(LcpInterval & interval, unsigned int queryIndex,
		std::int64_t targetIndex, unsigned int queryRevIndex, int revUnchecked,
		HairpinQuery & query, bool forward) :
		UniDirTask(interval, queryIndex, targetIndex, query.getForwardGapNo()), forward_(
				forward), queryRevIndex_(queryRevIndex), reverseGaps_(0), reverseGapNo_(
				query.getReverseGapNo()), revUnchecked_(revUnchecked), revJob_(
				false) {
	reverseGaps_ = new char[query.getReverseGapNo()];
	clearGaps(reverseGaps_, query.getReverseGapNo());
}
BiDirTask::BiDirTask(LcpInterval & interval, unsigned int queryIndex,
		std::int64_t targetIndex, unsigned int queryRevIndex, int revUnchecked,
		HairpinQuery & query, char * forwardGaps, char * reverseGaps,
		bool forward) :
		UniDirTask(interval, queryIndex, targetIndex, query.getForwardGapNo(),
				forwardGaps), forward_(forward), queryRevIndex_(queryRevIndex), reverseGaps_(
				0), reverseGapNo_(query.getReverseGapNo()), revUnchecked_(
				revUnchecked), revJob_(false) {
	reverseGaps_ = new char[query.getReverseGapNo()];
	copyGaps(reverseGaps_, reverseGaps, query.getReverseGapNo());
}

BiDirTask::BiDirTask(const BiDirTask & other) :
		UniDirTask(other), forward_(other.forward_), queryRevIndex_(
				other.queryRevIndex_), reverseGaps_(0), reverseGapNo_(
				other.reverseGapNo_), revUnchecked_(other.revUnchecked_), revJob_(
				false) {
	reverseGaps_ = new char[reverseGapNo_];
	copyGaps(reverseGaps_, other.reverseGaps_, reverseGapNo_);
}

BiDirTask::~BiDirTask() {
	if (reverseGaps_ != 0)
		delete[] reverseGaps_;
}

int BiDirTask::getForQueryIndex() const {
	return getQueryIndex(getIsForward());
}

void BiDirTask::setForQueryIndex(int queryIndex) {
	setQueryIndex(queryIndex, getIsForward());
}

int BiDirTask::getRevQueryIndex() const {
	return getQueryIndex(!getIsForward());
}

void BiDirTask::setRevQueryIndex(int queryIndex) {
	setQueryIndex(queryIndex, !getIsForward());
}

bool BiDirTask::getIsForward() const {
	return forward_;
}
void BiDirTask::changeDirection() {
	forward_ = !forward_;
}

void BiDirTask::setDirection(bool isForward) {
	forward_ = isForward;
}

int BiDirTask::getQueryIndex(bool isForward) const {
	if (isForward)
		return queryIndex_;
	else
		return queryRevIndex_;
}
std::int64_t BiDirTask::getTargetIndex() const {
	return targetIndex_;
}
void BiDirTask::setQueryIndex(int queryIndex, bool isForward) {
	if (isForward)
		queryIndex_ = queryIndex;
	else
		queryRevIndex_ = queryIndex;
}
void BiDirTask::setTargetIndex(std::int64_t targetIndex) {
	targetIndex_ = targetIndex;
}
char * BiDirTask::getGaps(bool isForward) {
	if (isForward)
		return gaps_;
	else
		return reverseGaps_;
}

/**
 * returns all gaps initiated up to maxGap, -1 for all
 */
int BiDirTask::getGapNo(bool isForward) const {
	if (isForward)
		return gapNo_;
	else
		return reverseGapNo_;
}

int BiDirTask::getTotalGap(bool isForward) {
	int result = 0;
	for (int i = 0; i < getGapNo(isForward); ++i) {
		if (getGaps(isForward)[i] == -1)
			break;
		result += getGaps(isForward)[i];
	}
	return result;
}

BiDirTask & BiDirTask::operator=(const BiDirTask & other) {
	if (this == &other) {
		return *this;
	}
	UniDirTask::operator=(other);

	forward_ = other.forward_;
	queryRevIndex_ = other.queryRevIndex_;
	revUnchecked_ = other.revUnchecked_;
	reverseGapNo_ = other.reverseGapNo_;
	if (reverseGaps_ != 0) {
		delete reverseGaps_;
	}
	reverseGaps_ = new char[reverseGapNo_];
	copyGaps(reverseGaps_, other.reverseGaps_, reverseGapNo_);

	return *this;
}
