/*
 * HairpinQuery.h
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#ifndef INCLUDE_HAIRPINQUERY_H_
#define INCLUDE_HAIRPINQUERY_H_

#include <SubQuery.h>

class HairpinQuery {
private:
	int index_;
	SubQuery * forwardQuery_;
	SubQuery * reverseQuery_;
	int getStartIndex(const std::string & hairpinStructure,
			const std::string & hairpinSequence);
public:
	HairpinQuery(SubQuery &);
	virtual ~HairpinQuery();
	int getEndDistance(int, bool isForward);
	virtual void getGapsInIndexes(std::vector<Gap *> &, unsigned int,
			unsigned int, Input::GapMode, bool isForward);
	int getForwardGapNo();
	int getReverseGapNo();
	const std::string & getQuerySequence(bool isForward);
	const std::string & getQueryStructure(bool isForward);
	Gap & getGapById(int id, bool isForward);
	Gap * getGapByIndex(int, bool);
	int getIndexInQuery() {
		return index_;
	}
	unsigned int getQuerySize() {
		return reverseQuery_->getQuerySequence().length() + forwardQuery_->getQuerySequence().length();
	}
};

#endif /* INCLUDE_HAIRPINQUERY_H_ */
