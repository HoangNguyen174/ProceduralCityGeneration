#ifndef BUILDING_FACADE_TEXTURE
#define BUILDING_FACADE_TEXTURE

#include "SoulStoneEngine/Render/FBO.hpp"
#include "SoulStoneEngine/Utilities/GameCommon.hpp"
#include "SoulStoneEngine/Render/OpenGLShaderProgram.hpp"
#include "SoulStoneEngine/Render/GLRender.hpp"

enum LitIntensity { FULL, MEDIUM, REFLECT, UNLIT };

class BuildingFacadeTexture
{
	public:
		FBO*						m_buildingFacadeFBO;
		OpenGLShaderProgram	*		m_shaderProgram;
		unsigned int				m_textureSize;

	private:
		unsigned int				m_windowInARow;
		std::vector<SimpleVertex3D>	m_windowVertexList;
		std::vector<LitIntensity>	m_windows;
		unsigned int				m_vboId;
		unsigned int				m_numVertex;
		Texture*					m_noiseTexture;

	public:
		BuildingFacadeTexture();
		~BuildingFacadeTexture();
		//Vector2 GetRandom
		void Render();
	
	private:
		void GenerateWindowLight();
		void Get8NeighborWindowIndex( int windowIndex, int indexList[8] );
		void GenerateVertexList();
		void CreateVBO();
};

#endif