/*
 * AffixSearch.cpp
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#include <BiDirSearch.h>
#include <utility>
#include <stack>
#include <algorithm>
#include <limits>
#include <HairpinSearch.h>
#include <SuffixSearch.h>
#include <Log.h>

BiDirSearch::BiDirSearch(Input & input, AffixArray & affixArray, int targetId) :
		input_(input), affixArray_(affixArray), hairpins_(), intermediaryLengths_(), intermediaryGaps_(), results_(), keepGoing_(
				true), targetId_(targetId) {
}

BiDirSearch::~BiDirSearch() {
}

void BiDirSearch::testBreakQuery() {
	breakSubQueries();
	std::cout << "Sizes: " << intermediaryLengths_.size() << ","
			<< hairpins_.size() << std::endl;
	std::cout << "Lengths: ";
	for (unsigned int i = 0; i < intermediaryLengths_.size(); ++i) {
		std::pair<int, int> lenPair = intermediaryLengths_[i];
		std::cout << "<" << lenPair.first << "," << lenPair.second << ">";
		if (hairpins_.size() > i) {
			std::cout << " - " << hairpins_[i].first->getQueryStructure(false)
					<< "|" << hairpins_[i].first->getQueryStructure(true)
					<< " <" << hairpins_[i].first->getReverseGapNo() << ","
					<< hairpins_[i].first->getForwardGapNo() << ">";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

}

void BiDirSearch::breakSubQueries() {
	std::vector<Gap *> gaps;
	const std::string & structure = input_.getQueryStracture();
	intermediaryGaps_.clear();
	intermediaryLengths_.clear();
	hairpins_.clear();
	std::stack<int> bracketIndex;
	int closeCount = 0;
	int lastClose = 0;
	int lastStart = 0;
	unsigned int i = 0;
	for (; i < structure.length(); ++i) {
		if (structure[i] == '(' || i == structure.length() - 1) {
			if (structure[i] == ')') {
				closeCount++;
			}
			int start = -1;
			while (closeCount > 0 && !bracketIndex.empty()) {
				closeCount--;
				start = bracketIndex.top();
				bracketIndex.pop();
			}
			if (start != -1) {
				while (!bracketIndex.empty()) {
					bracketIndex.pop();
				}
				lastClose = input_.getComplamentryIndex(start, false) + 1;
				// setup lengths of non-hairpins
				gaps.clear();
				input_.getGapsInIndexes(gaps, lastStart, start,
						Input::INC_BOTH);
				int sumGaps = 0;
				for (std::vector<Gap *>::iterator gapIt = gaps.begin();
						gapIt != gaps.end(); ++gapIt) {
					sumGaps += (*gapIt)->maxLength;
				}
				intermediaryGaps_.push_back(gaps.size());
				intermediaryLengths_.push_back(
						std::make_pair(start - lastStart,
								sumGaps + start - lastStart));
				lastStart = lastClose;
				// setup hairpins
				gaps.clear();
				input_.getGapsInIndexes(gaps, start, lastClose,
						Input::INC_NONE);
				std::string hairpinSeq = input_.getQuerySeq().substr(start,
						lastClose - start);
				std::string hairpinStruct = input_.getQueryStracture().substr(
						start, lastClose - start);
				SubQuery hp(start, gaps, hairpinSeq, hairpinStruct);
				HairpinQuery * query = new HairpinQuery(hp);
				hairpins_.push_back(
						std::make_pair(query,
								scoreQuery(hairpinSeq, hairpinStruct)));
			}
			if (structure[i] == '(')
				bracketIndex.push(i);
		} else if (structure[i] == ')') {
			closeCount++;
		}
	}
	// setup lengths last section
	gaps.clear();
	int sumGaps = 0;
	input_.getGapsInIndexes(gaps, lastClose, i, Input::INC_BOTH);
	for (std::vector<Gap *>::iterator gapIt = gaps.begin(); gapIt != gaps.end();
			++gapIt) {
		sumGaps += (*gapIt)->maxLength;
	}
	intermediaryGaps_.push_back(gaps.size());
	intermediaryLengths_.push_back(
			std::make_pair(i - lastClose, sumGaps + i - lastClose));
}

long BiDirSearch::scoreQuery(const std::string & querySeq,
		const std::string & queryStruc) {
	long result = 0;
	std::stack<int> pairStack;
	for (unsigned int i = 0; i < querySeq.size(); ++i) {
		if (queryStruc[i] == '.')
			result *= input_.getFastaSize(querySeq[i]);
		else if (queryStruc[i] == '(') {
			pairStack.push(i);
		} else {
			// approximating most letters have one pair
			int pair = pairStack.top();
			pairStack.pop();
			result *= std::min(input_.getFastaSize(querySeq[i]),
					input_.getFastaSize(querySeq[pair]));
		}
	}
	return result;
}

double BiDirSearch::search() {
	std::clock_t begin = clock();
	Log & log = Log::getInstance();
	results_.clear();
	std::vector<PossibleResult> possibleResults;
	bool firstResults = true;
	// break queries and generate order
	breakSubQueries();
	int lowestBranching = INT_MAX;
	int indexRight = 0;
	int indexLeft;
	for (unsigned int i = 0; i < hairpins_.size(); ++i) {
		if (lowestBranching > hairpins_[i].second) {
			indexRight = i;
			lowestBranching = hairpins_[i].second;
		}
	}
	indexLeft = indexRight - 1;
	// search hairpins
	int currentIndex;
	int intermediaryIndex;
	for (unsigned int i = 0; i < hairpins_.size(); ++i) {
		// select which one to calculate (smallest amount of branching)
		HairpinQuery * current;
		if (indexLeft < 0) {
			currentIndex = indexRight;
			intermediaryIndex = indexRight;
			current = hairpins_[indexRight++].first;
		} else if (indexRight >= (int) hairpins_.size()) {
			currentIndex = indexLeft;
			intermediaryIndex = indexLeft + 1;
			current = hairpins_[indexLeft--].first;
		} else if (hairpins_[indexLeft].second > hairpins_[indexRight].second) {
			currentIndex = indexRight;
			intermediaryIndex = indexRight;
			current = hairpins_[indexRight++].first;
		} else {
			currentIndex = indexLeft;
			intermediaryIndex = indexLeft + 1;
			current = hairpins_[indexLeft--].first;
		}
		// Run search
		HairpinSearch oneSearch(input_, affixArray_, *current);
		oneSearch.search();

#ifdef DEBUG
		std::cout << "______________________________________" << std::endl;
		oneSearch.printResults();
		std::cout << "______________________________________" << std::endl;
#endif /* DEBUG */

		const std::vector<BiDirTask *>& results = oneSearch.getResults();
		// calculate possible results for comparison
		std::vector<HairpinResult> newHairPinResults;
		generateIndexResults(newHairPinResults, results, currentIndex);
		log << "Hairpin " << currentIndex << " produced "
				<< newHairPinResults.size() << " matches";
		int currentResults;
		if (!firstResults) {
			// only take results that match by distance
			currentResults = mergeResults(possibleResults, newHairPinResults,
					currentIndex - intermediaryIndex,
					intermediaryLengths_[intermediaryIndex]);
			log << " merged into " << currentResults << " matches";
		} else {
			// add all results
			currentResults = initInitialResults(possibleResults,
					newHairPinResults);
			firstResults = false;
		}
		log << "." << std::endl;
		if (currentResults == 0) {
			// no results
			return double(clock() - begin) / CLOCKS_PER_SEC;
		}
	}
	// Now we start matching non results with general information, start by creating gap array for all gaps no in hairpins
	for (std::vector<PossibleResult>::iterator possibleResultIt =
			possibleResults.begin(); possibleResultIt != possibleResults.end();
			++possibleResultIt) {
		generateResult(*possibleResultIt);
		if (!keepGoing_)
			break;
	}

	return double(clock() - begin) / CLOCKS_PER_SEC;
}

