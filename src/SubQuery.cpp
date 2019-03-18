/*
 * SubQuery.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#include <SubQuery.h>
#include <stack>

SubQuery::SubQuery(int index, const std::vector<Gap *> & gaps,
		const std::string & querySequence, const std::string & queryStructure) :
		index_(index), querySequence_(querySequence), queryStructure_(
				queryStructure), gaps_(), maxGaps_(0) {
	// initiate gaps - normalize for local string
	int endIndex = index + querySequence.size();
	int gapNo = 0;
	for (unsigned int i = 0; i < gaps.size(); ++i) {
		if (gaps[i]->index >= (unsigned) index
				&& gaps[i]->index <= (unsigned) endIndex) {
			gaps_.push_back(Gap());
			gaps_[gapNo].index = gaps[i]->index - index_;
			gaps_[gapNo].gapNo = gapNo;
			gaps_[gapNo].maxLength = gaps[i]->maxLength;
			maxGaps_ += (int) gaps_[gapNo].maxLength;
			++gapNo;
		}
	}
}

SubQuery::~SubQuery() {
}

int SubQuery::index() {
	return 0;
}

unsigned int SubQuery::getGapNo() {
	return gaps_.size();
}

void SubQuery::getGapsInIndexes(std::vector<Gap *> & gaps, unsigned int start,
		unsigned int end, Input::GapMode mode) {
	for (unsigned int i = 0; i < gaps_.size(); ++i) {
		if (gaps_[i].index > start
				|| ((mode == Input::INC_START || mode == Input::INC_BOTH)
						&& gaps_[i].index == start)) {
			if (gaps_[i].index < end
					|| ((mode == Input::INC_END || mode == Input::INC_BOTH)
							&& gaps_[i].index == end)) {
				gaps.push_back(&(gaps_[i]));
			} else {
				break;
			}
		}
	}
}

int SubQuery::getMaxGap() {
	return maxGaps_;
}

Gap * SubQuery::getGapByIndex(int queryIndex) {
	Gap * result = NULL;
	for (unsigned int i = 0; i < gaps_.size(); ++i) {
		if (gaps_[i].index > (unsigned) queryIndex) {
			break;
		} else if (gaps_[i].index == (unsigned) queryIndex) {
			result = &gaps_[i];
			break;
		}
	}
	return result;
}

const std::string & SubQuery::getQuerySequence() {
	return querySequence_;
}

const std::string & SubQuery::getQueryStructure() {
	return queryStructure_;
}

int SubQuery::getIndexInFullQuery() {
	return index_;
}

Gap & SubQuery::getGapById(int id) {
	return gaps_[id];
}
