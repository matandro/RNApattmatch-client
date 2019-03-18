#include <AffixArray.h>
#include <iostream>

#include <Input.h>
#include <Log.h>
#include <DC3Algorithm.h>
#include <SuffixSearch.h>
#include <exception>
#include <HairpinQuery.h>
#include <HairpinSearch.h>
#include <BiDirSearch.h>

int main(int argc, const char ** argv) {
	bool success;
	Input input = Input(Input::ProgramInput::SEARCH);

	success = input.Initiate(argc, argv);
	if (!success) {
		std::cerr << input.ErrString() << std::endl;
		std::cerr << Input::Usage() << std::endl;
		return -1;
	}

	Log& log = Log::getInstance();
	log << "Initialization Successful" << std::endl;

	for (int i = 0; i < input.getTargetLength(); ++i) {
		// START AFFIX ARRAY WITH FULL AFFIX SEARCH
		log << "Starting affix array construction on target " << (i + 1)
				<< std::endl;
		AffixArray affixArray(input.getTargetSequence(i).c_str(), input, i);
		log << "Creating enhanced affix array took "
				<< affixArray.RunAlgorithm() << " seconds" << std::endl;
		BiDirSearch biDirSearch = BiDirSearch(input, affixArray, i);
		//biDirSearch.testBreakQuery();
		log << "Performing search took " << biDirSearch.search() << " seconds (Single CPU time)"
				<< std::endl;

		biDirSearch.printResults();
/*

		 // START SUFFIX ARRAY WITH SUFFIX SEARCH
		 log << "Starting suffix array on target " << (i + 1) << std::endl;
		 DC3Algorithm suffixArray(input.getTargetSequence(i).c_str(), false, input, i);

		 log << "Creating enhanced suffix array took "
		 << suffixArray.RunAlgorithm() << " seconds" << std::endl;
		 //suffixArray.testPrintDC3();

		 SuffixSearch suffixSearch(&suffixArray, &input, i);

		 log << "Performing search took " << suffixSearch.performSearch()
		 << " seconds" << std::endl;

		 suffixSearch.printResults();


		 // START AFFIX ARRAY WITH HAIRPIN SEARCH
		 log << "Starting affix array construction on target " << (i + 1)
		 << std::endl;
		 AffixArray affixArray(input.getTargetSequence(i).c_str(), input, i);
		 log << "Creating enhanced affix array took "
		 << affixArray.RunAlgorithm() << " seconds" << std::endl;
		 // affixArray.printArrayTest();
		 std::vector<Gap *> allGaps;
		 input.getGapsInIndexes(allGaps, 0, input.getQuerySeq().length(),
		 Input::INC_BOTH);
		 //std::cout << "init sub query" << std::endl;
		 SubQuery subQuery(0, allGaps, input.getQuerySeq(),
		 input.getQueryStracture());
		 //std::cout << "init hairpin query" << std::endl;
		 HairpinQuery hairpinQuery(subQuery);
		 //std::cout << "init hairpin search" << std::endl;
		 HairpinSearch hairpinSearch(input, affixArray, hairpinQuery);
		 log << "Performing search took " << hairpinSearch.search() << " seconds (Single CPU time)"
		 << std::endl;
		 //std::cout << "print results" << std::endl;
		 hairpinSearch.printResults();*/


	}

	log << "Finished" << std::endl;
	return 0;
}
