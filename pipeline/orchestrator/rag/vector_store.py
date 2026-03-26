"""ChromaDB vector store wrapper for the RAG system.

Manages four collections for Big Brother knowledge domains and provides
methods to add, query, and inspect document chunks.
"""

from __future__ import annotations

import logging
from typing import Any

import chromadb
from chromadb.config import Settings

from pipeline.orchestrator.models import ChunkMetadata, DocumentChunk

logger = logging.getLogger(__name__)

COLLECTION_NAMES: list[str] = [
    "social_dynamics",
    "psychological_decay",
    "game_mechanics",
    "agent_architecture",
]


class VectorStore:
    """Thin wrapper around ChromaDB that stores and retrieves DocumentChunks."""

    def __init__(self, persist_directory: str = "./pipeline/output/chromadb") -> None:
        self.persist_directory = persist_directory
        self._client = chromadb.Client(Settings(
            persist_directory=persist_directory,
            anonymized_telemetry=False,
            is_persistent=True,
        ))
        self._collections: dict[str, chromadb.Collection] = {}

        # Ensure all four default collections exist
        for name in COLLECTION_NAMES:
            self.create_collection(name)

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def create_collection(self, name: str) -> chromadb.Collection:
        """Create (or retrieve) a ChromaDB collection by *name*.

        Uses the default ``all-MiniLM-L6-v2`` embedding function.
        """
        collection = self._client.get_or_create_collection(
            name=name,
            metadata={"hnsw:space": "cosine"},
        )
        self._collections[name] = collection
        return collection

    def add_chunks(self, collection_name: str, chunks: list[DocumentChunk]) -> None:
        """Add *chunks* to the named collection.

        Documents, ids, and metadata are passed to ChromaDB which computes
        embeddings via the default embedding function.
        """
        if not chunks:
            return

        collection = self._ensure_collection(collection_name)

        ids: list[str] = []
        documents: list[str] = []
        metadatas: list[dict[str, Any]] = []

        for chunk in chunks:
            ids.append(chunk.id)
            documents.append(chunk.text)
            metadatas.append({
                "category": chunk.metadata.category,
                "subtopic": chunk.metadata.subtopic,
                "applicable_game_phase": chunk.metadata.applicable_game_phase,
                "applicable_archetype": chunk.metadata.applicable_archetype,
                "document_source": chunk.document_source,
            })

        # ChromaDB can handle batches; add all at once
        collection.add(ids=ids, documents=documents, metadatas=metadatas)
        logger.info(
            "Added %d chunks to collection '%s'", len(chunks), collection_name
        )

    def query(
        self,
        collection_name: str,
        query_text: str,
        top_k: int = 5,
    ) -> list[DocumentChunk]:
        """Retrieve the *top_k* most similar chunks to *query_text*."""
        collection = self._ensure_collection(collection_name)

        if collection.count() == 0:
            return []

        results = collection.query(
            query_texts=[query_text],
            n_results=min(top_k, collection.count()),
            include=["documents", "metadatas", "distances"],
        )

        chunks: list[DocumentChunk] = []
        if not results or not results["ids"] or not results["ids"][0]:
            return chunks

        for idx, chunk_id in enumerate(results["ids"][0]):
            doc_text = results["documents"][0][idx] if results["documents"] else ""
            meta_raw = results["metadatas"][0][idx] if results["metadatas"] else {}

            metadata = ChunkMetadata(
                category=meta_raw.get("category", "general"),
                subtopic=meta_raw.get("subtopic", ""),
                applicable_game_phase=meta_raw.get("applicable_game_phase", "any"),
                applicable_archetype=meta_raw.get("applicable_archetype", "all"),
            )

            chunks.append(DocumentChunk(
                id=chunk_id,
                text=doc_text,
                metadata=metadata,
                document_source=meta_raw.get("document_source", ""),
            ))

        return chunks

    def get_collection_stats(self, collection_name: str) -> dict[str, Any]:
        """Return count and metadata summary for the named collection."""
        collection = self._ensure_collection(collection_name)
        count = collection.count()

        stats: dict[str, Any] = {
            "name": collection_name,
            "count": count,
            "categories": set(),
            "subtopics": set(),
            "game_phases": set(),
            "archetypes": set(),
        }

        if count > 0:
            # Peek at up to 100 items to summarize metadata
            peek_count = min(count, 100)
            peek = collection.peek(limit=peek_count)
            if peek and peek.get("metadatas"):
                for meta in peek["metadatas"]:
                    if meta:
                        stats["categories"].add(meta.get("category", ""))
                        stats["subtopics"].add(meta.get("subtopic", ""))
                        stats["game_phases"].add(meta.get("applicable_game_phase", ""))
                        stats["archetypes"].add(meta.get("applicable_archetype", ""))

        # Convert sets to sorted lists for JSON serialization
        for key in ("categories", "subtopics", "game_phases", "archetypes"):
            stats[key] = sorted(stats[key] - {""})

        return stats

    def get_all_stats(self) -> dict[str, Any]:
        """Return stats for every known collection."""
        all_stats: dict[str, Any] = {}
        total_chunks = 0
        for name in COLLECTION_NAMES:
            coll_stats = self.get_collection_stats(name)
            all_stats[name] = coll_stats
            total_chunks += coll_stats["count"]
        all_stats["_total_chunks"] = total_chunks
        return all_stats

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _ensure_collection(self, name: str) -> chromadb.Collection:
        if name not in self._collections:
            self._collections[name] = self.create_collection(name)
        return self._collections[name]
