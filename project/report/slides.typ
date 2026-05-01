#import "@preview/typslides:1.3.3": *

// Project configuration
#show: typslides.with(
  ratio: "16-9",
  theme: "bluey",
  font: "Fira Sans",
  font-size: 20pt,
  link-style: "color",
  show-progress: true,
)

// The front slide is the first slide of your presentation
#front-slide(
  title: "Presentation Scientific computing",
  subtitle: [LINMA2710],
  authors: "D. Guerand",
)

// Custom outline
#table-of-contents()

// Title slides create new sections
#title-slide[
  Part 1 : Basic matrix operation and SIMD
]

// A simple slide


#slide[
  - As the matrix is stored in row major we transpose to optimise for cach miss. 
  - We also do tilling to have a more efficient memory access (see later on)
]

#slide[
  - Visualisation of the time taken for each operation with each flags : 
  - Computation made on the cluster 
  #figure(
    image("pics/plot_matmul_server_tilling.png")
  )
  #figure(
    image("pics/plot_matmul_server_tilling.png") // NOrmally flags on my computer gota relaunch to highlight the difference when better CPU
  )
]

#title-slide[
  Part 2 : OpenMP
]
#slide[
  #columns(2, gutter : 1cm)[
      Plot of computation time VS size of matrix VS number of threads *sans tilling* on my pc : 
      #colbreak()
      #figure(
    image("pics/heatmap_matmul_previous_my_pc.png" , width : 120% ) // CLearly see the diminution of time AND the time to start the thread 
  )
  ]
]
#slide[
  #columns(2, gutter : 1cm)[
      Plot of computation time VS size of matrix VS number of threads *sans tilling* on the server : 
      #colbreak()
      #figure(
    image("pics/heatmap_matmul_previous_server.png" , width : 120% ) // CLearly see the diminution of time but no time to begin 
  )
  ]

  
]
#slide[
  #columns(2, gutter : 1cm)[
      Plot of computation time VS size of matrix VS number of threads *avec tilling* : 
      #colbreak()
      #figure(
    image("pics/heatmap_matmul_tilling.png" , width : 120% ) // DOes not have a diminution of time 
  )
  ]

  
]

#title-slide[
  Part 3 : Distributed matrix operations (MPI)
]

#slide[

]
#title-slide[
  Part 4 : GPU Matrix Operations (OpenCL)
]



#slide[
  - This is a simple `slide` with no title.
  - #stress("Bold and coloured") text by using `#stress(text)`.
  - Sample link: #link("typst.app").
    - Link styling using `link-style`: `"color"`, `"underline"`, `"both"`
  - Font selection using `font: "Fira Sans"`, `size: 21pt`.

  #framed[This text has been written using `#framed(text)`. The background color of the box is customisable.]

  #framed(title: "Frame with title")[This text has been written using `#framed(title:"Frame with title")[text]`.]
]

// Focus slide
#focus-slide[
  This is an auto-resized _focus slide_.
]

// Blank slide
#blank-slide[
  - This is a `#blank-slide`.

  - Available #stress[themes]#footnote[Use them as *color* functions! e.g., `#reddy("your text")`]:

  #framed(back-color: white)[
    #bluey("bluey"), #reddy("reddy"), #greeny("greeny"), #yelly("yelly"), #purply("purply"), #dusky("dusky"), darky.
  ]

  // #show: typslides.with(
  //   ratio: "16-9",
  //   theme: "bluey",
  //   ...
  // )
  

  - Or just use *your own theme color*:
    - `theme: rgb("30500B")`
]

// Slide with title
#slide(title: "Outlined slide", outlined: true)[
  - Check out the *progress bar* at the bottom of the slide.

    #h(1cm) `show-progress: true`

  - Outline slides with `outlined: true`.

  #grayed([This is a `#grayed` text. Useful for equations.])
  #grayed($ P_t = alpha - 1 / (sqrt(x) + f(y)) $)

]

// Columns
#slide(title: "Columns")[

  #cols(columns: (2fr, 1fr, 2fr), gutter: 2em)[
    #grayed[Columns can be included using `#cols[...][...]`]
  ][
    #grayed[And this is]
  ][
    #grayed[an example.]
  ]

  - Custom spacing: `#cols(columns: (2fr, 1fr, 2fr), gutter: 2em)[...]`

  - Sample references: .
    - Add a #stress[bibliography slide]...

    1. `#let bib = bibliography("you_bibliography_file.bib")`
    2. `#bibliography-slide(bib)`
]

// Bibliography
/* #let bib = bibliography("bibliography.bib")
#bibliography-slide(bib) */