
#include "pch.h"
#include "Scene.hpp"


const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 2000.0f;
const bool USE_BLOOM = false;

static VertexFormat::Element VertexElements[] =
    {
        VertexFormat::Element(VertexFormat::POSITION, 3),
        VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
        VertexFormat::Element(VertexFormat::NORMAL, 3),
  
    };   
static int VertexElemntsCount =3;





const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;





Scene*  Scene::m_singleton = 0x0;


  

Scene& Scene::Instance()
{
    DEBUG_BREAK_IF(!m_singleton);
    return (*m_singleton);
}
Scene* Scene::InstancePtr()
{
    return m_singleton;

}

Scene::Scene()
{
    m_singleton = this;
}

Scene::~Scene()
{
     m_singleton = 0x0;
}

void Scene::Init()
{

    int width = Device::Instance().GetWidth();
    int height = Device::Instance().GetHeight();


    LoadModelShader();
    
    LoadDepthShader();

    depthBuffer.Init(SHADOW_MAP_CASCADE_COUNT,SHADOW_WIDTH, SHADOW_HEIGHT);

    quadRender.Init(width, height);

    renderTexture.Init(width, height);

    lightPosition = Vec3(0.5f, 4.0f,  7.5f);




   
}


void Scene::Release()
{


    m_quadShader.Release();
    m_debugShader.Release();
    depthBuffer.Release();
    m_depthShader.Release();
    m_modelShader.Release();
    renderTexture.Release();

    

    

}




void Scene::Render()
{
   

        int width = Device::Instance().GetWidth();
        int height = Device::Instance().GetHeight();

        



    
       updateCascades(lightPosition);



        Mat4 lightProjection;
        Mat4 lightView;
        Mat4 lightSpaceMatrix;




        Driver::Instance().SetClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        Driver::Instance().SetBlend(true);
        Driver::Instance().SetBlendMode(BlendMode::BLEND);
        Driver::Instance().SetDepthTest(true);
        Driver::Instance().SetDepthClamp(true);


        m_depthShader.Use();
        glCullFace(GL_FRONT);
        depthBuffer.Begin();

            for (u32 j = 0; j < SHADOW_MAP_CASCADE_COUNT; j++)
            {

                depthBuffer.Set(j);
                lightSpaceMatrix = cascades[j].viewProjMatrix;
                m_depthShader.SetMatrix4("lightSpaceMatrix", &lightSpaceMatrix.c[0][0]);
            
                // for (u32 i = 0; i < m_nodes.size(); i++)
                // {
                //     StaticNode *node = m_nodes[i];
                //     if (!node->shadow || !node->visible) continue;
                //     Mat4 m = m_nodes[i]->GetRelativeTransformation();
                //     m_depthShader.SetMatrix4("model", m.x);
                //     m_nodes[i]->RenderNoMaterial();
                // }

               
            }

        depthBuffer.End();
        glCullFace(GL_BACK);
        
        
        Driver::Instance().SetViewport(0, 0, width, height);
        
        Driver::Instance().SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        Driver::Instance().Clear();
    


    Vec3 lightDir = Vec3::Normalize(Vec3(-20.0f, 50, 20.0f));

    m_modelShader.Use();
    m_modelShader.SetMatrix4("view", viewMatrix.x);
    m_modelShader.SetMatrix4("projection", projectionMatrix.x);
    m_modelShader.SetFloat("viewPos", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    m_modelShader.SetFloat("lightDir",lightDir.x, lightDir.y, lightDir.z);
    m_modelShader.SetFloat("farPlane", FAR_PLANE);

    for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        m_modelShader.SetMatrix4("cascadeshadows["+std::to_string(i)+"].projViewMatrix", &cascades[i].viewProjMatrix.c[0][0]);
        m_modelShader.SetFloat("cascadeshadows["+std::to_string(i)+"].splitDistance", cascades[i].splitDepth);
        m_modelShader.SetInt("shadowMap["+std::to_string(i)+"]", i + 1);
            
    }
    depthBuffer.BindTextures(1);    




    
    m_modelShader.SetInt("isStatic", 1);


    if (USE_BLOOM)
        renderTexture.Begin();

   
  
      

    
    
    // for (u32 i = 0; i < m_nodes.size(); i++)
    // {
    //     if (!m_nodes[i]->visible) continue;
    //     Mat4 m = m_nodes[i]->GetRelativeTransformation();
    //     m_modelShader.SetMatrix4("model", m.x);
    //     m_nodes[i]->Render();
    // }


    
    if (USE_BLOOM)
        renderTexture.End();

  
    Driver::Instance().SetDepthTest(true);

    if (USE_BLOOM)
    {
        renderTexture.BindTexture(0);
        m_quadShader.Use();
        Driver::Instance().SetViewport(0, 0, width, height);
        quadRender.Render();
    }


    //  depthBuffer.BindTexture(1);   
    //  quadRender.Render(-0.8f,0.35f,0.4f);

    //  depthBuffer.BindTexture(2);   
    //  quadRender.Render(-0.8f,-0.1f,0.4f);

    //     Driver::Instance().SetBlend(true);

}

