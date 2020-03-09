\page tools Tools

In the \c tools directory, tdc provides several small applications that follow the Unix philosophy of doing one thing and doing it well.

\section permutation permutation

Draws numbers from a random permutation and prints them to the standard output.
Since we draw from a permutation, no number will occur more than once.
The distribution of the drawn numbers is near-uniform, however, there is no strict guarantee for this.

Consider the following example, where we draw 10 random numbers between 0 and 100:
\code{.sh}
$ tools/permutation -u 100 -n 10
42
32
73
46
57
22
82
95
17
65
\endcode

The output is \em not sorted.
To sort it, pipe it into `sort -n`.
