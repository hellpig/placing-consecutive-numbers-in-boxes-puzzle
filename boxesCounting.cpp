/*
  Mess around with boxes.cpp first so that you can understand this file.

  This code lets you assume that final box(es) are counting boxes!
  A counting box will be [countStart, countStart + 1, ..., 2*countStart - 1]
  The code will represent this as [countStart, 0, 2*countStart - 1]
  Note that, if using countStart=37 as the first counting box,
    you can actually use 13 then 37 because there is only one way to get to 36,
    and it has a 13-25 counting box.
    Then, for the next box, it must be a counting box with countStart=2*37.
    If RAM usage becomes an issue, consider starting by placing up to 74 by hand.

  The code also assumes that subsequent filling of a counting box always occurs
    boxes[box][0] numbers in a row. By commenting out a section of code, this
    assumption can be turned off.
    I have evidence that this assumption can lose repeated solutions.


  g++ -O3 -std=c++11 boxesCounting.cpp
  ./a.out

  For boxNumAll=14 (or 12 if not using the subsequent-filling trick),
  on my macOS or Ubuntu, I have to first run
    ulimit -s 16384
  before running a.out, else I get a segmentation fault due to a stack overflow.
  This doubles the default stack size limit for the terminal for as long as it is open.
  For boxNumAll=15...
    ulimit -s 65532
  which is the highest macOS will go. Linux can keep going! For boxNumAll=16, 128 MiB works...
    ulimit -s 131072

  On Windows, I need the following to increase a tiny (1 MiB??) stack size limit...
    g++ -O3 -std=c++11 "-Wl,--stack,16777216" boxesCounting.cpp
  For 128 MiB...
    g++ -O3 -std=c++11 "-Wl,--stack,134217728" boxesCounting.cpp


  To print ALL of the best solutions, there are two commented lines of code to change.
  Search this file for "ALL best solutions" to find them.

  Technical ideas to speed things up...
   - Since now counting boxes are a thing, consider limiting the non-initial pruning???
   - Should I make sums[] smaller since some counting boxes can never be "reactivated" after counting?
   - Use an array of pointers to std::array so that only a small part of the sums 
     array can be deep copied.
     This slows things down for small boxNumAll, but, if larger,
     data structures are quite huge and should probably be on the heap anyway?
*/


#include <iostream>
#include <chrono>
#include <ctime>
#include <vector>

uint32_t best;
bool increaseNeeded;




////////////////////////////////////
// set the following before each run
////////////////////////////////////

/*
  Set number of boxes

  not greater than 8 while possType is uint8_t
  uint16_t is only about 2 percent slower

  not greater than 14 while nType is uint16_t
  not greater than 16 while possType is uint16_t
*/

const uint8_t boxNumAll = 16;

#define possType uint16_t
#define nType uint32_t



/*
  Set counting boxes.
  Set min starting value in the boxes, where later counting boxes get at least the min of any previous counting boxes.
  The min of a non-counting box is ignored.
  Note that counting boxes will never start at n<5.
*/

bool isCounting[boxNumAll]         = {false, false, false, true, true, true, true, true, true, true, true, true,  true,  true,  true, true};
const uint32_t minStart[boxNumAll] = {    0,     0,     0,   13,   37,    0,  157,  329,  659, 1329, 2695, 5404, 10893, 21786, 43730,    0};



/*
  Use the following to be able to "pick up where you left off".
  Else, all should be set to max.
  Note that it doesn't exactly pick up where you left off, and,
    especially for later boxes, will have to do some repeated calculation.
    To test the time for repeated calculations, set the maxStart below the minStart.
*/

const nType max =  ~( (nType)0 );
const nType maxStart[boxNumAll]    = {  max,   max,   max,  max,  max,  max,  max,  max,  max,  max,  max,  max,   max,   max, 43730,  max};


////////////////////////////////////
////////////////////////////////////




/*
  found max number will go up to maxSteps-1
*/

const nType maxSteps = (nType) 3 << boxNumAll;   // this seems safe




const nType sumsLength = ( maxSteps>>6 ) + 1;

//optional. Gives useful information to print
uint64_t counts[boxNumAll] = {0};



std::vector<nType> boxes[boxNumAll];       // array of empty vectors



void printBoxes() {

    std::cout << "[";

    for (int j=0; j<boxNumAll; j++) {
      std::cout << '[';

      for (int i=0; i < (int)boxes[j].size() - 1; i++)
        std::cout << boxes[j][i] << ",";
      if ( boxes[j].size() )
        std::cout << boxes[j][boxes[j].size()-1];

      if (j < boxNumAll-1)
        std::cout << "], ";
      else
        std::cout << "]";
    }

    std::cout << ']';

}



