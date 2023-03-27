# Presentations

- [*"Basic usage of PMRs for better performance"*](./Basic&#32;usage&#32;of&#32;PMRs&#32;for&#32;better&#32;performance.pdf), Meeting C++ Conference 2022, 17-19.11.2022
<br/><br/>
  This presentation is targeted **at beginner and intermediate programmers**. 
  It starts with explanation of the role of memory allocators and the design of Allocators in STL.
  Then it discusses what problems are PMRs supposed to solve and how they are implemented in C++17's standard library.
  After thet **we show 5 scenarios** where usage of PMRs is likely to improve performance and explain how this can be done.
  At last we discuss tuining and customization techniques in *jemalloc* and compare them to PMR techniques.

  example code: [*"PmrTests-2022.cpp"*](./PmrTests-2022.cpp) - tests using buffer traces and debug allocator outputs(!)


- [*"Two Advanced PMR Techniques in C++17-20"*](./Two&#32;advanced&#32;PMR&#32;techniques.pdf), Meeting C++ Conference 2021, 10-12.11.2021
<br/><br/>
  This presentation goes deeper on the two advanced techniques mentioned in the preceeding presentation.
  It starts with explanation of the role of Allocators in STL design and the flaws of the original design.
  Then it discusses what problems are PMRs supposed to solve and how they are implemented in C++17/20.
  At last two advanced PMR techniques are introduced: **wink-out and localized GC in graphs**. 
  We show their implementation in the original **Bloomberg library**, then in C++17 and lastly in C++20. 
  We discuss **the UB question** and the interplay between PMRs and smart pointers.

  example code: also [*"PmrTests.cpp"*](./PmrTests.cpp)


- [*"PMRs for Performance in C++17-20"*](./PMRs&#32;for&#32;performance.pdf), C++Italy 2021 - Italian C++ Conference 2021, 19.06.2021
<br/><br/>
  This presentation explains usage of PMRs (polymorfic memory resources) in STL.
  It starts with discussion of **Allocator design evolution in STL** - how it was, what were the original flaws and how they were fixed in Bl C++11.
  Then we show **how PMRs fix the remaining Allocator flaws** in C++17 and have a little overview of changes done in C++20.
  At last we introduce two advanced techniques enabled by PMRs: wink-out and localized GC in graphs.

  example code: [*"PmrTests.cpp"*](./PmrTests.cpp)


- [*"EMMA Software Architecture Pattern for Embedded Systems"*](./EMMA&#32;architectural&#32;pattern.pdf), emBO++ 2021 | the embedded c++ conference in Bochum, 25.-27.03.2021 
<br/><br/>
  In this presentation I discuss **a software architecture pattern** I learned from one of my clients.
  The EMMA architecture was successfully used in embedded devices at my client's, it follows an interesting idea and it wasn't published outside of the company before.
  So I secured an OK from my client and presented it for the general public!