/**
 * checks if index is in hairpin
 * @return -1 if not, index of the hairpin otherwise
 */
int BiDirSearch::isIndexInHairpin(int index) {
	int result = -1;
	for (unsigned int j = 0; j < hairpins_.size(); ++j) {
		int hairpinQueryStart = hairpins_[j].first->getIndexInQuery();
		if (index >= hairpinQueryStart
				&& (unsigned) index
						< hairpinQueryStart
								+ hairpins_[j].first->getQuerySize()) {
			result = j;
			break;
		}
	}
	return result;
}

/**
 * Helper function to sort out results by start index and gaps used
 */
bool BiDirSearch::compareStartResults(HairpinResult i, HairpinResult j) {
	bool result = i.startIndex < j.startIndex;
	if (i.startIndex == j.startIndex) {
		int gapIndex = 0;
		while (i.gapsInHairpin[gapIndex] == j.gapsInHairpin[gapIndex]) {
			++gapIndex;
		}
		result = i.gapsInHairpin[gapIndex] < j.gapsInHairpin[gapIndex];
	}
	return result;
}

/**
 * Helper function to sort out results by end index and gaps used
 */
bool BiDirSearch::compareEndResults(HairpinResult i, HairpinResult j) {
	return i.endIndex < j.endIndex;
}

