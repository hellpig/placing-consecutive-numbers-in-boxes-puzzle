/*
  Place numbers in boxNum boxes starting with 1 and increasing by 1 each time.
  Number cannot be placed in a box if it is the sum of any set of numbers in the box.
  Number cannot be placed in a box if double any number in the box.
  Find the max number that can be placed in boxNum boxes.


  To compile...
    g++ -O3 -std=c++11 boxes.cpp
  To run, do something like either of the following...
    ./a.out
    ./a.out 0,1,0,2,0,1 > log1.txt
  If a string of comma-separated numbers is an argument, these are the starting boxes.
  So, above, 1 would be placed in box 0, 2 in box 1, 3 in box 0, etc.
  Using the command-line argument allows you to split up the task across many CPU cores.
  Make sure empty boxes aren't skipped. For example, 0,2,0,1 is not valid
    because placing in box 2 skipped box 1.

  To compile for deploying on any Windows machine, I did...
    x86_64-w64-mingw32-g++ -static -O3 -std=c++11 boxes.cpp
  I obtained this command via Cygwin's mingw64-x86_64-gcc-g++ package.

  Each current-best solution is printed. The final printed solution is the actual best.
    If you input a state, the actual best is just the best given that starting state.


  Before compiling, all you have to set is boxNum !

  You may also want to change the "best = 0" line to try to speed things up,
    though I never worried about changing it.

  To print ALL of the best solutions, there are two commented lines of code to change.
  Search this file for "ALL best solutions" to find them.

  For boxNum > 4, uncommenting out the code that says "print progress" above it
    can be helpful to see the progress made by the most shallow levels of recursion.

  For boxNum > 4, uncommenting out the code that says
    "print all possible things for state[]" above it can be helpful for getting a
    list of comma-separated-number strings to use as command-line arguments.
    This allows you to distribute the calculation to many computers and CPU cores.
*/


#include <iostream>
#include <chrono>
#include <ctime>
#include <vector>

int best;
bool increaseNeeded;




const uint8_t boxNum = 5;
// not greater than 8 while possibilities is uint8_t
// not greater than 10 while argv[1] processing only looks for single-digit arguments
//
// I do not pass this as a command line argument because I want the compiler to
//   try to optimize as much as it can! Though I have not tested making this a
//   a command line argument...



/*
  found max number will go up to maxSteps-1
*/

const uint16_t maxSteps = 3 << (uint16_t) boxNum;   // this seems safe




const uint16_t sumsLength = ( maxSteps>>6 ) + 1;

uint8_t boxes[maxSteps + 1];  // faster using int instead?

// bit mask for updating the "final" 64-bit chunk of a sums[box]
//   to remove out-of-bounds (greater than maxSteps) bits
const uint64_t sumsMask = (~((uint64_t)0)) >> (63 - (maxSteps & 63));



// I decided to print the current best each time a new best is found
//   because I really want to find ALL forms of the optimal solution.

void printBoxes( int step ) {

/*
    // print boxes[] directly
    for (int j=1; j<step; j++)
      std::cout << (int) boxes[j] << ',';
    std::cout << boxes[step];
*/


    std::cout << "[";

    for (int j=0; j<boxNum; j++) {
      std::cout << '[';

      bool noComma = true;
      for (int i=1; i <= step; i++) {
        if (boxes[i]==j) {
          if (noComma) {  // for the first print of each box
            std::cout << i;
            noComma = false;
          } else {
            std::cout << "," << i;
          }
        }
      }

      if (j < boxNum-1)
        std::cout << "], ";
      else
        std::cout << "]";
    }

    std::cout << ']';


}





// the following function only does anything if starting boxes are passed as a command line argument

