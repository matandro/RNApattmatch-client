/*
 * BiDirSearch.h
 *
 *  Created on: Jan 26, 2015
 *      Author: matan
 */

#ifndef BIDIRSEARCH_H_
#define BIDIRSEARCH_H_

#include <AffixArray.h>
#include <HairpinQuery.h>
#include <Input.h>
#include <SubQuery.h>
#include <BiDirTask.h>
#include <vector>

class BiDirSearch {
private:
	struct PossibleResult {
		int startIndex;
		int endIndex;
		std::vector<std::vector<char>> gapsInHairpins;
		std::vector<int> intermediaryGaps;
	};

	struct HairpinResult {
		int startIndex;
		int endIndex;
		std::vector<char> gapsInHairpin;
	};

	enum CompareState {
		KEEP_COMPARING, MISMATCH, MATCH
	};

	Input & input_;
	AffixArray & affixArray_;
	std::vector<std::pair<HairpinQuery *, int>> hairpins_;
	std::vector<std::pair<int, int>> intermediaryLengths_;
	std::vector<unsigned int> intermediaryGaps_;
	std::vector<std::pair<int, std::vector<char>>>results_;
	bool keepGoing_;
	int targetId_;

	std::vector<HairpinResult>::iterator getLowerBound(std::vector<HairpinResult>::iterator first, std::vector<HairpinResult>::iterator last, int minDistance, int addedSide, PossibleResult & possibleResult) {
		std::vector<HairpinResult>::iterator it;
		std::iterator_traits<std::vector<HairpinResult>::iterator>::difference_type count, step;
		count = distance(first,last);
		while (count>0)
		{
			it = first;
			step=count/2;
			std::advance (it,step);
			int distance;
			if (addedSide)
			distance = possibleResult.startIndex - it->endIndex;
			else
			distance = it->startIndex - possibleResult.endIndex;
			if (distance < minDistance) {
				first=++it;
				count-=step+1;
			}
			else count=step;
		}
		return first;
	}

	bool isAboveThreashold (std::vector<HairpinResult>::iterator it, int maxDistance, int addedSide, PossibleResult & possibleResult) {
		int distance;
		if (addedSide)
			distance = possibleResult.startIndex - it->endIndex;
		else
			distance = it->startIndex - possibleResult.endIndex;
		return distance <= maxDistance;
	}

	long scoreQuery(const std::string &, const std::string & queryStruc);
	void generateIndexResults(std::vector<HairpinResult>&valuedResults,
			const std::vector<BiDirTask *>& result, int hairpinId);
	int initInitialResults(std::vector<PossibleResult>& possibleResults,
			std::vector<HairpinResult>& hairPinResults);
	int mergeResults(std::vector<PossibleResult>& possibleResults,
			std::vector<HairpinResult>& hairPinResults, int addedSide,
			std::pair<int, int> intermediaryLength);
	void generateResult(PossibleResult & possibleResult);
	int isIndexInHairpin(int index);
	bool compareSingleton(PossibleResult & possibleResult, std::pair<int,std::vector<char>>& result, unsigned int gapsInStart);
	char getQueryChar(const std::vector<char>& gaps, unsigned int queryIndex,
			unsigned int targetIndex, unsigned int targetLocation);
	void breakSubQueries();
	static bool compareStartResults(HairpinResult i,
			HairpinResult j);
	static bool compareEndResults(HairpinResult i,
			HairpinResult j);
public:
	BiDirSearch(Input &, AffixArray &, int targetId);
	virtual ~BiDirSearch();

	//testing functions
	void testBreakQuery();
	double search();
	void printResults();
};

#endif /* BIDIRSEARCH_H_ */
