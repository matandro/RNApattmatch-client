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

	std::cout << "Reading inputs" << std::endl;
	success = input.Initiate(argc, argv);
	if (!success) {
		std::cerr << input.ErrString() << std::endl;
		std::cerr << Input::Usage() << std::endl;
		return -1;
	}
	Log& log = Log::getInstance();
	log << "Initialization Successful" << std::endl;

	for (unsigned int i = 0; i < input.getTargetLength(); ++i) {
		if (input.isStructured()) {
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

			if (input.getCacheMode() == Input::CacheMode::LOAD
				|| input.getCacheMode() == Input::CacheMode::SAVE) {
				log << "Saving affix link lazy calculations (if changed)" << std::endl;
				affixArray.saveAflk();
			}
		} else {
			// START SUFFIX ARRAY FOR SUFFIX SEARCH
			log << "Starting suffix array on target " << (i + 1) << std::endl;
			DC3Algorithm suffixArray(input.getTargetSequence(i).c_str(), false, input, i);

			log << "Creating enhanced suffix array took "
				<< suffixArray.RunAlgorithm() << " seconds" << std::endl;
			//suffixArray.testPrintDC3();

			SuffixSearch suffixSearch(&suffixArray, &input, i);

			log << "Performing search took " << suffixSearch.performSearch()
				<< " seconds" << std::endl;

			suffixSearch.printResults();
		}
	}

	log << "Finished" << std::endl;
	return 0;
}
