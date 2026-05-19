This is an OpenGL C++ project. We are building a 3D virtual museum.                                                                                  
Use the base code structure already present in the working directory.                                                                                   
                                                                                                                                                          
Do NOT use glm::lookAt, glm::perspective, glm::rotate, glm::translate,                                                                                  
glm::scale, gluLookAt, or any similar library functions. All matrices
must be implemented manually.                                                                                                                           
                                                                                                                                                          
Implement the following, in this order:                                                                                                                 
1. A first-person free camera controlled by WASD and mouse, using a manually computed View matrix.                                                                                                                       
2. A perspective projection using a manually computed Projection matrix.                                                                                

3. A simple scene with a floor plane and box-shaped walls so the environment is visible.

Replace the current floor texture with ../../data/textures/laminate_floor_02_diff_1k.png. Use the existing texture loading function already present in the codebase. Apply the texture to the floor mesh. Do not change anything else. Do not run any git commands.