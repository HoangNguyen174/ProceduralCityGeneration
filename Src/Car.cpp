#include "Car.hpp"
#include "CityManager.hpp"

Car::Car( const Vector2& position, const Vector2& velocity, const RGBColor& color )
{
	m_position = position;
	m_color = color;
	m_initPosition = position;
	m_velocity = velocity;
	m_vboID = 0;
	CreateCircleVertices();
}

Car::~Car()
{

}

void Car::CreateCircleVertices()
{
	int sideNum = 24;
	float radius = 0.25f;
	float theta = 0.f;
	SimpleVertex3D vertex;
	vertex.m_color = m_color;

	m_vertices.push_back( vertex );

	for( int i = 0; i <= sideNum; i++ )
	{
		float delta = 2 * PI / sideNum;
		vertex.m_position.x = radius * cos( theta );
		vertex.m_position.y = radius * sin( theta );
		vertex.m_position.z = 0.05f;
		m_vertices.push_back( vertex );

		theta += delta;
	}
}

void Car::Update(float elapsedTime)
{
	m_position += m_velocity * elapsedTime;
	if( m_position.x > CityManager::s_cityWidthX )
		m_position.x = 0.f;
	else if( m_position.x < 0 )
		m_position.x = CityManager::s_cityWidthX;

	if( m_position.y > CityManager::s_cityDepthY )
		m_position.y = 0.f;
	else if( m_position.y < 0 )
		m_position.y = CityManager::s_cityDepthY;

	m_modelMatrix = Matrix44::CreateTranslationMatrix( Vector3( m_position.x, m_position.y, 0.f ) );
}

void Car::Render()
{
	if( m_vboID == 0 )
	{
		GraphicManager::s_render->GenerateBuffer( 1, &m_vboID );
	}
	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboID );
	GraphicManager::s_render->BufferData( GL_ARRAY_BUFFER, sizeof( SimpleVertex3D ) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW );

	CityManager::s_buildingShaderProgram->SetMat4UniformValue( "u_modelMatrix", m_modelMatrix );
	CityManager::s_buildingShaderProgram->SetIntUniformValue( "u_renderGlowPart", 0 );

	glEnableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glEnableVertexAttribArray( VERTEX_ATTRIB_COLORS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, m_vboID );

	glVertexAttribPointer( VERTEX_ATTRIB_POSITIONS, 3, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_position ) );
	glVertexAttribPointer( VERTEX_ATTRIB_COLORS, 4, GL_FLOAT, GL_FALSE, sizeof( SimpleVertex3D ), ( const GLvoid* ) offsetof( SimpleVertex3D,m_color ) );

	GraphicManager::s_render->DrawArray( GL_TRIANGLE_FAN ,0 , m_vertices.size() );

	glDisableVertexAttribArray( VERTEX_ATTRIB_POSITIONS );
	glDisableVertexAttribArray( VERTEX_ATTRIB_COLORS );

	GraphicManager::s_render->BindBuffer( GL_ARRAY_BUFFER, 0 );
}
