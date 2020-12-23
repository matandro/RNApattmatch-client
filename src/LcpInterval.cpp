/*
 * LcpInterval.cpp
 *
 *  Created on: Sep 28, 2014
 *      Author: matan
 */

#include <LcpInterval.h>

LcpInterval::LcpInterval(int s, int e) :
		start(s), end(e) {
}

LcpInterval::LcpInterval() :
		start(-1), end(-1) {
}

LcpInterval::LcpInterval(const LcpInterval & other) :
		start(other.start), end(other.end) {
}

LcpInterval & LcpInterval::operator=(const LcpInterval & other) {
	if (&other == this) {
		return *this;
	}
	start = other.start;
	end = other.end;
	return *this;
}

bool LcpInterval::isEmpty() const {
	return (start == -1 && end == -1);
}

void LcpInterval::setEmpty() {
	start = -1;
	end = -1;
}

LcpInterval::~LcpInterval() {
}

bool LcpInterval::isSingle() const {
	return (start != -1 && start == end);
}
