#ifndef INPUT_H_
#define INPUT_H_

#include <string>
#include <vector>
#include <map>
#include <boost/regex.hpp>

struct Gap {
	unsigned int gapNo;
	unsigned int index;
	// Up to 255 gap
	char maxLength;
};

class Input {
public:
	enum GapMode {
		INC_START, INC_END, INC_BOTH, INC_NONE
	};
	enum CacheMode {
		NONE, SAVE, LOAD
	};
	enum ProgramInput {
		SEARCH, BUILDER
	};

private:
	std::string errorStr_;
	std::string querySeq_;
	std::string queryStructure_;
	std::vector<Gap> queryGap_;
	std::map<unsigned int, Gap*> gapMaps_;
	std::vector<std::string> targetSequences_;
	std::vector<std::string> targetNames_;
	int * sequencePreCount_;
	std::map<unsigned int, unsigned int> bracketFind_; // <close index, open index>
	std::map<char, std::string> compMatrix_;
	std::map<std::string, char> baseToFasta_;
	std::map<char, std::string> fastaToBase_;
	int maxGap_;
	int threadNo_;
	// Max result per target
	int maxResults_;
	std::string dsCachePrefix_;
	CacheMode cacheMode_;
	ProgramInput programInput_;

	bool ValidateQuery(std::string &, std::string &);
	bool ValidateTargetFile(std::string &);
	void ReadTargetSequences(std::ifstream *);
	bool setupComplementaryMatric(const char *);
	void insertPair(std::string &, char);

public:
	static const boost::regex kFasta;
	static const boost::regex kRNA;
	static const boost::regex kStracture;
	static const boost::regex kGap;

	static const std::string kLegalSeq;
	static const std::string kLegalStracture;

	Input(const ProgramInput & programInput);
	virtual ~Input();
	bool Initiate(int, const char **);
	const char * ErrString();
	static const std::string Usage();
	int getTargetLength();
	const std::string & getTargetName(int);
	const std::string & getTargetSequence(int);
	const std::string & getQuerySeq();
	const std::string & getQueryStracture();
	unsigned int getTotalLength();
	unsigned int getComplamentryIndex(unsigned int, bool);
	const Gap * getGapByIndex(unsigned int);
	void getGapsInIndexes(std::vector<Gap *> &, unsigned int, unsigned int,
			Input::GapMode);
	int getGapNo();
	char getMatrixChar(char, char);
	int getMaxGap();
	int getThreadNo();
	int getMaxResults();
	int getFastaSize(char);
	Gap * getGapByGapNo(int gapNo);
	const std::string& getCachePrefix();
	CacheMode getCacheMode();
};

#endif /* INPUT_H_ */
