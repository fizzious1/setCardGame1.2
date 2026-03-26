"""RAG retrieval layer.

Provides high-level retrieval methods that query one or more ChromaDB
collections and return contextualised document chunks.
"""

from __future__ import annotations

from pipeline.orchestrator.models import DocumentChunk
from pipeline.orchestrator.rag.vector_store import COLLECTION_NAMES, VectorStore

# Mapping from scenario type to (query_text, categories)
_SCENARIO_MAP: dict[str, tuple[str, list[str]]] = {
    "alliance_formation": (
        "alliance formation patterns, first-week dynamics, building trust, "
        "initial social bonds, alliance structures in Big Brother",
        ["social_dynamics"],
    ),
    "nomination_threat": (
        "nomination avoidance strategies, social lobbying before nominations, "
        "campaigning to stay off the block, persuading the HoH",
        ["social_dynamics", "game_mechanics"],
    ),
    "betrayal_decision": (
        "betrayal triggers, loyalty versus self-preservation, breaking trust, "
        "when to flip on an alliance, consequences of betrayal",
        ["social_dynamics", "psychological_decay"],
    ),
    "psychological_pressure": (
        "psychological stress in isolation, emotional crisis phase, sleep deprivation, "
        "paranoia, mental health pressure in the Big Brother house",
        ["psychological_decay"],
    ),
    "jury_management": (
        "jury voting behaviour, endgame jury management, bitter jury prevention, "
        "final speech strategy, securing jury votes",
        ["game_mechanics", "social_dynamics"],
    ),
}


class RAGRetriever:
    """High-level retriever that wraps :class:`VectorStore` queries."""

    def __init__(self, vector_store: VectorStore) -> None:
        self.vector_store = vector_store

    def retrieve(
        self,
        query: str,
        categories: list[str] | None = None,
        top_k: int = 5,
    ) -> list[DocumentChunk]:
        """Retrieve the most relevant chunks for *query*.

        Parameters
        ----------
        query:
            Natural-language query.
        categories:
            If provided, only search these collections.  If ``None``,
            search all collections and merge results by relevance.
        top_k:
            Maximum number of chunks to return.
        """
        target_collections = categories if categories else COLLECTION_NAMES

        if len(target_collections) == 1:
            return self.vector_store.query(
                collection_name=target_collections[0],
                query_text=query,
                top_k=top_k,
            )

        # Query every target collection and merge
        all_chunks: list[DocumentChunk] = []
        per_collection_k = max(2, top_k)  # fetch enough to merge
        for coll_name in target_collections:
            results = self.vector_store.query(
                collection_name=coll_name,
                query_text=query,
                top_k=per_collection_k,
            )
            all_chunks.extend(results)

        # Deduplicate by chunk id, keeping first occurrence (highest rank)
        seen: set[str] = set()
        unique: list[DocumentChunk] = []
        for chunk in all_chunks:
            if chunk.id not in seen:
                seen.add(chunk.id)
                unique.append(chunk)

        return unique[:top_k]

    def retrieve_for_scenario(self, scenario_type: str) -> list[DocumentChunk]:
        """Retrieve chunks relevant to a predefined scenario type.

        Supported scenario types:
            ``alliance_formation``, ``nomination_threat``, ``betrayal_decision``,
            ``psychological_pressure``, ``jury_management``.
        """
        if scenario_type not in _SCENARIO_MAP:
            # Fallback: use the scenario_type itself as a query across all collections
            return self.retrieve(query=scenario_type, categories=None, top_k=5)

        query_text, categories = _SCENARIO_MAP[scenario_type]
        return self.retrieve(query=query_text, categories=categories, top_k=5)

    def format_context(self, chunks: list[DocumentChunk]) -> str:
        """Format retrieved chunks into a single context string for prompt injection.

        Each chunk is wrapped with source metadata so the LLM can cite provenance.
        """
        if not chunks:
            return ""

        sections: list[str] = []
        for i, chunk in enumerate(chunks, start=1):
            header = (
                f"[Source {i}: {chunk.metadata.category} / {chunk.metadata.subtopic} "
                f"| Phase: {chunk.metadata.applicable_game_phase} "
                f"| Archetype: {chunk.metadata.applicable_archetype}]"
            )
            sections.append(f"{header}\n{chunk.text}")

        return "\n\n---\n\n".join(sections)
