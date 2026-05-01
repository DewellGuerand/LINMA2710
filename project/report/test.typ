#set page(width: auto, height: auto, margin: 1cm)

// Matrice 8x8, TILE_I=2, TILE_J=2, TILE_K=4
// Proportions réelles : 8/2 = 4 tiles par dimension (comme 256/64 = 4)

#let N = 8   // taille matrice (représente R=K=C)
#let TI = 2  // représente TILE_I = 64
#let TJ = 2  // représente TILE_J = 64
#let TK = 4  // représente TILE_K = 128

#let draw-matrix(n-rows, n-cols, tile-row, tile-col, tile-rows, tile-cols, color: blue) = {
  let cells = ()
  for i in range(n-rows) {
    for j in range(n-cols) {
      let in-tile = (
        i >= tile-row and i < tile-row + tile-rows and
        j >= tile-col and j < tile-col + tile-cols
      )
      if in-tile {
        cells.push(
          table.cell(fill: color.lighten(50%))[
            #text(fill: color.darken(20%), weight: "bold", size: 7pt)[✕]
          ]
        )
      } else {
        cells.push(
          table.cell(fill: luma(235))[
            #text(fill: luma(160), size: 7pt)[·]
          ]
        )
      }
    }
  }
  table(
    columns: (0.45cm,) * n-cols,
    rows:    (0.45cm,) * n-rows,
    align: center + horizon,
    stroke: luma(190) + 0.5pt,
    inset: 0pt,
    ..cells
  )
}

= Tiling — visualisation avec matrice 8×8
#text(gray, size: 9pt)[Proportionnel à ton code : TILE\_I=64, TILE\_J=64, TILE\_K=128 sur matrice R×K×C]

#v(0.4cm)

// Boucle ii=0, jj=0 : premier tile de C
== Itération ii=0, jj=0, kk=0

#text(size: 9pt)[
  On calcule le coin haut-gauche de *C* (i∈\[0,TI\[, j∈\[0,TJ\[)
  en utilisant k∈\[0,TK\[ de *A* et *BT*.
]
#v(0.2cm)

#grid(
  columns: (auto, 0.6cm, auto, 0.6cm, auto),
  align: horizon + center,
  
  [*A* #text(size:8pt)[(R×K)]\
   #v(0.15cm)
   #draw-matrix(N, N, 0, 0, TI, TK, color: blue)],

  [$ times $],

  [*B#super[T]* #text(size:8pt)[(C×K)]\
   #v(0.15cm)
   #draw-matrix(N, N, 0, 0, TJ, TK, color: green)],

  [$ → $],

  [*C* #text(size:8pt)[(R×C)]\
   #v(0.15cm)
   #draw-matrix(N, N, 0, 0, TI, TJ, color: red)],
)

#v(0.5cm)

// kk=4 : même tile de C, deuxième tranche de K
== Itération ii=0, jj=0, kk=4 (accumulation)

#text(size: 9pt)[
  *Même case de C*, on accumule avec la tranche suivante de K.
  C\[0,0\] += A\[0, kk:kk+TK\] · BT\[0, kk:kk+TK\]
]
#v(0.2cm)

#grid(
  columns: (auto, 0.6cm, auto, 0.6cm, auto),
  align: horizon + center,

  [*A*\
   #v(0.15cm)
   #draw-matrix(N, N, 0, TK, TI, TK, color: blue)],

  [$ times $],

  [*B#super[T]*\
   #v(0.15cm)
   #draw-matrix(N, N, 0, TK, TJ, TK, color: green)],

  [$ += $],

  [*C*\
   #v(0.15cm)
   #draw-matrix(N, N, 0, 0, TI, TJ, color: red)],
)

#v(0.5cm)

// ii=0, jj=2 : tile suivant de C
== Itération ii=0, jj=2 (tile suivant de C)

#text(size: 9pt)[
  On passe au tile suivant de *C* (j∈\[TJ, 2·TJ\[), on relit A depuis kk=0.
]
#v(0.2cm)

#grid(
  columns: (auto, 0.6cm, auto, 0.6cm, auto),
  align: horizon + center,

  [*A*\
   #v(0.15cm)
   #draw-matrix(N, N, 0, 0, TI, TK, color: blue)],

  [$ times $],

  [*B#super[T]*\
   #v(0.15cm)
   #draw-matrix(N, N, TJ, 0, TJ, TK, color: green)],

  [$ → $],

  [*C*\
   #v(0.15cm)
   #draw-matrix(N, N, 0, TJ, TI, TJ, color: red)],
)

#v(0.5cm)

#rect(fill: blue.lighten(88%), stroke: blue.lighten(40%), radius: 4pt, inset: 10pt)[
  *Clé du tiling :* le tile de *A* (bleu) reste en cache L2 pendant toute 
  la boucle `jj` — il est relu `C/TILE\_J` fois depuis le cache, 
  pas depuis la RAM. C'est ça qui augmente l'arithmetic intensity.
]