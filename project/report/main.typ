#set page(numbering: "-1-")
#import "@preview/showybox:2.0.4": showybox


= Part 1 

Pourquoi ```bash march=native ``` produit plus d'erreur car il s'emmele avec les autres paramètres. 
#showybox(
  [Assume I use the copy constructor Matrix(const Matrix& other) to copy a matrix. Then, I modify an element of the copied matrix. What happens to the original matrix?]
)
Nothing because of the fact that we call ```C this->data = other.data ``` we call the copy constructor of ```C std::vector``` which allocate a new buffer and copy all the element. 

_Proof_ : 

#image("pics/proof_12.png")
#image("pics/proof_11.png")





#showybox(
    [How would you handle special cases like sparse matrices?
]
)
See analyse numérique were we have sort of storage for 0 entries. 
CSR , CRR 
#image("pics/CSR.png" , width: 100%)


#showybox([
Explain why the Matrix class does not need an explicitly defined destructor ~Matrix().
])
THe only ressources is ```C std::vector<double> data ```
When out of scope it is automatically destroy leave {} for instance. RAII = Resource Acquisition Is Initialization. Principle in which : 
On acquiert la ressource dans le constructeur
  - On libère la ressource dans le destructeur                                                                
  - Quand l'objet sort du scope → destructeur appelé automatiquement → ressource libérée


#showybox([
Can you speed up matrix operations using SIMD instructions? Measure the speedup compared to the non-SIMD version. 

])

Upgrade : 
- Added some -Flags and compare them 
- Used memory next to each other in multiplication 


Question : TAkes a lot of time to produce number but roughly the same 
SHould I not recompile at each time the makefile 


= Part 2 
#showybox([
What is the speedup you observe when using OpenMP for matrix multiplication? How does it vary with matrix size and number of threads?
])
Directly see the Heatmap that I have represented for each operations

#showybox([
Explain Amdahl's law and how it applies to the parallelization of matrix multiplication.
])


= Loi d'Amdahl

== Intuition

La loi d'Amdahl répond à une question fondamentale : *quel est le gain maximal qu'on peut obtenir en parallélisant un programme ?*

Tout programme contient deux types de parties :
- une partie *séquentielle* qui ne peut pas être parallélisée,
- une partie *parallélisable* que l'on peut distribuer sur plusieurs processeurs.

Même avec une infinité de processeurs, la partie séquentielle reste un goulot d'étranglement incompressible.

== Définitions

Soit $T(p)$ le temps d'exécution avec $p$ processeurs.

Le *speed-up* mesure combien de fois le programme est plus rapide avec $p$ processeurs :

$ S(p) = T(1) / T(p) $

L'*efficacité* mesure dans quelle mesure les processeurs sont bien utilisés :

$ E(p) = S(p) / p $

On distingue trois cas :
- $E > 1$ : impossible en théorie,
- $E = 1$ : idéal (utilisation parfaite),
- $E < 1$ : réaliste (overhead, synchronisation, partie séquentielle...).

== Énoncé de la loi

Notons $s$ la fraction *séquentielle* du travail total, et $(1 - s)$ la fraction parallélisable. Le temps d'exécution avec $p$ processeurs vaut :

$ T(p) = s dot T(1) + (1 - s) / p dot T(1) $

Ce qui donne le speed-up :

$ S(p) = 1 / (s + (1 - s) / p) $

Et lorsque $p -> infinity$ :

$ S(infinity) = 1 / s $

== Conséquence clé

Même une toute petite fraction séquentielle plafonne drastiquement le speed-up maximal atteignable :

#table(
  columns: (auto, auto),
  stroke: 0.5pt,
  fill: (col, row) => if row == 0 { luma(220) } else { white },
  [*Fraction séquentielle $s$*], [*Speed-up maximal $S(infinity)$*],
  [50%], [×2],
  [10%], [×10],
  [1%],  [×100],
)

#v(0.5em)

#block(
  fill: luma(240),
  inset: 10pt,
  radius: 4pt,
  [
    *Conclusion :* L'effort de parallélisation doit se concentrer sur les parties les plus coûteuses du code. Paralléliser une portion minoritaire du programme n'apporte presque aucun bénéfice global.
  ]
)

== Application à la somme parallèle

Dans l'exemple du notebook, avec un vecteur de taille $n$ et un facteur 2 :
- La *première somme parallèle* effectue $n$ opérations → parallélisable,
- Les *sommes de réduction* suivantes effectuent chacune une seule opération → quasi-séquentielle.

Lorsque $p > n$, l'algorithme ne peut plus utiliser tous les processeurs : le speed-up est plafonné à $S(n)$, indépendamment du nombre de cœurs disponibles.

#line(length: 100%)

= Parallélisation de la multiplication matricielle

== Rappel : multiplication matricielle

Pour $C = A times B$ avec des matrices $n times n$, chaque entrée de $C$ est définie par :

$ C_(i j) = sum_(k=1)^(n) A_(i k) dot B_(k j) $

