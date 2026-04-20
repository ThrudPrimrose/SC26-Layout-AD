# No "copy library node" PR exists in spcl/dace — but related work is underway

**No Pull Request introducing a dedicated "CopyLibNode" or "copy library node" exists in the spcl/dace repository.** After exhaustive searching across open PRs, closed PRs, issues, discussions, code search, and branches using every plausible naming variant (CopyLibNode, CopyNode, copy_library_node, CopyLibraryNode, copy libnode), the concept does not appear in the repository as of March 2026. However, several closely related efforts address the underlying motivations — eliminating `other_subset` from memlets, making copies explicit, and reforming how memory copies are represented in the SDFG IR.

## How DaCe currently handles memory copies

In DaCe's SDFG intermediate representation, data copies between memory containers are **not** represented by dedicated nodes. Instead, they are encoded as **memlet edges between two AccessNodes** — a pattern where `AccessNode(A) → memlet → AccessNode(B)` implicitly means "copy data from A to B." The memlet carries both a `subset` (primary side) and an `other_subset` (opposite side), along with `src_subset` and `dst_subset` properties, to express source and destination ranges that may differ in shape or offset.

This design creates several well-known problems. The `other_subset` mechanism is **confusing and error-prone** — it requires tracking which end of an edge a subset belongs to, complicates memlet propagation, and makes transformations like `LocalStorage` difficult or impossible. When copies cannot be expressed as native library calls (like `cudaMemcpy`), a configuration option (`allow_implicit_memlet_to_map_conversion`) forces the code generator to silently convert the memlet into an explicit Map that loops over elements — an ad-hoc workaround rather than a principled IR-level solution. Existing library nodes in DaCe cover operations like **MatMul, Einsum, and Reduce** (from the BLAS and standard libraries), but no "Copy" library exists.

## Issue #1695 targets the root cause for DaCe 2.0

The most architecturally significant finding is **Issue #1695** ("Nested SDFGs reduce either readability or optimization capability"), opened by Tal Ben-Nun (tbennun) on **October 18, 2024**, and labeled `2.0`, `core`, `enhancement`. This issue proposes fundamental changes to nested SDFG semantics that would **effectively eliminate the need for `other_subset`** in its primary use case.

The issue identifies that nested SDFGs originally served dual purposes: introducing control flow inside dataflow scopes, and reshaping/offsetting/reinterpreting data containers. With the later introduction of **Views and References**, the reshaping purpose became redundant but remained entangled with memlet semantics. The proposal for DaCe 2.0 specifies that memlets entering and leaving nested SDFGs should behave as **passthrough connectors** — no offsetting during code generation, only union operations during memlet propagation. Nested SDFGs would share descriptor repositories with their parent, and the `symbol_mapping` property would be removed. Critically, **"squeezing and unsqueezing memlets will no longer be necessary"**, which directly eliminates the context in which `other_subset` is most heavily used.

## Three active PRs push toward explicit copy handling

While no single PR introduces a copy library node, three open PRs collectively advance the goal of making copies more explicit in the IR:

**PR #2260 — "Explicit gpu global copies"** (ThrudPrimrose, January 6, 2026, 20 comments) is part of a GPU codegen overhaul alongside PR #2259 ("New GPU Codegen Complete PR") and PR #2261 ("Explicit Stream Management Passes"). The title strongly suggests it replaces implicit memlet-based GPU global memory copies with an explicit representation, though the full PR description could not be retrieved. This is the **closest existing work** to the concept of explicit copy nodes for GPU memory operations.

**PR #2225 — "Remove Memlet Squeezing from BLAS library node expansions"** (affifboudaoud, November 9, 2025, Draft) directly implements part of the Issue #1695 vision by eliminating memlet squeezing from BLAS library node expansions. Squeezing is the dimensionality-reduction operation that relies on `other_subset` semantics, so removing it from library node expansions is a concrete step toward deprecating `other_subset`.

**PR #2123 — "Add CopyND support to copy structs / data types that are not trivially copyable"** (ThrudPrimrose, August 7, 2025) extends DaCe's CopyND code generation infrastructure to handle complex data types like structs. This operates at the **code generation layer** rather than the IR level — it improves how copies are emitted, not how they are represented in the graph.

