/*
 * UniDirTask.h
 *
 *  Created on: Sep 27, 2014
 *      Author: matan
 */

#ifndef UNIDIRTASK_H_
#define UNIDIRTASK_H_

#include <LcpInterval.h>

class UniDirTask {
protected:
	static void clearGaps(char * gaps, int gapNo) {
		for (int i = 0; i < gapNo; ++i) {
			gaps[i] = -1;
		}
	}
	static void copyGaps(char * gaps, const char * newGaps, int gapNo) {
		for (int i = 0; i < gapNo; ++i)
			gaps[i] = newGaps[i];
	}
public:
	LcpInterval interval_;
	unsigned int queryIndex_;
	unsigned int targetIndex_;
	unsigned int gapNo_;
	char * gaps_;
	UniDirTask(unsigned int, unsigned int, unsigned int);
	UniDirTask(LcpInterval &, unsigned int, unsigned int, unsigned int);
	UniDirTask(LcpInterval &, unsigned int, unsigned int, unsigned int, char *);
	UniDirTask(const UniDirTask & other);
	UniDirTask & operator=(const UniDirTask & other);
	virtual ~UniDirTask();
};

#endif /* UNIDIRTASK_H_ */
