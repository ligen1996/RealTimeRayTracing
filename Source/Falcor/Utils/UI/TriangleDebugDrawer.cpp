#include "stdafx.h"
#include "TriangleDebugDrawer.h"
#include "Core/API/RenderContext.h"
#include "Scene/Camera/Camera.h"

namespace Falcor
{
    TriangleDebugDrawer::SharedPtr TriangleDebugDrawer::create(uint32_t maxVertices)
    {
        return SharedPtr(new TriangleDebugDrawer(maxVertices));
    }

    void TriangleDebugDrawer::addPoint(const float3& point,const float2& texcrd)
    {
        if (mVertexData.capacity() - mVertexData.size() >= 1)
        {
            mVertexData.push_back({ point, mCurrentColor ,texcrd });
        }
    }

    void TriangleDebugDrawer::render(RenderContext* pContext, GraphicsState* pState, GraphicsVars* pVars, Camera* pCamera)
    {
        //      ParameterBlock* pCB = pVars->getParameterBlock("InternalPerFrameCB").get();
        //      if (pCB != nullptr) pCamera->setShaderData(pCB, 0);

        uploadBuffer();
        pState->setVao(mpVao);
        pContext->draw(pState, pVars, (uint32_t)mVertexData.size(), 0);
    }

    void TriangleDebugDrawer::uploadBuffer()
    {
        if (mDirty)
        {
            auto pVertexBuffer = mpVao->getVertexBuffer(0);
            pVertexBuffer->setBlob(mVertexData.data(), 0, sizeof(PointVertex) * mVertexData.size());
            mDirty = false;
        }
    }

    TriangleDebugDrawer::TriangleDebugDrawer(uint32_t maxVertices)
    {
        Buffer::SharedPtr pVertexBuffer = Buffer::create(sizeof(PointVertex) * maxVertices, Resource::BindFlags::Vertex, Buffer::CpuAccess::Write, nullptr);

        VertexBufferLayout::SharedPtr pBufferLayout = VertexBufferLayout::create();
        pBufferLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
        pBufferLayout->addElement("COLOR", sizeof(float3), ResourceFormat::RGB32Float, 1, 1);
        pBufferLayout->addElement("TEXCOORD", sizeof(float3)+sizeof(float3), ResourceFormat::RG32Float, 1, 2);

        VertexLayout::SharedPtr pVertexLayout = VertexLayout::create();
        pVertexLayout->addBufferLayout(0, pBufferLayout);

        mpVao = Vao::create(Vao::Topology::TriangleList, pVertexLayout, { pVertexBuffer });

        mVertexData.resize(maxVertices);
    }
}
