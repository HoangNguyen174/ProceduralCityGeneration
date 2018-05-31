#ifndef CAR_H
#define CAR_H
#include "SoulStoneEngine/Utilities/GameCommon.hpp"
#include "SoulStoneEngine/Render/GraphicManager.hpp"

class Car
{
	public:
		std::vector<SimpleVertex3D>				m_vertices;
		Vector2									m_position;
		Vector2									m_initPosition;
		Vector2									m_velocity;
		RGBColor								m_color;
		Matrix44								m_modelMatrix;
		unsigned int							m_vboID;

	public:
		Car( const Vector2& position, const Vector2& velocity, const RGBColor& color );
		~Car();
		void CreateCircleVertices();
		void Update( float elapsedTime );
		void Render();
};

#endif