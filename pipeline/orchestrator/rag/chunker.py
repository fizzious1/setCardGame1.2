"""Document chunking for the RAG system.

Splits markdown files into overlapping chunks that respect paragraph boundaries,
and generates metadata for each chunk based on content analysis.
"""

from __future__ import annotations

import re
from pathlib import Path

from pipeline.orchestrator.models import ChunkMetadata, DocumentChunk


# Keywords used to infer game phase from chunk content
_PHASE_KEYWORDS: dict[str, list[str]] = {
    "early_game": [
        "first week", "week 1", "week one", "opening", "initial",
        "first impression", "move-in", "move in", "day 1", "day one",
        "early game", "early-game", "first few days",
    ],
    "mid_game": [
        "mid game", "mid-game", "middle weeks", "week 3", "week 4",
        "week 5", "jury phase begins", "power shift", "majority",
        "swing vote", "week three", "week four", "week five",
    ],
    "late_game": [
        "late game", "late-game", "final four", "final five", "final 4",
        "final 5", "week 7", "week 8", "week 9", "endgame approaches",
        "week seven", "week eight", "week nine",
    ],
    "endgame": [
        "endgame", "end game", "finale", "final two", "final 2",
        "jury vote", "jury management", "final speech", "final hoh",
        "final head of household",
    ],
}

# Keywords used to infer archetype relevance from chunk content
_ARCHETYPE_KEYWORDS: dict[str, list[str]] = {
    "puppet_master": [
        "puppet master", "puppetmaster", "manipulat", "mastermind",
        "orchestrat", "pull strings", "pulling strings", "control the house",
    ],
    "floater": [
        "floater", "float", "middle ground", "non-committal", "both sides",
        "play the middle", "avoid taking sides",
    ],
    "comp_beast": [
        "comp beast", "competition beast", "win competitions", "physical threat",
        "challenge dominator", "comp wins", "winning streak",
    ],
    "social_butterfly": [
        "social butterfly", "social game", "befriend everyone", "social capital",
        "likability", "likeable", "everyone loves",
    ],
    "loyal_soldier": [
        "loyal soldier", "loyalty", "ride or die", "loyal to the end",
        "never betray", "faithful", "devoted ally",
    ],
    "charming_villain": [
        "charming villain", "villain", "ruthless charm", "backstab with a smile",
        "strategic betrayal", "charismatic threat",
    ],
    "wild_card": [
        "wild card", "wildcard", "unpredictable", "chaos", "loose cannon",
        "volatile", "erratic",
    ],
    "underdog": [
        "underdog", "against the odds", "survival mode", "on the block",
        "back against the wall", "fighting to stay",
    ],
    "goat": [
        "goat", "dragged to the end", "easy to beat", "no jury votes",
        "final 2 goat", "disposable ally",
    ],
    "analytical_player": [
        "analytical", "strategic thinker", "calculated", "logical",
        "game theory", "probability", "odds",
    ],
}


def _infer_game_phase(text: str) -> str:
    """Return the most likely game phase for the given text, or 'any'."""
    text_lower = text.lower()
    scores: dict[str, int] = {}
    for phase, keywords in _PHASE_KEYWORDS.items():
        score = sum(1 for kw in keywords if kw in text_lower)
        if score > 0:
            scores[phase] = score
    if not scores:
        return "any"
    return max(scores, key=scores.get)  # type: ignore[arg-type]


def _infer_archetype(text: str) -> str:
    """Return the most relevant archetype for the given text, or 'all'."""
    text_lower = text.lower()
    scores: dict[str, int] = {}
    for archetype, keywords in _ARCHETYPE_KEYWORDS.items():
        score = sum(1 for kw in keywords if kw in text_lower)
        if score > 0:
            scores[archetype] = score
    if not scores:
        return "all"
    return max(scores, key=scores.get)  # type: ignore[arg-type]


def _find_nearest_heading(text: str, position: int) -> str:
    """Find the nearest markdown heading (# or ##) before *position* in *text*."""
    heading_pattern = re.compile(r"^(#{1,2})\s+(.+)$", re.MULTILINE)
    best_heading = ""
    for match in heading_pattern.finditer(text):
        if match.start() <= position:
            best_heading = match.group(2).strip()
        else:
            break
    return best_heading if best_heading else "General"


