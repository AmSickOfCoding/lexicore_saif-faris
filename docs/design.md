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