inline uint32_t firstAllowed(uint32_t countStart) {
    return ((3*countStart - 5) * countStart) >> 1;    // valid for countStart > 4
}



// updates sums and possibilities when doing a subsequent fill
void subsequentFill(uint64_t sumsNew[sumsLength], possType possibilitiesNew[maxSteps+1], nType n0, nType len, possType mask) {

    for (nType n = n0; n < n0+len; n++) {

        uint64_t sums[sumsLength];
        for (int j=0; j<sumsLength; j++)
          sums[j] = sumsNew[j];

        // remove 2*n from possibilitiesNew
        uint32_t j = (uint32_t) n << 1;
        if (j <= maxSteps)
          possibilitiesNew[j] &= mask;  // remove from possibilities

        // these should probably be passed in as function parameters
        uint64_t nmod = ((uint64_t)n << 58) >> 58;   // 64 - 6 = 58
        nType ndiv = n >> 6;

        // remove sums from possibilitiesNew and update sumsNew
        for (int i=0; i<sumsLength; i++) {         // i represents 64 possible sums
          uint64_t temp = sums[i];

          // Basically, bit shift sums[] by n to get the new sums.
          // Tricky since sums[] is uint64_t, so sums come in groups of 64
          if (i + ndiv < sumsLength)
            sumsNew[i + ndiv] |= (temp << nmod);
          if (i + ndiv + 1 < sumsLength)
            sumsNew[i + ndiv + 1] |= (temp >> (64 - nmod));

          while(temp) {
            int k = __builtin_ctzll( temp );       // count trailing zeros
            uint32_t j = k + (i << 6) + (uint32_t)n;   // k + (i<<6) is the sum being added to
            if (j > maxSteps)
              goto endloops;
            possibilitiesNew[j] &= mask;         // remove from possibilitiesNew[]
            //sumsNew[j >> 6] |= ((uint64_t)1 << (j & 0b111111));  // add to sumsNew[]
            temp -= ((uint64_t)1 << k);
          }
        }

endloops:

        // place n
        sumsNew[n >> 6] |= ((uint64_t) 1 << (n & 0b111111));

    }

}


// the recursive function

