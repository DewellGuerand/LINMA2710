#import "@preview/typslides:1.3.3": *

#show: typslides.with(
  ratio: "16-9",
  theme: "bluey",
  font: "Fira Sans",
  font-size: 20pt,
  link-style: "color",
  show-progress: true,
)

#front-slide(
  title: "Presentation Scientific computing",
  subtitle: [LINMA2710],
  authors: "D. Guerand",
)

#table-of-contents()

// ────────────────────────────────────────────────────────────
// PART 1 — Basic matrix operations and SIMD
// ────────────────────────────────────────────────────────────
#title-slide[Part 1 : Basic matrix operations and SIMD]

#slide(title: "Row-major storage & optimisations")[
  - Matrices stored in *row-major* order → transposing #stress[B#super[T]] before multiply avoids column-stride cache misses
  - *Tilling* splits the computation into small blocks that fit in L1/L2 cache (detailed in Part 2)
  - Compiler flags: `O1`, `O2`, `O3 -march=native` enable auto-vectorisation (SIMD)
  - Unrolling and helping for vectorisation 
  - `__restrict__` : Deux valeurs ne se chevauchent pas 
  - `collapse(2)` : Permet de dire au thread qu'on peut paralleliser les deux prochaines boucles 
  - `schedule(static)`

  // #framed(title: "Key takeaway")[
  //   Transpose + tilling together bring the matmul close to the *roofline bound*.
  // ]
  #figure(image("pics/code1.png"))
]

#slide(title: "Operation times — matmul")[
  #cols(columns: (1fr, 1fr), gutter: 1cm)[
    *Server (avec tilling)*
    #figure(image("pics/mesures_perso_on_server_tilling_matmul.svg", format: "svg"))
  ][
    *My PC*
    #figure(image("pics/mesures_perso_my_pc_matmul.svg", format: "svg"))
  ]
]

// ────────────────────────────────────────────────────────────
// PART 2 — OpenMP
// ────────────────────────────────────────────────────────────
#title-slide[Part 2 : OpenMP]

// draw-matrix helper (from test.typ — proportional to TILE 64/64/128)
#let draw-matrix(n-rows, n-cols, tile-row, tile-col, tile-rows, tile-cols, color: blue) = {
  let cells = ()
  for i in range(n-rows) {
    for j in range(n-cols) {
      let in-tile = (
        i >= tile-row and i < tile-row + tile-rows and
        j >= tile-col and j < tile-col + tile-cols
      )
      if in-tile {
        cells.push(table.cell(fill: color.lighten(50%))[
          #text(fill: color.darken(20%), weight: "bold", size: 6pt)[✕]
        ])
      } else {
        cells.push(table.cell(fill: luma(235))[
          #text(fill: luma(160), size: 6pt)[·]
        ])
      }
    }
  }
  table(
    columns: (0.38cm,) * n-cols,
    rows:    (0.38cm,) * n-rows,
    align: center + horizon,
    stroke: luma(190) + 0.5pt,
    inset: 0pt,
    ..cells
  )
}

#slide(title: "Tilling — principe")[
  #cols(columns: (1fr, 1.5fr), gutter: 0.8cm)[
    - Découpe le calcul en #stress[blocs (tiles)] qui tiennent en cache L2
    - Le tile de *A* reste en cache pendant toute la boucle `jj`
    - Relu `C/TILE_J` fois *depuis le cache*, pas depuis la RAM
    - ↑ arithmetic intensity → approche la limite roofline

    
  ][
    
    #text(size: 8.5pt, fill: gray)[Matrice 8×8, proportionnel à TILE 64/64/32]
    #v(0.15cm)
    #v(0.1cm)
    #grid(columns: (auto, 0.45cm, auto, 0.45cm, auto), align: horizon + center,
      [*A* #v(0.1cm) #draw-matrix(8,8, 0,0, 2,4, color: blue)],   [#text(size:14pt)[×]],
      [*B#super[T]* #v(0.1cm) #draw-matrix(8,8, 0,0, 2,4, color: green)], [#text(size:14pt)[→]],
      [*C* #v(0.1cm) #draw-matrix(8,8, 0,0, 2,2, color: red)],
    )
    
    
  ]
]
#slide(title: "Tilling — continued")[
  #cols(columns: (1fr, 1.5fr), gutter: 0.8cm)[
      #framed(title: "Tailles de tiles choisies")[
        - *Mon PC :* `TILE_I = TILE_J = TILE_K = 64`
        - *Serveur :* `TILE_I = TILE_J = 64`, `TILE_K = 32`
      ]
    
  ][
      #figure(image("pics/lstopo.png" , width: 70%))
      - $64 times 64 times 8 = 32 "KB"$
    
  ]
    
    

]


