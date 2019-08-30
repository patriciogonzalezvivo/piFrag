#include "scene.h"
#include "window.h"

#include <unistd.h>

#include "tools/text.h"
#include "tools/shapes.h"

#include "default_scene_shaders.h"

Scene::Scene(): 
    // Camera.
    m_culling(NONE), m_mvp(1.0), m_lat(180.0), m_lon(0.0),
    // Light
    m_light_vbo(nullptr), m_dynamicShadows(false),
    // CubeMap
    m_cubemap_vbo(nullptr), m_cubemap(nullptr), m_cubemap_skybox(nullptr), m_cubemap_draw(false), 
    // UI
    m_grid_vbo(nullptr), m_axis_vbo(nullptr),
    // State
    m_change(true) {
}

Scene::~Scene(){
    clear();
}

void Scene::setup(CommandList &_commands, Uniforms &_uniforms) {
    m_camera.setViewport(getWindowWidth(), getWindowHeight());
    m_camera.setPosition(glm::vec3(0.0,0.0,-3.));
    m_light.setPosition(glm::vec3(0.0,100.0,100.0));

    // COMMANDS 
    _commands.push_back(Command("culling", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 1) {
            if (getCulling() == NONE) {
                std::cout << "none" << std::endl;
            }
            else if (getCulling() == FRONT) {
                std::cout << "front" << std::endl;
            }
            else if (getCulling() == BACK) {
                std::cout << "back" << std::endl;
            }
            else if (getCulling() == BOTH) {
                std::cout << "both" << std::endl;
            }
            return true;
        }
        else if (values.size() == 2) {
            if (values[1] == "none") {
                setCulling(NONE);
            }
            else if (values[1] == "front") {
                setCulling(FRONT);
            }
            else if (values[1] == "back") {
                setCulling(BACK);
            }
            else if (values[1] == "both") {
                setCulling(BOTH);
            }
            return true;
        }

        return false;
    },
    "culling[,<none|front|back|both>]   get or set the culling modes"));

    _commands.push_back(Command("camera_distance", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_camera.setDistance(toFloat(values[1]));
            flagChange();
            return true;
        }
        else {
            std::cout << m_camera.getDistance() << std::endl;
            return true;
        }
        return false;
    },
    "camera_distance[,<dist>]       get or set the camera distance to the target."));

    _commands.push_back(Command("camera_fov", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_camera.setFOV(toFloat(values[1]));
            flagChange();
            return true;
        }
        else {
            std::cout << m_camera.getFOV() << std::endl;
            return true;
        }
        return false;
    },
    "camera_fov[,<field_of_view>]   get or set the camera field of view."));

    _commands.push_back(Command("camera_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_camera.setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            m_camera.lookAt(m_camera.getTarget());
            flagChange();
            return true;
        }
        else {
            glm::vec3 pos = m_camera.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "camera_position[,<x>,<y>,<z>]  get or set the camera position."));

    _commands.push_back(Command("camera_exposure", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_camera.setExposure(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            flagChange();
            return true;
        }
        else {
            std::cout << m_camera.getExposure() << std::endl;
            return true;
        }
        return false;
    },
    "camera_exposure[,<aperture>,<shutterSpeed>,<sensitivity>]  get or set the camera exposure. Defaults: 16, 1/125s, 100 ISO"));

    _commands.push_back(Command("light_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_light.setPosition(glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])));
            m_light.bChange = true;
            flagChange();
            return true;
        }
        else {
            glm::vec3 pos = m_light.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "light_position[,<x>,<y>,<z>]  get or set the light position."));

    _commands.push_back(Command("light_color", [&](const std::string& _line){ 
         std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_light.color = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            flagChange();
            return true;
        }
        else {
            glm::vec3 color = m_light.color;
            std::cout << color.x << ',' << color.y << ',' << color.z << std::endl;
            return true;
        }
        return false;
    },
    "light_color[,<r>,<g>,<b>]      get or set the light color."));
    

    _commands.push_back(Command("dynamic_shadows", [&](const std::string& _line){ 
        if (_line == "dynamic_shadows") {
            std::string rta = getDynamicShadows() ? "on" : "off";
            std::cout <<  rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setDynamicShadows( (values[1] == "on") );
            }
        }
        return false;
    },
    "dynamic_shadows[on|off]        get or set dynamic shadows"));

    _commands.push_back(Command("skybox_ground", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_skybox.groundAlbedo = glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3]));
            setCubeMap(&m_skybox);
            flagChange();
            return true;
        }
        else {
            std::cout << m_skybox.groundAlbedo.x << ',' << m_skybox.groundAlbedo.y << ',' << m_skybox.groundAlbedo.z << std::endl;
            return true;
        }
        return false;
    },
    "skybox_ground[,<r>,<g>,<b>]      get or set the ground color of the skybox."));

    _commands.push_back(Command("skybox_elevation", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.elevation = toFloat(values[1]);
            setCubeMap(&m_skybox);
            flagChange();
            return true;
        }
        else {
            std::cout << m_skybox.elevation << std::endl;
            return true;
        }
        return false;
    },
    "skybox_elevation[,<sun_elevation>]  get or set the sun elevation (in rads) of the skybox."));

    _commands.push_back(Command("skybox_azimuth", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.azimuth = toFloat(values[1]);
            setCubeMap(&m_skybox);

            flagChange();
            return true;
        }
        else {
            std::cout << m_skybox.azimuth << std::endl;
            return true;
        }
        return false;
    },
    "skybox_azimuth[,<sun_azimuth>]  get or set the sun azimuth (in rads) of the skybox."));

    _commands.push_back(Command("skybox_turbidity", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            m_skybox.turbidity = toFloat(values[1]);
            setCubeMap(&m_skybox);
            flagChange();
            return true;
        }
        else {
            std::cout << m_skybox.turbidity << std::endl;
            return true;
        }
        return false;
    },
    "skybox_turbidity[,<sky_turbidty>]  get or set the sky turbidity of the m_skybox."));

    _commands.push_back(Command("skybox", [&](const std::string& _line){
        if (_line == "skybox") {
            std::string rta = getCubeMapVisible() ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setCubeMap(&m_skybox);
                setCubeMapVisible( values[1] == "on" );
            }
        }
        return false;
    },
    "skybox[,on|off]       show/hide skybox"));

    _commands.push_back(Command("cubemap", [&](const std::string& _line){
        if (_line == "cubemap") {
            std::string rta = getCubeMapVisible() ? "on" : "off";
            std::cout << rta << std::endl; 
            return true;
        }
        else {
            std::vector<std::string> values = split(_line,',');
            if (values.size() == 2) {
                setCubeMapVisible( values[1] == "on" );
            }
        }
        return false;
    },
    "cubemap[,on|off]       show/hide cubemap"));
    
    _commands.push_back(Command("model_position", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 4) {
            m_model.setPosition( glm::vec3(toFloat(values[1]),toFloat(values[2]),toFloat(values[3])) );
            m_light.bChange = true;
            flagChange();
            return true;
        }
        else {
            glm::vec3 pos = m_model.getPosition();
            std::cout << pos.x << ',' << pos.y << ',' << pos.z << std::endl;
            return true;
        }
        return false;
    },
    "model_position[,<x>,<y>,<z>]  get or set the model position."));

    _commands.push_back(Command("wait", [&](const std::string& _line){ 
        std::vector<std::string> values = split(_line,',');
        if (values.size() == 2) {
            usleep( toFloat(values[1])*1000000 ); 
        }
        return false;
    },
    "wait,<seconds>                 wait for X <seconds> before excecuting another command."));

    // LIGHT UNIFORMS
    //
    _uniforms.functions["u_light"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_light", m_light.getPosition());
    },
    [this]() { return toString(m_light.getPosition(), ','); });

    _uniforms.functions["u_lightColor"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_lightColor", m_light.color);
    },
    [this]() { return toString(m_light.color, ','); });

    _uniforms.functions["u_lightMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_lightMatrix", m_light.getBiasMVPMatrix() );
    });

    _uniforms.functions["u_ligthShadowMap"] = UniformFunction("sampler2D", [this](Shader& _shader) {
        if (m_light_depthfbo.getDepthTextureId()) {
            _shader.setUniformDepthTexture("u_ligthShadowMap", &m_light_depthfbo);
        }
    });

    // IBL UNIFORM
    _uniforms.functions["u_cubeMap"] = UniformFunction("samplerCube", [this](Shader& _shader) {
        if (m_cubemap) {
            _shader.setUniformTextureCube("u_cubeMap", (TextureCube*)m_cubemap);
        }
    });

    _uniforms.functions["u_SH"] = UniformFunction("vec3", [this](Shader& _shader) {
        if (m_cubemap) {
            _shader.setUniform("u_SH", m_cubemap->SH, 9);
        }
    });

    _uniforms.functions["u_iblLuminance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_iblLuminance", 30000.0f * m_camera.getExposure());
    },
    [this]() { return toString(30000.0f * m_camera.getExposure()); });
    
    // CAMERA UNIFORMS
    //
    _uniforms.functions["u_camera"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_camera", -m_camera.getPosition());
    },
    [this]() { return toString(-m_camera.getPosition(), ','); });

    _uniforms.functions["u_cameraDistance"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraDistance", m_camera.getDistance());
    },
    [this]() { return toString(m_camera.getDistance()); });

    _uniforms.functions["u_cameraNearClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraNearClip", m_camera.getNearClip());
    },
    [this]() { return toString(m_camera.getNearClip()); });

    _uniforms.functions["u_cameraFarClip"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraFarClip", m_camera.getFarClip());
    },
    [this]() { return toString(m_camera.getFarClip()); });

    _uniforms.functions["u_cameraEv100"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraEv100", m_camera.getEv100());
    },
    [this]() { return toString(m_camera.getEv100()); });

    _uniforms.functions["u_cameraExposure"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraExposure", m_camera.getExposure());
    },
    [this]() { return toString(m_camera.getExposure()); });

    _uniforms.functions["u_cameraAperture"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraAperture", m_camera.getAperture());
    },
    [this]() { return toString(m_camera.getAperture()); });

    _uniforms.functions["u_cameraShutterSpeed"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraShutterSpeed", m_camera.getShutterSpeed());
    },
    [this]() { return toString(m_camera.getShutterSpeed()); });

    _uniforms.functions["u_cameraSensitivity"] = UniformFunction("float", [this](Shader& _shader) {
        _shader.setUniform("u_cameraSensitivity", m_camera.getSensitivity());
    },
    [this]() { return toString(m_camera.getSensitivity()); });
    
    // MATRIX UNIFORMS
    _uniforms.functions["u_model"] = UniformFunction("vec3", [this](Shader& _shader) {
        _shader.setUniform("u_model", m_model.getPosition());
    },
    [this]() { return toString(m_model.getPosition(), ','); });

    _uniforms.functions["u_normalMatrix"] = UniformFunction("mat3", [this](Shader& _shader) {
        _shader.setUniform("u_normalMatrix", m_camera.getNormalMatrix());
    });

    _uniforms.functions["u_modelMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_modelMatrix", m_model.getTransformMatrix() );
    });

    _uniforms.functions["u_viewMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_viewMatrix", m_camera.getViewMatrix());
    });

    _uniforms.functions["u_projectionMatrix"] = UniformFunction("mat4", [this](Shader& _shader) {
        _shader.setUniform("u_projectionMatrix", m_camera.getProjectionMatrix());
    });
}

