# RakeAndCompress


issues with c++: tried currying for partially resolved nodes (one leaf but not other) but explicit typing and other ew stuff that wasn't conducive to functions as values made things fucky
-- we had to have an aarray of everything so we could do things in parallel. Like there was no point of the tree data structure if we have to literally represent the whole thing as an array as well.
-- how to get all leaves at once to do in parallel - issues in c++ vs sml

-- need to tailor it to the specific use case. can't abstract away from operations. relies on keeping the ax+b structure for expression trees thus need to case on if the op is + or *

- design choices, don't compress a leaf: makes casework sm easier and doesn't change bounds because leaves are at most half the tree

- don't need to lock yourself or your parent by design of conflips in compress

- caveat for time comparison: went through lots of design changes and concept clarifications and did C first so SML was much quicker. We kept logs of how much time we took for every part so we subtracted everything we didn't use or rewrote for C and the time it took for us to figure out the concept to attempt to even the comparison.

- mention that time comparison was one of our most exciting comparisons bc we haven't really quantified something like this before 