def chunk_document(filepath: str, category: str) -> list[DocumentChunk]:
    """Split a markdown file into overlapping ~2000-char chunks.

    Chunks respect paragraph boundaries (double newlines) and never split
    mid-paragraph. Each chunk carries metadata inferred from its content.

    Parameters
    ----------
    filepath:
        Path to the markdown file.
    category:
        Document category (e.g. ``social_dynamics``, ``game_mechanics``).

    Returns
    -------
    list[DocumentChunk]
        Ordered list of document chunks with metadata.
    """
    path = Path(filepath)
    full_text = path.read_text(encoding="utf-8")

    # Split on double newlines to get paragraphs
    paragraphs = re.split(r"\n{2,}", full_text)
    paragraphs = [p.strip() for p in paragraphs if p.strip()]

    target_size = 2000  # characters (~500 tokens)
    overlap_size = 200  # characters (~50 tokens)

    chunks: list[DocumentChunk] = []
    current_paragraphs: list[str] = []
    current_length = 0
    # Track the character offset of the first paragraph in the current chunk
    char_offset = 0
    # Running offset through the original text so we can locate headings
    paragraph_offsets: list[int] = []
    offset = 0
    for para in paragraphs:
        idx = full_text.find(para, offset)
        paragraph_offsets.append(idx if idx >= 0 else offset)
        offset = (idx if idx >= 0 else offset) + len(para)

    para_index = 0

    while para_index < len(paragraphs):
        para = paragraphs[para_index]
        para_len = len(para)

        # If adding this paragraph stays within target, accumulate
        if current_length + para_len + (2 if current_paragraphs else 0) <= target_size:
            current_paragraphs.append(para)
            current_length += para_len + (2 if len(current_paragraphs) > 1 else 0)
            para_index += 1
        else:
            # If we have accumulated paragraphs, emit a chunk
            if current_paragraphs:
                chunk_text = "\n\n".join(current_paragraphs)
                chunk_position = paragraph_offsets[para_index - len(current_paragraphs)]
                subtopic = _find_nearest_heading(full_text, chunk_position)

                metadata = ChunkMetadata(
                    category=category,
                    subtopic=subtopic,
                    applicable_game_phase=_infer_game_phase(chunk_text),
                    applicable_archetype=_infer_archetype(chunk_text),
                )

                chunk = DocumentChunk(
                    id=f"{category}_{len(chunks):03d}",
                    text=chunk_text,
                    metadata=metadata,
                    document_source=str(filepath),
                )
                chunks.append(chunk)

                # Compute overlap: walk backward through paragraphs to find
                # those whose combined length is closest to overlap_size
                overlap_paras: list[str] = []
                overlap_len = 0
                for p in reversed(current_paragraphs):
                    if overlap_len + len(p) > overlap_size and overlap_paras:
                        break
                    overlap_paras.insert(0, p)
                    overlap_len += len(p)

                current_paragraphs = overlap_paras
                current_length = sum(len(p) for p in current_paragraphs) + max(0, (len(current_paragraphs) - 1) * 2)
            else:
                # Single paragraph exceeds target — emit it as its own chunk
                chunk_text = para
                chunk_position = paragraph_offsets[para_index]
                subtopic = _find_nearest_heading(full_text, chunk_position)

                metadata = ChunkMetadata(
                    category=category,
                    subtopic=subtopic,
                    applicable_game_phase=_infer_game_phase(chunk_text),
                    applicable_archetype=_infer_archetype(chunk_text),
                )

                chunk = DocumentChunk(
                    id=f"{category}_{len(chunks):03d}",
                    text=chunk_text,
                    metadata=metadata,
                    document_source=str(filepath),
                )
                chunks.append(chunk)
                para_index += 1
                current_paragraphs = []
                current_length = 0

    # Emit remaining paragraphs
    if current_paragraphs:
        chunk_text = "\n\n".join(current_paragraphs)
        chunk_position = paragraph_offsets[max(0, para_index - len(current_paragraphs))]
        subtopic = _find_nearest_heading(full_text, chunk_position)

        metadata = ChunkMetadata(
            category=category,
            subtopic=subtopic,
            applicable_game_phase=_infer_game_phase(chunk_text),
            applicable_archetype=_infer_archetype(chunk_text),
        )

        chunk = DocumentChunk(
            id=f"{category}_{len(chunks):03d}",
            text=chunk_text,
            metadata=metadata,
            document_source=str(filepath),
        )
        chunks.append(chunk)

    return chunks
