#ifndef CITY_MANAGER_H
#define CITY_MANAGER_H
#include "SoulStoneEngine/Utilities/GameCommon.hpp"
#include "SoulStoneEngine/Render/SkyBox.hpp"
#include "Building.hpp"
#include "BuildingFacadeTexture.hpp"
#include "BuildingBlock.hpp"
#include "Car.hpp"

struct BlockInfo 
{
	unsigned char widthX;
	unsigned char depthY;
	unsigned char rightOffset;
	unsigned char topOffset;
	Vector2       bottomLeft;
};

const float HALF_RESOLUTION_WIDTH = WINDOW_PHYSICAL_WIDTH * 0.5f;
const float HALF_RESOLUTION_HEIGHT = WINDOW_PHYSICAL_HEIGHT * 0.5f;
const float A_FOURTH_RESOLUTION_WIDTH = WINDOW_PHYSICAL_WIDTH * 0.25f;
const float A_FOUTH_RESOLUTION_HEIGHT = WINDOW_PHYSICAL_HEIGHT * 0.25f;

class CityManager
{
	public:
		static BuildingFacadeTexture*	s_buildingFacadeTexture;
		static OpenGLShaderProgram*		s_buildingShaderProgram;
		static OpenGLShaderProgram*		s_bloomShaderProgram;
		static int						s_cityWidthX;
		static int						s_cityDepthY;
		unsigned char					m_numBlockX;
		unsigned char					m_numBlockY;
		Texture*						m_skyboxTexture;
		float							m_bloomIntensity;
		float							m_bloomSaturation;
		float							m_originalIntensity;
		float							m_originalSaturation;
		bool							m_bloomIntensitySelected;
		bool							m_loomSaturationSelected;
		bool							m_originalIntensitySelected;
		bool							m_originalSaturationSelected;


	private:
		std::vector<Building*>			m_buildings;
		std::vector<BuildingBlock*>		m_buildingBlocks;
		std::vector<BlockInfo>			m_blocksInfo;
		std::vector<SimpleVertex3D>		m_streetVertices;
		std::vector<Car*>				m_cars;
		std::vector<SimpleVertex3D>		m_carVertices;
		unsigned int					m_streetVBOid;
		bool							m_isStreetVBOdirty;
		bool							m_isRenderingGlow;
		Texture*						m_streetTexture;
		Texture*						m_lowResTexture;
		FBO*							m_colorFBO;
		FBO*							m_glowFBO;
		FBO*							m_glowFBOHalfRes;
		FBO*							m_glowFBOAFourthRes;
		FBO*							m_hBlurFBO;
		FBO*							m_vBlurFBO;
		FBO*							m_hBlurFBOHalfRes;
		FBO*							m_vBlurFBOHalfRes;
		FBO*							m_hBlurFBOAFourthRes;
		FBO*							m_vBlurFBOAFourthRes;
		FBO*							m_finalFBO;
		SkyBox*							m_skybox;

	public:
		CityManager();
		~CityManager();
		void Render();
		void Update( float elapsedTime );
		void VisualizeBuildingBlocks();

	private:
		void GenerateBuildingTexture();
		void GenerateBuildingBlocks();
		void GenerateStreetVertices();
		void GenerateCars();
		void CreateVBOForStreetVertices();
		void RenderBlocks();
		void RenderStreet();
		void RenderCars();
		void CombiningBloom();
};

#endif