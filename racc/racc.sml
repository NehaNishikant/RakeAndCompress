val r = Random.rand (123124, 192423985)

datatype OP = PLUS | TIMES

datatype Coinflip = HEADS | TAILS

datatype Tree =  leaf of int
                | half of (int * int * Tree)
                | full of (OP * Tree * Tree)

datatype CTree = cleaf of int
               | chalf of (int * int * CTree * Coinflip)
               | cfull of (OP * CTree * CTree * Coinflip)

fun rakehelp (leaf _ : Tree) : Tree = raise Fail "not possible"
  | rakehelp (full (PLUS, leaf n, leaf m) : Tree) : Tree = leaf (n+m)
  | rakehelp (full (TIMES, leaf n, leaf m)) = leaf (n*m)
  | rakehelp (full (PLUS, leaf n, t)) = half(1, n, t)
  | rakehelp (full (PLUS, t, leaf n)) = half(1, n, t)
  | rakehelp (full (TIMES, leaf n, t)) = half(n, 0, t)
  | rakehelp (full (TIMES, t, leaf n)) = half(n, 0, t)
  | rakehelp (half (a, b, leaf n)) = leaf (a*n+b)
  | rakehelp (full (oper, t1, t2)) =
    (let
      val (t1r, t2r) = Primitives.par (fn ()=> rakehelp t1, fn () => rakehelp t2)
    in
      full(oper, t1r, t2r)
    end)
  | rakehelp (half (a, b, t)) = half(a, b, rakehelp t)

fun assigncoinflips (leaf n : Tree) : CTree = cleaf n
  | assigncoinflips (half (a, b, t)) =
    (let
      val flip = HEADS (*(case Random.randRange (0,1) r of 1 => HEADS | 0 =>
      TAILS)*)
    in
      chalf (a, b, assigncoinflips t, flip)
    end)
  | assigncoinflips (full (oper, t1, t2)) =
    (let
      val flip = HEADS (*case Random.randRange (0,1) r of 1=> HEADS | 0=>TAILS*)
    in
      cfull (oper, assigncoinflips t1, assigncoinflips t2, flip)
    end)

fun compresshelp (cleaf num : CTree) : Tree = leaf num
  | compresshelp (cfull (oper, t1, t2, _) : CTree) : Tree =
    (let
        val (t1c, t2c) = Primitives.par (fn () => compresshelp t1, fn () => compresshelp t2)
    in
        full (oper, t1c, t2c)
    end)
  | compresshelp (T as chalf (a, b, cleaf n, _)) = half (a, b, leaf n)
  | compresshelp (chalf (a, b, cfull x, _)) = half (a, b, compresshelp(cfull x))
  | compresshelp (chalf (a, b, t, HEADS)) = half (a, b, compresshelp t)
  | compresshelp (chalf (a, b, t as chalf(_, _, _, TAILS), TAILS)) = half (a, b,
  compresshelp t)
  | compresshelp (chalf (a, b, chalf(a2, b2, t, HEADS), TAILS)) =
    (case t of
         t2 as cfull (_, _, _, TAILS) => half (a * a2, a * b2 + b, compresshelp
         t2)
       | t2 as chalf (_, _, _, TAILS) => half (a * a2, a * b2 + b, compresshelp
       t2)
       | t2 as cleaf _ => half (a * a2, a * b2 + b, compresshelp t2)
       | _ => half (a, b, half (a2, b2, compresshelp t)))

fun rake (leaf num : Tree) : int = num
  | rake T = compress (rakehelp T)

and compress (leaf num : Tree) : int = num
  | compress T = rake (compresshelp (assigncoinflips T))

val tree1 = leaf 1

val solution = rake tree1
val () = print(Int.toString solution ^ "\n")
val tree2 = leaf 2
val tree3 = full(PLUS, tree1, tree2)
val solution = rake tree3
val () = print(Int.toString solution ^ "\n")

val tree4 = leaf 3
val tree5 = full(TIMES, tree3, tree4)

val tree6 = leaf 4
val tree7 = full(PLUS, tree5, tree6)

val solution = rake tree7
val () = print(Int.toString solution ^ "\n")

val tree8 = leaf 5
val tree9 = full(TIMES, tree7, tree8)

val solution = rake tree9
val () = print(Int.toString solution ^ "\n")

val tree10 = leaf 6
val tree11 = full(PLUS, tree9, tree10)

val solution = rake tree11
val () = print(Int.toString solution ^ "\n")

val tree12 = full(PLUS, tree11, tree1)
val tree13 = full(PLUS, tree12, tree1)
val tree14 = full(PLUS, tree13, tree1)
val tree15 = full(PLUS, tree14, tree1)
val tree16 = full(PLUS, tree15, tree1)
val tree17 = full(PLUS, tree16, tree1)
val tree18 = full(PLUS, tree17, tree1)
val tree19 = full(PLUS, tree18, tree1)
val tree20 = full(PLUS, tree19, tree1)
val tree21 = full(PLUS, tree20, tree1)
val tree22 = full(PLUS, tree21, tree1)
val tree23 = full(PLUS, tree22, tree1)
val tree24 = full(PLUS, tree23, tree1)
val tree25 = full(PLUS, tree24, tree1)

val solution = rake tree25
val () = print(Int.toString solution ^ "\n")

(* depth 20 tree *)

(*val r = Random.rand (123, 455)
fun exp2 0 = 1
  | exp2 n =
      let
        val x = Random.randRange (0, 1) r
        val (a, b) = Primitives.par (fn () => exp2 (n - 1), fn () => exp2 (n -
        1))
      in
        a + b + x
      end

val () = print ("here is your answer: " ^ Int.toString (exp2 5))*)

fun exp2 0 = 1
  | exp2 n =
      let
        val (a, b) = Primitives.par(fn () => exp2 (n - 1), fn () => exp2 (n - 1))
      in
        a + b
      end
val () = print ("bye")

(*val () = print ("here is your answer: " ^ Int.toString (exp2 5))*)
