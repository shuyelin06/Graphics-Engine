#include "VSPSMesh.h"

namespace Engine {
namespace Graphics {
VSMesh::VSMesh() {
    mesh = nullptr;
    worldFromLocal = Matrix4::Identity();
}

void VSMesh::update(const std::shared_ptr<Mesh>& _mesh,
                    const Matrix4& _localMatrix) {
    mesh = _mesh;
    worldFromLocal = _localMatrix;
}

void VSMesh::bindAndDraw(Pipeline* pipeline, PipelineRenderPass pass) {
    if (!mesh->ready)
        return;

    pipeline->bindVertexShader("TexturedMesh");

    // CB1 set outside

    // Vertex Shader Bindings:
    // CB2: Transform matrices
    {
        IConstantBuffer vCB2 = pipeline->loadVertexCB(CB2);

        // Load mesh vertex transformation matrix
        const Matrix4& mLocalToWorld = worldFromLocal;
        vCB2.loadData(&mLocalToWorld, FLOAT4X4);
        // Load mesh normal transformation matrix
        Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
        vCB2.loadData(&(normalTransform), FLOAT4X4);
    }

    // Draw each mesh
    pipeline->drawMesh(mesh.get(), INDEX_LIST_START, INDEX_LIST_END, 1);
}

PSMesh::PSMesh(LightManager* lightManager, ResourceManager* resourceManager)
    : mLightManager(lightManager), mResourceManager(resourceManager) {
    mMaterial = Material();
}

void PSMesh::update(Material material) { mMaterial = material; }

void PSMesh::bind(Pipeline* pipeline) {
    pipeline->bindPixelShader("TexturedMesh");
    pipeline->bindRenderTarget(Target_UseExisting, Depth_TestAndWrite,
                               Blend_Default);

    // Pixel Constant Buffer 1: Light Data
    // Stores data that is needed for lighting / shadowing.
    {
        IConstantBuffer pCB1 = pipeline->loadPixelCB(CB1);
        mLightManager->bindLightData(pCB1);
    }

    // Pixel Shader Bindings:
    // Texture 0: Color Map
    std::shared_ptr<Texture> colormap =
        mResourceManager->getTexture(SystemTexture_FallbackColormap);
    if (mMaterial.colormap != nullptr) {
        colormap = mMaterial.colormap;
    }
    pipeline->bindPixelTexture(*colormap, 0);
}

/*

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