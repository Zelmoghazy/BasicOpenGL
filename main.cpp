#include <IMGUI/imgui.h>
#include <IMGUI/imgui_impl_glfw.h>
#include <IMGUI/imgui_impl_opengl3.h>

#include <GLAD/glad.h>
#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <Shaders.hpp>

#define M_PI            3.14159265358979323846

// Use the better GPU ?
#ifdef _WIN32
    #include <windows.h>
    extern "C" {
        __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;           // Optimus: force switch to discrete GPU
        __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;     // AMD
    }
#endif

struct Vec4
{
    float x;
    float y;
    float z;
    float w;
};

struct Color4
{
    float r;
    float g;
    float b;
    float a;
};

struct Vertex 
{
    Vec4 pos;
    Color4 color;
};

struct global_context 
{
    int         width           = 800;
    int         height          = 600;
    float       currentTime     = 0.0f;
    float       deltaTime       = 0.0f;
    float       lastFrame       = 0.0f;

    bool        debug; 
    bool        wireframe;
    bool        sphere;

    bool        firstMouse      = true;
    float       mouseX          = 0;
    float       mouseY          = 0;
    float       mouseLastX      = 400;
    float       mouseLastY      = 300;

    GLFWwindow  *window;
};

global_context gc;

struct Texture 
{
    GLuint id;
    std::string path;
    std::string uniform;
    int width, height, nrChannels;

    Texture(const std::string& texturePath, const std::string& uniform) 
        : path(texturePath), uniform(uniform), width(0), height(0), nrChannels(0)
    {
        // Generate texture ID
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        // Set texture wrapping and filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // scaling down
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // scaling up

        // Load image
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        stbi_image_free(data);
    }

    void bind(GLenum textureUnit = GL_TEXTURE0) const 
    {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

    void useTextures(GLuint shaderProgram,  unsigned int textureUnit = 0)
    {
        // pass textures to the shader
        setInt(shaderProgram, uniform.c_str(), textureUnit);

        GLenum glTextureUnit = GL_TEXTURE0 + textureUnit;
        bind(glTextureUnit);
    }
    ~Texture() 
    {
        glDeleteTextures(1, &id);
    }
};

struct Camera
{
    glm::vec3   pos;
    glm::vec3   front;
    glm::vec3   up;
    glm::vec3   right;
    glm::vec3   world_up;

    glm::mat4   view;
    glm::mat4   projection;

    float zNear = 0.1f;
    float zFar  = 100.0f;

    // Euler angles
    float       yaw     = -90.0f;
    float       pitch   = 0.0f;

    // Options
    float       speed       = 2.5f;
    float       sensitivity = 0.1f;
    float       zoom        = 45.0f;

    Camera(glm::vec3 p  = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 fr = glm::vec3(0.0f, 0.0f, -1.0f),
           glm::vec3 u  = glm::vec3(0.0f, 1.0f, 0.0f))
        : pos(p), front(fr), up(u), world_up(u) 
    {
        updateViewMatrix();
        updateProjectionMatrix();

        updateVectors();
    };

    /*  
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
        glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);
    */    
    void updateViewMatrix() 
    {
        view = glm::lookAt(pos, pos + front, up);
    }

    void updateProjectionMatrix()
    {
        projection = glm::perspective(glm::radians(zoom), (float)gc.width / (float)gc.height, zNear, zFar);
    }

    glm::mat4 getViewMatrix()
    {
        return view;
    } 

    glm::mat4 getProjectionMatrix()
    {
        return projection;
    } 

