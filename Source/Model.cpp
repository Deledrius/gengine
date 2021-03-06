//
// Model.cpp
//
// Clark Kromenaker
// 
#include "Model.h"

#include <fstream>
#include <iostream>

#include "BinaryReader.h"
#include "Mesh.h"
#include "Quaternion.h"
#include "Submesh.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Debug.h"

//#define DEBUG_OUTPUT

Model::Model(std::string name, char* data, int dataLength) :
    Asset(name)
{
    ParseFromData(data, dataLength);
}

void Model::WriteToObjFile(std::string filePath)
{
	std::ofstream out(filePath, std::ios::out);
    if(!out.good())
    {
        std::cout << "Can't write to file " << filePath << "!" << std::endl;
		return;
    }
	
	// Don't write out scientific notation.
	out << std::fixed;
	
	int count = 0;
	
	// Write out vertices.
	out << "# Vertices\n";
	for(auto& mesh : mMeshes)
	{
		Matrix4 localTransformMatrix = mesh->GetMeshToLocalMatrix();
		for(auto& submesh : mesh->GetSubmeshes())
		{
			float* positions = submesh->GetPositions();
			if(positions != nullptr)
			{
				int vertexCount = submesh->GetVertexCount();
				for(int i = 0; i < vertexCount; ++i)
				{
					Vector3 vertex(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
					vertex = localTransformMatrix.TransformPoint(vertex);
					
					out << "v " << vertex.x;
					out << " "  << vertex.y;
					out << " "  << vertex.z << "\n";
					++count;
				}
			}
		}
	}
			
	// Write out texture coordinates.
	out << "# Texture Coordinates\n";
	for(auto& mesh : mMeshes)
	{
		for(auto& submesh : mesh->GetSubmeshes())
		{
			float* uvs = submesh->GetUV1s();
			if(uvs != nullptr)
			{
				int vertexCount = submesh->GetVertexCount();
				for(int i = 0; i < vertexCount; ++i)
				{
					out << "vt " << uvs[i * 2] << " " << uvs[i * 2 + 1] << "\n";
				}
			}
		}
	}
				
	// Write out normals.
	out << "# Normals\n";
	for(auto& mesh : mMeshes)
	{
		for(auto& submesh : mesh->GetSubmeshes())
		{
			float* normals = submesh->GetNormals();
			if(normals != nullptr)
			{
				int vertexCount = submesh->GetVertexCount();
				for(int i = 0; i < vertexCount; ++i)
				{
					out << "vn " << normals[i * 3] << " " << normals[i * 3 + 1] << " " << normals[i * 3 + 2] << "\n";
				}
			}
		}
	}
	
	// Write out faces.
	out << "# Faces\n";
	int submeshOffset = 1;
	for(auto& mesh : mMeshes)
	{
		for(auto& submesh : mesh->GetSubmeshes())
		{
			unsigned short* indexes = submesh->GetIndexes();
			if(indexes != nullptr)
			{
				int indexCount = submesh->GetIndexCount();
				for(int i = 0; i < indexCount; i += 3)
				{
					int f1 = submeshOffset + indexes[i];
					int f2 = submeshOffset + indexes[i + 1];
					int f3 = submeshOffset + indexes[i + 2];
					
					if(f1 < 1 || f1 > count || f2 < 1 || f2 > count || f3 < 1 || f3 > count)
					{
						std::cout << "Using face index " << f1 << " which is greater than max of " << count << std::endl;
					}
					
					out << "f " << f1;
					out << " " << f2;
					out << " " << f3 << "\n";
				}
				submeshOffset += submesh->GetVertexCount();
			}
		}
	}
}

void Model::ParseFromData(char *data, int dataLength)
{
    #ifdef DEBUG_OUTPUT
    std::cout << "MOD " << mName << std::endl;
    #endif
    BinaryReader reader(data, dataLength);
    
    // First 4 bytes: file identifier "LDOM" (MODL backwards).
    std::string identifier = reader.ReadString(4);
    if(identifier != "LDOM")
    {
        std::cout << "MOD file does not have MODL identifier!" << std::endl;
        return;
    }
    
    // 4 bytes: First 2 are a major/minor version number. Next 2 unknown.
    reader.ReadUInt();
    
    // 4 bytes: Number of meshes in this model.
    unsigned int numMeshes = reader.ReadUInt();
    #ifdef DEBUG_OUTPUT
    std::cout << "  Mesh Count: " << numMeshes << std::endl;
    #endif
    
    // 4 bytes: Size of the model data in bytes.
    // Always 48 bytes LESS than the total size (b/c header data is 48 bytes).
	// Not really needed to parse everything.
    reader.ReadUInt();
    
    // 4 bytes: unknown - usually zero, but not always (GAB.MOD had 0x0000C842).
    // Could maybe be a floating-point value? 0x000C842 = 100.0f
    reader.ReadUInt();
	
	// Unknown
	reader.Skip(4);
    
    // 24 bytes: mostly unknown - most files have had zeros here thus far.
	// These are likely flags of some sort - will document as I figure it out.
	{
		// A value of "2" indicates that this model should render as a billboard.
		unsigned int flags = reader.ReadUInt();
		if((flags & 2) != 0)
		{
			//std::cout << "  Billboard Model!" << std::endl;
			mBillboard = true;
		}
		
		// Unknown
		reader.Skip(16);
		
		// 4 bytes: unknown - has thus far always been the number 8.
		reader.Skip(4);
	}
	
    // Now, we iterate over each mesh in the file.
    for(int i = 0; i < numMeshes; i++)
    {
        #ifdef DEBUG_OUTPUT
        std::cout << "  Mesh " << i << std::endl;
        #endif
        
        // 4 bytes: mesh block identifier "HSEM" (MESH backwards).
        identifier = reader.ReadString(4);
        if(identifier != "HSEM")
        {
            std::cout << "Expected MESH identifier. Instead got " << identifier << std::endl;
            return;
        }

		// 36 bytes: i/j/k bases (aka x/y/z axes of mesh coordinate system).
		// j/k are flipped b/c this data is in "Z-up" format, but our engine in "Y-up".
        Vector3 iBasis = reader.ReadVector3();
        Vector3 kBasis = reader.ReadVector3();
        Vector3 jBasis = reader.ReadVector3();
        
        // 12 bytes: an (X, Y, Z) position in "mesh space."
        // Each mesh within a model has a local position relative to the model origin.
		// Ex: if a human model has arms, legs, etc - this positions them all correctly relative to one another.
		Vector3 meshPos = reader.ReadVector3();
		#ifdef DEBUG_OUTPUT
        std::cout << "    Mesh Position: " << meshPos << std::endl;
		#endif
        
		// Generate transform matrix from i/j/k bases and mesh position.
		// This mesh allows us to go from "mesh space" to "local space" (i.e. local space of an Actor).
		// Assuming the actor is at world origin (or there is no actor), this is actually "world space."
		Matrix4 meshToLocalMatrix;
		meshToLocalMatrix.SetColumns(Vector4(iBasis), Vector4(jBasis), Vector4(kBasis), Vector4(meshPos));
		meshToLocalMatrix(3, 3) = 1.0f;
		
		// Create our mesh.
		Mesh* mesh = new Mesh();
		mesh->SetMeshToLocalMatrix(meshToLocalMatrix);
		mMeshes.push_back(mesh);
        
        // 4 bytes: Number of submeshes in this mesh.
        unsigned int numSubMeshes = reader.ReadUInt();
        
        // 24 bytes: Two more sets of floating point values.
        // Based on plot test, seems very likely these are min/max bound values for the mesh.
		Vector3 min;
		Vector3 max;
		min.x = reader.ReadFloat();
		min.z = reader.ReadFloat();
		min.y = reader.ReadFloat();
		
		max.x = reader.ReadFloat();
		max.z = reader.ReadFloat();
		max.y = reader.ReadFloat();
		
		mesh->SetAABB(AABB(min, max));
		#ifdef DEBUG_OUTPUT
		std::cout << "    Min: " << min << std::endl;
		std::cout << "    Max: " << max << std::endl;
		#endif
        
        // Now, we iterate over each submesh in this mesh.
        for(int j = 0; j < numSubMeshes; j++)
        {
            #ifdef DEBUG_OUTPUT
            std::cout << "    Submesh " << j << std::endl;
            #endif
            
            // 4 bytes: submesh block identifier "PRGM" (MGRP backwards).
			// I think GK3 called these "mesh groups", which is why the identifier is "MGRP".
            identifier = reader.ReadString(4);
            if(identifier != "PRGM")
            {
                std::cout << "Expected MGRP identifier." << std::endl;
                return;
            }
			
            // 32 bytes: the name of the texture for this submesh
            std::string textureName = reader.ReadString(32);
            #ifdef DEBUG_OUTPUT
            std::cout << "      Texture name: " << textureName << std::endl;
            #endif
            
            // 4 bytes: unknown - often is (0x00FFFFFF), but not always.
            // Have also seen: 0x03773BB3, 0xFF000000, 0x50261200
            // Maybe a color value?
            reader.ReadUInt();

            // 4 bytes: unknown - seems to usually be 1, sometimes 0.
            reader.ReadUInt();
            
            // 4 bytes: Vertex count for this submesh.
            int vertexCount = reader.ReadUInt();
            #ifdef DEBUG_OUTPUT
            std::cout << "      Vertex count: " << vertexCount << std::endl;
            #endif
            
            // Based on vertex count, we can allocate some arrays for data.
            float* vertexPositions = new float[vertexCount * 3];
            float* vertexNormals = new float[vertexCount * 3];
            float* vertexUVs = new float[vertexCount * 2];
            
            // 4 bytes: Face count, for drawing indexed mesh. Each face has 3 vertices (triangles).
            int faceCount = reader.ReadUInt();
            unsigned short* vertexIndexes = new unsigned short[faceCount * 3];
            #ifdef DEBUG_OUTPUT
            std::cout << "      Face count: " << faceCount << std::endl;
            #endif
            
            // 4 bytes: Number of LODK blocks in this submesh. Not uncommon to be 0.
			// My guess: this is for level-of-detail variants for the submesh?
            unsigned int lodkCount = reader.ReadUInt();
			#ifdef DEBUG_OUTPUT
            std::cout << "      LODK count: " << lodkCount << std::endl;
			#endif
            
            // 4 bytes: unknown - always zero thus far.
            reader.ReadUInt();
            
            // Next we have vertex positions.
            #ifdef DEBUG_OUTPUT
            //std::cout << "      Vertex positions: " << std::endl;
            #endif
            for(int k = 0; k < vertexCount; k++)
            {
                float x = reader.ReadFloat();
				float z = reader.ReadFloat();
                float y = reader.ReadFloat();
                vertexPositions[k * 3] = x;
                vertexPositions[k * 3 + 1] = y;
                vertexPositions[k * 3 + 2] = z;
                
                #ifdef DEBUG_OUTPUT
                //std::cout << Vector3(x, y, z);
                #endif
            }
            #ifdef DEBUG_OUTPUT
            //std::cout << std::endl;
            #endif
            
            // Then we have vertex normals.
            for(int k = 0; k < vertexCount; k++)
            {
                Vector3 normal = reader.ReadVector3();
                vertexNormals[k * 3] = normal.x;
                vertexNormals[k * 3 + 1] = normal.z;
                vertexNormals[k * 3 + 2] = normal.y;
            };
            
            // Vertex UV coordinates.
            for(int k = 0; k < vertexCount; k++)
            {
                Vector2 uv = reader.ReadVector2();
                vertexUVs[k * 2] = uv.x;
                vertexUVs[k * 2 + 1] = uv.y;
            }
            
            // Next comes vertex indexes for drawing from an IBO.
            // Common sequence would be (2, 1, 0) or (5, 4, 3), referring to vertex indexes above.
            for(int k = 0; k < faceCount; k++)
            {
                vertexIndexes[k * 3] = reader.ReadUShort();
                vertexIndexes[k * 3 + 1] = reader.ReadUShort();
                vertexIndexes[k * 3 + 2] = reader.ReadUShort();
                
                // Every 4th number seems out of place - not sure what they mean.
                // Seen: 0xF100 (241), 0x0000 (0), 0x0701 (263), 0x7F3F (16255), 0x56B1 (45398),
                // 0x9B3E (16027), 0x583F (16216), 0xCC0D (3532), 0xCD0D (3533)
				reader.ReadUShort(); // WHAT IS IT!?
            }
            
            // Generate mesh from data.
            MeshDefinition meshDefinition;
            meshDefinition.meshUsage = MeshUsage::Dynamic;
            
            meshDefinition.vertexDefinition.layout = VertexDefinition::Layout::Packed;
            meshDefinition.vertexDefinition.attributes.push_back(VertexAttribute::Position);
            meshDefinition.vertexDefinition.attributes.push_back(VertexAttribute::Normal);
            meshDefinition.vertexDefinition.attributes.push_back(VertexAttribute::UV1);
            
            meshDefinition.vertexCount = vertexCount;
            std::vector<float*> vertexData;
            vertexData.push_back(vertexPositions);
            vertexData.push_back(vertexNormals);
            vertexData.push_back(vertexUVs);
            meshDefinition.vertexData = &vertexData[0];
        
            meshDefinition.indexCount = faceCount * 3;
            meshDefinition.indexData = vertexIndexes;
            
            // Create submesh.
            Submesh* submesh = mesh->AddSubmesh(meshDefinition);
            submesh->SetPositions(vertexPositions);
            submesh->SetNormals(vertexNormals);
            submesh->SetUV1s(vertexUVs);
            submesh->SetIndexes(vertexIndexes);
            
            // Save texture name.
            submesh->SetTextureName(textureName);
            
            // Next comes LODK blocks for this mesh group.
            // Not totally sure what these are for, but maybe LOD groups?
            for(int k = 0; k < lodkCount; k++)
            {
                // Identifier should be "KDOL" for this block.
                identifier = reader.ReadString(4);
                if(identifier != "KDOL")
                {
                    std::cout << "Expected LODK identifier. Instead found " << identifier << std::endl;
                    return;
                }
                
                // First three values in LODK block are counts for how much data to read after.
                int unknownCount1 = reader.ReadUInt();
                int unknownCount2 = reader.ReadUInt();
                int unknownCount3 = reader.ReadUInt();
                //std::cout << k << ": " << unknownCount1 << ", " << unknownCount2 << ", " << unknownCount3 << std::endl;
                
                // Read in all values. Currently don't know what they are though.
                for(int l = 0; l < unknownCount1; l++)
                {
                    reader.ReadUShort();
                    reader.ReadUShort();
                    reader.ReadUShort();
                    reader.ReadUShort();
                }
                for(int l = 0; l < unknownCount2; l++)
                {
                    reader.ReadUShort();
                    reader.ReadUShort();
                }
                for(int l = 0; l < unknownCount3; l++)
                {
                    reader.ReadUShort();
                }
            }
        }
    }
    
    // After all meshes and mesh groups, there is some additional data.
    // 4 bytes: identifier "XDOM" (MODX backwards).
    identifier = reader.ReadString(4);
    if(identifier != "XDOM")
    {
        std::cout << "Expected MODX identifier." << std::endl;
        return;
    }
    
    /*
    // There seems to always be exactly one GRPX block for each MGRP block earlier.
    // Each GRPX block's size correlates in some way to the size of the earlier MGRP block...
    for(int i = 0; i < meshGroupCount; i++)
    {
        // 4 bytes: identifier "XPRG" (GRPX backwards).
        identifier = reader.ReadString(4);
        if(identifier != "XPRG")
        {
            cout << "Expected GRPX identifier." << endl;
            return;
        }
        
        // 4 bytes: vertex count (why, what good is that?)
        unsigned int vertexCount = reader.ReadUInt();
        
        // The next n bytes represent a certain number of ushort values.
        // Not yet clear how to derive the size of this block, however.
    }
    */
}
