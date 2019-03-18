/*
 * BiDirTask.h
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#ifndef INCLUDE_BIDIRTASK_H_
#define INCLUDE_BIDIRTASK_H_

#include <UniDirTask.h>
#include <HairpinQuery.h>

class BiDirTask: public UniDirTask {
private:
	bool forward_;
	int queryRevIndex_;
	char * reverseGaps_;
	int reverseGapNo_;
	int revUnchecked_; // k from algorithm
public:
	bool revJob_;
	BiDirTask(unsigned int, unsigned int, unsigned int, int, HairpinQuery &,
			bool);
	BiDirTask(LcpInterval &, unsigned int, unsigned int, unsigned int, int,
			HairpinQuery &, bool);
	BiDirTask(LcpInterval &, unsigned int, unsigned int, unsigned int, int,
			HairpinQuery &, char *, char *, bool);
	BiDirTask(const BiDirTask &);
	BiDirTask & operator=(const BiDirTask & other);
	virtual ~BiDirTask();
	/**
	 * This getter and setters fix access by current direction
	 */
	int getForQueryIndex() const;
	void setForQueryIndex(int queryIndex);
	int getRevQueryIndex() const;
	void setRevQueryIndex(int queryIndex);
	bool getIsForward() const;
	void changeDirection();
	void setDirection(bool isForward);
	// with opposite direction
	int getQueryIndex(bool isForward) const;
	int getTargetIndex() const;
	void setQueryIndex(int queryIndex, bool isForward);
	void setTargetIndex(int targetIndex);
	char * getGaps(bool isForward);
	int getGapNo(bool isForward) const;
	int getTotalGap(bool isForward);

	int getRevUnChecked() const {
		return revUnchecked_;
	}

	void setRevUnChecked(int nonCheckedStart) {
		revUnchecked_ = nonCheckedStart;
	}
};

#endif /* INCLUDE_BIDIRTASK_H_ */