    void updateVectors()
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        front = glm::normalize(direction);

        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));

        updateViewMatrix();
    }

    void updateAngle(float xoffs, float yoffs)
    {
        xoffs *= sensitivity;
        yoffs *= sensitivity;

        yaw   += xoffs;
        pitch += yoffs;

        if(pitch > 89.0f)
            pitch =  89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;

        updateVectors();
    }

    void snapToXYPlane()
    {
        yaw = -90.0f;  // Face along the negative Z-axis
        pitch = 0.0f;  // No tilt up or down
        pos = glm::vec3(0.0f, 0.0f, 10.0f); // Position the camera along the Z-axis

        updateVectors();
    }

    void snapToYZPlane()
    {
        yaw = 0.0f;    // Face along the negative X-axis
        pitch = 0.0f;  // No tilt up or down
        pos = glm::vec3(-10.0f, 0.0f, 0.0f); // Position the camera along the X-axis

        updateVectors();
    }

    void snapToXZPlane()
    {
        yaw = -90.0f;  // Face along the negative Z-axis
        pitch = 90.0f; // Look straight down along the Y-axis
        pos = glm::vec3(0.0f, -10.0f, 0.0f); // Position the camera along the Y-axis;

        updateVectors();
    }

    void snapToIsometricView()
    {
        yaw = -45.0f;  // Diagonal view
        pitch = -45.0f; // Tilt down slightly
        pos = glm::vec3(-10.0f, -10.0f, 10.0f); // Position the camera diagonally

        updateVectors();
    }

    void snapToTopDownView()
    {
        yaw = -90.0f;  // Face along the negative Z-axis
        pitch = -89.9f; // Look straight down (slightly less than 90 to avoid gimbal lock)
        pos = glm::vec3(0.0f, -10.0f, 0.0f); // Position the camera above the scene

        updateVectors();
    }

    void snapToFrontView()
    {
        yaw = -90.0f;  // Face along the negative Z-axis
        pitch = 0.0f;  // No tilt
        pos = glm::vec3(0.0f, 0.0f, 10.0f); // Position the camera in front of the scene

        updateVectors();
    }

    void snapToSideView()
    {
        yaw = 0.0f;    // Face along the negative X-axis
        pitch = 0.0f;  // No tilt
        pos = glm::vec3(-10.0f, 0.0f, 0.0f); // Position the camera to the side of the scene

        updateVectors();
    }

    void moveForward()
    {
        pos += speed * front;

        updateViewMatrix();
    }

    void moveBackward()
    {
        pos -= speed * front;

        updateViewMatrix();
    }

    void moveLeft()
    {
        // pos -= glm::normalize(glm::cross(front, up)) * speed;
        pos -= right * speed;

        updateViewMatrix(); 
    }

    void moveRight()
    {
        // pos += glm::normalize(glm::cross(front, up)) * speed;
        pos += right * speed;

        updateViewMatrix(); 
    }

    void tiltUp()
    {
        updateAngle(0.0f, 10.0f);
    }

    void tiltDown()
    {
        updateAngle(0.0f, -10.0f);
    } 

    void tiltRight()
    {
        updateAngle(10.0f, 0.0f);
    }
        
    void tiltLeft()
    {
        updateAngle(-10.0f, 0.0f);
    }

    void Zoom(float yoffs)
    {
        zoom -= yoffs;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;

        updateProjectionMatrix();
    }

    void inputPoll(GLFWwindow *window)
    {
        speed  = 2.5f * gc.deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            moveForward();
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            moveBackward();
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            moveLeft();
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            moveRight();
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            tiltUp();
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            tiltDown();
        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
            tiltLeft();
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            tiltRight();
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            snapToXYPlane();
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            snapToYZPlane();
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
            snapToXZPlane();
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
            snapToIsometricView();
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
            snapToTopDownView();
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
            snapToFrontView();
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
            snapToSideView();
    }

    void updateOrbitPosition(float time, float radius) 
    {
        pos.x = sin(time) * radius;
        pos.z = cos(time) * radius;
        pos.y = 0.0;
    }    
};
Camera camera;

struct Coordinates
{
    GLuint VAO;
    GLuint VBO;

    GLuint shaderProgram;

    glm::mat4 model;

    Coordinates() : model(glm::mat4(1.0f))
    {
        setupAxes();
        initShaders();
    }

    void setupAxes()
    {
       const float axesVertices[] = 
       {
            // X axis (Red)
            0.0f, 0.0f, 0.0f,       1.0f, 0.0f, 0.0f, // Origin
            5.0f, 0.0f, 0.0f,       1.0f, 0.0f, 0.0f, // X direction

            // Y axis (Green)
            0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f, // Origin
            0.0f, 5.0f, 0.0f,       0.0f, 1.0f, 0.0f, // Y direction

            // Z axis (Blue)
            0.0f, 0.0f, 0.0f,       0.0f, 0.0f, 1.0f, // Origin
            0.0f, 0.0f, 5.0f,       0.0f, 0.0f, 1.0f  // Z direction
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0); 
    }

    void initShaders()
    {
        std::string vertexSource    = readShaderSource("../shaders/axes_vs.glsl");
        std::string fragmentSource  = readShaderSource("../shaders/axes_fs.glsl");
        shaderProgram   = createShaderProgram(vertexSource, fragmentSource);
    }

    void updateShaders()
    {
        glDeleteProgram(shaderProgram);
        initShaders();
    }

    void render()
    {
        glUseProgram(shaderProgram);

        setMat4(shaderProgram, "model", model);
        setMat4(shaderProgram, "view", camera.getViewMatrix());
        setMat4(shaderProgram, "projection", camera.getProjectionMatrix());

        glLineWidth(2.0f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, 6); // 6 vertices for 3 lines (X, Y, Z)
        glBindVertexArray(0);
        glLineWidth(1.0f); // Reset to default
    }
};
Coordinates *world_axes;

struct Grid
{
    GLuint VAO;
    GLuint VBO;

    GLuint shaderProgram;

    glm::mat4 model;

    Grid(): model(glm::mat4(1.0f)) 
    {
        setupGrid();
        initShader();
    }

