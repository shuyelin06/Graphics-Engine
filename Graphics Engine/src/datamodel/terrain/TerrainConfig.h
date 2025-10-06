#pragma once

// --- EDITABLE PARAMETERS ---
// Chunk Size
constexpr float TERRAIN_CHUNK_SIZE = 75.f;

// Samples Per Chunk (must be >= 2)
// Chunk will share the data values along their borders,
// so a higher sample count will mean less memory is wasted.
// Must be >= 2, since we must at least sample the borders.
constexpr int TERRAIN_CHUNK_SAMPLES = 7;

// # Chunks out that will be loaded at once.
constexpr int TERRAIN_CHUNK_EXTENT = 7;

// Water Line
constexpr float TERRAIN_FADE_LINE = 0.f;
// ---

// # Chunks in 1 dimension
constexpr int TERRAIN_CHUNK_COUNT = 2 * TERRAIN_CHUNK_EXTENT + 1;