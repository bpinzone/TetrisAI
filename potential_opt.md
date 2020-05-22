Cache EVERYTHING the utility function needs.
Inline all board member functions.

inline EVERYTHING
things that are probably inlined and can use optimizations:
    get num trenches
    is rest board 4
    height of second lowest

Don't store hole placements
Use a generator for iterating over placements

get rid of const Block& 's