/**
 * for each group of indexes, generate single result
 *
 * @param valuedResults - vector of results to add to, start index and gap selection
 * @param result - an original result (block of some indexes)
 */
void BiDirSearch::generateIndexResults(
		std::vector<HairpinResult>& valuedResults,
		const std::vector<BiDirTask *>& result, int hairpinId) {
	for (std::vector<BiDirTask *>::const_iterator currentResultIt =
			result.begin(); currentResultIt != result.end();
			++currentResultIt) {
		BiDirTask *currentResult = *currentResultIt;
		for (int intervalIndex = currentResult->interval_.start;
				intervalIndex <= currentResult->interval_.end;
				++intervalIndex) {
			int targetIndex = affixArray_.getIntervalsIndex(intervalIndex,
					currentResult->getIsForward());
			targetIndex += currentResult->getRevUnChecked();
			if (!currentResult->getIsForward()) {
				targetIndex = affixArray_.getTargetLength() - targetIndex;
				targetIndex -= (currentResult->getTargetIndex()
						- currentResult->getRevUnChecked());
			}
			// is the position possible?
			if (targetIndex < intermediaryLengths_.front().first
					|| (unsigned int) (targetIndex
							+ currentResult->getTotalGap(true)
							+ currentResult->getTotalGap(false))
							> affixArray_.getTargetLength()
									- intermediaryLengths_.back().first)
				continue;
			// Gaps are organized as
			valuedResults.push_back(HairpinResult());
			valuedResults.back().startIndex = targetIndex;
			unsigned int totalGap = 0;
			for (int i = currentResult->getGapNo(false) - 1; i >= 0; --i) {
				valuedResults.back().gapsInHairpin.push_back(
						currentResult->getGaps(false)[i]);
				totalGap += currentResult->getGaps(false)[i];
			}
			for (int i = 0; i < currentResult->getGapNo(true); ++i) {
				valuedResults.back().gapsInHairpin.push_back(
						currentResult->getGaps(true)[i]);
				totalGap += currentResult->getGaps(true)[i];
			}
			valuedResults.back().endIndex = targetIndex + totalGap
					+ hairpins_[hairpinId].first->getQuerySize();
		}
	}
}

int BiDirSearch::initInitialResults(
		std::vector<PossibleResult>& possibleResults,
		std::vector<HairpinResult>& hairPinResults) {
	for (std::vector<HairpinResult>::iterator it = hairPinResults.begin();
			it != hairPinResults.end(); ++it) {
		possibleResults.push_back(PossibleResult());
		possibleResults.back().startIndex = it->startIndex;
		possibleResults.back().gapsInHairpins.push_back(it->gapsInHairpin);
		possibleResults.back().endIndex = it->endIndex;
	}
	return possibleResults.size();
}

/**
 * @param  addedSide = -1 => hairPinResults ______ possibleResult | addedSide = 0 => possibleResult ______ hairPinResults
 */