void Scene::clear() {
    m_model.clear();

    if (m_light_vbo) {
        delete m_light_vbo;
        m_light_vbo = nullptr;
    }

    if (m_cubemap) {
        delete m_cubemap;
        m_cubemap = nullptr;
    }

    if (m_cubemap_vbo) {
        delete m_cubemap_vbo;
        m_cubemap_vbo = nullptr;
    }

    if (m_grid_vbo) {
        delete m_grid_vbo;
        m_grid_vbo = nullptr;
    }

    if (m_axis_vbo) {
        delete m_axis_vbo;
        m_axis_vbo = nullptr;
    }
}

void Scene::loadModel( const std::string &_path, List &defines ) {
    m_model.load( _path, defines );
    m_camera.setDistance( m_model.getArea() * 2.0 );
    flagChange();
}

void Scene::render(Shader &_shader, Uniforms &_uniforms) {
    // RENDER GEOMETRY WITH MAIN SHADER 
    m_mvp = m_camera.getProjectionViewMatrix();

    if (m_model.loaded()) {
        m_mvp = m_mvp * m_model.getTransformMatrix();
    }
    
    renderGeometry(_shader, _uniforms);
}

void Scene::renderGeometry(Shader &_shader, Uniforms &_uniforms) {
    // Begining of DEPTH for 3D 
    glEnable(GL_DEPTH_TEST);

    if (m_culling != 0) {
        glEnable(GL_CULL_FACE);

        if (m_culling == 1) {
            glCullFace(GL_FRONT);
        }
        else if (m_culling == 2) {
            glCullFace(GL_BACK);
        }
        else if (m_culling == 3) {
            glCullFace(GL_FRONT_AND_BACK);
        }
    }

    // Load main shader
    _shader.use();

    // Update Uniforms and textures variables
    _uniforms.feedTo( _shader);

    // Pass special uniforms
    _shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
    m_model.getVbo()->draw( &_shader );

    glDisable(GL_DEPTH_TEST);

    if (m_culling != 0) {
        glDisable(GL_CULL_FACE);
    }
}

