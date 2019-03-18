/*
 * LcpInterval.h
 *
 *  Created on: Sep 28, 2014
 *      Author: matan
 */

#ifndef LCPINTERVAL_H_
#define LCPINTERVAL_H_

class LcpInterval {
public:
	int start;
	int end;
	LcpInterval(int , int );
	LcpInterval();
	LcpInterval(const LcpInterval & other);
	bool isEmpty () const;
	bool isSingle() const;
	void setEmpty();
	virtual ~LcpInterval();
	LcpInterval & operator=(const LcpInterval & other);
};

#endif /* LCPINTERVAL_H_ */