int BiDirSearch::mergeResults(std::vector<PossibleResult>& possibleResults,
		std::vector<HairpinResult>& hairPinResults, int addedSide,
		std::pair<int, int> intermediaryLength) {
	if (addedSide) {
		std::sort(hairPinResults.begin(), hairPinResults.end(),
				compareEndResults);
	} else {
		std::sort(hairPinResults.begin(), hairPinResults.end(),
				compareStartResults);
	}

#ifdef DEBUG
	for (unsigned int i = 0; i < hairPinResults.size(); ++i) {
		std::cout << i << ") start:" << hairPinResults[i].startIndex << "[";
		for (unsigned int j = 0; j < hairPinResults[i].gapsInHairpin.size();
				++j) {
			std::cout << (int) hairPinResults[i].gapsInHairpin[j] << ",";
		}
		std::cout << "] end:" << hairPinResults[i].endIndex << std::endl;
	}
#endif /* DEBUG */
	std::vector<PossibleResult> newPossibles;
	for (std::vector<PossibleResult>::iterator possibleResultIt =
			possibleResults.begin(); possibleResultIt != possibleResults.end();
			) {
		// Find hairpin's that are in distance range
		for (std::vector<HairpinResult>::iterator fitFind = getLowerBound(
				hairPinResults.begin(), hairPinResults.end(),
				intermediaryLength.first, addedSide, *possibleResultIt);
				fitFind != hairPinResults.end()
						&& isAboveThreashold(fitFind, intermediaryLength.second,
								addedSide, *possibleResultIt); ++fitFind) {
#ifdef DEBUG
			std::cout << "Match found: dir " << addedSide << " ["
			<< fitFind->startIndex << "," << fitFind->endIndex << "] ["
			<< possibleResultIt->startIndex << ","
			<< possibleResultIt->endIndex << "]" << std::endl;
#endif /* DEBUG*/
			// this fits, now we can connect it to the original results
			std::vector<PossibleResult>::iterator newPossibleResult;
			newPossibles.push_back(PossibleResult());

			newPossibles.back().startIndex = possibleResultIt->startIndex;
			newPossibles.back().endIndex = possibleResultIt->endIndex;
			for (auto gaps = possibleResultIt->gapsInHairpins.begin();
					gaps != possibleResultIt->gapsInHairpins.end(); gaps++) {
				newPossibles.back().gapsInHairpins.push_back(*gaps);
			}
			for (auto intermid = possibleResultIt->intermediaryGaps.begin();
					intermid != possibleResultIt->intermediaryGaps.end();
					intermid++) {
				newPossibles.back().intermediaryGaps.push_back(*intermid);
			}
			if (addedSide) {
				newPossibles.back().startIndex = fitFind->startIndex;
				newPossibles.back().gapsInHairpins.insert(
						newPossibles.back().gapsInHairpins.begin(),
						fitFind->gapsInHairpin);
				newPossibles.back().intermediaryGaps.insert(
						newPossibles.back().intermediaryGaps.begin(),
						possibleResultIt->startIndex - fitFind->endIndex
								- intermediaryLength.first);
			} else {
				newPossibles.back().endIndex = fitFind->endIndex;
				newPossibles.back().gapsInHairpins.push_back(
						fitFind->gapsInHairpin);
				newPossibles.back().intermediaryGaps.push_back(
						fitFind->startIndex - possibleResultIt->endIndex
								- intermediaryLength.first);
			}
		}
		possibleResults.erase(possibleResultIt);
	}
	possibleResults.clear();
	possibleResults = newPossibles;
	return possibleResults.size();
}