void initialize(uint8_t possibilities[maxSteps+1], uint64_t sums[boxNum][sumsLength], uint16_t n_final, int state[]) {

  for(uint16_t n=1; n < n_final; n++) {

      int box = state[n];

      // optionally check possibilities[n]
      if ( !( possibilities[n] & ((uint8_t)1 << box) ) ) {
        std::cout << "\n\nBad state provided!!\n\n" << std::flush;
        for (int i=1; i<=maxSteps; i++)
          possibilities[i] = 0;
        boxes[n] = box;
        return;
      }

      // bit mask for removing from possibilitiesNew
      uint8_t mask = ~((uint8_t)1 << box);

      // remove 2*n from possibilities
      uint16_t j = n << 1;
      if (j <= maxSteps)
        possibilities[j] &= mask;  // remove from possibilities

      // deep copy
      uint64_t sumsOld[boxNum][sumsLength];
      for (int i=0; i<boxNum; i++)
        for (int j=0; j<sumsLength; j++)
          sumsOld[i][j] = sums[i][j];

      // remove sums from possibilities[] and update sums[]
      // This is a very old and inefficient way of doing it, but it gives equivalent results!
      for (int i=0; i<sumsLength; i++) {         // i represents 64 possible sums
        uint64_t temp = sumsOld[box][i];
        while(temp) {
          int k = __builtin_ctzll( temp );       // count trailing zeros
          uint16_t j = k + (i << 6) + n;   // k + (i<<6) is the sum being added to
          if (j > maxSteps)
            goto endloops;
          possibilities[j] &= mask;         // remove from possibilities[]
          sums[box][j >> 6] |= ((uint64_t) 1 << (j & 0b111111));  // add to sums[]
          temp -= ((uint64_t)1 << k);
        }
      }

endloops:

      // place n
      boxes[n] = box;
      sums[box][n >> 6] |= ((uint64_t) 1 << (n & 0b111111));

      // update boxes[0]
      if (box == boxes[0])
        boxes[0] = box+1;

  }
}






// the recursive function to place n

void step(uint8_t possibilities[maxSteps+1], uint64_t sums[boxNum][sumsLength], uint16_t n) {

  // prune by looking ahead!
  for (int i = n+1; i < best+2; i++)    // change best+2 to best+1 to get ALL best solutions
    if (!possibilities[i])
      return;

  if (n > maxSteps) {
    increaseNeeded = true;
    return;
  }

  // see if we might have a new best!
  int temp = n-1;   // candidate for new best
  if (temp > best && !possibilities[n]) {     // change > to >= to get ALL best solutions
    best = temp;
    std::cout << temp << '\n';
    printBoxes(temp);
    std::cout << "\n\n" << std::flush;
    return;
  }


/*
  // print progress
  if (temp <= 9) {    // feel free to change the number (9 is great for boxNum=5)
    std::time_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << temp << "   " << ctime(&timeNow);
    printBoxes(temp);
    std::cout << "\n\n" << std::flush;
  }
*/



/*
  // print all possible things for state[]
  int depth = 6;    // feel free to change the number (6 is great for boxNum=5)
  if (temp == depth) {
    printBoxes(temp);
    std::cout << "\n" << std::flush;
  }
  else if (temp > depth) return;
*/

  // for updating sums[], assuming that it is uint64_t
  int nmod = n & 63;   // n%64
  int ndiv = n >> 6;   // n/64

  // try to place n in each box
  while(possibilities[n]) {
      int box = __builtin_ctz( possibilities[n] );   // count trailing zeros

      // copy starting at n+1
      uint8_t possibilitiesNew[maxSteps+1];
      for (int i=n+1; i<maxSteps+1; i++)
        possibilitiesNew[i] = possibilities[i];

      // deep copy
      uint64_t sumsNew[boxNum][sumsLength];
      for (int i=0; i<boxNum; i++)
        for (int j=0; j<sumsLength; j++)
          sumsNew[i][j] = sums[i][j];

      // bit mask for removing from possibilitiesNew
      uint8_t mask = ~((uint8_t)1 << box);

      // remove 2*n from possibilitiesNew
      uint16_t j = n << 1;
      if (j <= maxSteps)
        possibilitiesNew[j] &= mask;  // remove from possibilities



      // NEWsums are the new sums, meaning the values in sumsNew that aren't also in sums...
      //   NEWsums = sumsNew & ~sums
      // Though this is not exactly the way the following "magic" code does it.
      uint64_t NEWsums[sumsLength];

      // magic happens!
      // The goal is to find NEWsums using bitshifts of sums[],
      //   keeping in mind that sums[] comes in chunks of 64 bits,
      //   so each original chunk can affect 2 chunks.
      // Note that NEWsums[i] where i < ndiv is never stored or accessed.
      NEWsums[ndiv] = (sums[box][0] << nmod) & (~sums[box][ndiv]);
      if (nmod == 0) {   // needs to be handled separately to prevent annoyingly-undefined behavior of right bitshift when nmod=0
        for (int i = ndiv + 1; i < sumsLength; i++)
          NEWsums[i] = sums[box][i - ndiv] &
                       (~sums[box][i]);
      } else {
        for (int i = ndiv + 1; i < sumsLength; i++)
          NEWsums[i] = ((sums[box][i - ndiv] << nmod) |
                        (sums[box][i - ndiv - 1] >> (64 - nmod))) &
                       (~sums[box][i]);
      }

      // necessary to prevent possibilitiesNew being written to out-of-bounds
      NEWsums[sumsLength - 1] &= sumsMask;

      // updating possibilitiesNew by removing NEWsums
      for (int i = ndiv; i < sumsLength; i++) {
        uint64_t temp = NEWsums[i]; // temp represents 64 possible sums
        while (temp) {
          int k = __builtin_ctzll(temp);          // count trailing zeros
          possibilitiesNew[k + (i << 6)] &= mask; // remove from possibilities[]
          temp -= ((uint64_t)1 << k);
        }
      }

      // update sumsNew by adding NEWsums
      NEWsums[ndiv] |= ((uint64_t)1 << nmod);   // add n to NEWsums
      for (int i = ndiv; i < sumsLength; i++)
        sumsNew[box][i] |= NEWsums[i];



      // place n
      boxes[n] = box;

/*
      // useful for boxNum=3 to map out the recursion tree
      // If doing this, perhaps comment out the non-initial pruning
      std::cout << ' ' << n << ' ' << box << '\n';
      // An unimplemented idea to get a sense of the tree (useful even for boxNum > 3) is to
      // have an array that counts how many times a branch terminates at each depth.
*/


      // To speed things up, prune initial identical steps.
      // If box is empty, no need to continue placing n in subsequent empty boxes.
      if (box == boxes[0]) {
        boxes[0] = box+1;
        step(possibilitiesNew, sumsNew, n+1);
        boxes[0] = box;
        return;
      }


      step(possibilitiesNew, sumsNew, n+1);

      possibilities[n] -= ((uint8_t)1 << box);  // so that the while loop progresses

  }
}