Chaque entrée $C_(i j)$ nécessite $n$ multiplications et $n$ additions. Le coût total est donc $O(n^3)$ opérations.

== Pourquoi la parallélisation semble idéale

Chaque entrée $C_(i j)$ est *indépendante* de toutes les autres entrées de $C$. On peut donc distribuer le calcul des $n^2$ entrées sur autant de processeurs que l'on souhaite, sans aucune communication entre eux pendant le calcul.

La fraction séquentielle $s$ est ainsi très faible, ce qui donne un speed-up théorique excellent selon Amdahl. En théorie, la multiplication matricielle est l'un des algorithmes les plus favorables à la parallélisation.

== Les vraies difficultés en pratique

=== Problème de localité mémoire

La multiplication matricielle est souvent *bandwidth-bound* si elle est mal implémentée. En Julia, les matrices sont stockées *colonne par colonne* (column-major). Pour calculer $C_(i j)$, l'accès à $B$ se fait colonne par colonne, ce qui est *non-contigu* en mémoire :

- $A_(i, *)$ : accès contigu ✓ (une ligne entière),
- $B_(*, j)$ : accès non-contigu ✗ (sauts mémoire à chaque élément).

Chaque accès non-local à $B$ risque de provoquer un *cache miss*, forçant le processeur à aller chercher les données dans un niveau de cache plus lent ou en mémoire principale — indépendamment du nombre de cœurs utilisés.

=== Contention sur les caches partagés

Même si les calculs sont indépendants, les threads *partagent les caches L2 et L3*. Si plusieurs threads accèdent simultanément aux mêmes blocs de $B$, ils se disputent la bande passante mémoire. C'est un phénomène analogue au *false sharing* :

#block(
  fill: luma(240),
  inset: 10pt,
  radius: 4pt,
  [
    *False sharing :* lorsque deux threads modifient des variables distinctes mais situées dans le même bloc de cache, chaque modification invalide le bloc entier pour les autres cœurs, qui doivent alors le recharger depuis la mémoire.
  ]
)

=== Le modèle Roofline

L'intensité arithmétique $a$ est le rapport entre opérations arithmétiques et accès mémoire :

$ a = "opérations" / "accès mémoire" = n^3 / n^2 = n $

On distingue alors deux régimes :

#table(
  columns: (auto, auto, auto),
  stroke: 0.5pt,
  fill: (col, row) => if row == 0 { luma(220) } else { white },
  [*Régime*], [*Condition*], [*Effet de la parallélisation*],
  [Compute-bound], [$n$ grand, $a$ grand], [La parallélisation aide efficacement],
  [Bandwidth-bound], [$n$ petit, $a$ petit], [Ajouter des cœurs n'apporte presque rien],
)

=== Overhead de création des threads

Pour de petites matrices, le coût de lancement des threads OpenMP peut dépasser le gain de la parallélisation. La partie séquentielle (création, synchronisation, agrégation) devient alors dominante — exactement ce que prédit la loi d'Amdahl.

== Résumé des problèmes

#table(
  columns: (auto, auto, auto),
  stroke: 0.5pt,
  fill: (col, row) => if row == 0 { luma(220) } else { white },
  [*Problème*], [*Cause*], [*Lien avec le cours*],
  [Cache miss sur $B$], [Accès non-contigu en mémoire], [Localité mémoire],
  [Bande passante saturée], [Caches L2/L3 partagés entre threads], [Modèle Roofline],
  [Pas de gain pour petites matrices], [Overhead $>$ gain], [Loi d'Amdahl],
  [Faux partage], [Variables proches dans un même bloc cache], [False sharing],
)

== Conclusion

En pratique, c'est pour ces raisons que l'on n'implémente jamais naïvement la multiplication matricielle avec des boucles simples. Des bibliothèques hautement optimisées comme *BLAS* et *LAPACK* utilisent des techniques de *tiling* (découpage en blocs) pour maximiser la réutilisation du cache et minimiser les accès mémoire, tout en tirant pleinement parti de la parallélisation.


#showybox([
For small matrices, OpenMP parallelization may actually slow things down. Explain why and discuss potential solutions.
])

Time to start the threads>> time for computation. Logical, maybe interesting to comput this time 


= Part 3
 Reregarder le code et bien savoir expliquer le sync ect 
Pour lancer sur le cluster : 
- se connecter 
- modifier le bash 
- lancer via "sbatch mon_script.sh"

#showybox([
Profile and analyze the communication overhead (MPI operations) versus actual computation time in DistributedMatrix::multiplyTransposed.
])

VOir graphique et excel fait sur le cluster du coup 

#showybox([
What is the expected speedup for the distributed DistributedMatrix::multiplyTransposed operation? Compare this with the speedup you measure in your numerical experiments.
])

Ici 
- $alpha$ = temps de démarrage : le coût fixe pour initier une communication. 
- $beta$ = inverse de la bande passante — le coût par octet transféré
- $gamma$ = coût par opération flottante (pour la réduction elle-même)
- $n$ = nombre d'octets à transférer

