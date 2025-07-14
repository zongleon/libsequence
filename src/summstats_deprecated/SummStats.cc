#include <Sequence/polySiteVector.hpp>
#include <Sequence/SimData.hpp>
#include <algorithm>
#include <numeric>
#include <cctype>
#include <stdexcept>
#include <limits>
using namespace std;

namespace Sequence
{
  double Dij(const polymorphicSite & p, const std::vector< unsigned > & config, const unsigned & i, const unsigned & j)
  {
    unsigned rv = 0;
    unsigned N = 0;
    unsigned start1 = accumulate(config.begin(),config.begin()+i,0u),
      start2 = accumulate(config.begin(),config.begin()+j,0u);
    for( unsigned x = start1 ; x < start1 + config[i] ; ++x )
      {
	for(unsigned y = start2 ; y < start2 + config[j] ; ++y)
	  {
	    char ch1 = char(std::toupper(p.second[x])),ch2=char(std::toupper(p.second[y]));
	    if(ch1 != 'N' && ch2 != 'N')
	      {
		rv += (ch1 != ch2) ? 1u : 0u;
	      }
	    else
	      {
		++N;
	      }
	  }
      }
    return double(rv)/(double(config[i]+config[j]-N));
  }
  
  double Gmin(const polySiteVector & , const std::vector< unsigned > & )
  {
    throw std::runtime_error("not implemented yet");
    unsigned mdxy = numeric_limits<unsigned>::max();
    return mdxy;
  }
}//ns Sequence
