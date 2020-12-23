/*
 * LcpInterval.h
 *
 *  Created on: Sep 28, 2014
 *      Author: matan
 */

#ifndef LCPINTERVAL_H_
#define LCPINTERVAL_H_

#include <cstdint>

class LcpInterval {
public:
	std::int64_t start;
	std::int64_t end;
	LcpInterval(std::int64_t , std::int64_t );
	LcpInterval();
	LcpInterval(const LcpInterval & other);
	bool isEmpty () const;
	bool isSingle() const;
	void setEmpty();
	virtual ~LcpInterval();
	LcpInterval & operator=(const LcpInterval & other);
};

#endif /* LCPINTERVAL_H_ */