int main(int argc, char* argv[]) {

  // process command line arguments to create temp[]
  std::vector<int> temp; 
  if (argc == 2) {
    for (int i=0; argv[1][i] != '\0'; i++) {
       char letter = argv[1][i];
       if ( '0' <= letter && letter <= '9' )   // if letter is a digit
         temp.emplace_back(int(letter - '0'));
    }
  } else if (argc > 2) {
    std::cout << "bad user! bad!\n" << std::flush;
    return 1;
  }

  // create state[] from temp[]
  int state[temp.size() + 1];
  state[0] = -1;               // state[0] is never used
  for (int i=0; i<temp.size(); i++)
    state[i+1] = temp[i];



  best = 0;    // current best max steps found; increasing this here can speed up the code!
  increaseNeeded = false;



  /* initialize data structures */

  uint8_t possibilities[maxSteps+1];   // possibilities[0] is never used
  for (int i=1; i<=maxSteps; i++)
    possibilities[i] = (1 << boxNum) - 1;   // each bit is a box

  uint64_t sums[boxNum][sumsLength];
  for (int i=0; i<boxNum; i++)
    for (int j=0; j<sumsLength; j++)
      sums[i][j] = 0;

  boxes[0] = 0;   // boxes[0] is the number of used boxes



  // initialize to state[]
  uint16_t n = sizeof(state)/sizeof(state[0]);
  initialize(possibilities, sums, n, state);



  // start timer
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  // do it
  step(possibilities, sums, n);

  // stop timer
  long long duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

  // print
  std::cout << "  time for " << static_cast<unsigned>(boxNum) << " boxes is " << duration_ms << " ms\n" << std::flush;
  if (increaseNeeded)   std::cout << "  increase maxSteps!\n" << std::flush;



  return 0;
}