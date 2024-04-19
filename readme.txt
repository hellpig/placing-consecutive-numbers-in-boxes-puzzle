
# placing-consecutive-numbers-in-boxes-puzzle

A colleague of mine teaches math and gave her AP Calculus BC students a fun optimization puzzle. Students had to place numbers in a certain number of boxes. The first number to be placed is 1, then 2, etc. Numbers cannot be skipped; they must be consecutive. The goal is to place the most numbers possible under the following constraints...
* Rule \#1: no number in a box can be the sum of any set of other numbers in the box
* Rule \#2: no number in a box can be the double of any other number in the box

This problem reminds me of the Collatz conjecture made into a game, but with more complex rules. Like the Collatz conjecture, you can modify the rules to make new puzzles. Modifying my code to handle these new rules shouldn't be extremely difficult.

My colleague got the puzzle from a friend, and I cannot find a similar puzzle, but it feels to be a mixture of the following and other things...
* [https://en.wikipedia.org/wiki/Bin_packing_problem](https://en.wikipedia.org/wiki/Bin_packing_problem)
* [https://math.stackexchange.com/questions/570428/prison-problem-locking-or-unlocking-every-nth-door-for-n-1-2-3](https://math.stackexchange.com/questions/570428/prison-problem-locking-or-unlocking-every-nth-door-for-n-1-2-3)
* [https://en.wikipedia.org/wiki/Multiway_number_partitioning](https://en.wikipedia.org/wiki/Multiway_number_partitioning)

I thank [Nacho-Meter-Stick](https://github.com/Nacho-Meter-Stick) because he did a large portion of the work...
* wrote the first (very slow) Python code that inspired me to write mine
* found various ways of greatly optimizing the data structures after I wrote mine
* simplified the firstAllowed proof
* made various optimizations for speed



# solutions

If there is one box, the optimal placement is the only placement...
```
 [1]
```
That is, 1 goes in the first (and only) box, then the 2 cannot be placed because of Rule \#2.

If there are two boxes, the optimal placement places 4...
```
 [1,4]
 [2,3]
```
The 2 must be placed in the 2nd box. If the 3 is placed in the 1st box, the 4 cannot be placed. Placing the 3 in the 2nd box allows the 4 to be placed, but then the 5 has nowhere to go.

Three boxes is an interesting problem to do by hand. The following gives the optimal solution of 13...
```
 [1, 4, 10, 13]
 [2, 3, 11, 12]
 [5, 6, 7, 8, 9]
```
There is a second equivalent optimal solution...
```
 [1, 4, 7, 10, 13]
 [2, 3, 11, 12]
 [5, 6, 8, 9]
```
There are more trivial optimal solution found by shuffling the boxes, so I am only writing the ones where the first number in each box is in increasing order. A better way to think of this is to imagine the boxes as being a set (in the mathematical and Pythonic sense), not having an order.

Four boxes is an interesting problem to do with code. The rare advanced high school student could probably write some Python code that would take many minutes of runtime (probably an hour). Using my boxes.cpp and much less than one second of runtime, the single optimal solution for 4 boxes is found to give 36...
```
 [1, 3, 8, 10, 27, 29, 34]
 [2, 5, 11, 12, 26, 32, 35, 36]
 [4, 6, 7, 9, 28, 30, 31, 33]
 [13 - 25]
```
We now see a pattern arising. At least one of the optimal solutions has the final box be what I will call a *counting box*. The 13 - 25 is all consecutive numbers starting at 13 and ending at 25. The final number in the counting box is 2 * start - 1,
and all numbers between start and the final number are allowed. 2 * start is the first to be excluded (by Rule \#2). By avoiding small numbers less than start, this counting box can obey the constraints and place a large number of numbers. The larger the starting number, the larger the number of numbers in the counting box.

Studying these patterns and trying to solve the problem for more than 4 boxes is the goal of my codes. This is a strange goal in that this box problem is not known by the world, so I don't know how anyone will ever usefully find this code, but it is a great problem requiring very fast code and mathematical proofs.




# 5 boxes with boxes.cpp

See the top of boxes.cpp for instructions on how to run the file. It has been highly optimized for speed. Although 4 boxes takes much less than 1 second, 5 boxes takes a couple days to finish its recursive (brute force) search on a single CPU core. It would take years for code that is not optimized.

I ran boxes.cpp with the proper lines uncommented so that it printed all 66 states that can result at a recursion depth of 6, meaning that there are 66 ways of placing the first 6 numbers. This runtime was 0 milliseconds.

Using these 66 states as command-line arguments, I employed 66 CPU cores (on 8 computers) in a computer lab to run (to prevent a massive amount of printing, the code was set to *not* print repeated solutions but to print progress). I initialized *best* to be 0. The process that started with 0,1,0,2,1,2 (that is, 1 in box 0, 2 in box 1, 3 in box 0, etc.) uniquely did the best; it placed up to 73. I then reran this state but allowed printing of repeated solutions, and 73 can only be obtained one way...
```
 [1, 3, 8, 10, 27, 29, 34]
 [2, 5, 11, 12, 26, 32, 35, 36]
 [4, 6, 7, 9, 28, 30, 31, 33]
 [13 - 25]
 [37 - 73]
```
We can notice that the first four boxes are filled as if they were the only 4 boxes. Because the solution with 4 boxes, after placing 36, had 86 be the next number that could be placed, I was pretty confident that the above would be the best solution, but now this 73 has been proven to be the unique best! In hindsight, I should have started running it in the printing-repeats mode, but also initializing *best* to 73 because 73 could obviously be obtained by just adding a counting box starting at 37.

I obtained my wonderful runtimes by using pruning and efficient data structures. Pruning is removing branches of the recursion tree as early as possible, which can exponentially speed up the code at the cost of a linear slowdown. Good data structures can combined speed up the code by a factor of more than 100 times!

I do two types of pruning: initial and non-initial.
* The initial pruning makes sure that "shuffling the boxes" never occurs so that trivially repeated solutions do not appear. Basically, when trying to place a number in a box, the code does not allow that number to placed in any other empty box by having the recursive function return after the first empty box.
* The non-initial pruning looks at all numbers between the current and the current best, and, if any are currently known to be unplaceable, prune!

I have three main data structures...
```
  uint8_t possibilities[maxSteps+1]
  uint64_t sums[boxNum][sumsLength]
  uint8_t boxes[maxSteps + 1]
```
where
```
  maxSteps = 3 * 2^boxNum
  sumsLength = ( maxSteps / 2^6 ) + 1
```
For 5 boxes, boxNum is 5, so maxSteps = 96 and sumsLength = 2 .
Clearly, *maxSteps* does not need to be this large since 73 is the best for 5 boxes, but *maxSteps* does not hugely affect the speed, and the formula seems generally safe. If not safe, the code will let you know that you need to increase *maxSteps*.
* *possibilities* is indexed by the integer that could be placed, and each of the bits of possibilities[n] represents a box. If the bit is 1, we have not yet ruled out placing that integer in that box. For 5 boxes, only 5 of the 8 bits are used. See my variable called *mask* to see how bits are individually accessed. The code could probably be simplified with std::bitset, though I wanted more general code that could be used for more than 64 bits, especially for *sums*, so I do bit operations by hand. *possibilities* is "shallow copied" at each step of recursion. Using *sums* only and not *possibilities* by recalculating Rule \#2 is slower than using *possibilities*.
* *sums* records the current list of sums (out to *maxSteps*) of all combinations of sums of integers in each box. When adding the next integer to a box, *sums* can be efficiently updated by adding it to each integer in *sums* rather than having to calculate all combinations again. Each bit corresponds to a sum, where a bit being 1 means that the sum can be obtained. *sums* uses the largest unsigned int, which is only 64 bits, which could only hold sums up to 63 (since the 0th bit of the first 64-bit integer is ignored), so, to store each of the *maxSteps* sums for 5 boxes, we need 2 uint64\_t integers, which is why sumsLength is 2 for 5 boxes. For example, for 5 boxes, let's look at the following box=0: [1,40,60]. Then, the sums would be 1,40,41,60,61,100,101, so sums[0] would become 0011000000000000000000110000000000000000000000000000000000000010, and the second uint64\_t could be all zeros because the 100 and 101 are larger than *maxSteps*. Note that *possibilities* is accessed via step then box, but *sums* is accessed via box then step. This is to make the code have the smallest possible runtime due to the different ways that these data structures are accessed and modified. I found that *contiguous* data in arrays was faster than worrying about using pointers for shallow copying.
* *boxes* is a single global array of length *maxSteps* + 1 made of uint8\_t integers that store the box in which each integer is placed (for 5 boxes, values in *boxes* would be 0 through 4). Then, as long as you never print beyond where you are currently trying to place a number, there is no need for any copying of this data because the next branch of the recursion can just start overwriting the data as it traverses the new branch. Note that boxes[0] stores the number of boxes used and is used for initial pruning.





# more than 5 boxes using boxesCounting.cpp


For 6 boxes, boxes.cpp takes almost a minute for progress to be made at a recursion depth of 36. This will take at least 2^36 minutes, which is over 100,000 years on a single CPU core. We need a better way! That way is having the code handle counting boxes differently.

6 boxes can be run using boxesCounting.cpp in much less than an hour if the final two boxes are assumed to be counting boxes whose minimum countStart is 27...
```
bool isCounting[boxNumAll]         = {false, false, false, false, true, true};
const uint32_t minStart[boxNumAll] = {    0,     0,     0,     0,   27,    0};
```
The same time can be achieved if the final three boxes are assumed to be counting boxes whose minStart is 8...
```
bool isCounting[boxNumAll]         = {false, false, false, true, true, true};
const uint32_t minStart[boxNumAll] = {    0,     0,     0,    8,    0,    0};
```
Results are that there are three ways to get the best of 156...
```
[1, 3, 8, 10, 27, 29, 34, 148, 150, 155]
[2, 5, 11, 12, 26, 32, 35, 36, 153, 156]
[4, 6, 7, 9, 28, 30, 31, 33, 149, 151, 152, 154]
[13 - 25]
[37 - 73]
[74 - 147]

[1, 3, 8, 10, 27, 29, 34, 148, 153, 155]
[2, 5, 11, 12, 26, 32, 35, 36, 150, 156]
[4, 6, 7, 9, 28, 30, 31, 33, 149, 151, 152, 154]
[13 - 25]
[37 - 73]
[74 - 147]

[1, 3, 8, 10, 27, 29, 34, 148, 155]
[2, 5, 11, 12, 26, 32, 35, 36, 150, 153, 156]
[4, 6, 7, 9, 28, 30, 31, 33, 149, 151, 152, 154]
[13 - 25]
[37 - 73]
[74 - 147]
```

For an even larger number of boxes, counting boxes can have numbers placed into them after the initial sequence. For a counting box with countStart = 13, 221 is the firstAllowed, and 247 is the finalExcluded. That is, once 221 is reached, it could be placed in the box, and, once 247 is reached, any larger number can be placed in the box.

For countStart > 4...
```
  finalExcluded = sum of i for i equals countStart to 2*countStart - 1
  firstAllowed = finalExcluded - 2*countStart
```
This finalExcluded formula is obvious.
This firstAllowed formula can be proven! Let's do that!

First, let's note that
```
  finalExcluded = countStart (3*countStart - 1) / 2
```
so
```
  firstAllowed = (3*countStart*countStart - 5*countStart)/2
```

Let's look at an example when countStart=5.  
1 number gives...
```
   5
   6
   7
   8
   9 = 9
```
Adding 2 numbers gives...
```
   5 + 6 = 11
   5 + 7
   5 + 8
   5 + 9
   6 + 9
   7 + 9
   8 + 9 = 17
```
Adding 3 numbers gives...
```
   5 + 6 + 7 = 18
   5 + 6 + 8
   5 + 6 + 9
   5 + 7 + 9
   5 + 8 + 9
   6 + 8 + 9
   7 + 8 + 9 = 24
```
Adding 4 numbers gives...
```
   5 + 6 + 7 + 8 = 26
   5 + 6 + 7 + 9
   5 + 6 + 8 + 9
   5 + 7 + 8 + 9
   6 + 7 + 8 + 9 = 30
```
We see that 25 and 10 cannot be achieved as sums. 10 is still excluded by the doubling rule (it is double 5), so firstAllowed = 25, which agrees with the formula.
The important thing to notice from this example is that there is a gap size between groups of sequentially excluded numbers. What I will later call gapSize(1) is 2 because 11-9=2, and what I will later call gapSize(3) is also 2 because 26-24=2. For different countStart values, these gap sizes may vary, and there will be more of them. We will see that, for gapSize(i), i will need to go from 1 to countStart-2.

Let's do the proof for any countStart > 4 keeping in mind the example to help us understand the proof.
For  finalNum = 2\*countStart - 1, we can derive the following...
```
   sumEnd(i) = sum of n for n equals finalNum - i + 1 to finalNum
             = (4 countStart - i - 1) i/2
   sumStart(i) = sum of n for n equals countStart to countStart + i
               = (1 + i) (2 countStart + i) / 2
```
where sumEnd(i) is the sum of the last i numbers in the counting box, and sumStart(i) is the sum of the smallest i+1 things in the counting box.
If the following gap size is 1 or less, you can keep sequentially excluding numbers. If it is 2 or more, you cannot exclude some numbers.
```
   gapSize(i) = sumStart(i) - sumEnd(i) = i^2 + i + (1-i) countStart
```
For i=1, we get a gap size of 2, which corresponds to  2\*countStart, which is always excluded by the doubling rule. We now have to prove that all larger i values have gaps less than 2 until the i where firstAllowed occurs.
Setting  gapSize(i)  equal to 2 gives two solutions: i = 1 and i = countStart - 2
The derivative of  gapSize(i)  at i = 1 is  3 - countStart, which is negative for countStart > 4, so we can keep excluding above  2\*countStart  until i = countStart - 2.
For  i = countStart - 2, we can calculate sumEnd(i)
```
   sumEnd(countStart - 2) = (4 countStart - (countStart - 2) - 1) (countStart - 2)/2
                          = (3 countStart^2 - 5 countStart)/2 - 1
                          = finalExcluded - 2*countStart - 1
```
Noting that
```
   sumEnd(countStart - 2) = firstAllowed - 1
```
because firstAllowed is 1 more than the lower bound of a gap of 2, we have proven that...
```
   firstAllowed = finalExcluded - 2*countStart
```
This is only true for countStart > 4 else firstAllowed can be excluded by the doubling rule (for example, for countStart=4, the formula gives firstAllowed=14, but that can be excluded by 2\*7).

After firstAllowed, there is consecutive band of allowed numbers of length countStart-1.
This exists immediately below finalExcluded.
Immediately above finalExcluded, you can place a consecutive band of numbers of length countStart.

The best I have found...
```
boxes  __best found__
 7        328
 8        660
 9        1328
 10       2694
 11       5403
 12       10892
 13       21785
 14       43733
 15       87483
 16       175340
```
As the number of boxes increases, my certainty that it is the actual best decreases.

One of the solutions for 16 boxes that gives 175340 has the following form for its final boxes...
```
[13 - 25, 314 - 326, 5391 - 5403, 87469 - 87481]
[37 - 73, 2658 - 2694, 174964 - 175000]
[74 - 147, 10808 - 10881]
[157 - 313, 43577 - 43733]
[329 - 657, 175001 - 175329]
[659 - 1317]
[1329 - 2657]
[2695 - 5389]
[5404 - 10807]
[10893 - 21785]
[21786 - 43571]
[43734 - 87467]
[87482 - 174963]
```
To achieve this, I set the (sometimes bad) assumption in the code to always have subsequent filling of a counting box be the length of the original countStart for that box.

The optimal countStart values can change. For 8 boxes, the best solutions I found had the following final boxes...
```
[13 - 25, 312 - 324]
[37 - 73]
[74 - 147]
[156 - 311]
[325 - 649]
```
Note that the 156 and 325 are not the same countStart values as with other number of boxes.




# something interesting

The patterns formed when printing the boxes in which the integers are placed are surprising!
```
1 box:   0
2 boxes: 0110
3 boxes: 0110222220110  or  0110220220110
4 boxes: 010212202011333333333333310202212011
5 boxes: 0102122020113333333333333102022120114444444444444444444444444444444444444
```
If *boxNum* is less than 4, the result is perfectly symmetric. If *boxNum* equals 4, it is *almost* symmetric. Is this anything meaningful?

What other interesting patterns can you find? What if you changed Rule \#1 or Rule \#2? Maybe some patterns will be useful for solving this placing-consecutive-numbers-in-boxes puzzle?



