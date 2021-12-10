#pragma once
#include "Core/API/VAO.h"

namespace Falcor
{
    struct BoundingBox;
    class RenderContext;
    class Camera;
    class GraphicsState;
    class GraphicsVars;

    /** Utility class to assist in drawing debug geometry
    */
    class dlldecl PointDrawer
    {
    public:

        using SharedPtr = std::shared_ptr<PointDrawer>;
        using SharedConstPtr = std::shared_ptr<const PointDrawer>;

        static const uint32_t kMaxVertices = 10000;     ///< Default max number of vertices per DebugDrawer instance
        static const uint32_t kPathDetail = 10;         ///< Segments between keyframes

        using Quad = std::array<float3, 4>;

        /** Create a new object for drawing debug geometry.
            \param[in] maxVertices Maximum number of vertices that will be drawn.
            \return New object, or throws an exception if creation failed.
        */
        static SharedPtr create(uint32_t maxVertices = kMaxVertices);

        /** Sets the color for following geometry
        */
        void setColor(const float3& color) { mCurrentColor = color; }

        void addPoint(const float3& point);
   
        /** Renders the contents of the debug drawer
        */
        void render(RenderContext* pContext, GraphicsState* pState, GraphicsVars* pVars, Camera* pCamera);

        /** Get how many vertices are currently pushed
        */
        uint32_t getVertexCount() const { return (uint32_t)mVertexData.size(); }

        /** Get the Vao of vertex data
        */
        const Vao::SharedPtr& getVao() const { return mpVao; };

        /** Clears vertices
        */
        void clear() { mVertexData.clear(); mDirty = true; }

    private:

        void uploadBuffer();

        PointDrawer(uint32_t maxVertices);

        float3 mCurrentColor;

        struct PointVertex
        {
            float3 position;
            float3 color;
        };

        Vao::SharedPtr mpVao;
        std::vector<PointVertex> mVertexData;
        bool mDirty = true;
    };
}
