#include <Input.h>
#include <Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stack>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

const boost::regex Input::kGap = boost::regex("\\[[0-9]+\\]");
const boost::regex Input::kFasta = boost::regex("[ACGUTRYKMSWBDHVN]+");
const boost::regex Input::kRNA = boost::regex("[AGCURYKMSWBDHVNX-]+");
const boost::regex Input::kStracture = boost::regex("[)(.]+");

const std::string Input::kLegalSeq = "ACGUTRYKMSWBDHVN0123456789[]";
const std::string Input::kLegalStracture = ".()0123456789[]";

int Input::getFastaSize(char fastaLetter) {
	unsigned int result = 0;
	try {
		const std::string & range = fastaToBase_.at(fastaLetter);
		result = strlen(range.c_str());
	} catch (...) {
		std::cerr << "ERROR: unknown symbol in query: " << fastaLetter
				<< std::endl;
		exit(-1);
	}
	return result;
}

Input::Input(const ProgramInput & programInput) :
		errorStr_("Error: "), querySeq_(""), queryStructure_(""), queryGap_(), gapMaps_(), targetSequences_(), targetNames_(), sequencePreCount_(), bracketFind_(), compMatrix_(), baseToFasta_(), fastaToBase_(), maxGap_(
				0), threadNo_(4), maxResults_(1000), dsCachePrefix_(""), cacheMode_(NONE), programInput_(programInput) {
	// mapping fasta name to rna alternatives
	fastaToBase_.insert(std::pair<char, std::string>('A', "A"));
	fastaToBase_.insert(std::pair<char, std::string>('G', "G"));
	fastaToBase_.insert(std::pair<char, std::string>('C', "C"));
	fastaToBase_.insert(std::pair<char, std::string>('U', "U"));
	fastaToBase_.insert(std::pair<char, std::string>('N', "ACGU"));
	fastaToBase_.insert(std::pair<char, std::string>('R', "AG"));
	fastaToBase_.insert(std::pair<char, std::string>('Y', "CU"));
	fastaToBase_.insert(std::pair<char, std::string>('K', "GU"));
	fastaToBase_.insert(std::pair<char, std::string>('M', "AC"));
	fastaToBase_.insert(std::pair<char, std::string>('S', "CG"));
	fastaToBase_.insert(std::pair<char, std::string>('W', "AU"));
	fastaToBase_.insert(std::pair<char, std::string>('B', "CGU"));
	fastaToBase_.insert(std::pair<char, std::string>('D', "AGU"));
	fastaToBase_.insert(std::pair<char, std::string>('H', "ACU"));
	fastaToBase_.insert(std::pair<char, std::string>('V', "ACG"));
	// mapping from rna alternatives to fasta name
	baseToFasta_.insert(std::pair<std::string, char>("A", 'A'));
	baseToFasta_.insert(std::pair<std::string, char>("G", 'G'));
	baseToFasta_.insert(std::pair<std::string, char>("C", 'C'));
	baseToFasta_.insert(std::pair<std::string, char>("U", 'U'));
	baseToFasta_.insert(std::pair<std::string, char>("ACGU", 'N'));
	baseToFasta_.insert(std::pair<std::string, char>("AG", 'R'));
	baseToFasta_.insert(std::pair<std::string, char>("CU", 'Y'));
	baseToFasta_.insert(std::pair<std::string, char>("GU", 'K'));
	baseToFasta_.insert(std::pair<std::string, char>("AC", 'M'));
	baseToFasta_.insert(std::pair<std::string, char>("CG", 'S'));
	baseToFasta_.insert(std::pair<std::string, char>("AU", 'W'));
	baseToFasta_.insert(std::pair<std::string, char>("CGU", 'B'));
	baseToFasta_.insert(std::pair<std::string, char>("AGU", 'D'));
	baseToFasta_.insert(std::pair<std::string, char>("ACU", 'H'));
	baseToFasta_.insert(std::pair<std::string, char>("ACG", 'V'));
	// default complementary matrix (with dangling pairs)
	compMatrix_.insert(std::pair<char, std::string>('A', "U"));
	compMatrix_.insert(std::pair<char, std::string>('C', "G"));
	compMatrix_.insert(std::pair<char, std::string>('G', "CU"));
	compMatrix_.insert(std::pair<char, std::string>('U', "AG"));
}

#define MODE_NON 0
#define MODE_NAME 1
#define MODE_SEQUENCE 2

