/*

  Copyright (C) 2003-2009 Kevin Thornton, krthornt[]@[]uci.edu

  Remove the brackets to email me.

  This file is part of libsequence.

  libsequence is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  libsequence is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  long with libsequence.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <cassert>
#include <map>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <Sequence/SeqEnums.hpp>
#include <Sequence/Translate.hpp>
#include <Sequence/RedundancyCom95.hpp>
#include <Sequence/SeqAlphabets.hpp>
#include <Sequence/Comparisons.hpp>

namespace Sequence
{
#ifndef DOXYGEN_SKIP
  struct RedundancyCom95impl
  {
    explicit RedundancyCom95impl(const GeneticCodes code);

    const GeneticCodes genetic_code;
      
    //matrices to be filled on the fly
    double firstNon[4][4][4];
    double first2S[4][4][4];
    double first2V[4][4][4];
    double thirdFour[4][4][4];
    double thirdNon[4][4][4];
    double third2S[4][4][4];
    double third2V[4][4][4];
    double l0_vals[4][4][4];
    double l2S_vals[4][4][4];
    double l2V_vals[4][4][4];
    double l4_vals[4][4][4];
    void FillFirstPositionCounts ();
    void FillThirdPositionCounts ();
    void FillLValues ();
    void codonPrecondition(const std::string & codon);
  };

  RedundancyCom95impl::RedundancyCom95impl(const GeneticCodes code) : genetic_code(code)
  {
    FillFirstPositionCounts ();
    FillThirdPositionCounts ();
    FillLValues ();
  }    

  void RedundancyCom95impl::codonPrecondition(const std::string & codon) 

  /*!
    Checks to see if this class can handle the codon.
    A valid codon is:
    1.) of length 3, 
    2.) contains only characters in the set {A,G,C,T} (case sensitive)
  */
  {
    if ( codon.length() != 3 ||
	 std::find_if(codon.begin(),codon.end(),ambiguousNucleotide()) != codon.end() )
      {
	throw(std::runtime_error("Sequence::RedundancyCom95 -- precondition failed, invalid codon"));
      }
  }

  void
  RedundancyCom95impl::FillFirstPositionCounts ()
  {
    std::string codon,mutation;
    int numN, numTsS, numTvS;	//numbers of changes of various types, N = nonsynon, S = synon
    unsigned codonState;
    int numPossTs, numPossTv;
    double FirstN, First2S, First2V;
    std::string codonTrans;
    std::string mutationTrans;

    bool S, N;
    //make every codon
    codon.resize(3);
    mutation.resize(3);
    for( unsigned i = 0 ; i < 4 ; ++i )
      {
	for( unsigned j = 0 ; j < 4 ; ++j )
          {
	    for( unsigned k = 0 ; k < 4 ; ++k )
              {
                numN = numTsS = numTvS = 0;	//initialize
                numPossTs = numPossTv = 0;
                S = N = 0;

		codon[0] = dna_alphabet[i];
		codon[1] = dna_alphabet[j];
		codon[2] = dna_alphabet[k];

                //now make all possible mutations at first position
                codonState = i;
		for( unsigned l = 0 ; l < 4 ; ++l )
                  {
                    if (l != codonState)
                      {	//only include mutations
			mutation[0] = dna_alphabet[l];
                        mutation[1] = codon[1];
                        mutation[2] = codon[2];	//the mutant codon is now defined
			auto type = TsTv(dna_alphabet[codonState],mutation[0]);
                        codonTrans = Translate (codon.begin(),
                                                codon.end(), genetic_code);
                        mutationTrans = Translate (mutation.begin(),
                                                   mutation.end(),genetic_code);

                        //if neither codon is a stop
                        if ((codonTrans)[0] != '*'
                            && (mutationTrans)[0] != '*')
                          {
                            if(codonTrans == mutationTrans)
                              {	//synonymous change
                                S = 1;
                                N = 0;
                              }
                            else
                              {	//nonsynonymous change
                                S = 0;
                                N = 1;
                              }
                            if (type == Mutations::Ts)
                              ++numPossTs;
                            else if (type == Mutations::Tv)
                              ++numPossTv;
                            if (N == 1)	//if change is nonsynonymous
                              ++numN;
                            else if (S == 1 && type == Mutations::Ts)
                              //if change is synonymous transition
                              ++numTsS;
                            else if (S == 1 && type == Mutations::Tv)
                              //if change is synonymous transversion
                              ++numTvS;
                          }
                      }
                  }
                //process here
                if ((numPossTs + numPossTv) == 0)
                  {	//stop codons
                    First2S = 0;
                    First2V = 0;
                    FirstN = 0;
                  }
                else if ((numTsS + numTvS) == 0)	//non-degenerate sites
                  {
                    First2S = 0;
                    First2V = 0;
                    FirstN = 1.0;
                  }
                else if (numPossTs != 0 && numPossTv != 0)
                  {	//special cases
                    if (double (numTsS) /
                        double (numPossTs) != 1.0
                        && double (numTvS) /
                        double (numPossTv) != 1.0)
                      {	//fractional redundancy
                        First2S =
                          double (numTsS) /
                          double (numPossTs +
                                  numPossTv);
                        First2V =
                          double (numTvS) /
                          double (numPossTs +
                                  numPossTv);
                        FirstN = 1.0 - First2S -
			  First2V;
                      }
                    else
                      {	//odd types of degeneracy
                        First2S =
                          double (numTsS) /
                          double (numPossTs);
                        First2V =
                          double (numTvS) /
                          double (numPossTv);
                        FirstN = 1.0 - First2S -
			  First2V;
                      }
                  }
                else
                  {
                    if (numPossTs > 0)
                      First2S =
                        double (numTsS) /
                        double (numPossTs + numPossTv);
                    else
                      First2S = 0.0;

                    if (numPossTv > 0)
                      First2V =
                        double (numTvS) /
                        double (numPossTv + numPossTs);
                    else
                      First2V = 0.0;
                    FirstN = 1.0 - First2S - First2V;
                  }
                //fill array
		firstNon[i][j][k]=FirstN;
		first2S[i][j][k]=First2S;
		first2V[i][j][k]=First2V;
              }
          }
      }
  }

  void
  RedundancyCom95impl::FillThirdPositionCounts ()
  {
    std::string codon, mutation;	//the two codons
    int numN, numTsS, numTvS;	//numbers of changes of various types, N = nonsynon, S = synon
    unsigned codonState;
    int numPossTs, numPossTv;
    double ThirdN, Third2S, Third2V, Third4;
    std::string codonTrans;
    std::string mutationTrans;
    bool S, N;
    //make every codon
    codon.resize(3);
    mutation.resize(3);
    for( unsigned i = 0 ; i < 4 ; ++i )
      {
	for( unsigned j = 0 ; j < 4 ; ++j )
          {
	    for( unsigned k = 0 ; k < 4 ; ++k )
              {
                numN = numTsS = numTvS = 0;	//initialize
                numPossTs = numPossTv = 0;
                S = N = 0;
		codon[0] = dna_alphabet[i];
		codon[1] = dna_alphabet[j];
		codon[2] = dna_alphabet[k];
                //now make all possible mutations at first position
                codonState = k;

		for( unsigned l = 0 ; l < 4 ; ++l )
                  {
                    if (l != codonState)
                      {	//only include mutations
                        mutation[0] = codon[0];
                        mutation[1] = codon[1];
			mutation[2] = dna_alphabet[l];
			auto type = TsTv( dna_alphabet[codonState],mutation[2] );
                        codonTrans = Translate (codon.begin(),
                                                codon.end(),genetic_code);
                        mutationTrans = Translate (mutation.begin(),
                                                   mutation.end(),genetic_code);

                        if ((codonTrans)[0] != '*'
                            && (mutationTrans)[0] != '*')
                          {
                            if(codonTrans==mutationTrans)
                              {
                                S = 1;
                                N = 0;
                              }
			    else
                              {
                                S = 0;
                                N = 1;
                              }
                            if (type == Mutations::Ts)
                              ++numPossTs;
                            else if (type == Mutations::Tv)
                              ++numPossTv;
                            if (N)
                              ++numN;
                            else if (S && type == Mutations::Ts)
                              ++numTsS;
                            else if (S && type == Mutations::Tv)
                              ++numTvS;
                          }
                      }
                  }
                //process here
                ThirdN = Third2S = Third2V = Third4 = 0.0;
                if (numPossTs + numPossTv == 0)
                  {	//stop codons
                    ThirdN = 0.0;
                    Third2S = 0.0;
                    Third2V = 0.0;
                    Third4 = 0.0;
                  }
                else if (numTsS + numTvS == 0)
                  {	//nondegenerate
                    ThirdN = 1.0;
                    Third2S = 0.0;
                    Third2V = 0.0;
                    Third4 = 0.0;
                  }
                else if (numTsS + numTvS == 3)
                  {	//4fold degenerate
                    ThirdN = 0.0;
                    Third2S = 0.0;
                    Third2V = 0.0;
                    Third4 = 1.0;
                  }
                else if (numTsS == 0 || numTvS == 0)
                  {	//if there is non-degeneracy for one type
                    if (numPossTs > 0)
                      Third2S =
                        double (numTsS) /
                        double (numPossTs);
                    else
                      Third2S = 0.0;
                    if (numPossTv > 0)
                      Third2V =
                        double (numTvS) /
                        double (numPossTv);
                    else
                      Third2V = 0.0;
                    ThirdN = 1.0 - Third2S - Third2V;
                    Third4 = 0.0;
                  }
                else if (numTsS > 0 && numTvS > 0
                         && (double (numTsS) /
                             double (numPossTs) != 1.0
                             || double (numTvS) /
                             double (numPossTv) != 1.0))
                  {	//fractional degeneracy
                    Third2S = 1.0 / 3.0;
                    Third2V = 1.0 / 3.0;
                    ThirdN = 1.0 / 3.0;
                    Third4 = 0.0;
                  }
		thirdNon[i][j][k]=ThirdN;
		third2S[i][j][k]=Third2S;
		third2V[i][j][k]=Third2V;
		thirdFour[i][j][k]=Third4;
              }
          }
      }
  }

  //the L values are just the sums of the site degeneracy values for each codon
  void
  RedundancyCom95impl::FillLValues ()
  {
    std::string trans;

    std::string codon(3,'A');
    for(unsigned i = 0 ; i < 4 ; ++i )
      for(unsigned j = 0 ; j < 4 ; ++j )
	for(unsigned k = 0 ; k < 4 ; ++k )
          {
	    codon[0] = dna_alphabet[i];
	    codon[1] = dna_alphabet[j];
	    codon[2] = dna_alphabet[k];

            trans =Translate (codon.begin(),codon.end(),  genetic_code);
            if(strcmp(trans.c_str(),"*")!=0)
              {	//if not a stop codon
		l0_vals[i][j][k] = 1. + firstNon[i][j][k] + thirdNon[i][j][k];
		l2S_vals[i][j][k] = first2S[i][j][k] + third2S[i][j][k];
		l2V_vals[i][j][k] = first2V[i][j][k] + third2V[i][j][k];
		l4_vals[i][j][k] = thirdFour[i][j][k];
              }
            else
              {
		l0_vals[i][j][k] = l2S_vals[i][j][k] = l2V_vals[i][j][k] = l4_vals[i][j][k] = 0.;
              }
          }
  }
