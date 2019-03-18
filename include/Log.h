/*
 * OutputHandler.h
 *
 *  Created on: Oct 19, 2014
 *      Author: matan
 */

#ifndef LOG_H_
#define LOG_H_

#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <fstream>

class Log {
private:
	std::string logHeader_;
	std::ostream * outStream_;
	bool nextIsBegin_;
	std::ofstream * fileStream_;

	using endl_type = std::ostream&(std::ostream&);
	void release() {
		if (fileStream_ != NULL) {
			try {
				fileStream_->close();
			} catch (...) {
			}
			delete fileStream_;
			fileStream_ = NULL;
		}
	}

	Log(const std::string& logHeader = std::string(""),
			std::ostream& outStream = std::cout) :
			logHeader_(logHeader), outStream_(&outStream), nextIsBegin_(true), fileStream_(
			NULL) {
	}

public:
	static Log & getInstance() {
		static Log singleton;

		return singleton;
	}

	virtual ~Log() {
		release();
	}

	// overload for endline
	Log& operator<<(endl_type endl) {
		nextIsBegin_ = true;

		(*outStream_) << endl;

		return *this;
	}

	//Overload for anything else:
	template<typename T>
	Log& operator<<(const T& data) {
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		auto now_tm = std::localtime(&now_time_t);

		if (nextIsBegin_)
			(*outStream_) << logHeader_ << "(" << now_tm->tm_hour << ":"
					<< now_tm->tm_min << ":" << now_tm->tm_sec << "): " << data;
		else
			(*outStream_) << data;

		nextIsBegin_ = false;

		return *this;
	}

	bool initiate(const std::string& logHeader, const char * fileName) {
		bool result;

		try {
			this->fileStream_ = new std::ofstream(fileName);
			this->outStream_ = this->fileStream_;
			this->logHeader_ = logHeader;
			result = true;
		} catch (...) {
			this->fileStream_ = NULL;
			result = false;
		}

		return result;
	}

};

#endif /* LOG_H_ */