void step(possType possibilities[maxSteps+1], uint64_t sums[boxNumAll][sumsLength], nType n, uint8_t boxNum, bool isCountingStill[boxNumAll]) {

  // prune sooner rather than later
  for (int i = n+1; i < best+1; i++)    // change best+2 to best+1 to get ALL best solutions
    if (!possibilities[i])
      return;

  if (n > maxSteps) {
    increaseNeeded = true;
    return;
  }

  // see if we might have a new best!
  int temp = n-1;   // candidate for new best
  if (temp >= best && !possibilities[n]) {     // change > to >= to get ALL best solutions
    best = temp;
    std::cout << temp << '\n';
    printBoxes();
    std::cout << "\n\n" << std::flush;
    return;
  }


/*
  // print progress
  if (temp <= 9) {
    std::time_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << temp << "   " << ctime(&timeNow);
    printBoxes();
    std::cout << "\n\n" << std::flush;
  }
*/


  bool putInEmptyBox = false;          // has a previous empty non-counting box had n put inside?
  bool putInEmptyCountingBox = false;  // has a previous empty counting box had n put inside?

  // for updating sums[], assuming that it is uint64_t
  uint64_t nmod = ((uint64_t)n << 58) >> 58;   // 64 - 6 = 58
  nType ndiv = n >> 6;

  // try to place n in each box
  while(possibilities[n]) {
      int box = __builtin_ctz( possibilities[n] );   // count trailing zeros

      // bit mask
      possType mask0 = ((possType)1 << box);

      uint32_t n2 = (uint32_t)n << 1;

      // declare
      uint64_t sumsNew[boxNumAll][sumsLength];
      possType possibilitiesNew[maxSteps+1];

/*
      // strangely, this code once sped things up if -O2 or -O3 compiler flag was used !!
      if (n==4) {
        std::cout << n << '\n';
      }
*/

/*
      // old way to enforce certain subsequent counting patterns
      //if (n>=5391 && n<=5403 && box!=3) {
      if (n>=5390 && n<=5402 && box!=3) {
        possibilities[n] -= mask0;
        continue;
      }
      if (n>=10819 && n<=10892 && box!=5) {
      //if (n>=10808 && n<=10881 && box!=5) {
        possibilities[n] -= mask0;
        continue;
      }
*/



      ///////////////////////////////////////
      ///////////////////////////////////////
      ///////////////////////////////////////

      if (isCountingStill[box]) {

        // to speed things up, prune initial identical steps
        if ( !boxes[box].size() ) {  // is this check necessary??
          if (putInEmptyCountingBox) {
            possibilities[n] -= mask0;
            continue;
          }
          putInEmptyCountingBox = true;
        }

        // this should occur after pruning
        if (n > maxStart[box]) {
          possibilities[n] -= mask0;
          continue;
        }

        // add to boxes[]
        // Only put in starting box, zero, and ending box
        boxes[box].emplace_back(n);
        boxes[box].emplace_back(0);
        boxes[box].emplace_back(n2 - 1);


        counts[box]++;

        // bit mask for removing from possibilitiesNew
        possType mask = ~mask0;

        uint32_t temp0 = firstAllowed((uint32_t) n);

        if ( temp0 <= maxSteps ) {

          uint32_t temp2 = temp0 + n2;

          // set this box to no longer be a counting box
          bool isCountingStillNew[boxNumAll];
          for (int i=0; i<boxNumAll; i++)
            isCountingStillNew[i] = isCountingStill[i];
          isCountingStillNew[box] = false;

          // set possibilitiesNew[] starting at 2*n (valid for n>4)
          for (int i=n2; i <= temp0+n; i++) {   // exclude through firstAllowed + n
            if (i > maxSteps)
              break;
            possibilitiesNew[i] = possibilities[i] & mask;
          }
          for (int i = temp0 + n + 1; i<=maxSteps; i++) {  // allow after firstAllowed + n
            possibilitiesNew[i] = possibilities[i];
          }
          possibilitiesNew[temp0] = possibilities[temp0];   // allow firstAllowed
          if (temp2 <= maxSteps)
            possibilitiesNew[temp2] = possibilities[temp2] & mask;  // exclude firstAllowed + 2*n


          // deep copy sums[] up to boxNum
          for (int i=0; i<boxNum; i++)
            for (int j=0; j<sumsLength; j++)
              sumsNew[i][j] = sums[i][j];

          // add box to sums[]; would it be faster to initialize as 1's then remove??? or to not initialize here??
          for (int j=0; j<sumsLength; j++)
            sumsNew[boxNum][j] = 0;

          // add sums to sumsNew[boxNum]; valid for n>4
          for (int i=n; i <= temp0+n; i++) {   // n through firstAllowed + n set as sums
            if (i > maxSteps)
              break;
            sumsNew[boxNum][i >> 6] |= ((uint64_t) 1 << (i & 0b111111));
          }
          sumsNew[boxNum][n2 >> 6]    &=  ~((uint64_t) 1 << (n2 & 0b111111));     // remove 2*n as a sum
          sumsNew[boxNum][temp0 >> 6] &=  ~((uint64_t) 1 << (temp0 & 0b111111));  // remove firstAllowed as sum
          if (temp2 <= maxSteps)
            sumsNew[boxNum][temp2 >> 6] |= ((uint64_t) 1 << (temp2 & 0b111111));   // firstAllowed + 2*n is a sum


          step(possibilitiesNew, sumsNew, n2, boxNum+1, isCountingStillNew);

        } else {

          // copy possibilities[] starting at 2*n, but remove box
          for (int i=n2; i<maxSteps+1; i++)
            possibilitiesNew[i] = possibilities[i] & mask;

          step(possibilitiesNew, sums, n2, boxNum, isCountingStill);
        }

        // put boxes[box] back the way it was
        boxes[box].resize( boxes[box].size() - 3 );


      ///////////////////////////////////////
      ///////////////////////////////////////
      ///////////////////////////////////////

      //  if you don't want to force how subsequent counting boxes are filled,
      //    comment out from here to the else
      } else if (isCounting[box]) {   // handle subsequent counting boxes

        nType len = boxes[box][0];

        // look ahead len steps to see if a subsequent box is even possible
        bool stop = false;
        for (int i = n+1; i < n+len; i++) {
          if (i > maxSteps) {
            increaseNeeded = true;
            return;
          }
          if (! (possibilities[i] & mask0) ) {
            stop = true;
            break;
          }
        }
        if (stop) {
            possibilities[n] -= mask0;
            continue;
        }


        // update boxes[]
        boxes[box].emplace_back(n);
        boxes[box].emplace_back(0);
        boxes[box].emplace_back(n+len-1);


        // deep copy sums[] up to boxNum
        for (int i=0; i<boxNum; i++)
          for (int j=0; j<sumsLength; j++)
            sumsNew[i][j] = sums[i][j];

        // copy possibilities[] starting at n+1
        for (int i=n+1; i<maxSteps+1; i++)
          possibilitiesNew[i] = possibilities[i];

        // update sumsNew[box][] and possibilitiesNew[]
        subsequentFill(sumsNew[box], possibilitiesNew, n, len, ~mask0);

        step(possibilitiesNew, sumsNew, n+len, boxNum, isCountingStill);

        // put boxes[box] back the way it was
        boxes[box].resize( boxes[box].size() - 3 );



      ///////////////////////////////////////
      ///////////////////////////////////////
      ///////////////////////////////////////

      } else {

        // to speed things up, prune initial identical steps
        if ( !boxes[box].size() ) {
          if (putInEmptyBox) {
            possibilities[n] -= mask0;
            continue;
          }
          putInEmptyBox = true;
        }


        // deep copy sums[] up to boxNum
        for (int i=0; i<boxNum; i++)
          for (int j=0; j<sumsLength; j++)
            sumsNew[i][j] = sums[i][j];

        // copy possibilities[] starting at n+1
        for (int i=n+1; i<maxSteps+1; i++)
          possibilitiesNew[i] = possibilities[i];


        // bit mask for removing from possibilitiesNew
        possType mask = ~mask0;

        // remove 2*n from possibilitiesNew
        if (n2 <= maxSteps)
          possibilitiesNew[n2] &= mask;  // remove from possibilities

        // remove sums from possibilitiesNew and update sumsNew
        for (int i=0; i<sumsLength; i++) {         // i represents 64 possible sums
          uint64_t temp = sums[box][i];

          // Basically, bit shift sums[] by n to get the new sums.
          // Tricky since sums[] is uint64_t, so sums come in groups of 64
          if (i + ndiv < sumsLength)
            sumsNew[box][i + ndiv] |= (temp << nmod);
          if (i + ndiv + 1 < sumsLength)
            sumsNew[box][i + ndiv + 1] |= (temp >> (64 - nmod));

          while(temp) {
            int k = __builtin_ctzll( temp );       // count trailing zeros
            uint32_t j = k + (i << 6) + (uint32_t)n;   // k + (i<<6) is the sum being added to
            if (j > maxSteps)
              goto endloops;
            possibilitiesNew[j] &= mask;         // remove from possibilitiesNew[]
            //sumsNew[box][j >> 6] |= ((uint64_t)1 << (j & 0b111111));  // add to sumsNew[]
            temp -= ((uint64_t)1 << k);
          }
        }

endloops:

        // place n
        boxes[box].emplace_back(n);
        sumsNew[box][n >> 6] |= ((uint64_t) 1 << (n & 0b111111));

        step(possibilitiesNew, sumsNew, n+1, boxNum, isCountingStill);

        // remove n
        boxes[box].pop_back();

      }



      possibilities[n] -= mask0;  // so that the while loop progresses

  }
}