void Input::ReadTargetSequences(std::ifstream * targetFile) {
	int mode = MODE_NON;
	std::string currentRead;
	char c = targetFile->get();
	while (targetFile->good()) {
		if (c == '>') {
			if (mode == MODE_SEQUENCE) {
				boost::to_upper(currentRead);
				targetSequences_.push_back(currentRead);
				currentRead = "";
			}
			mode = MODE_NAME;
		} else if (c == '\n') {
			if (mode == MODE_NAME) {
				targetNames_.push_back(currentRead);
				mode = MODE_SEQUENCE;
				currentRead = "";
			}
		} else if (mode == MODE_NAME || (c >= 'a' && c <= 'z')
				|| (c >= 'A' && c <= 'Z') || c == '-') {
			currentRead += c;
		}
		c = targetFile->get();
	}
	if (currentRead != "" && mode == MODE_SEQUENCE) {
		boost::to_upper(currentRead);
		targetSequences_.push_back(currentRead);
	}
}

bool Input::ValidateQuery(std::string & argQuerySeq,
		std::string & argQueryStracture) {
	boost::sregex_iterator noIterator;
	unsigned int findNum = 0, gapCounter = 0;
	// retrieve all gaps, check that they match and turn into numbers
	std::vector<std::string> queryGapsStr = std::vector<std::string>();
	boost::sregex_iterator seqGapIterator(argQuerySeq.begin(),
			argQuerySeq.end(), kGap);
	for (; seqGapIterator != noIterator; ++seqGapIterator) {
		queryGapsStr.push_back(seqGapIterator->str());
	}
	boost::sregex_iterator stractureGapIterator(argQueryStracture.begin(),
			argQueryStracture.end(), kGap);
	for (; stractureGapIterator != noIterator; ++stractureGapIterator) {
		if (queryGapsStr[findNum].compare(stractureGapIterator->str())) {
			errorStr_ += "Gaps must match in query sequence and structure.";
			return false;
		}
		std::string gapNumStr = queryGapsStr[findNum].substr(1,
				queryGapsStr[findNum].length() - 2);
		try {
			Gap currentGap = Gap();
			currentGap.maxLength = atoi(gapNumStr.c_str());
			maxGap_ += currentGap.maxLength;
			currentGap.gapNo = gapCounter++;
			queryGap_.push_back(currentGap);
		} catch (std::exception const & e) {
			errorStr_ += "Gaps must contain numbers only: ";
			errorStr_ += queryGapsStr[findNum];
			return false;
		}
		findNum++;
	}
	// Retrieve non gaps and compare sizes
	boost::sregex_iterator seqIterator(argQuerySeq.begin(), argQuerySeq.end(),
			kFasta);
	boost::sregex_iterator stractureIterator(argQueryStracture.begin(),
			argQueryStracture.end(), kStracture);
	for (unsigned int i = 0; seqIterator != noIterator; ++seqIterator, ++i) {
		querySeq_ += seqIterator->str();
		if (i < queryGap_.size()) {
			queryGap_[i].index = querySeq_.length();
			gapMaps_.insert(
					std::pair<int, Gap*>(queryGap_[i].index, &(queryGap_[i])));
		}
	}
	for (; stractureIterator != noIterator; ++stractureIterator) {
		queryStructure_ += stractureIterator->str();
	}
	if (querySeq_.length() != queryStructure_.length()) {
		errorStr_ += "Query sequence and structure length must match.";
		return false;
	}

	std::stack<int> bracket = std::stack<int>();
	for (unsigned int i = 0; i < queryStructure_.length(); ++i) {
		if (queryStructure_[i] == '(') {
			isStructured_ = false;
			bracket.push(i);
		} else if (queryStructure_[i] == ')') {
			if (bracket.empty()) {
				errorStr_ += "Query structure brackets are not balanced!";
				return false;
			} else {
				bracketFind_.insert(std::pair<int, int>(i, bracket.top()));
				bracketFind_.insert(std::pair<int, int>(bracket.top(), i));
				bracket.pop();
			}
		}
	}
	if (!bracket.empty()) {
		errorStr_ += "Query structure brackets are not balanced!";
		return false;
	}
	return true;
}

