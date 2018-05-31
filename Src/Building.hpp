#ifndef BUILDING_H
#define BUILDING_H
#include "SoulStoneEngine/Utilities/Vector3.h"
#include "SoulStoneEngine/Utilities/GameCommon.hpp"
#include "SoulStoneEngine/Utilities/AABB3.hpp"
#include "SoulStoneEngine/Render/GraphicManager.hpp"
#include "SoulStoneEngine/Render/GraphicText.hpp"


enum BuildingType { BLOCKY_BUILDING, MODERN_ROUND_BUILDING };
enum Orientation { COLINEAR = 0, CLOCKWISE = 1, COUNTER_CLOCKWISE = 2 };
const unsigned char BUILDING_MIN_FOOTPRINT_SIZE = 6;
const unsigned char BUILDING_MAX_FOOTPRINT_SIZE = 12;
const unsigned char MIN_HEIGHT = 5;

struct EnginePolygon
{
	std::vector<SimpleVertex3D> m_vertexList;
};

class Building
{
	public:
		unsigned char					m_height;
		Vector3							m_center;
		BuildingType					m_type;
		unsigned char					m_footprintSize;
		RGBColor						m_buildingLightColor;
		GraphicText*					m_buildingSign;
		static std::string				m_signName[5];

	private:
		std::vector<AABB3>				m_buildingBlocks;
		unsigned char					m_floorIteration;
		std::vector<SimpleVertex3D>		m_vertexList;
		unsigned int					m_numVertex;
		unsigned int					m_vboId;
		bool							m_isVboDirty;
		int								m_roundBuildingBodyVertexNum;
		int								m_isLightOn;
		int								m_lightOffPeriod;
		float							m_lightOffDuration;
		float							m_lightOffTime;
		float							m_accumulateTime;

	private:
		void GenerateBlockyBuildingTop( const Vector3& bottomLeft, float widthX, float depthY );
		void GenerateBlockyBuilding();
		void GenerateModernRoundBuilding();
		void GenerateClassicBuilding();
		EnginePolygon GenerateRegularPolygon( const Vector3& center, float radius, unsigned short numSize, float orientationRadian );
		void CreateVBO();
		void RenderVBO();
		Orientation CalcOrientation( const Vector3& p1, const Vector3& p2, const Vector3& p3 );
		void ConstructBlock( const Vector3& bottomLeft, float width, float depth, float height, const RGBColor& color );
		void ConstructCylinder( const Vector3& center, float radius, int sideNum );
		void CalcBuildingSignPositionAndRotation( const Vector3& bottomLeft, float width, float depth );

	public:
		Building( BuildingType type, const Vector3& center, unsigned char iteration, unsigned char numFloor );
		~Building() {}
		void Render();
		void Update( float elapsedTime );
};


#endif