## Discussion #1768 likely covers `other_subset` deprecation

**Discussion #1768** ("Deprecated features to remove"), started by tbennun on **November 17, 2024** with **11 comments** from tbennun, romanc, and tim0s, is categorized under "Ideas" and was opened one month after Issue #1695. Given its timing and title, it very likely lists `other_subset` among features targeted for deprecation. The full discussion content could not be retrieved, but its existence confirms that the DaCe team is actively cataloging legacy mechanisms for removal.

## The gap between current work and a true copy library node

The architectural gap is clear: DaCe's current IR has **no first-class node representing a copy operation**. Copies are implicit in memlet edges between access nodes, and the code generator dispatches them through `copy_memory()` methods on target-specific backends. A "CopyLibNode" — analogous to existing library nodes like `MatMul` or `Reduce` — would make copies explicit graph nodes that can be pattern-matched by transformations, expanded to target-specific implementations (memcpy, cudaMemcpy, DMA transfers, element-wise maps), and reasoned about independently of memlet edge semantics.

The pieces being assembled point toward this direction: Issue #1695 removes the need for `other_subset` at nested SDFG boundaries, PR #2225 eliminates squeezing from library expansions, PR #2260 makes GPU copies explicit, and Discussion #1768 catalogs deprecated features. However, **no one has yet proposed a unified "Copy" library node class** in a PR or issue. The concept may exist in internal planning or future DaCe 2.0 design documents that are not publicly visible on GitHub.

## Conclusion

The specific PR described in the query — introducing a CopyLibNode to replace access-node-to-access-node copy paths and eliminate `other_subset` — **does not exist** in the spcl/dace repository. The closest related artifacts are Issue #1695 (proposing DaCe 2.0 nested SDFG reform that eliminates `other_subset` usage), PR #2260 (making GPU global copies explicit), PR #2225 (removing memlet squeezing from BLAS library nodes), and Discussion #1768 (cataloging deprecated features). The DaCe team appears to be incrementally moving toward explicit copy representation, but a dedicated copy library node has not yet materialized as a concrete proposal.

# No "copy library node" PR exists in spcl/dace — but related work is underway

**No Pull Request introducing a dedicated "CopyLibNode" or "copy library node" exists in the spcl/dace repository.** After exhaustive searching across open PRs, closed PRs, issues, discussions, code search, and branches using every plausible naming variant (CopyLibNode, CopyNode, copy_library_node, CopyLibraryNode, copy libnode), the concept does not appear in the repository as of March 2026. However, several closely related efforts address the underlying motivations — eliminating `other_subset` from memlets, making copies explicit, and reforming how memory copies are represented in the SDFG IR.

## How DaCe currently handles memory copies

In DaCe's SDFG intermediate representation, data copies between memory containers are **not** represented by dedicated nodes. Instead, they are encoded as **memlet edges between two AccessNodes** — a pattern where `AccessNode(A) → memlet → AccessNode(B)` implicitly means "copy data from A to B." The memlet carries both a `subset` (primary side) and an `other_subset` (opposite side), along with `src_subset` and `dst_subset` properties, to express source and destination ranges that may differ in shape or offset.

This design creates several well-known problems. The `other_subset` mechanism is **confusing and error-prone** — it requires tracking which end of an edge a subset belongs to, complicates memlet propagation, and makes transformations like `LocalStorage` difficult or impossible. When copies cannot be expressed as native library calls (like `cudaMemcpy`), a configuration option (`allow_implicit_memlet_to_map_conversion`) forces the code generator to silently convert the memlet into an explicit Map that loops over elements — an ad-hoc workaround rather than a principled IR-level solution. Existing library nodes in DaCe cover operations like **MatMul, Einsum, and Reduce** (from the BLAS and standard libraries), but no "Copy" library exists.

## Issue #1695 targets the root cause for DaCe 2.0

The most architecturally significant finding is **Issue #1695** ("Nested SDFGs reduce either readability or optimization capability"), opened by Tal Ben-Nun (tbennun) on **October 18, 2024**, and labeled `2.0`, `core`, `enhancement`. This issue proposes fundamental changes to nested SDFG semantics that would **effectively eliminate the need for `other_subset`** in its primary use case.

