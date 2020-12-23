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

std::string Usage() {
	std::ostringstream os;
	os		<< "This programs builds and saves a the data structures needed for bi-directional search.\n"
			<< "The information can later be loaded by the search using the -l option on the same output target data prefix.\n"
			<< "RNAdsBuilder Usage: RNAdsBuilder <output target data prefix> <Target sequence file>\n";
	return os.str();
}

int main(int argc, const char ** argv) {
	bool success;
	Input input = Input(Input::ProgramInput::BUILDER);

	success = input.Initiate(argc, argv);
	if (!success) {
		std::cerr << input.ErrString() << std::endl;
		std::cerr << Usage() << std::endl;
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
	}

	log << "Finished" << std::endl;
	return 0;
}