On a des $log_2$ car quand on double le nombre de processus ayant la donnée à chaque communication. 

Mais ici considérons des matrices $A$ et $B$ de taille $m times n$ 
Considérons que $P$ est le nombre de processus. Le temps de calcul est donc fait en $cal(O) (m^2 times 2n/P)$ (addition et multiplication) et le temps de communication de ```C all_reduce``` est de $cal(O)(log_2(P) alpha + beta n + gamma n)$ donc ici $=> cal(O)(log_2(P) alpha + beta m^2 + gamma m^2)$ ($m^2$ car matrice de taille $m times m$ donc autant d'élément a faire passer en communication). 

Finalement on a un speedup attendu de : 
$
  S = (2m^2 n)/((log_2(P) alpha + beta m^2 + gamma m^2) + (m^2 times 2n/P)) 
$

So as $P -> infinity$ then $S -> 0$. Maximum obtain for some intermediate trade off. 

#showybox([
Compare this distributed approach (splitting columns) with an alternative where data is partitioned among processes and gradients are synchronized afterward.])


= Part 4 
#import "@preview/codly:1.3.0": *
#import "@preview/codly-languages:0.1.1": *
#show: codly-init.with()

#codly(languages: codly-languages)
```cpp
std::vector<cl::Platform> platforms; // 
cl::Platform::get(&platforms);
assert(!platforms.empty());

cl::Platform platform = platforms.front();
std::cout << "Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

```
ON récupère tous les drivers et on fait une liste exemple : NVIDIA 


#codly(languages: codly-languages)
```cpp
std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if (devices.empty())
        platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
    assert(!devices.empty());

    cl::Device device = devices.front();
    std::cout << "Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

```
On récupère les types de Device donc ici les GPU's
#codly(languages: codly-languages)
```cpp
cl::Context context(device);
```
Le Context est l'environnement partagé entre le CPU et le GPU. C'est lui qui gère la mémoire et les kernels. Il faut en créer un avant de faire quoi que ce soit.
cde 
#codly(languages: codly-languages)
```cpp
cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
```
La CommandQueue est la file de commandes envoyées au GPU. CL_QUEUE_PROFILING_ENABLE active la mesure de temps des kernels. Les commandes sont exécutées dans l'ordre dans lequel tu les enfiles.


EN suit on buildles kernels qui vont tourner sur les GPU's 

Ici directmeent crée dans notre class kernel 
En suit on doit créer les buffer, mettre les arguments de kernels ect. 
#codly(languages: codly-languages)
```cpp
cl::Buffer d_x(context, CL_MEM_READ_ONLY,  N * sizeof(float));
cl::Buffer d_y(context, CL_MEM_WRITE_ONLY, N * sizeof(float));
```
Les `Buffer` sont des zones mémoire allouées sur le GPU (VRAM). Le préfixe `d_` signifie *device*. `CL_MEM_READ_ONLY` signifie que le GPU ne peut que lire dans `d_x`, et `CL_MEM_WRITE_ONLY` que le GPU ne peut qu'écrire dans `d_y`. À ce stade les buffers sont alloués mais vides.

#codly(languages: codly-languages)
```cpp
queue.enqueueWriteBuffer(d_x, CL_TRUE, 0, N * sizeof(float), h_x.data());
```
On copie les données depuis la RAM du CPU (`h_x`) vers la VRAM du GPU (`d_x`) via le bus PCIe. Le `CL_TRUE` rend l'opération *bloquante* : le CPU attend que le transfert soit terminé avant de continuer. Ce transfert est souvent le goulot d'étranglement en pratique.

#codly(languages: codly-languages)
```cpp
kernel_square.setArg(0, d_x);
kernel_square.setArg(1, d_y);
kernel_square.setArg(2, N);
queue.enqueueNDRangeKernel(kernel_square, cl::NullRange, cl::NDRange(N), cl::NullRange);
queue.finish();
```
On passe les arguments au kernel via `setArg`, puis on le lance avec `enqueueNDRangeKernel`. `cl::NDRange(N)` signifie que N threads sont lancés en parallèle sur le GPU — chaque thread calcule `d_y[i] = d_x[i] * d_x[i]` pour un indice `i` différent. `queue.finish()` bloque le CPU jusqu'à ce que tous les threads aient terminé.

#codly(languages: codly-languages)
```cpp
queue.enqueueReadBuffer(d_y, CL_TRUE, 0, N * sizeof(float), h_y_gpu.data());
```
Une fois le calcul terminé, on rapatrie le résultat depuis la VRAM (`d_y`) vers la RAM (`h_y_gpu`). C'est l'opération inverse du `enqueueWriteBuffer`.

Le flux complet est donc :
```
CPU RAM          PCIe Bus         GPU VRAM
  h_x    ──WriteBuffer──>   d_x
                              │
                         kernel_square
                         (N threads en parallèle)
                              │
  h_y_gpu  <──ReadBuffer──   d_y
```

Okay ici quand on crée notre classe matrice elle a déjà une taille de buffer un buffer donc ici on va le crée dans l'initialisation de la classe 
