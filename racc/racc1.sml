datatype OP = plus of unit | times of unit

datatype Coinflip = heads of unit | tails of unit

type Node = OP * Coinflip * int * int

datatype Tree = empty | leaf of (int) | inner of (Tree * Node * Tree)

fun solve (empty : Tree) : int = 0
  | solve (leaf num) = num
  | solve T = solve (roc T)

fun roc T