bool Input::ValidateTargetFile(std::string & targetFileName_) {
	std::ifstream targetFile;
	targetFile.open(targetFileName_.c_str(), std::ifstream::in);
// Check if file exists
	if (!targetFile.is_open()) {
		errorStr_ += "Could not open file ";
		errorStr_ += targetFileName_;
		return false;
	}
// Read from file
	ReadTargetSequences(&targetFile);
	targetFile.close();

	for (unsigned int i = 0; i < targetSequences_.size(); ++i) {
		boost::to_upper(targetSequences_[i]);
		boost::replace_all(targetSequences_[i], "T", "U");
		if (!boost::regex_match(targetSequences_[i].c_str(), kRNA)) {
			std::cout << targetNames_[i].c_str() << std::endl;
			errorStr_ += "Target ";
			errorStr_ += boost::lexical_cast<std::string>(i);
			errorStr_ += " \"";
			errorStr_ += targetNames_[i].c_str();
			errorStr_ += "\"";
			errorStr_ +=
					"\nSequences supports DNA/RNA letters only (A,G,C,U or T)\n";
			return false;
		}
	}
	return true;
}

bool Input::setupComplementaryMatric(const char * matrixArgument) {
	std::string aComplementary = "";
	std::string cComplementary = "";
	std::string gComplementary = "";
	std::string uComplementary = "";

	const char * current = matrixArgument;
	while (current[0] != 0) {
		if (current[1] == 0)
			return false;

		for (int i = 0; i <= 1; ++i) {
			char base = current[i];
			char comp = current[1 - i];
			switch (base) {
			case 'A':
				insertPair(aComplementary, comp);
				break;
			case 'C':
				insertPair(cComplementary, comp);
				break;
			case 'G':
				insertPair(gComplementary, comp);
				break;
			case 'U':
				insertPair(uComplementary, comp);
				break;
			default:
				return false;
			}
		}

		current += 2;
		if (current[0] == ',') {
			current++;
		}
	}

	compMatrix_.clear();
	compMatrix_.insert(std::pair<char, std::string>('A', aComplementary));
	compMatrix_.insert(std::pair<char, std::string>('C', cComplementary));
	compMatrix_.insert(std::pair<char, std::string>('G', gComplementary));
	compMatrix_.insert(std::pair<char, std::string>('U', uComplementary));

	return true;
}

/**
 * insert complementary to the base string while keeping the string sorted
 */
void Input::insertPair(std::string & baseStr, char comp) {
	unsigned int index = 0;
	while (index < baseStr.length()) {
		if (baseStr[index] < comp)
			++index;
		else if (baseStr[index] == comp)
			return;
		else
			break;
	}
	baseStr = baseStr.substr(0, index) + comp
			+ baseStr.substr(index, baseStr.length() - index);
	return;
}

const std::string Input::Usage() {
	std::ostringstream os;
	os		<< "RNAPattMatch Usage: RNAPattMatch [options] <Query sequence> <Query structure> <Target sequence file>\n"
			<< "options include:\n"
			<< "\t-t [num] - number of concurent threads on search\n"
			<< "\t-m [BP1,BP2,...,BPn] - The legal base pairs\n"
			<< "\t-r [num] - set max results to be found in target\n"
			<< "\t-f [file path] - setup output to file\n"
			<< "\t-s [file prefix] - save data structures to file\n"
			<< "\t\t * this option will cause a full calculation of the affix array and will take a while\n"
			<< "\t-l [file prefix] - load data structure from file\n";
	return os.str();
}

bool Input::Initiate(int argc, const char ** argv) {
	bool attr = true;
	for (int i = 1; i < argc; ++i) {
		// read attributes
		if (attr && argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 't':
				++i;
				try {
					threadNo_ = atoi(argv[i]);
				} catch (...) {
					errorStr_ += "The -t [num] option only receives numbers";
					return false;
				}
				break;
			case 'm':
				++i;
				if (setupComplementaryMatric(argv[i]) == false) {
					errorStr_ += "adding complementary matrix information. ";
					errorStr_ += argv[i];
					return false;
				}
				break;
			case 'f':
				++i;
				if (Log::getInstance().initiate(std::string(">"), argv[i])
						== false) {
					errorStr_ == "Failed to initiate output file";
					errorStr_ += argv[i];
				}
				break;
			case 'r':
				++i;
				try {
					maxResults_ = atoi(argv[i]);
				} catch (...) {
					errorStr_ += "The -r [num] option only receives numbers";
					return false;
				}
				break;
			case 's':
				++i;
				dsCachePrefix_ = argv[i];
				cacheMode_ = CacheMode::SAVE;
				break;
			case 'l':
				++i;
				dsCachePrefix_ = argv[i];
				cacheMode_ = CacheMode::LOAD;
				break;
			default:
				errorStr_ += "Unknown attribute ";
				errorStr_ += argv[i][1];
				return false;
			}
			continue;
		}
		// read 3 last parameters
		if (i + 2 > argc || (programInput_ == ProgramInput::SEARCH && i + 3 > argc)) {
			errorStr_ += "Missing parameters";
			return false;
		}
		bool validated;
		if (programInput_ == ProgramInput::BUILDER) {
			validated = true;
			dsCachePrefix_ = argv[i++];
			cacheMode_ = CacheMode::SAVE;
		}
		else  {
			std::string argQuerySeq = std::string(argv[i++]);
			boost::to_upper(argQuerySeq);
			boost::replace_all(argQuerySeq, "T", "U");
			std::string argQueryStructure = std::string(argv[i++]);
			validated = ValidateQuery(argQuerySeq, argQueryStructure);
		}
		std::string targetFileName_ = std::string(argv[i++]);

		return (validated
				& ValidateTargetFile(targetFileName_));
	}
	return true;
}

