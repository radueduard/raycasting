//
// Created by Eduard Andrei Radu on 19.03.2026.
//

#pragma once

#include <mesh.h>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
};

class CubeMesh : public gfx::CustomMesh<CubeMesh> {
public:
    static void DefineMesh()
    {
        _vertexBindingDescription = {
            { 0, sizeof(Vertex) }
        };

        _vertexAttributeDescription = {
            { 0, 0, 3, gfx::ChannelType::eFloat, offsetof(Vertex, position) },
            { 1, 0, 3, gfx::ChannelType::eFloat, offsetof(Vertex, color) },
        };
    }

    explicit CubeMesh(Builder& createInfo) : CustomMesh(createInfo) {}

    static std::unique_ptr<CubeMesh> Create() {
        Builder builder;
        std::vector<Vertex> vertices
        {
            { glm::vec3(0, 0, 0), glm::vec3(0, 0, 0) },
            { glm::vec3(0, 0, 1), glm::vec3(0, 0, 1) },
            { glm::vec3(0, 1, 0), glm::vec3(0, 1, 0) },
            { glm::vec3(0, 1, 1), glm::vec3(0, 1, 1) },
            { glm::vec3(1, 0, 0), glm::vec3(1, 0, 0) },
            { glm::vec3(1, 0, 1), glm::vec3(1, 0, 1) },
            { glm::vec3(1, 1, 0), glm::vec3(1, 1, 0) },
            { glm::vec3(1, 1, 1), glm::vec3(1, 1, 1) }

        };
        std::vector<unsigned int> indices =
        {
            1, 5, 7,
            7, 3, 1,
            0, 2, 6,
            6, 4, 0,
            0, 1, 3,
            3, 2, 0,
            7, 5, 4,
            4, 6, 7,
            2, 3, 7,
            7, 6, 2,
            1, 0, 4,
            4, 5, 1
        };
        {
            auto vertexBuffer = gfx::Buffer::Builder()
                .setSize(vertices.size() * sizeof(Vertex))
                .setUsage(gfx::Buffer::Usage::eVertex)
                .addUsage(gfx::Buffer::Usage::eTransferDst)
                .setMemoryProperties(gfx::Buffer::MemoryProperty::eDeviceLocal)
                .build();

            const auto stagingBuffer = gfx::Buffer::Builder()
                .setSize(vertices.size() * sizeof(Vertex))
                .setUsage(gfx::Buffer::Usage::eTransferSrc)
                .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostVisible)
                .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
                .build();

            stagingBuffer->Map();
            stagingBuffer->Write(std::span<Vertex> { vertices });
            stagingBuffer->Unmap();

            vertexBuffer->CopyFrom(*stagingBuffer, 0, 0, vertices.size() * sizeof(Vertex));
            builder.SetVertexBuffer(0, std::move(vertexBuffer));
        }
        {
            auto indexBuffer = gfx::Buffer::Builder()
                .setSize(indices.size() * sizeof(unsigned int))
                .setUsage(gfx::Buffer::Usage::eIndex)
                .addUsage(gfx::Buffer::Usage::eTransferDst)
                .setMemoryProperties(gfx::Buffer::MemoryProperty::eDeviceLocal)
                .build();

            const auto stagingBuffer = gfx::Buffer::Builder()
                .setSize(indices.size() * sizeof(unsigned int))
                .setUsage(gfx::Buffer::Usage::eTransferSrc)
                .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostVisible)
                .addMemoryProperty(gfx::Buffer::MemoryProperty::eHostCoherent)
                .build();

            stagingBuffer->Map();
            stagingBuffer->Write(std::span { indices });
            stagingBuffer->Unmap();


            indexBuffer->CopyFrom(*stagingBuffer, 0, 0, indices.size() * sizeof(unsigned int));
            builder.SetIndexBuffer(std::move(indexBuffer), gfx::ChannelType::eUInt);
        }
        return builder.Build();
    }
};