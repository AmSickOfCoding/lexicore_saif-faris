# LexiCore Architecture & Design Specification

## 1. Hash Table Architecture & Data Structures

LexiCore employs an array of linked-list buckets to provide amortized $\mathcal{O}(1)$ performance for lookups, insertions, updates, and deletions.

### Separate Chaining Mechanics
* **Collision Resolution:** When two distinct words hash to the same bucket index, the collision is resolved using **Separate Chaining** via singly-linked lists.
* **Insertion Strategy:** New entries are prepended to the head of the corresponding bucket chain in $\mathcal{O}(1)$ time:
  $$\text{newEntry}\rightarrow\text{next} = *(\text{dictionary}\rightarrow\text{buckets} + \text{index})$$
  $$*(\text{dictionary}\rightarrow\text{buckets} + \text{index}) = \text{newEntry}$$
* **Scoped Traversal:** Queries only traverse the targeted linked list chain, maintaining high throughput when the load factor $\alpha = \frac{N}{K}$ is balanced.

### Case-Insensitive DJB2 Hash Algorithm
The DJB2 algorithm is utilized for uniform distribution and low collision rates across textual datasets:
* **Initial seed:** `5381`
* **Iterative hash formula:** `hash = (hash * 33) + tolower(c)`
* Case normalization ensures that `"Lexicon"`, `"lexicon"`, and `"LEXICON"` map to the exact same bucket.

---

## 2. Memory Management Strategy & Protocols

### Strict Dynamic Ownership Hierarchy
Every word entry in the dictionary owns distinct, dynamically allocated heap buffers:
1. `Dictionary` struct
2. `buckets` pointer array (`sizeof(DictionaryEntry*) * bucketCount`)
3. `DictionaryEntry` node
4. Four individual string buffers:
   * `word`
   * `partOfSpeech`
   * `definition`
   * `exampleSentence`

### Pointer Arithmetic Protocol
In accordance with system design constraints, all hash table bucket lookups and manipulations strictly avoid bracketed array indexing (`[]`) and utilize direct pointer arithmetic:
* Head dereferencing: `*(dictionary->buckets + bucketIndex)`
* Memory cleanup pass: `*(dictionary->buckets + i)`

### Deallocation Protocol (`destroyDictionary`)
To guarantee zero memory leaks and eliminate dangling pointer dereferences:
1. Traverse each bucket from index `0` to `bucketCount - 1`.
2. Walk the linked list while caching the `current->next` pointer before deallocating `current`.
3. Sequentially free all 4 dynamic strings (`word`, `partOfSpeech`, `definition`, `exampleSentence`).
4. Free the `DictionaryEntry` node.
5. Free the `buckets` pointer array.
6. Free the `Dictionary` structure.
