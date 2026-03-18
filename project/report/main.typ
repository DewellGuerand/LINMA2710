#set page(numbering: "-1-")
#import "@preview/showybox:2.0.4": showybox


= Introduction 

Pourquoi ```bash march=native ``` produit plus d'erreur car il s'emmele avec les autres paramètres. 
#showybox(
  [Assume I use the copy constructor Matrix(const Matrix& other) to copy a matrix. Then, I modify an element of the copied matrix. What happens to the original matrix?]
)
Nothing because of the fact that we call ```C this->data = other.data ``` we call the copy constructor of ```C std::vector``` which allocate a new buffer and copy all the element 



#showybox(
    [How would you handle special cases like sparse matrices?
]
)
See analyse numérique were we have sort of storage for 0 entries 


#showybox([
Explain why the Matrix class does not need an explicitly defined destructor ~Matrix().
])
THe only ressources is ```C std::vector<double> data ```
When out of scope it is automatically destroy leave {} for instance. See OZ. 


Can you speed up matrix operations using SIMD instructions? Measure the speedup compared to the non-SIMD version. 

Upgrade : 
- Added some -Flags and compare them 
- Used memory next to each other in multiplication 


Question : TAkes a lot of time to produce number but roughly the same 
SHould I not recompile at each time the makefile 