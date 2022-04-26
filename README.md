# RakeAndCompress


issues with c++: tried currying for partially resolved nodes (one leaf but not other) but explicit typing and other ew stuff that wasn't conducive to functions as values made things fucky
-- we had to have an aarray of everything so we could do things in parallel. Like there was no point of the tree data structure if we have to literally represent the whole thing as an array as well.
-- how to get all leaves at once to do in parallel - issues in c++ vs sml

-- need to tailor it to the specific use case. can't abstract away from operations. relies on keeping the ax+b structure for expression trees thus need to case on if the op is + or *