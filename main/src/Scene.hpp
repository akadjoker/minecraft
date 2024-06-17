
#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include "Config.hpp"
#include "Math.hpp"
#include "Texture.hpp"
#include "SceneNode.hpp"
#include "Animation.hpp"
#include "Buffer.hpp"

const int SHADOW_MAP_CASCADE_COUNT = 4;
  

class   Scene
{
public:
    Scene();
    ~Scene();

    static Scene& Instance();
    static Scene* InstancePtr();



    void Init();

    void Render();

    void Update(float elapsed);


    void Release();

    void SetViewMatrix(const Mat4 &m) { viewMatrix = m; }
    void SetProjectionMatrix(const Mat4 &m) { projectionMatrix = m; }
    void SetCameraPosition(const Vec3 &p) { cameraPosition = p; }

  
  
    

private:
    static Scene* m_singleton;
    Shader m_modelShader;
    Shader m_depthShader;
    Shader m_debugShader;
    Shader m_quadShader;

struct Cascade
{
    float splitDepth;
    Mat4 viewProjMatrix;

};

    CascadeShadow depthBuffer;
    RenderQuad  quadRender;
    Cascade cascades[SHADOW_MAP_CASCADE_COUNT];
    Vec3 lightPosition;
    TextureBuffer renderTexture;

    Mat4 viewMatrix;
    Mat4 projectionMatrix;
    Vec3 cameraPosition;
    Vec3 lightPos;


    bool LoadModelShader();
    bool LoadDepthShader();
    void updateCascades(const Vec3 &lightPos);
};