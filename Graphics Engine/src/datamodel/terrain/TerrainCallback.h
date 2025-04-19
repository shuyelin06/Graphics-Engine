#pragma once

namespace Engine {
namespace Datamodel {
// TerrainCallback Class:
// Represents a handler for a terrain callback.
// When a terrain chunk is reloaded, this callback function will
// be invoked by the thread.
// This lets various systems pull data as needed from the TerrainChunk.
struct TerrainChunk;

class TerrainCallback {
  public:
    virtual void reloadTerrainData(const TerrainChunk* chunk_data) = 0;
};

} // namespace Datamodel
} // namespace Engine