#include <windows.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GL/glext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "InitShader.h"    //Functions for loading shaders from text files
#include "LoadMesh.h"      //Functions for creating OpenGL buffers from mesh files
#include "LoadTexture.h"   //Functions for creating OpenGL textures from image files

static const std::string vertex_shader("Vertex_vs.glsl");
static const std::string fragment_shader("Fragment_fs.glsl");
GLuint shader_program = -1;

const int num_meshes = 2;

static const std::string mesh_name[num_meshes] = { "Amago0.obj", "Sharkie.obj" };
static const std::string texture_name[num_meshes] = { "AmagoT.bmp", "Sharkie.bmp" };

GLuint texture_id[num_meshes] = { -1, -1 }; //Texture map for mesh
MeshData mesh_data[num_meshes];

const glm::vec3 y_axis = glm::vec3(0.0f, 1.0f, 0.0f);
float u = 0.0f; //interpolation parameter
glm::mat4 R_start[num_meshes] = { glm::rotate(-180.0f, y_axis), glm::rotate(+0.0f, y_axis) };  //start points for interpolaton
glm::mat4 R_end[num_meshes] = { glm::rotate(+0.0f, y_axis), glm::rotate(+180.0f, y_axis) }; //end points for interpolation
glm::mat4 R[num_meshes]; //interpolation results will be stored here
glm::mat4 T[num_meshes] = { glm::translate(glm::vec3(0.0f, 0.0f, +0.5f)), glm::translate(glm::vec3(0.0f, 0.0f, +0.5f)) };
glm::mat4 S[num_meshes];
glm::mat4 M[num_meshes];

//Begin scene definition

bool use_quats = true;
bool auto_rotate = true;

float angle = 50.0f;
float scale = 1.0f;

float dist = 1.0f;
float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };//color
float Acolor[4] = { 0.0f, 0.32f, 0.57f, 1.0f };//ambient color
float AScolor[4] = { 0.74f, 0.74f, 0.74f, 1.0f };//ambient surface color
float Dcolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };//diffuse light color
float DScolor[4] = { 0.32f ,0.78f, 0.62f, 1.0f };//diffuse surface color
float Scolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };//specular light color
float SScolor[4] = { 0.19f, 0.61f, 0.74f, 1.0f };//specular surface color
float shine = 8.0f;//glossiness
bool Check = false;//texture visibility

float camPos[3] = { 0.0f, 0.0f, 3.0f };
float FoV = 40.0f;
float aspectRatio = 1.0f;


void draw_gui(GLFWwindow* window)
{
    //Begin ImGui Frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    aspectRatio = (float)w / h;

    //Draw Gui
    ImGui::Begin("toon shading");                       
    if (ImGui::Button("Quit"))                          
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }    
    // lighting 
    ImGui::Text("lighting");
    //ambient,diffuse,specular temrs
    ImGui::ColorEdit4("Ambient light color", Acolor);
    float a = glGetUniformLocation(shader_program, "la");
    glUniform4f(a, Acolor[0], Acolor[1], Acolor[2], Acolor[3]);

    ImGui::ColorEdit4("Diffuse light color", Dcolor);
    float d = glGetUniformLocation(shader_program, "ld");
    glUniform4f(d, Dcolor[0], Dcolor[1], Dcolor[2], Dcolor[3]);

    ImGui::ColorEdit4("Specular light color", Scolor);
    float s = glGetUniformLocation(shader_program, "ls");
    glUniform4f(s, Scolor[0], Scolor[1], Scolor[2], Scolor[3]);

    /*  ImGui::ColorEdit4("Object color", color);
    float c = glGetUniformLocation(shader_program, "objectColor");
    glUniform4f(c, color[0], color[1], color[2], color[3]); */

    ImGui::ColorEdit4("Ambient surface color", AScolor);
    float as = glGetUniformLocation(shader_program, "ka");
    glUniform4f(as, AScolor[0], AScolor[1], AScolor[2], AScolor[3]);

    ImGui::ColorEdit4("Diffuse surface color", DScolor);
    float ds = glGetUniformLocation(shader_program, "kd");
    glUniform4f(ds, DScolor[0], DScolor[1], DScolor[2], DScolor[3]);

    ImGui::ColorEdit4("Specular surface color", SScolor);
    float ss = glGetUniformLocation(shader_program, "ks");
    glUniform4f(ss, SScolor[0], SScolor[1], SScolor[2], SScolor[3]);

    ImGui::SliderFloat("Shininess", &shine, 0.0f, 100.0f); // shininess/ glossiness of the object
    int Alpha = glGetUniformLocation(shader_program, "alpha");
    glUniform1f(Alpha, shine);


    // Use texture
    ImGui::Checkbox("Texture", &Check);
    float ch = glGetUniformLocation(shader_program, "check");
    glUniform1f(ch, Check);

    //to observe shadows  
    ImGui::SliderFloat("Source distance", &dist, 0.0f, 2.0f); // distance of the light from the object
    int d_loc = glGetUniformLocation(shader_program, "di");
    glUniform1f(d_loc, dist);
    ImGui::SliderFloat("angle", &angle, 0.0f, 360.0f); // to rotate the object
    ImGui::SliderFloat("Scale", &scale, -10.0f, +10.0f); // to scale the object 

    // Camera movements
    ImGui::SliderFloat3("Camera Position", camPos, -5, 5);
    ImGui::SliderFloat("Field Of View: ", &FoV, 18.0f, 90.0f);

   ImGui::End();
   ImGui::Render();
   ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// This function gets called every time the scene gets redisplayed
