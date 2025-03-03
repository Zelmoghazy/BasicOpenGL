#include <iostream>
#include <GLFW/glfw3.h>
#include <cmath>

#define M_PI 3.14159265358979323846


int main(void)
{
    GLFWwindow *window;

    if(!glfwInit()){
        std::cerr << "Error initializing GLFW..\n";
        return -1;
    }

    window = glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);
    if(!window){
        std::cerr << "Error in window creation..\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    const float steps = 100;
    const float angle = M_PI * 2.f / steps;

    float xpos      = 0.0f;
    float ypos      = 0.0f;
    float radius    = 1.0f;

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        float prevx = xpos;
        float prevy = ypos-radius;
        
        for(size_t i = 0;
            i<= steps;
            i++)
        {
            float newx = radius  * sin(angle*i);
            float newy = -radius * cos(angle*i);

            glBegin(GL_TRIANGLES);
                glColor3f(1.0f,0.5f,1.0f);          // red

                glVertex3f(0.0f,0.0f,0.0f);

                glVertex3f(prevx,prevy,0.0f);

                glVertex3f(newx,newy,0.0f);
            glEnd();

            prevx = newx;
            prevy = newy;
        }
    
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}