#include <iostream>
#include <GLFW/glfw3.h>

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

    while(!glfwWindowShouldClose(window))
    {
        glClearColor(1.0, 1.0, 1.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glBegin(GL_TRIANGLES);
            glColor3f(1.0f,0.0f,1.0f);          // red
            glVertex3f(-0.6f,-0.4f,0.0f);

            glColor3f(1.0f,1.0f,0.0f);          // green
            glVertex3f(0.6f,-0.4f,0.0f);

            glColor3f(0.0f,0.5f,1.0f);          // blue
            glVertex3f(0.0f,0.6f,0.0f);
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
}