bool BiDirSearch::compareSingleton(PossibleResult & possibleResult,
		std::pair<int, std::vector<char>>& result, unsigned int gapsInStart) {
#ifdef DEBUG
	std::cout << "CompareSingletin - start: " << result.first << " end: "
	<< possibleResult.endIndex << " [" << gapsInStart;
	for (unsigned int i = 0; i < possibleResult.gapsInHairpins.size(); ++i) {
		std::cout << " <";
		for (unsigned int j = 0; j < possibleResult.gapsInHairpins[i].size();
				++j) {
			std::cout << (int) possibleResult.gapsInHairpins[i][j] << ",";
		}
		std::cout << " >";
		if (i < possibleResult.intermediaryGaps.size()) {
			std::cout << (int) possibleResult.intermediaryGaps[i];
		}
	}
	std::cout << std::endl;
#endif /* DEBUG */
	bool found = false;
	std::vector<std::pair<unsigned int, unsigned int>> gapUsageVector;
	std::stack<unsigned int> queryIndex;
	std::stack<unsigned int> targetIndex;
	std::stack<std::vector<char>> gapsBefore;
	unsigned int currentQueryIndex = 0;
	unsigned int currentTargetIndex = 0;
	std::vector<char> currentGaps;
	do {
		CompareState compareState = CompareState::KEEP_COMPARING;
		while (compareState == CompareState::KEEP_COMPARING) {
			// end of query, match found
			if (currentQueryIndex == input_.getQuerySeq().length()) {
				compareState = CompareState::MATCH;
				break;
			}
			// if in gap initiate it or push past if already initiated
			const Gap * gap;
			if ((gap = input_.getGapByIndex(currentQueryIndex)) != NULL) {
				if (gapUsageVector.empty()
						|| gapUsageVector.back().first != gap->gapNo) {
					// TODO: get minimum possible, can be calculated... now its 0
					queryIndex.push(currentQueryIndex);
					targetIndex.push(currentTargetIndex);
					gapsBefore.push(currentGaps);
					gapUsageVector.push_back(std::make_pair(gap->gapNo, 0));
				}
				currentGaps.push_back(gapUsageVector.back().second);
				int push = gapUsageVector.back().second;
				currentTargetIndex += push;
			}
			// case in hairpin skip to end of hairpin
			int pushed;
			if ((pushed = isIndexInHairpin(currentQueryIndex)) != -1) {
				unsigned int totalGaps = 0;
				for (unsigned int i = gapUsageVector.size()
						- intermediaryGaps_[pushed]; i < gapUsageVector.size();
						++i) {
					totalGaps += gapUsageVector[i].second;
				}
				// Did we use the right amount of gaps?
				if ((pushed == 0 && totalGaps != gapsInStart)
						|| (pushed > 0
								&& (unsigned) possibleResult.intermediaryGaps[pushed
										- 1] != totalGaps)) {
#ifdef DEBUG
					std::cout << "MISMATCH " << pushed << "," << totalGaps
					<< "," << gapsInStart << ","
					<< ((possibleResult.intermediaryGaps.size()
									> (unsigned) (pushed - 1)) ?
							possibleResult.intermediaryGaps[pushed - 1] : 0)
					<< std::endl;
#endif /* DEBUG */
					compareState = CompareState::MISMATCH;
					break;
				}
				// add gaps from hairpin to result
				totalGaps = 0;
				for (unsigned int i = 0;
						i < possibleResult.gapsInHairpins[pushed].size(); ++i) {
					totalGaps += possibleResult.gapsInHairpins[pushed][i];
					currentGaps.push_back(
							possibleResult.gapsInHairpins[pushed][i]);
				}
				// push target index and query index
				pushed = hairpins_[pushed].first->getQuerySize();
				currentTargetIndex += pushed + totalGaps;
				currentQueryIndex += pushed;
			}
			// case normal comparison
			else {
				char postMatchChar;
				unsigned int targetLocation = currentTargetIndex + result.first;
				if (targetLocation
						>= input_.getTargetSequence(targetId_).length()
						|| (postMatchChar = getQueryChar(currentGaps,
								currentQueryIndex, currentTargetIndex,
								result.first)) == 0
						|| !SuffixSearch::isFASTAEqual(
								input_.getTargetSequence(targetId_)[targetLocation],
								postMatchChar)) {
#ifdef DEBUG
					std::cout << "MISMATCH "
					<< input_.getTargetSequence(targetId_)[targetLocation]
					<< ","
					<< getQueryChar(currentGaps, currentQueryIndex,
							currentTargetIndex, result.first) << ","
					<< currentQueryIndex << std::endl;
#endif /* DEBUG*/
					compareState = CompareState::MISMATCH;
				}
				currentTargetIndex++;
				currentQueryIndex++;
			}
		}

		if (compareState == CompareState::MATCH) {
			std::pair<int, std::vector<char>> newResult;
			newResult.first = result.first;
			newResult.second = currentGaps;
			results_.push_back(newResult);
			found = true;
			if (results_.size() > (unsigned) input_.getMaxResults()) {
				keepGoing_ = false;
			}
		}

		while (gapUsageVector.empty() == false
				&& gapUsageVector.back().second
						>= (unsigned) input_.getGapByGapNo(
								gapUsageVector.back().first)->maxLength) {
			// Checked all values for current gap, go back one gap
			gapUsageVector.pop_back();
			queryIndex.pop();
			targetIndex.pop();
			gapsBefore.pop();
		}
		if (gapUsageVector.empty() == false) {
			// We have more lengths for the current gap to test
			currentQueryIndex = queryIndex.top();
			currentTargetIndex = targetIndex.top();
			currentGaps.clear();
			currentGaps = gapsBefore.top();
			gapUsageVector.back().second++;
		}
	} while (!gapUsageVector.empty() && keepGoing_);
	return found;
}