void Scene::renderShadowMap(Shader &_shader, Uniforms &_uniforms) {
    if ( m_light.bChange || m_dynamicShadows ) {

        // Temporally move the MVP matrix from the view of the light 
        m_mvp = m_light.getMVPMatrix( m_model.getTransformMatrix(), m_model.getArea() );
        if (m_light_depthfbo.getDepthTextureId() == 0) {
#ifdef PLATFORM_RPI
            m_light_depthfbo.allocate(512, 512, COLOR_DEPTH_TEXTURES);
#else
            m_light_depthfbo.allocate(1024, 1024, COLOR_DEPTH_TEXTURES);
#endif
        }

        m_light_depthfbo.bind();
        renderGeometry(_shader, _uniforms);
        m_light_depthfbo.unbind();
    }
}

void Scene::renderCubeMap() {
    // If there is a skybox and it had changes re generate
    if (m_cubemap_skybox) {
        if (m_cubemap_skybox->change) {
            if (!m_cubemap) {
                m_cubemap = new TextureCube();
            }
            m_cubemap->generate(m_cubemap_skybox);
            m_cubemap_skybox->change = false;
        }
    }

    if (m_cubemap && m_cubemap_draw) {

        if (!m_cubemap_vbo) {
            m_cubemap_vbo = cube(1.0f).getVbo();
            m_cubemap_shader.load(cube_frag, cube_vert, m_defines, false);
        }

        m_cubemap_shader.use();

        m_cubemap_shader.setUniform("u_modelViewProjectionMatrix", m_camera.getProjectionMatrix() * glm::toMat4(m_camera.getOrientationQuat()) );
        m_cubemap_shader.setUniformTextureCube( "u_cubeMap", m_cubemap, 0 );

        m_cubemap_vbo->draw( &m_cubemap_shader );
    }
}