#slide(title: "Sans tilling — my PC")[
  #cols(columns: (1fr, 1.4fr), gutter: 1cm)[
    - Pour les *petites matrices* : le temps de création des threads domine
    - Saturation entre 4 et 8 threads (nb de cœurs physiques)
    - Speedup sous-linéaire dû aux contentions mémoire
  ][
    #figure(image("pics/mesures_perso_threads_my_pc_previous_heatmap_matmul.svg",
                   width: 110%, format: "svg"))
  ]
]

#slide(title: "Sans tilling — serveur")[
  #cols(columns: (1fr, 1fr), gutter: 0.6cm)[
    #figure(image("pics/mesures_perso_threads_on_server_previous_imp_heatmap_matmul.svg",
                   format: "svg"))
  ][
    #figure(image("pics/mesures_perso_threads_on_server_previous_imp_speedup_matmul.svg",
                   format: "svg"))
  ]
]

#slide(title: "Avec tilling — my PC")[
  #cols(columns: (1fr, 1.4fr), gutter: 1cm)[
    // - Modèle roofline : limitation de la *bande passante mémoire*
    // - Le tilling sature la BW → peu de gain avec plus de threads

    // #framed[Le bottleneck passe de *compute* à *mémoire*.]
    - Amélioration du temps
  ][
    #figure(image("pics/mesures_perso_threads__my_pc_tilling_test_heatmap_matmul.svg",
                   width: 110%, format: "svg"))
  ]
]



#slide(title: "Avec tilling — serveur")[
  #cols(columns: (1fr, 1fr), gutter: 0.6cm)[
    #figure(image("pics/mesures_perso_threads__on_server_tilling_heatmap_matmul.svg",
                   format: "svg"))
  ][
    #figure(image("pics/mesures_perso_threads__on_server_tilling_speedup_matmul.svg",
                   format: "svg"))
  ]
]

// ────────────────────────────────────────────────────────────
// PART 3 — Distributed matrix operations (MPI)
// ────────────────────────────────────────────────────────────
#title-slide[Part 3 : Distributed matrix operations (MPI)]

#slide(title: "Temps absolu : compute & comm vs taille (log-log)")[
  #figure(image("pics/mesures_perso_node_time_vs_size.svg", width: 95%, format: "svg"))
]

#slide(title: "Speedup breakdown — compute / comm / total")[
  #figure(image("pics/mesures_perso_node_speedup_breakdown.svg", width: 95%, format: "svg"))
]

#slide(title: "Communication overhead")[
  #cols(columns: (1.1fr, 1fr), gutter: 1cm)[
    #figure(image("pics/mesures_perso_node_comm_fraction.svg", format: "svg"))
  ][
    - Pour les *petites matrices*, la communication domine
    - Pour les *grandes matrices*, le compute redevient dominant
    - Speedup proche de l'idéal pour les grandes tailles

    #framed(title: "Conclusion MPI")[
      Bon speedup en compute, limité par l'Allreduce pour les petites tailles.
    ]
  ]
]

// ────────────────────────────────────────────────────────────
// PART 4 — GPU Matrix Operations (OpenCL)
// ────────────────────────────────────────────────────────────
#title-slide[Part 4 : GPU Matrix Operations (OpenCL)]

#slide(title: "OpenCL — résultats")[
  - À compléter
]
