/*
 * HairpinQuery.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#include <HairpinQuery.h>
#include <climits>

HairpinQuery::HairpinQuery(SubQuery & hairPinQuery) :
		index_(hairPinQuery.getIndexInFullQuery()), forwardQuery_(NULL), reverseQuery_(NULL) {
	// TODO maybe check if hairpin?
	unsigned int startIndex = getStartIndex(hairPinQuery.getQueryStructure(),
			hairPinQuery.getQuerySequence());
	std::vector<Gap *> allGaps;
	hairPinQuery.getGapsInIndexes(allGaps, 0,
			hairPinQuery.getQuerySequence().length(), Input::INC_BOTH);
	std::vector<Gap *> backwardGaps;
	std::vector<Gap *> forwardGaps;
	for (unsigned int i = 0; i < allGaps.size(); ++i) {
		if (allGaps[i]->index >= startIndex) {
			forwardGaps.push_back(allGaps[i]);
		} else {
			Gap * temp = new Gap(*allGaps[i]);
			temp->index = startIndex - temp->index;
			backwardGaps.insert(backwardGaps.begin(), temp);
		}
	}
	//init forward
	forwardQuery_ = new SubQuery(startIndex, forwardGaps,
			hairPinQuery.getQuerySequence().substr(startIndex,
					hairPinQuery.getQuerySequence().length() - startIndex),
			hairPinQuery.getQueryStructure().substr(startIndex,
					hairPinQuery.getQueryStructure().length() - startIndex));

	//init backward
	std::string backwardQuerySequence(
			hairPinQuery.getQuerySequence().substr(0, startIndex));
	std::string backwardQueryStructure(
			hairPinQuery.getQueryStructure().substr(0, startIndex));
	reverseQuery_ = new SubQuery(0, backwardGaps,
			std::string(backwardQuerySequence.rbegin(),
					backwardQuerySequence.rend()),
			std::string(backwardQueryStructure.rbegin(),
					backwardQueryStructure.rend()));
	// clean backward query (we used copies to fix index)
	for (unsigned int i = 0; i < backwardGaps.size(); ++i) {
		delete backwardGaps[i];
	}
}

HairpinQuery::~HairpinQuery() {
	if (forwardQuery_ != NULL) {
		delete forwardQuery_;
	}
	if (reverseQuery_ != NULL) {
		delete reverseQuery_;
	}
}

/**
 * first inner loop char that isn't a wild card
 */
int HairpinQuery::getStartIndex(const std::string & hairpinStructure,
		const std::string & hairpinSequence) {
	int innerLoopIndex = -1;
	int firstInnerLoopIndex = -1;
	bool wildCard = true;
	for (unsigned int i = 0; i < hairpinStructure.length(); ++i) {
		if (hairpinStructure[i] == '(') {
			innerLoopIndex = -1;
			wildCard = true;
		} else if (wildCard && hairpinStructure[i] == '.') {
			if (innerLoopIndex == -1) {
				firstInnerLoopIndex = i;
			}
			innerLoopIndex = i;
			if (hairpinSequence[i] == 'A' || hairpinSequence[i] == 'G'
					|| hairpinSequence[i] == 'C' || hairpinSequence[i] == 'U')
				wildCard = false;
		} else if (hairpinStructure[i] == ')') {
			if (innerLoopIndex == -1) {
				innerLoopIndex = i;
			}
			break;
		}
	}
	if (wildCard)
		innerLoopIndex = firstInnerLoopIndex;
	return innerLoopIndex;
}

int HairpinQuery::getEndDistance(int index, bool isForward) {
	int distance = 0;
	if (isForward)
		distance = forwardQuery_->getQuerySequence().length() - index;
	else
		distance = reverseQuery_->getQuerySequence().length() - index;
	return distance;
}

void HairpinQuery::getGapsInIndexes(std::vector<Gap *> & gaps,
		unsigned int start, unsigned int end, Input::GapMode mode,
		bool isForward) {
	if (isForward)
		forwardQuery_->getGapsInIndexes(gaps, start, end, mode);
	else
		reverseQuery_->getGapsInIndexes(gaps, start, end, mode);
}

const std::string & HairpinQuery::getQuerySequence(bool isForward) {
	if (isForward)
		return forwardQuery_->getQuerySequence();
	else
		return reverseQuery_->getQuerySequence();
}

const std::string & HairpinQuery::getQueryStructure(bool isForward) {
	if (isForward)
		return forwardQuery_->getQueryStructure();
	else
		return reverseQuery_->getQueryStructure();
}

int HairpinQuery::getForwardGapNo() {
	return forwardQuery_->getGapNo();
}

int HairpinQuery::getReverseGapNo() {
	return reverseQuery_->getGapNo();
}

Gap & HairpinQuery::getGapById(int id, bool isForward) {
	if (isForward)
		return forwardQuery_->getGapById(id);
	else
		return reverseQuery_->getGapById(id);
}

Gap * HairpinQuery::getGapByIndex(int index, bool isForward) {
	if (isForward)
		return forwardQuery_->getGapByIndex(index);
	else
		return reverseQuery_->getGapByIndex(index);
}