/**
 * given a possible result that fits all hairpin we want to test the entire sequence
 * 1) for any possible gap combination that is equal to intermediaryGaps
 * 2) for any possible gap in start or end of the sequence
 * 3) compare generated sequence (match sequence)
 * Whichever sequence passes this process is a true match!
 *
 * @param untestedGaps a vector with gaps for each non hairpin section set to -1
 */
void BiDirSearch::generateResult(PossibleResult & possibleResult) {
	for (int i = intermediaryLengths_[0].first;
			i <= intermediaryLengths_[0].second; ++i) {
		int startIndex = possibleResult.startIndex - i;
		unsigned int gapsInStart = i - intermediaryLengths_[0].first;
		std::pair<int, std::vector<char>> result;
		result.first = startIndex;

		compareSingleton(possibleResult, result, gapsInStart);
	}
}

/**
 * Retrieve the query char (if complementary check with matrix)
 * assume we are not in a gap
 */
char BiDirSearch::getQueryChar(const std::vector<char>& gaps,
		unsigned int queryIndex, unsigned int targetIndex,
		unsigned int targetLocation) {
	char queryChar = input_.getQuerySeq()[queryIndex];
	int openBracketQueryIndex = input_.getComplamentryIndex(queryIndex, true);
	if (openBracketQueryIndex != -1) {
		// remove used gaps to get the open bracket target index
		std::vector<Gap *> gapsBefore;
		input_.getGapsInIndexes(gapsBefore, openBracketQueryIndex, queryIndex,
				Input::INC_END);
		for (unsigned int i = 0; i < gapsBefore.size(); ++i) {
			targetIndex -= gaps[gapsBefore[i]->gapNo];
		}
		queryChar = input_.getMatrixChar(
				input_.getTargetSequence(targetId_)[targetLocation + targetIndex
						- (queryIndex - openBracketQueryIndex)], queryChar);
	}
	return queryChar;
}

/**
 *  print result to log, format <index>;<gaps>;<sequence>
 */
void BiDirSearch::printResults() {
	Log & log = Log::getInstance();
	int queryLength = input_.getTotalLength();

	log << "Results: " << input_.getTargetName(targetId_) << "\n";
	for (unsigned int resultIndex = 0; resultIndex < results_.size();
			++resultIndex) {
		std::pair<int, std::vector<char>> & currentResult =
				results_[resultIndex];
		log << (currentResult.first + 1) << ";"; // start counting from 1
		// pushing gap information (for secondary structure alignment)
		int totalGaps = 0;
		for (int l = 0; l < input_.getGapNo(); ++l) {
			totalGaps += currentResult.second[l];
			log << (int) (currentResult.second[l]);
			if (l + 1 < input_.getGapNo())
				log << ",";
		}
		log << ";";
		// pushing sequence
		log
				<< input_.getTargetSequence(targetId_).substr(
						currentResult.first, totalGaps + queryLength) << "\n";
	}
	log << std::endl;
}