Input::~Input() {
	if (sequencePreCount_ != 0) {
		delete sequencePreCount_;
		sequencePreCount_ = 0;
	}
}

const std::string& Input::getCachePrefix() {
	return dsCachePrefix_;
}
Input::CacheMode Input::getCacheMode() {
	return cacheMode_;
}

const char * Input::ErrString() {
	return errorStr_.c_str();
}

unsigned int Input::getTargetLength() {
	return targetNames_.size();
}

const std::string & Input::getTargetName(int index) {
	return targetNames_[index];
}

const std::string & Input::getTargetSequence(int index) {
	return targetSequences_[index];
}

const std::string & Input::getQuerySeq() {
	return querySeq_;
}

const std::string & Input::getQueryStracture() {
	return queryStructure_;
}

unsigned int Input::getTotalLength() {
	return querySeq_.length();
}

/**
 * This function recives the index of a closing bracket and returns the complamentry open index
 * if the index is not a closing bracket index, returns -1
 */
unsigned int Input::getComplamentryIndex(unsigned int closeIndex,
		bool backOnly) {
	unsigned int result;
	try {
		result = bracketFind_.at(closeIndex);
		if (backOnly && result > closeIndex) {
			result = -1;
		}
	} catch (...) {
		result = -1;
	}
	return result;
}

const Gap * Input::getGapByIndex(unsigned int index) {
	Gap * result;
	try {
		result = gapMaps_.at(index);
	} catch (...) {
		result = 0;
	}
	return result;
}

/**
 * returns all gaps between 2 query indexes ordered by ascending indexes
 */
void Input::getGapsInIndexes(std::vector<Gap *> & gaps, unsigned int start,
		unsigned int end, Input::GapMode mode) {
	for (unsigned int i = 0; i < queryGap_.size(); ++i) {
		if (queryGap_[i].index > start
				|| ((mode == INC_START || mode == INC_BOTH)
						&& queryGap_[i].index == start)) {
			if (queryGap_[i].index < end
					|| ((mode == INC_END || mode == INC_BOTH)
							&& queryGap_[i].index == end)) {
				gaps.push_back(&(queryGap_[i]));
			} else {
				break;
			}
		}
	}
}

int Input::getGapNo() {
	return queryGap_.size();
}

int Input::getMaxGap() {
	return maxGap_;
}

/**
 * This function receives an open bracket sequence character (only A,C,U,G)
 * and a close bracket character (any fasta letter)
 * and combines to complementary result given matrix
 *
 * @param openBracket The character on target on open bracket index
 * @param closeBracket The character on query on the close bracket index
 * @return The concatenation between the complementary matrix
 * 		   for the open bracket character and the close bracket character
 * 		   as defined by FASTA
 */
char Input::getMatrixChar(char openBracket, char closeBracket) {
	char newCloseBracket = closeBracket;
	std::string comp;
	std::string close;
	std::string resultString = "";
	try {
		comp = compMatrix_.at(openBracket);
		close = fastaToBase_.at(closeBracket);
		for (unsigned int i = 0, j = 0; i < comp.length() && j < close.length();
				) {
			if (i < comp.length() && j < close.length()
					&& comp[i] == close[j]) {
				resultString += comp[i++];
				++j;
			} else if (comp[i] > close[j] && comp[i] != 0) {
				j++;
			} else {
				i++;
			}
		}
		newCloseBracket = baseToFasta_.at(resultString);
	} catch (...) {
		newCloseBracket = 0;
	}

	return newCloseBracket;
}

int Input::getThreadNo() {
	return threadNo_;
}

int Input::getMaxResults() {
	return maxResults_;
}

Gap * Input::getGapByGapNo(int gapNo) {
	Gap * result = NULL;
	try {
		result = &queryGap_[gapNo];
	} catch (...) {
		result = NULL;
	}
	return result;
}

bool Input::isStructured() {
	return isStructured_;
}