void Scene::renderDebug() {
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    if (!m_wireframe3D_shader.isLoaded())
        m_wireframe3D_shader.load(wireframe3D_frag, wireframe3D_vert, m_defines, false);

    if (m_model.getBboxVbo()) {
        // Bounding box
        glLineWidth(3.0f);
        m_wireframe3D_shader.use();
        m_wireframe3D_shader.setUniform("u_color", glm::vec4(1.0,1.0,0.0,1.0));
        m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
        m_model.getBboxVbo()->draw( &m_wireframe3D_shader );
    }
    
    // Axis
    if (m_axis_vbo == nullptr)
        m_axis_vbo = axis(m_camera.getFarClip()).getVbo();

    glLineWidth(2.0f);
    m_wireframe3D_shader.use();
    m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.75));
    m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
    m_axis_vbo->draw( &m_wireframe3D_shader );
    
    // Grid
    if (m_grid_vbo == nullptr)
        m_grid_vbo = grid(m_camera.getFarClip(), m_camera.getFarClip() / 20.0).getVbo();
    glLineWidth(1.0f);
    m_wireframe3D_shader.use();
    m_wireframe3D_shader.setUniform("u_color", glm::vec4(0.5));
    m_wireframe3D_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);
    m_grid_vbo->draw( &m_wireframe3D_shader );

    // Light
    if (!m_light_shader.isLoaded())
        m_light_shader.load(light_frag, light_vert, m_defines, false);

    if (m_light_vbo == nullptr)
        m_light_vbo = rect(0.0,0.0,0.0,0.0).getVbo();
        // m_light_vbo = rect(0.0,0.0,1.0,1.0).getVbo();

    m_light_shader.use();
    m_light_shader.setUniform("u_scale", 24, 24);
    m_light_shader.setUniform("u_translate", m_light.getPosition());
    m_light_shader.setUniform("u_color", glm::vec4(m_light.color, 1.0));
    m_light_shader.setUniform("u_viewMatrix", m_camera.getViewMatrix());
    m_light_shader.setUniform("u_modelViewProjectionMatrix", m_mvp);

    m_light_vbo->draw( &m_light_shader );

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void Scene::onMouseDrag(float _x, float _y, int _button) {
    if (_button == 1) {
        // Left-button drag is used to rotate geometry.
        float dist = m_camera.getDistance();

        float vel_x = getMouseVelX();
        float vel_y = getMouseVelY();

        if (fabs(vel_x) < 50.0 && fabs(vel_y) < 50.0) {
            m_lat -= vel_x;
            m_lon -= vel_y * 0.5;
            m_camera.orbit(m_lat, m_lon, dist);
            m_camera.lookAt(glm::vec3(0.0));
        }
    } 
    else {
        // Right-button drag is used to zoom geometry.
        float dist = m_camera.getDistance();
        dist += (-.008f * getMouseVelY());
        if (dist > 0.0f) {
            m_camera.setDistance( dist );
        }
    }

    flagChange();
}

void Scene::onViewportResize(int _newWidth, int _newHeight) {
    m_camera.setViewport(_newWidth, _newHeight);
}