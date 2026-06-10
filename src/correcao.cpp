#include <cstdlib>
#include <GLFW/glfw3.h>

void Correcao_KeyCallback(int key, int action, int mod)
{
    
    
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
}