    ~Grid() 
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }

    void setupGrid()
    {
        // A large quad for the grid (XZ plane)
        const float vertices[] = {
            // Positions                 // UVs
            -50.0f, 0.0f, -50.0f,        0.0f, 0.0f,
             50.0f, 0.0f, -50.0f,       50.0f, 0.0f,
             50.0f, 0.0f,  50.0f,       50.0f, 50.0f,
            -50.0f, 0.0f,  50.0f,        0.0f, 50.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // UV attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void initShader() 
    {
        std::string vertexSource = readShaderSource("../shaders/grid_vs.glsl");
        std::string fragmentSource = readShaderSource("../shaders/grid_fs.glsl");
        shaderProgram = createShaderProgram(vertexSource, fragmentSource);
    }

    void updateShader()
    {
        glDeleteProgram(shaderProgram);
        initShader();
    }

    void render() 
    {
        glUseProgram(shaderProgram);
        
        setFloat(shaderProgram, "time", gc.currentTime);

        setMat4(shaderProgram, "model", model);
        setMat4(shaderProgram, "view", camera.getViewMatrix());
        setMat4(shaderProgram, "projection", camera.getProjectionMatrix());

        setVec3(shaderProgram, "cameraPos", camera.pos);

        GLboolean cullingEnabled;
        glGetBooleanv(GL_CULL_FACE, &cullingEnabled);
        GLint cullFaceMode;
        glGetIntegerv(GL_CULL_FACE_MODE, &cullFaceMode);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);  // So we see it from both sides
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        // Restore culling state
        if (cullingEnabled) {
            glEnable(GL_CULL_FACE);
            glCullFace(cullFaceMode);
        }
    }
};
Grid *grid;

struct Light
{
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    enum class Type
    {
        DIRECTIONAL,
        POINT,
        SPOT,
    };

    GLuint shaderProgram;

    glm::mat4 model;

    glm::vec3 lightPos;
    glm::vec3 lightDir;
    glm::vec3 lightCol;

    glm::vec3 lightDiffuse;
    glm::vec3 lightAmbient;
    glm::vec3 lightSpecular;

    float lightCutoffAngle = 12.5f;
    float lightOuterCutoffAngle = 19.5f;

    float lightCutoff = glm::cos(glm::radians(lightCutoffAngle));
    float lightOuterCutoff = glm::cos(glm::radians(lightOuterCutoffAngle));

    // attenuation
    float constant  = 1.0f;
    float linear    = 0.09f;
    float quadratic = 0.032f;

    Coordinates axes;

    Light(GLuint CubeVBO, GLuint CubeEBO) 
        : lightPos(glm::vec3(1.2f, 1.0f, 2.0f)),
          lightDir(glm::vec3(0.0f, 0.0f, -1.0f)),
          lightCol(glm::vec3(1.0f, 1.0f, 1.0f)),
          lightDiffuse(glm::vec3(0.5f, 0.5f, 0.5f)),
          lightAmbient(glm::vec3(0.2f, 0.2f, 0.2f)),
          lightSpecular(glm::vec3(1.0f, 1.0f, 1.0f)),
          VBO(CubeVBO),
          EBO(CubeEBO)
    {
        positionDebugCube();   
        setupDebugCube();
        initShaders();
    }

    ~Light() 
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteProgram(shaderProgram);
    }

    void initShaders()
    {
        std::string vertexSource    = readShaderSource("../shaders/light_vs.glsl");
        std::string fragmentSource  = readShaderSource("../shaders/light_fs.glsl");
        shaderProgram   = createShaderProgram(vertexSource, fragmentSource);
    }

    void updateShaders()
    {
        glDeleteProgram(shaderProgram);
        initShaders();
    }

    void setupDebugCube()
    {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        // glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    void renderDebugCube() 
    {   
        positionDebugCube();

        if(gc.debug)
        {
            renderDebugAxes();
        }

        glUseProgram(shaderProgram);

        setVec3(shaderProgram, "lightColor", lightCol);

        setMat4(shaderProgram, "model", model);
        setMat4(shaderProgram, "view", camera.getViewMatrix());
        setMat4(shaderProgram, "projection", camera.getProjectionMatrix()); 

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // Use EBO
        glBindVertexArray(0);
    }

    void renderDebugAxes()
    {
        axes.model = model;
        axes.render();
    }

    void updateLightColors()
    {
        lightDiffuse = lightCol * glm::vec3(0.5f); 
        lightAmbient = lightDiffuse * glm::vec3(0.2f); 
    }

    void positionDebugCube()
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
    }
};

Light *light;
Light *dirLight;
Light *pointLight[4];
Light *spotLight;

struct Cube
{
    GLuint     VAO;
    GLuint     VBO;
    GLuint     EBO;

    GLuint     shaderProgram;

    glm::mat4  model;

    Coordinates axes;

    glm::vec3  cubePositions[10];
    glm::vec3  cubeColors[10];

    Texture* diffuseMap;
    Texture* specularMap;
    Texture* emissionMap;

    glm::vec3 materialAmbient;
    glm::vec3 materialDiffuse;
    glm::vec3 materialSpecular;

    float shininess = 32.0f;
    
    Cube()
    {
        setupCube();
        initShaders();
    }