void display(GLFWwindow* window)
{
   //Clear the screen to the color previously specified in the glClearColor(...) call.
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   //glm::mat4 M = glm::rotate(angle, glm::vec3(0.0f, 10.0f, 0.0f))*glm::scale(glm::vec3(scale*mesh_data[0].mScaleFactor));
   glm::mat4 V = glm::lookAt(glm::vec3(camPos[0], camPos[1], camPos[2]), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //for camera postion 
   glm::mat4 P = glm::perspective(FoV, aspectRatio, 0.1f, 100.0f);

   glUseProgram(shader_program);

   int M_loc = glGetUniformLocation(shader_program, "M");
   if (M_loc != -1)
   {
       glUniformMatrix4fv(M_loc, 1, false, glm::value_ptr(M[0]));
   }

   glActiveTexture(GL_TEXTURE0);
   int tex_loc = glGetUniformLocation(shader_program, "diffuse_tex");
   if (tex_loc != -1)
   {
       glUniform1i(tex_loc, 0); // we bound our texture to texture unit 0
   }

   int PVM_loc = glGetUniformLocation(shader_program, "PVM");

   for (int i = 0; i < num_meshes; i++)
   {
       glBindTexture(GL_TEXTURE_2D, texture_id[i]);
       if (PVM_loc != -1)
       {
           glm::mat4 PVM = P * V * M[i];
           //Set the value of the variable at a specific location
           glUniformMatrix4fv(PVM_loc, 1, false, glm::value_ptr(PVM));
       }

       glBindVertexArray(mesh_data[i].mVao);
       mesh_data[i].DrawMesh();
   }

   draw_gui(window);

   // Swap front and back buffers
   glfwSwapBuffers(window);
}

void idle()
{
    float time_sec = static_cast<float>(glfwGetTime());

    //Pass time_sec value to the shaders
    int time_loc = glGetUniformLocation(shader_program, "time");
    if (time_loc != -1)
    {
        glUniform1f(time_loc, time_sec);
    }

    if (auto_rotate == true)
    {
        u = sin(time_sec);
    }

    for (int i = 0; i < num_meshes; i++)
    {
        if (use_quats == true)
        {
            //Interpolate quaternions
            glm::quat q_start(R_start[i]), q_end(R_end[i]);//convert mat4 to quat
            glm::quat qi = glm::slerp(q_start, q_end, u);//slerp (spherical linear interpolate) quats
            R[i] = glm::mat4_cast(qi);//convert quat to 
        }
        else
        {
            //Interpolate rotation matrices
            R[i] = glm::mix(R_start[i], R_end[i], u);
        }

        M[i] = R[i] * T[i] * S[i];
    }
}

void reload_shader()
{
   GLuint new_shader = InitShader(vertex_shader.c_str(), fragment_shader.c_str());

   if (new_shader == -1) // loading failed
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f); //change clear color if shader can't be compiled
   }
   else
   {
      glClearColor(0.0f, 0.45f, 0.35f, 0.0f);

      if (shader_program != -1)
      {
         glDeleteProgram(shader_program);
      }
      shader_program = new_shader;
   }
}

//This function gets called when a key is pressed
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   std::cout << "key : " << key << ", " << char(key) << ", scancode: " << scancode << ", action: " << action << ", mods: " << mods << std::endl;

   if(action == GLFW_PRESS)
   {
      switch(key)
      {
         case 'r':
         case 'R':
            reload_shader();     
         break;

         case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
         break;     
      }
   }
}

//This function gets called when the mouse moves over the window.
void mouse_cursor(GLFWwindow* window, double x, double y)
{
    //std::cout << "cursor pos: " << x << ", " << y << std::endl;
}

//This function gets called when a mouse button is pressed.
void mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    //std::cout << "button : "<< button << ", action: " << action << ", mods: " << mods << std::endl;
}

//Initialize OpenGL state. This function only gets called once.
void initOpenGL()
{
   glewInit();

   glEnable(GL_DEPTH_TEST);

   reload_shader();
   for (int i = 0; i < num_meshes; i++)
   {
       mesh_data[i] = LoadMesh(mesh_name[i]);
       texture_id[i] = LoadTexture(texture_name[i]);
       S[i] = glm::scale(glm::vec3(scale * mesh_data[i].mScaleFactor));
   }
}


int main(void)
{
   GLFWwindow* window;

   /* Initialize the library */
   if (!glfwInit())
   {
      return -1;
   }

   /* Create a windowed mode window and its OpenGL context */
   window = glfwCreateWindow(1024, 1024, "Final_project", NULL, NULL);
   if (!window)
   {
      glfwTerminate();
      return -1;
   }

   //Register callback functions with glfw. 
   glfwSetKeyCallback(window, keyboard);
   glfwSetCursorPosCallback(window, mouse_cursor);
   glfwSetMouseButtonCallback(window, mouse_button);

   /* Make the window's context current */
   glfwMakeContextCurrent(window);

   initOpenGL();
   
   //Init ImGui
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 150");

   /* Loop until the user closes the window */
   while (!glfwWindowShouldClose(window))
   {
      idle();
      display(window);

      /* Poll for and process events */
      glfwPollEvents();
   }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

   glfwTerminate();
   return 0;
}