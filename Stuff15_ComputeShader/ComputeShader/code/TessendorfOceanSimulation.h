//
//  TessendorfOceanSimulation.h
//
#include <vulkan/vulkan.h>

/*
====================================================
Ocean
====================================================
*/
bool InitializeOceanSimulation( VkDevice vkDevice );
bool CreateTextureOceanSimulation( VkPhysicalDevice vkPhysicalDevice, VkDevice vkDevice, VkQueue vkGraphicsQueue, VkCommandBuffer vkCommandBuffer );
void UpdateOceanSimulation( VkDevice vkDevice, VkCommandBuffer vkCmdBuffer );
void CleanupOceanSimulation( VkDevice vkDevice );
