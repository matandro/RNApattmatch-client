# RNAPattMatch V1.1

This software uses the GNU General Public License (See gpl.txt) supported by the Free Software Foundation.

This package was build to run the core algorithm for the RNAPattMatch server.
This can be found at: http://www.cs.bgu.ac.il/rnapattmatch/

## Building

To build this software:
$make

This software requires the boost package installed (www.boost.org)
The package exists in many of the large linux distrecutions.
On ubuntu you can use:
$sudo apt-get install libboost-all-dev

## Running

The package contains 2 executables:

1) RNAPattMatch - main program
2) RNAdsBuilder - Prebuild data structures

### RNAPattMatch

This software is used to search for an RNA sequence / structure pattern in a target sequence file.

```shell
Usage: RNAPattMatch [options] <Query sequence> <Query structure> <Target sequence file>
<Query sequence> - A string of fasta sequence representation (as found in http://en.wikipedia.org/wiki/FASTA_format#Sequence_representation) without 'X' or '-'. This will represent the RNA sequence pattern you are searching for.

<Query structure> - A string in the dot bracket notation, '(' '.' and ')', which represent the secondary structure of the pattern.
* Both query sequence and structure can also include [x] which is read by a variable gap of up to x characters.

<Target sequence file> - A file containing a DNA / RNA sequence that the pattern will be searched in. The file should be in FASTA format, can inlcude multiple targets, with only A,C,G,U or T supported. white spaces are ignored.

options include:
    -t [num] - number of concurent threads on search
    -m [BP1,BP2,...,BPn] - The legal base pairs
    -r [num] - set max results to be found in target
    -f [file path] - setup output to file
    -s [file prefix] - save data structures to file
        * this option will cause a full calculation of the affix array and will take a while
    -l [file prefix] - load data structure from file
```

#### Example

`RNAPattMatch NNNN[2]TA[6]NNN[2]ATNNGG[2]NNN[5]GTNTCTAC[3]NNNNN[3]CCNNNAA[3]NNNNN[5]NNNN '(((([2]..[6]((([2]......[2])))[5]........[3]((((([3].......[3])))))[5]))))' Bacillus_subtilis.fna`
notice that the gaps mentioned in the pattern are aligned between sequence and structure, this must happen for every gap inserted.

### RNAdsBuilder

This software is used to build and save the neccesery data structures for a future search using the -l option for RNAPattMatch. You can perform a search with RNAPattMatch without using this program.
Affix links are calculated in a lazy manner. The file will include empty links unless calculated during an actual run. After an actual search the links will be updated to the file.

```shell
Usage: RNAdsBuilder <output target data prefix> <Target sequence file>
<Output target data prefix> - A string that will be the prefix to all output file names, same as [file prefix] from the -l and -s options for RNAPattMatch. (example below)

<Target sequence file> - A file containing a DNA / RNA sequence that the data structures will be built for. The file should be in FASTA format, can inlcude multiple targets, with only A,C,G,U or T supported. white spaces are ignored. 
```

#### Example

RNAdsBuilder Bacillus_subtilis Bacillus_subtilis.fna

Running the following line will create 3 files for every target in the target file:
`Bacillus_subtilis_<target No>_FOR.SFA` - this file contains Suffix array, longest common prefix array and chaild table.
`Bacillus_subtilis_<target No>_REV.SFA` - this file contains Reverse prefix array, longest common prefix array and child table.
`Bacillus_subtilis_<target No>.AFLK` - this file contains the affix links.

Each parameter is hold as a 64 integer (8 bytes), this will generate very large files.
