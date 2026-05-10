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
  - `__restrict__` : Two values do not overlap
  - `collapse(2)` : Tells the compiler that the next two loops can be parallelised
  - `schedule(static)`

  // #framed(title: "Key takeaway")[
  //   Transpose + tilling together bring the matmul close to the *roofline bound*.
  // ]
  #figure(image("pics/code1.png"))
]

#slide(title: "Operation times — matmul")[
  #cols(columns: (1fr, 1fr), gutter: 1cm)[
    *Server (with tiling)*
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

#slide(title: "Tiling — principle")[
  #cols(columns: (1fr, 1.5fr), gutter: 0.8cm)[
    - Splits the computation into #stress[blocks (tiles)] that fit in L2 cache
    - The tile of *A* stays in cache for the entire `jj` loop
    - Read `C/TILE_J` times *from cache*, not from RAM
    - ↑ arithmetic intensity → approche la limite roofline

    
  ][
    
    #text(size: 8.5pt, fill: gray)[8×8 matrix, proportional to TILE 64/64/32]
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
      #framed(title: "Chosen tile sizes")[
        - *My PC:* `TILE_I = TILE_J = TILE_K = 64`
        - *Server:* `TILE_I = TILE_J = 64`, `TILE_K = 32`
      ]
    
  ][
      #figure(image("pics/lstopo.png" , width: 70%))
      - $64 times 64 times 8 = 32 "KB"$
    
  ]
    
    

]


#slide(title: "Without tiling — my PC")[
  #cols(columns: (1fr, 1.4fr), gutter: 1cm)[
    - For *small matrices*: thread creation time dominates
    - Saturation between 4 and 8 threads (number of physical cores)
    - Sub-linear speedup due to memory contention
  ][
    #figure(image("pics/mesures_perso_threads_my_pc_previous_heatmap_matmul.svg",
                   width: 110%, format: "svg"))
  ]
]

#slide(title: "Without tiling — server")[
  #cols(columns: (1fr, 1fr), gutter: 0.6cm)[
    #figure(image("pics/mesures_perso_threads_on_server_previous_imp_heatmap_matmul.svg",
                   format: "svg"))
  ][
    #figure(image("pics/mesures_perso_threads_on_server_previous_imp_speedup_matmul.svg",
                   format: "svg"))
  ]
]

#slide(title: "With tiling — my PC")[
  #cols(columns: (1fr, 1.4fr), gutter: 1cm)[
    // - Roofline model: *memory bandwidth* bottleneck
    // - Tiling saturates BW → little gain with more threads

    // #framed[Bottleneck shifts from *compute* to *memory*.]
    - Improved execution time
  ][
    #figure(image("pics/mesures_perso_threads__my_pc_tilling_test_heatmap_matmul.svg",
                   width: 110%, format: "svg"))
  ]
]



#slide(title: "With tiling — server")[
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

#slide(title: "Absolute time: compute & comm vs size (log-log)")[
  #figure(image("pics/mesures_perso_node_time_vs_size.svg", width: 95%, format: "svg"))
]

#slide(title: "Speedup breakdown — compute / comm / total")[
  #figure(image("pics/mesures_perso_node_speedup_breakdown.svg", width: 95%, format: "svg"))
]

#slide(title: "Communication overhead")[
  #cols(columns: (1.1fr, 1fr), gutter: 1cm)[
    #figure(image("pics/mesures_perso_node_comm_fraction.svg", format: "svg"))
  ][
    - For *small matrices*, communication dominates
    - For *large matrices*, compute becomes dominant again
    - Speedup close to ideal for large sizes

    #framed(title: "MPI Conclusion")[
      Good compute speedup, limited by Allreduce for small sizes.
    ]
  ]
]

// ────────────────────────────────────────────────────────────
// PART 4 — GPU Matrix Operations (OpenCL)
// ────────────────────────────────────────────────────────────
#title-slide[Part 4 : GPU Matrix Operations (OpenCL)]

#slide(title: "OpenCL — Result fast implementation")[
    #figure(image("pics/mesures_perso_opencl_fast_loglog.svg" , width: 100%) )
  
]

#slide(title: "OpenCL — Result Naive implementation")[
    #figure(image("pics/mesures_perso_opencl_naive_loglog.svg" , width: 100%) )
  
]

#slide(title: "OpenCL — Energy consumption ")[
    #figure(image("pics/part4_energy_breakdown.svg" , width: 100%) )
  
]

#slide(title: "OpenCL — Energy consumption ")[
    #figure(image("pics/part4_energy_co2.svg" , width: 100%) )
  
]

#slide(title: "OpenCL — Code Fast ")[
    #figure(image("pics/kernel_fast.png" , width: 42%) )
  
]


#slide(title: "OpenCL — Code naive ")[
    #figure(image("pics/kernel_slow.png" , width: 100%) )
  
]

#slide(title: "OpenCL — Main function ")[
    #figure(image("pics/kernel_imp.png" , width: 100%) )
  
]