    ~Cube()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &VBO);

        glDeleteProgram(shaderProgram);
    }

    void setupCube()
    {
        // materialAmbient  = glm::vec3(1.0f, 0.5f, 0.31f);
        // materialDiffuse  = glm::vec3(1.0f, 0.5f, 0.31f);
        // materialSpecular = glm::vec3(0.5f, 0.5f, 0.5f);

        for(int i = 0; i < 10; i++)
        {
            cubeColors[i] = getRandomCubeColor();
        }

        cubePositions[0] = glm::vec3( 0.0f,  0.0f,  0.0f);
        cubePositions[1] = glm::vec3( 2.0f,  5.0f, -15.0f);
        cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
        cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
        cubePositions[4] = glm::vec3( 2.4f, -0.4f, -3.5f);
        cubePositions[5] = glm::vec3(-1.7f,  3.0f, -7.5f);
        cubePositions[6] = glm::vec3( 1.3f, -2.0f, -2.5f);
        cubePositions[7] = glm::vec3( 1.5f,  2.0f, -2.5f);
        cubePositions[8] = glm::vec3( 1.5f,  0.2f, -1.5f);
        cubePositions[9] = glm::vec3(-1.3f,  1.0f, -1.5f);

        /*------------------------- setup vertex data and buffers and configure attributes -------------------------*/
        const float vertices[] = 
        {
            // Front face (Z+)
            // Pos                 Color               Texture coord     Normal
            -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,      0.0f, 0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,      0.0f, 0.0f, 1.0f, 
             0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,      0.0f, 0.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,      0.0f, 0.0f, 1.0f,
            
            // Back face (Z-)
            -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,      0.0f, 0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,      0.0f, 0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,      0.0f, 0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,      0.0f, 0.0f, -1.0f,
            
            // Left face (X-)
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,      -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,      -1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,      -1.0f, 0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,      -1.0f, 0.0f, 0.0f,
            
            // Right face (X+)
             0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f,      1.0f, 0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,      1.0f, 0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,   1.0f, 1.0f,      1.0f, 0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,      1.0f, 0.0f, 0.0f,
            
            // Bottom face (Y-)
            -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f,      0.0f, -1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,   1.0f, 0.0f,      0.0f, -1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,   1.0f, 1.0f,      0.0f, -1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 1.0f,   0.0f, 1.0f,      0.0f, -1.0f, 0.0f,
            
            // Top face (Y+)
            -0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f,      0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 1.0f,   1.0f, 0.0f,      0.0f, 1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,   1.0f, 1.0f,      0.0f, 1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 1.0f,   0.0f, 1.0f,      0.0f, 1.0f, 0.0f
        };

        // To avoid duplicating vertices, indices are used to define 
        // how the vertices are connected to form triangles.
        const unsigned int indices[] = 
        {
            // Front
            0, 1, 2,
            2, 3, 0,
            // Back
            4, 5, 6,
            6, 7, 4,
            // Left
            8, 9, 10,
            10, 11, 8,
            // Right
            12, 13, 14,
            14, 15, 12,
            // Bottom
            16, 17, 18,
            18, 19, 16,
            // Top
            20, 21, 22,
            22, 23, 20
        };

        // vertex array object: any subsequent vertex attribute calls from 
        // that point on will be stored inside the VAO.
        // configuring vertex attribute pointers only needed once
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        /*----------------------------------------------------------------------*/
        // Element buffer object
        glGenBuffers(1, &EBO);
        // copy index array into element buffer for opengl to use
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); // copy them to GPU
        /*----------------------------------------------------------------------*/
        // vertex buffer object : memory on the GPU where we store the vertex data
        glGenBuffers(1, &VBO); // Generate a buffer object with unique ID
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        /*----------------------------------------------------------------------*/
        // copies the previously defined vertex data into the buffer's memory
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        /*----------------------------------------------------------------------*/
        // Tell OpenGL how it should interpret the vertex data (position attribute)
        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        // Enable the vertex attribute, giving the vertex attribute location as its argument
        glEnableVertexAttribArray(0);

        // Color attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3*sizeof(float)));
        // Enable the vertex attribute, giving the vertex attribute location as its argument
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute (location = 2)
        // 2D texture
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2); 

        // Normal attribute (location = 3)
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8*sizeof(float)));
        glEnableVertexAttribArray(3);

        // UNBIND
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void positionCube(int idx)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, cubePositions[idx]);
        model = glm::rotate(model, sin(gc.currentTime), glm::vec3(1.0f, 0.3f, 0.5f));
    }

    void initShaders()
    {
        // Load and compile shaders
        std::string vertexSource   = readShaderSource("../shaders/cube_vs.glsl");
        std::string fragmentSource = readShaderSource("../shaders/cube_fs.glsl");
        shaderProgram              = createShaderProgram(vertexSource, fragmentSource);
    }

    void updateShaders()
    {
        glDeleteProgram(shaderProgram);
        initShaders();
    }

    glm::vec3 getRandomCubeColor()
    {
        // Generate random float values between 0.0 and 1.0 for r, g, b components
        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        
        // Return color as vec3
        return glm::vec3(r, g, b);
    }

    void updateCubeColor(int idx)
    {
        materialDiffuse = cubeColors[idx] * glm::vec3(0.5f); 
        materialAmbient = materialDiffuse * glm::vec3(0.2f); 
    }

    void render()
    {
        if(gc.debug)
        {
            renderDebugAxes();
        }

        // Activate shader program
        glUseProgram(shaderProgram);

        // Pass uniform variables to the shader
        setFloat(shaderProgram, "iTime", gc.currentTime);
        setFloat2(shaderProgram,"iResolution", (float)gc.width, (float)gc.height);

        setVec3(shaderProgram, "viewPos", camera.pos);

        // directional light
        setVec3(shaderProgram, "dirLight.direction",    dirLight->lightPos);
        setVec3(shaderProgram, "dirLight.ambient",      glm::vec3(0.05f, 0.05f, 0.05f));
        setVec3(shaderProgram, "dirLight.diffuse",      glm::vec3(0.4f, 0.4f, 0.4f));
        setVec3(shaderProgram, "dirLight.specular",     glm::vec3(0.5f, 0.5f, 0.5f));

        for (int i = 0; i < 4; i++) 
        {
            std::string base = "pointLights[" + std::to_string(i) + "].";
            
            setVec3(shaderProgram, base + "position",   pointLight[i]->lightPos);
            setVec3(shaderProgram, base + "ambient",    pointLight[i]->lightAmbient);
            setVec3(shaderProgram, base + "diffuse",    pointLight[i]->lightDiffuse);
            setVec3(shaderProgram, base + "specular",   pointLight[i]->lightSpecular);

            setFloat(shaderProgram, base + "constant",  pointLight[i]->constant);
            setFloat(shaderProgram, base + "linear",    pointLight[i]->linear);
            setFloat(shaderProgram, base + "quadratic", pointLight[i]->quadratic);
        }

        setVec3(shaderProgram, "spotLight.position",    camera.pos);
        setVec3(shaderProgram, "spotLight.direction",   camera.front);
        setVec3(shaderProgram, "spotLight.diffuse",     spotLight->lightDiffuse);
        setVec3(shaderProgram, "spotLight.ambient",     spotLight->lightAmbient);
        setVec3(shaderProgram, "spotLight.specular",    spotLight->lightSpecular);

        setFloat(shaderProgram, "spotLight.constant",   spotLight->constant);
        setFloat(shaderProgram, "spotLight.linear",     spotLight->linear);
        setFloat(shaderProgram, "spotLight.quadratic",  spotLight->quadratic);

        setFloat(shaderProgram, "spotLight.cutoff",      spotLight->lightCutoff);
        setFloat(shaderProgram, "spotLight.outerCutoff", spotLight->lightOuterCutoff);

        // setVec3(shaderProgram, "material.specular", materialSpecular);
        // setVec3(shaderProgram, "material.ambient", materialAmbient);
        // setVec3(shaderProgram, "material.diffuse", materialDiffuse);
        setFloat(shaderProgram, "material.shininess", shininess);

        diffuseMap->useTextures(shaderProgram, 0);
        specularMap->useTextures(shaderProgram, 1);
        emissionMap->useTextures(shaderProgram, 2);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();
        
        for(int i = 0; i < 10; i++)
        {
            positionCube(i);
            // updateCubeColor(i);

            setMat4(shaderProgram, "model", model); 
            setMat4(shaderProgram, "view", view);
            setMat4(shaderProgram, "projection", projection);        

            // to render only the VAO is required to be bound
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    void renderDebugAxes()
    {
        positionCube(3);
        axes.model = model;
        axes.render(); 
    }
};

Cube *cube;

struct Sphere
{
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    GLuint shaderProgram;

    Coordinates axes;

    glm::vec3 spherePositions[10];
    glm::vec3 sphereColors[10];

    glm::mat4 model;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float radius = 0.5f;
    int sectorCount = 36;
    int stackCount = 18;

    float shininess = 32.0f;

    glm::vec3 materialAmbient;
    glm::vec3 materialDiffuse;
    glm::vec3 materialSpecular;


    Sphere()
    {
        setupSphere();
        initShaders();
    }

    ~Sphere()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &VBO);

        glDeleteProgram(shaderProgram);
    }

    void setupSphere()
    {
        materialAmbient  = glm::vec3(1.0f, 0.5f, 0.31f);
        materialDiffuse  = glm::vec3(1.0f, 0.5f, 0.31f);
        materialSpecular = glm::vec3(0.5f, 0.5f, 0.5f);

        for (int i = 0; i < 10; i++)
        {
            sphereColors[i] = getRandomSphereColor();
        }

        spherePositions[0] = glm::vec3(0.0f, 0.0f, 0.0f);
        spherePositions[1] = glm::vec3(2.0f, 5.0f, -15.0f);
        spherePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);
        spherePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);
        spherePositions[4] = glm::vec3(2.4f, -0.4f, -3.5f);
        spherePositions[5] = glm::vec3(-1.7f, 3.0f, -7.5f);
        spherePositions[6] = glm::vec3(1.3f, -2.0f, -2.5f);
        spherePositions[7] = glm::vec3(1.5f, 2.0f, -2.5f);
        spherePositions[8] = glm::vec3(1.5f, 0.2f, -1.5f);
        spherePositions[9] = glm::vec3(-1.3f, 1.0f, -1.5f);

        /*------------------------- setup vertex data and buffers and configure attributes -------------------------*/

        generateSphere(vertices, indices, radius, sectorCount, stackCount);

        // vertex array object: any subsequent vertex attribute calls from 
        // that point on will be stored inside the VAO.
        // configuring vertex attribute pointers only needed once
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        /*----------------------------------------------------------------------*/
        // Element buffer object
        glGenBuffers(1, &EBO);
        // copy index array into element buffer for opengl to use
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW); // copy them to GPU
        /*----------------------------------------------------------------------*/
        // vertex buffer object : memory on the GPU where we store the vertex data
        glGenBuffers(1, &VBO); // Generate a buffer object with unique ID
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        /*----------------------------------------------------------------------*/
        // copies the previously defined vertex data into the buffer's memory
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        /*----------------------------------------------------------------------*/
        // Tell OpenGL how it should interpret the vertex data (position attribute)
        // Position attribute (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        // Enable the vertex attribute, giving the vertex attribute location as its argument
        glEnableVertexAttribArray(0);

        // Normal attribute (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Texture coordinate attribute (location = 2)
        // 2D texture
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // UNBIND
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void positionSphere(int idx)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, spherePositions[idx]);
    }

    void initShaders()
    {
        // Load and compile shaders
        std::string vertexSource = readShaderSource("../shaders/sphere_vs.glsl");
        std::string fragmentSource = readShaderSource("../shaders/sphere_fs.glsl");
        shaderProgram = createShaderProgram(vertexSource, fragmentSource);
    }

    void updateShaders()
    {
        glDeleteProgram(shaderProgram);
        initShaders();
    }

    glm::vec3 getRandomSphereColor()
    {
        // Generate random float values between 0.0 and 1.0 for r, g, b components
        float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        // Return color as vec3
        return glm::vec3(r, g, b);
    }

    void updateSphereColor(int idx)
    {
        materialDiffuse = sphereColors[idx] * glm::vec3(0.5f); 
        materialAmbient = materialDiffuse * glm::vec3(0.2f); 
    }

    void render()
    {
        if (gc.debug)
        {
            renderDebugAxes();
        }

        // Activate shader program
        glUseProgram(shaderProgram);

        // Pass uniform variables to the shader
        setFloat(shaderProgram, "iTime", gc.currentTime);
        setFloat2(shaderProgram, "iResolution", (float)gc.width, (float)gc.height);

        setVec3(shaderProgram, "viewPos", camera.pos);

        setVec3(shaderProgram, "light.position", light->lightPos);
        setVec3(shaderProgram, "light.diffuse", light->lightDiffuse);
        setVec3(shaderProgram, "light.ambient", light->lightAmbient);
        setVec3(shaderProgram, "light.specular", light->lightSpecular);

        setVec3(shaderProgram, "material.specular",materialSpecular);
        setFloat(shaderProgram, "material.shininess", shininess);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();

        for (unsigned int i = 0; i < 10; i++)
        {
            // camera.updateOrbitPosition(gc.currentTime, 10.0f);
            positionSphere(i);
            updateSphereColor(i);

            setVec3(shaderProgram, "material.ambient", materialAmbient);
            setVec3(shaderProgram, "material.diffuse", materialDiffuse);
            
            setMat4(shaderProgram, "model", model);
            setMat4(shaderProgram, "view", view);
            setMat4(shaderProgram, "projection", projection);

            // to render only the VAO is required to be bound
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    void renderDebugAxes()
    {
        positionSphere(3);
        axes.model = model;
        axes.render();
    }

    void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int sectorCount, int stackCount)
    {
        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
        float s, t;                                     // vertex texCoord

        float sectorStep = (float)(2 * M_PI / sectorCount);
        float stackStep = (float)(M_PI / stackCount);
        float sectorAngle, stackAngle;

        for (int i = 0; i <= stackCount; ++i)
        {
            stackAngle = (float)(M_PI / 2 - i * stackStep);      // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);             // r * cos(u)
            z = radius * sinf(stackAngle);              // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for (int j = 0; j <= sectorCount; ++j)
            {
                sectorAngle = j * sectorStep;           // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / sectorCount;
                t = (float)i / stackCount;
                vertices.push_back(s);
                vertices.push_back(t);
            }
        }

        // generate CCW index list of sphere triangles
        int k1, k2;
        for (int i = 0; i < stackCount; ++i)
        {
            k1 = i * (sectorCount + 1);     // beginning of current stack
            k2 = k1 + sectorCount + 1;      // beginning of next stack

            for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if (i != (stackCount - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }
    }
};

Sphere *sphere;


struct Ui
{
    float rotation = 0.0f;
    float vec3a[3] = { 1.0f, 1.0f, 1.0f};
    float col1[3] = { 1.0f, 1.0f, 1.0f };

    float pcol[4][3] = {{ 1.0f, 1.0f, 1.0f},
                        { 1.0f, 1.0f, 1.0f},
                        { 1.0f, 1.0f, 1.0f},
                        { 1.0f, 1.0f, 1.0f}};

    float bgcol[3] = { 0.0f, 0.0f, 0.0f };
    float shininess = 32.0f;
    char str0[128];

    Ui(GLFWwindow *window)
    {
        /* IMGUI Stuff */
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO(); (void) io;

        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        //ImGui::StyleColorsDark();
        DarkTheme();
    }

    void DarkTheme()
    {
        ImVec4 *colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        // colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        // colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

        ImGuiStyle &style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(8.00f, 8.00f);
        style.FramePadding = ImVec2(5.00f, 2.00f);
        style.CellPadding = ImVec2(6.00f, 6.00f);
        style.ItemSpacing = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
        style.IndentSpacing = 25;
        style.ScrollbarSize = 15;
        style.GrabMinSize = 10;
        style.WindowBorderSize = 1;
        style.ChildBorderSize = 1;
        style.PopupBorderSize = 1;
        style.FrameBorderSize = 1;
        style.TabBorderSize = 1;
        style.WindowRounding = 7;
        style.ChildRounding = 4;
        style.FrameRounding = 3;
        style.PopupRounding = 4;
        style.ScrollbarRounding = 9;
        style.GrabRounding = 3;
        style.LogSliderDeadzone = 4;
        style.TabRounding = 4;
    }

    void beginFrame() 
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void debugWindow()
    {
        ImGui::Begin("Debug");
            ImGui::SliderFloat("rotation", &rotation, 0, 360); 
            ImGui::ColorEdit3("Background Color", bgcol);

            ImGui::SliderFloat3("lightPos", vec3a, -15.0f, 15.0f);
            ImGui::ColorEdit3("lightCol", col1);

            for (int i = 0; i < 4; i++) 
                ImGui::ColorEdit3(("PointCol" + std::to_string(i)).c_str(), pcol[i]);


            const char* items[] = { "7", "13", "20", "32", "50", "65", "100", "160", "200", "325", "600", "3250"};
            static int item_selected_idx = 6;

            const char* combo_preview_value = items[item_selected_idx];

            if (ImGui::BeginCombo("lightAttenuation", combo_preview_value, 0))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_selected_idx == n);
                    if (ImGui::Selectable(items[n], is_selected))
                        item_selected_idx = n;
                    switch(item_selected_idx)
                    {
                        case 0:
                            light->linear = 0.7f;
                            light->quadratic = 1.8f;
                            break;
                        case 1:
                            light->linear = 0.35f;
                            light->quadratic = 0.44f;
                            break;
                        case 2:
                            light->linear = 0.22f;
                            light->quadratic = 0.20f;
                            break;
                        case 3:
                            light->linear = 0.14f;
                            light->quadratic = 0.07f;
                            break;
                        case 4:
                            light->linear = 0.09f;
                            light->quadratic = 0.032f;
                            break;
                        case 5:
                            light->linear = 0.07f;
                            light->quadratic = 0.017f;
                            break;
                        case 6:
                            light->linear = 0.045f;
                            light->quadratic = 0.0075f;
                            break;
                        case 7:
                            light->linear = 0.027f;
                            light->quadratic = 0.0028f;
                            break;
                        case 8:
                            light->linear = 0.022f;
                            light->quadratic = 0.0019f;
                            break;    
                        case 9:
                            light->linear = 0.014f;
                            light->quadratic = 0.0007f;
                            break;   
                        case 10:
                            light->linear = 0.007f;
                            light->quadratic = 0.0002f; 
                            break;
                        case 11:
                            light->linear = 0.0014f;
                            light->quadratic = 0.000007f; 
                            break;
                        default:
                            break;                                        
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if(gc.sphere){
                ImGui::SliderFloat("shininess", &sphere->shininess, 1.0, 64.0);
            }else{
                ImGui::SliderFloat("shininess", &cube->shininess, 1.0, 64.0);
            }

            ImGui::Checkbox("Sphere", &gc.sphere);
            ImGui::Checkbox("Debug", &gc.debug);
            ImGui::Checkbox("Wireframe", &gc.wireframe);

            sprintf_s(str0, "Time: %f ms/frame", gc.deltaTime*1000.0f);
            ImGui::Text(str0);
        ImGui::End();
    }

    void demoWindow()
    {
        ImGui::ShowDemoWindow();
    }

    void render() 
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
Ui *ui;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    gc.width = width;
    gc.height = height;
    glViewport(0, 0, gc.width, gc.height);
    camera.updateProjectionMatrix();
}

void window_refresh_callback(GLFWwindow* window)
{
    glfwSwapBuffers(window);
}

void getMouseDelta(float *xoffset, float *yoffset)
{
    if (gc.firstMouse)
    {
        gc.mouseLastX = (float)gc.mouseX;
        gc.mouseLastY = (float)gc.mouseY;
        gc.firstMouse = false;
    }

    *xoffset = (float)gc.mouseX - gc.mouseLastX;
    *yoffset = gc.mouseLastY - (float)gc.mouseY; // reversed since y-coordinates range from bottom to top

    gc.mouseLastX = (float)gc.mouseX;
    gc.mouseLastY = (float)gc.mouseY;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    (void)window;

    gc.mouseX = (float)xpos;
    gc.mouseY = (float)ypos;

    float xoffset, yoffset;
    getMouseDelta(&xoffset, &yoffset);

    // gc.camera.updateAngle(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) 
{
    (void)window;
    (void)xoffset;
    camera.Zoom((float)yoffset);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        cube->updateShaders();
        sphere->updateShaders();
        light->updateShaders();
        grid->updateShader();
    }

    camera.inputPoll(window);
}

GLFWwindow *initGL()
{
    // Initialize GLFW
    if (!glfwInit()){
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a window
    GLFWwindow* window = glfwCreateWindow(gc.width, gc.height, "OpenGL", NULL, NULL);
    if (window == NULL){
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);

    // load opengl function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }

    /* 
       OpenGL data and coordinates with respect to the window.
       Note that processed coordinates in OpenGL are between -1 and 1 
       so we effectively map from the range (-1 to 1) to (0, 800) and (0, 600).
    */
    glViewport(0, 0, gc.width, gc.height);

    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);  

    // Register window resize callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowRefreshCallback(window, window_refresh_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);

    // Enable vsync
    glfwSwapInterval(1);

    return window;  
}

void cleanupGL()
{
    glfwTerminate();
}

void clearBackground(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    // only clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderScene()
{
    clearBackground(ui->bgcol[0], ui->bgcol[1], ui->bgcol[2], 1.0f);

    ui->beginFrame();
    
    // ui->demoWindow();
    ui->debugWindow();

    light->lightPos.x = ui->vec3a[0];
    light->lightPos.y = ui->vec3a[1];
    light->lightPos.z = ui->vec3a[2];

    light->lightCol.x = ui->col1[0];
    light->lightCol.y = ui->col1[1];
    light->lightCol.z = ui->col1[2];
    light->updateLightColors();

    for (int i = 0; i < 4; i++) 
    {
        pointLight[i]->lightCol.x = ui->pcol[i][0];
        pointLight[i]->lightCol.y = ui->pcol[i][1];
        pointLight[i]->lightCol.z = ui->pcol[i][2];
        pointLight[i]->updateLightColors();
    }

    // camera.updateOrbitPosition(gc.currentTime, 10.0f);


    if(gc.debug)
    {
        world_axes->render();
        grid->render();
    }

    static bool set = false;
    if(gc.wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        set = false;
    }
    else
    {
        if(!set)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            set = true;
        }
    }

    if(gc.sphere){
        sphere->render();
        light->renderDebugCube();
    }else{
        cube->render();
        for (int i = 0; i < 4; i++) 
        {
            pointLight[i]->renderDebugCube();
        }
    }

    ui->render();
}

int main(void)
{
    gc.window = initGL();

    ui = new Ui(gc.window);

    world_axes  = new Coordinates();
    grid        = new Grid();

    cube    = new Cube();
    sphere  = new Sphere();
    light   = new Light(cube->VBO, cube->EBO);

    dirLight    = new Light(cube->VBO, cube->EBO);
    dirLight->lightPos = glm::vec3(-0.2f, -1.0f, -0.3f); 

    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };

    for(size_t i = 0; i < 4 ; i++)
    {
        pointLight[i] = new Light(cube->VBO, cube->EBO);
        pointLight[i]->lightPos = pointLightPositions[i];
    }

    spotLight  = new Light(cube->VBO, cube->EBO);

    cube->diffuseMap  = new Texture("..\\assets\\metallic_texture.jpg", "material.diffuse");
    cube->specularMap = new Texture("..\\assets\\specular-map.png", "material.specular");
    cube->emissionMap = new Texture("..\\assets\\emission-map.jpg", "material.emission");

    // Render loop
    while(!glfwWindowShouldClose(gc.window))
    {
        gc.currentTime = (float)glfwGetTime();
        gc.deltaTime = gc.currentTime - gc.lastFrame;
        gc.lastFrame = gc.currentTime;

        processInput(gc.window);
        renderScene();

        glfwSwapBuffers(gc.window);
        glfwPollEvents();
    }

    cleanupGL();

    return 0;
}