The issue identifies that nested SDFGs originally served dual purposes: introducing control flow inside dataflow scopes, and reshaping/offsetting/reinterpreting data containers. With the later introduction of **Views and References**, the reshaping purpose became redundant but remained entangled with memlet semantics. The proposal for DaCe 2.0 specifies that memlets entering and leaving nested SDFGs should behave as **passthrough connectors** — no offsetting during code generation, only union operations during memlet propagation. Nested SDFGs would share descriptor repositories with their parent, and the `symbol_mapping` property would be removed. Critically, **"squeezing and unsqueezing memlets will no longer be necessary"**, which directly eliminates the context in which `other_subset` is most heavily used.

## Three active PRs push toward explicit copy handling

While no single PR introduces a copy library node, three open PRs collectively advance the goal of making copies more explicit in the IR:

**PR #2260 — "Explicit gpu global copies"** (ThrudPrimrose, January 6, 2026, 20 comments) is part of a GPU codegen overhaul alongside PR #2259 ("New GPU Codegen Complete PR") and PR #2261 ("Explicit Stream Management Passes"). The title strongly suggests it replaces implicit memlet-based GPU global memory copies with an explicit representation, though the full PR description could not be retrieved. This is the **closest existing work** to the concept of explicit copy nodes for GPU memory operations.

**PR #2225 — "Remove Memlet Squeezing from BLAS library node expansions"** (affifboudaoud, November 9, 2025, Draft) directly implements part of the Issue #1695 vision by eliminating memlet squeezing from BLAS library node expansions. Squeezing is the dimensionality-reduction operation that relies on `other_subset` semantics, so removing it from library node expansions is a concrete step toward deprecating `other_subset`.

**PR #2123 — "Add CopyND support to copy structs / data types that are not trivially copyable"** (ThrudPrimrose, August 7, 2025) extends DaCe's CopyND code generation infrastructure to handle complex data types like structs. This operates at the **code generation layer** rather than the IR level — it improves how copies are emitted, not how they are represented in the graph.

## Discussion #1768 likely covers `other_subset` deprecation

**Discussion #1768** ("Deprecated features to remove"), started by tbennun on **November 17, 2024** with **11 comments** from tbennun, romanc, and tim0s, is categorized under "Ideas" and was opened one month after Issue #1695. Given its timing and title, it very likely lists `other_subset` among features targeted for deprecation. The full discussion content could not be retrieved, but its existence confirms that the DaCe team is actively cataloging legacy mechanisms for removal.

## The gap between current work and a true copy library node

The architectural gap is clear: DaCe's current IR has **no first-class node representing a copy operation**. Copies are implicit in memlet edges between access nodes, and the code generator dispatches them through `copy_memory()` methods on target-specific backends. A "CopyLibNode" — analogous to existing library nodes like `MatMul` or `Reduce` — would make copies explicit graph nodes that can be pattern-matched by transformations, expanded to target-specific implementations (memcpy, cudaMemcpy, DMA transfers, element-wise maps), and reasoned about independently of memlet edge semantics.

The pieces being assembled point toward this direction: Issue #1695 removes the need for `other_subset` at nested SDFG boundaries, PR #2225 eliminates squeezing from library expansions, PR #2260 makes GPU copies explicit, and Discussion #1768 catalogs deprecated features. However, **no one has yet proposed a unified "Copy" library node class** in a PR or issue. The concept may exist in internal planning or future DaCe 2.0 design documents that are not publicly visible on GitHub.

## Conclusion

The specific PR described in the query — introducing a CopyLibNode to replace access-node-to-access-node copy paths and eliminate `other_subset` — **does not exist** in the spcl/dace repository. The closest related artifacts are Issue #1695 (proposing DaCe 2.0 nested SDFG reform that eliminates `other_subset` usage), PR #2260 (making GPU global copies explicit), PR #2225 (removing memlet squeezing from BLAS library nodes), and Discussion #1768 (cataloging deprecated features). The DaCe team appears to be incrementally moving toward explicit copy representation, but a dedicated copy library node has not yet materialized as a concrete proposal.