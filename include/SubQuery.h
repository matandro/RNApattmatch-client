/*
 * SubQuery.h
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#ifndef SUBQUERY_H_
#define SUBQUERY_H_

#include <Input.h>

class SubQuery {
protected:
	int index_;
	const std::string querySequence_;
	const std::string queryStructure_;
	std::vector<Gap> gaps_;
	int maxGaps_;
public:
	SubQuery(int index, const std::vector<Gap *> & gaps,
			const std::string & querySequence, const std::string & queryStructure);
	virtual ~SubQuery();
	unsigned int getGapNo();
	virtual void getGapsInIndexes(std::vector<Gap *> &, unsigned int,
			unsigned int, Input::GapMode);
	virtual int index();
	int getMaxGap();
	Gap * getGapByIndex(int);
	const std::string & getQuerySequence();
	const std::string & getQueryStructure();
	int getIndexInFullQuery();
	Gap & getGapById(int id);
};

#endif /* SUBQUERY_H_ */
