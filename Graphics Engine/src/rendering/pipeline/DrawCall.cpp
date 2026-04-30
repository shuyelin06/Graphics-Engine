#include "DrawCall.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
PixelTechnique::PixelTechnique()
    : shaderName(), cbuffers{}, textures{nullptr} {}
PixelTechnique::PixelTechnique(const std::string&& shader)
    : shaderName(shader), cbuffers{}, textures{nullptr} {}

void PixelTechnique::setShader(const std::string&& shader) {
    shaderName = std::move(shader);
}
const std::string& PixelTechnique::getShader() const { return shaderName; }

void PixelTechnique::setConstantBufferData(uint8_t slot, const void* data,
                                           size_t size) {
    assert(slot < kConstantBufferMax);
    std::vector<uint8_t>& cbuffer = cbuffers[slot];
    cbuffer.resize(size);
    memcpy(cbuffer.data(), data, size);
}
const std::vector<uint8_t>
PixelTechnique::getConstantBufferData(uint8_t slot) const {
    assert(slot < kConstantBufferMax);
    return cbuffers[slot];
}

void PixelTechnique::setTexture(uint8_t slot, Texture* texture) {
    textures[slot] = texture;
}
const Texture* PixelTechnique::getTexture(uint8_t slot) const {
    return textures[slot];
}

/*
    // Old skinning code that needs to be ported...
    for (const AssetComponent* comp : asset_components) {
        const Asset* asset = comp->getAsset();

        if (asset->isSkinned()) {
            pipeline->bindVertexShader("SkinnedMesh");
        } else
            pipeline->bindVertexShader("TexturedMesh");

        for (const auto& mesh : asset->getMeshes()) {

            const Material mat = mesh->material;

            // Pixel CB2: Mesh Material Data
            {
                IConstantBuffer pCB2 = pipeline->loadPixelCB(CB2);

                const TextureRegion& region = mat.tex_region;
                pCB2.loadData(&region.x, FLOAT);
                pCB2.loadData(&region.y, FLOAT);
                pCB2.loadData(&region.width, FLOAT);
                pCB2.loadData(&region.height, FLOAT);
            }

            // Vertex CB2: Transform matrices
            const Matrix4& mLocalToWorld = comp->getLocalToWorldMatrix();
            {
                IConstantBuffer vCB2 = pipeline->loadVertexCB(CB2);

                // Load mesh vertex transformation matrix
                vCB2.loadData(&mLocalToWorld, FLOAT4X4);
                // Load mesh normal transformation matrix
                Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
                vCB2.loadData(&(normalTransform), FLOAT4X4);
            }

            // Skinning
            if (asset->isSkinned()) {
                // Vertex CB3: Joint Matrices
                {
                    IConstantBuffer vCB3 = pipeline->loadVertexCB(CB3);

                    const std::vector<SkinJoint>& skin = asset->getSkinJoints();
                    for (int i = 0; i < skin.size(); i++) {
                        // SUPER INEFFICIENT RN
                        // TODO: THIS IS BOTTLE NECKING MY CODE
                        const Matrix4 skin_matrix =
                            skin[i].getTransform(skin[i].node) *
                            skin[i].m_inverse_bind;
                        const Matrix4 skin_normal_matrix =
                            skin_matrix.inverse().transpose();

                        vCB3.loadData(&skin_matrix, FLOAT4X4);
                        vCB3.loadData(&skin_normal_matrix, FLOAT4X4);
                    }
                }
            }

            // Draw each mesh
            pipeline->drawMesh(mesh.get(), INDEX_LIST_START, INDEX_LIST_END, 1);
        }
    }
*/

} // namespace Graphics
} // namespace Engine