void Scene::Update(float elapsed)
{

}

void Scene::updateCascades(const Vec3 & lightPos)
	{
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];
     	float cascadeSplitLambda = 0.95f;

        float nearClip = NEAR_PLANE;
		float farClip = FAR_PLANE;
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

        for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
        {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * Pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
        }
	
		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) 
        {
			float splitDist = cascadeSplits[i];

			Vec3 frustumCorners[8] = 
            {
				Vec3(-1.0f,  1.0f, 0.0f),
				Vec3( 1.0f,  1.0f, 0.0f),
				Vec3( 1.0f, -1.0f, 0.0f),
				Vec3(-1.0f, -1.0f, 0.0f),
				Vec3(-1.0f,  1.0f,  1.0f),
				Vec3( 1.0f,  1.0f,  1.0f),
				Vec3( 1.0f, -1.0f,  1.0f),
				Vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			Mat4 invCam =Mat4::Inverse(projectionMatrix * viewMatrix);   
			for (u32 j = 0; j < 8; j++) 
            {
				Vec4 invCorner = invCam * Vec4(frustumCorners[j], 1.0f);
                Vec4 div = invCorner / invCorner.w;
				frustumCorners[j] = Vec3(div.x, div.y, div.z);
			}

			for (u32 j = 0; j < 4; j++) 
            {
				Vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

			// Get frustum center
			Vec3 frustumCenter = Vec3(0.0f);
			for (u32 j = 0; j < 8; j++) 
            {
				frustumCenter += frustumCorners[j];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (u32 j = 0; j < 8; j++) 
            {
				float distance = Vec3::Length(frustumCorners[j] - frustumCenter);
				radius = Max(radius, distance);
			}
			radius = Ceil(radius * 16.0f) / 16.0f;

			Vec3 maxExtents = Vec3(radius);
			Vec3 minExtents = -maxExtents;

			Vec3 lightDir = Vec3::Normalize(-lightPos);
			Mat4 lightViewMatrix  = Mat4::LookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, Vec3(0.0f, 1.0f, 0.0f));
			Mat4 lightOrthoMatrix = Mat4::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);


      

			// Store split distance and matrix in cascade
			cascades[i].splitDepth = (NEAR_PLANE + splitDist * clipRange) * -1.0f;
			cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

bool Scene::LoadModelShader()
{
   
   LogWarning("Load default  shader");

   if (!m_modelShader.Load("assets/shaders/default.vert", "assets/shaders/default.frag"))
   {
        DEBUG_BREAK_IF(true);
        return false;
   }
 
 
    m_modelShader.LoadDefaults();
    m_modelShader.SetInt("diffuseTexture", 0);
    m_modelShader.SetInt("isStatic", 1);
    m_modelShader.SetFloat("viewPos", 0.0f, 0.0f, 0.0f);
    for (u32 i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        m_modelShader.SetInt("shadowMap["+std::to_string(i)+"]", i + 1);
    }

    return true;
}


bool Scene::LoadDepthShader()
{


        LogWarning("Loading Depth Shader");

        if (!m_depthShader.Load("assets/shaders/depth.vert", "assets/shaders/depth.frag"))
        {
            DEBUG_BREAK_IF(true);
            return false;
        }
        m_depthShader.LoadDefaults();
        m_depthShader.SetInt("isStatic", 1);

       


      LogWarning("Loading Debug Depth Shader");
     
       if (!m_debugShader.Load("assets/shaders/debugDepth.vert", "assets/shaders/debugDepth.frag"))
       {
           DEBUG_BREAK_IF(true);
           return false;
       }
        m_debugShader.LoadDefaults();
        m_debugShader.SetInt("depthMap", 0);



        
        LogWarning("Loading Quad Shader");
        if (!m_quadShader.Load("assets/shaders/bloom.vert", "assets/shaders/bloom.frag"))
        {
            DEBUG_BREAK_IF(true);
            return false;
        }
        m_quadShader.LoadDefaults();
        m_quadShader.SetInt("textureMap", 0);

    

        return true;
}



