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

void VSMesh::bindAndDraw(Pipeline* pipeline, RenderPass pass) {
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

} // namespace Graphics
} // namespace Engine