#endif
  RedundancyCom95::RedundancyCom95 (GeneticCodes code):
    impl(std::unique_ptr<RedundancyCom95impl>(new RedundancyCom95impl(code)))
    /*!
      \param code see Sequence::GeneticCodes
    */
  {
  }

  RedundancyCom95::~RedundancyCom95(void)
  {
  }
 
  
  //the functions below are klugdy, but there is no real loss of efficieny,
  //so it is left alone for now
  double
  RedundancyCom95::FirstNon (const std::string & codon) const 

  /*!
    \return number of times the first codon position is non-degenerate
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->firstNon[i][j][k];
  }

  double
  RedundancyCom95::First2S (const std::string & codon) const 

  /*!
    \return number of times the first codon position is synonymous via a transition
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */

  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->first2S[i][j][k];
  }

  double
  RedundancyCom95::First2V (const std::string & codon) const 

  /*!
    \return number of times the first codon position is synonymous via a transversion
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->first2V[i][j][k];
  }

  double
  RedundancyCom95::ThirdNon (const std::string & codon) const 

  /*!
    \return number of times the third position is non-degenerate
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->thirdNon[i][j][k];
  }

  double
  RedundancyCom95::ThirdFour (const std::string & codon) const 

  /*!
    \return number of times the third position is fourfold-degenerate
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->thirdFour[i][j][k];
  }

  double
  RedundancyCom95::Third2S (const std::string & codon) const 

  /*!
    \return number of times the third position is synonymous via a transition
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->third2S[i][j][k];
  }

  double
  RedundancyCom95::Third2V (const std::string & codon) const 

  /*!
    \return number of times the third position is synonymous via a transversion
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->third2V[i][j][k];
  }

  double
  RedundancyCom95::L0_vals (const std::string & codon) const 

  /*!
    \return the number of non-degenerate positions in \a codon
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
    \note the return value = 1.0+FirstNon(\a codon )+ThirdNon(\a codon)
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->l0_vals[i][j][k];
  }

  double
  RedundancyCom95::L2S_vals (const std::string & codon) const 

  /*!
    \return the number of transitional silent sites in \a codon
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
    \note the return value = First2S(\a codon )+Third2S(\a codon )
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->l2S_vals[i][j][k];
  }

  double
  RedundancyCom95::L2V_vals (const std::string & codon) const 

  /*!
    \return the number of transversional silent sites in \a codon
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
    \note the return value = First2V(\a codon )+Third2V(\a codon )
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->l2V_vals[i][j][k];
  }

  double
  RedundancyCom95::L4_vals (const std::string & codon) const 

  /*!
    \return the number of fourfold silent sites in \a codon
    \pre codon is of length 3, is all uppercase, and only contains the characters {A,G,C,T}
    \throw std::runtime_error if precondition is not met
    \note the return value = ThirdFour(\a codon )
  */
  {
    impl->codonPrecondition(codon);
    auto i = std::distance( dna_alphabet.begin(),
			    std::find(dna_alphabet.begin(),
				      dna_alphabet.end(),codon[0]) ),
      j = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[1]) ),
      k = std::distance( dna_alphabet.begin(),
			 std::find(dna_alphabet.begin(),
				   dna_alphabet.end(),codon[2]) );
    return impl->l4_vals[i][j][k];
  }
  
  GeneticCodes RedundancyCom95::gencode()const
  {
    return impl->genetic_code;
  }
}