int main() {



  best = 0;    // current best max steps found; increasing this here can speed up the code!
  increaseNeeded = false;


  // set boxNum0
  uint8_t boxNum0 = boxNumAll;
  for (int i=0; i<boxNumAll; i++)
    if (isCounting[i])
      boxNum0--;


  /* initialize data structures */

  possType possibilities[maxSteps+1];   // possibilities[0] is never used
  for (int i=1; i<=maxSteps; i++) {
    if (i < 5)             // firstAllowed formula is not true for n<5
      possibilities[i] = ((possType)1 << boxNum0) - (possType)1;   // each bit is a box
    else
      possibilities[i] = ((possType)1 << boxNumAll) - (possType)1;   // each bit is a box
  }
  for (int box=0; box<boxNumAll; box++) {   // handle minStart[]
    if (isCounting[box]) {

      for (int i=box; i<boxNumAll; i++) {   // loop over all current and later counting boxes
        if (isCounting[i]) {
          possType mask = ~((possType)1 << i);
          for (int j=5; j<minStart[box]; j++)
            possibilities[j] = possibilities[j] & mask;
        }
      }

    }
  }

  uint64_t sums[boxNumAll][sumsLength];
  for (int i=0; i<boxNum0; i++)
    for (int j=0; j<sumsLength; j++)
      sums[i][j] = 0;



  // start timer
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  // do it
  step(possibilities, sums, 1, boxNum0, isCounting);

  // stop timer
  long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

  // print
  for (int i=0; i<boxNumAll; i++)
    std::cout << " (" << isCounting[i] << "," << minStart[i] << ")";
  std::cout << "\n counts:";
  for (int i=0; i<boxNumAll; i++)
    std::cout << " " << counts[i];
  std::cout << "\n  time for " << static_cast<unsigned>(boxNumAll) << " boxes is " << duration_ms << " ms\n" << std::flush;
  if (increaseNeeded)   std::cout << "  increase maxSteps!!\n" << std::flush;


  return 0;
}