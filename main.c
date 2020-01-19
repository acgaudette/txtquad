#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define NAME "txtquad"
#define WIDTH 1024
#define HEIGHT 1024

#define SWAP_IMG_COUNT 3
#define FONT_SIZE (128 * 128)

struct Frame {
	size_t i;
	size_t last_i;
	float t;
	float last_t;
	float dt;
#ifdef DEBUG
	float acc;
#endif
};

struct DevData {
	VkPhysicalDevice hard;
	VkDevice log;
	size_t q_ind;
	VkQueue q;
};

struct SwapData {
	VkSwapchainKHR chain;
	VkFormat format;
	VkImage *img;
};

// TODO
struct BindingsData {
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;
};

struct GraphicsData {
	VkRenderPass pass;
	VkFramebuffer *fbuffers;
	VkPipelineLayout layout;
	VkPipeline pipeline;
};

struct SyncData {
	VkFence acquire;
	VkFence *submit;
	VkSemaphore *sem;
};

struct App {
	GLFWwindow *win;
	VkInstance inst;
	VkSurfaceKHR surf;
	struct DevData dev;
	struct SwapData swap;
	struct BindingsData bindings;
	struct GraphicsData graphics;
	struct SyncData sync;
	VkCommandPool pool;
	VkCommandBuffer *cmd;
} app;

static void panic()
{
	printf("Exit failure\n");
	exit(1);
}

static void panic_msg(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	panic();
}

// TODO: reorg on call order
// TODO: static
// TODO: free all mallocs

GLFWwindow *mk_win()
{
	if (!glfwInit()) {
		panic_msg("unable to initialize GLFW");
	}

	printf("Initialized GLFW\n");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // GLFW Vulkan support
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow *win = glfwCreateWindow(
		WIDTH,
		HEIGHT,
		NAME,
		NULL,
		NULL
	);

	printf("Created GLFW window \"%s\"\n", NAME);
	return win;
}

#define STYPE(NAME) .sType = VK_STRUCTURE_TYPE_ ## NAME ,

VkInstance mk_inst(GLFWwindow *win)
{
	VkApplicationInfo app_info = {
	STYPE(APPLICATION_INFO)
		.pApplicationName = NAME,
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = NAME,
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_0,
		.pNext = NULL,
	};

	printf(
		"Application \"%s\" with engine \"%s\"\n",
		app_info.pApplicationName,
		app_info.pEngineName
	);

	// Get WSI extensions
	unsigned int inst_ext_count;
	const char **inst_ext_names = glfwGetRequiredInstanceExtensions(
		&inst_ext_count
	);

	printf("WSI instance extensions:\n");
	for (size_t i = 0; i < inst_ext_count; ++i) {
		printf("\t%s\n", inst_ext_names[i]);
	}

	/* TODO: validate instance extensions */

	const char *layer_names[] = { "VK_LAYER_KHRONOS_validation" };
	VkValidationFeatureEnableEXT feature_names[] = {
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
	};

	/* TODO: validate layers */

	VkValidationFeaturesEXT features = {
	STYPE(VALIDATION_FEATURES_EXT)
		.enabledValidationFeatureCount = 1,
		.pEnabledValidationFeatures = feature_names,
		.pDisabledValidationFeatures = NULL,
		.pNext = NULL,
	};

	VkInstanceCreateInfo inst_create_info = {
	STYPE(INSTANCE_CREATE_INFO)
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = inst_ext_count,
		.ppEnabledExtensionNames = inst_ext_names,
		.enabledLayerCount = 1,
		.ppEnabledLayerNames = layer_names,
		.pNext = &features,
	};

	VkInstance inst;
	VkResult err = vkCreateInstance(&inst_create_info, NULL, &inst);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create Vulkan instance");
	}

	printf("Created Vulkan instance\n");
	return inst;
}

VkSurfaceKHR mk_surf(GLFWwindow *win, VkInstance inst)
{
	VkSurfaceKHR surf;
	VkResult err = glfwCreateWindowSurface(inst, win, NULL, &surf);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create window surface");
	}

	printf("Created window surface\n");
	return surf;
}

struct DevData mk_dev(VkInstance inst, VkSurfaceKHR surf)
{
	unsigned int dev_count = 1;
	VkPhysicalDevice hard_dev;
	vkEnumeratePhysicalDevices(inst, &dev_count, &hard_dev);

	if (!dev_count) {
		panic_msg("no physical devices available");
	}

	printf("Found physical device\n");

	VkPhysicalDeviceProperties dev_prop;
	vkGetPhysicalDeviceProperties(hard_dev, &dev_prop);
	printf("Found device \"%s\"\n", dev_prop.deviceName);

	unsigned int q_family_count = 1;
	VkQueueFamilyProperties q_prop;
	vkGetPhysicalDeviceQueueFamilyProperties(
		hard_dev,
		&q_family_count,
		&q_prop
	);

	size_t q_ind = 0;
	VkBool32 can_present;
	vkGetPhysicalDeviceSurfaceSupportKHR(
		hard_dev,
		q_ind,
		surf,
		&can_present
	);

	if (!can_present) {
		panic_msg("default queue family cannot present to window");
	}

	/* TODO: a more robust queue infrastructure */

	printf("Found queue [%lu]\n", q_ind);

	float pri = 1.f;
	VkDeviceQueueCreateInfo q_create_info = {
	STYPE(DEVICE_QUEUE_CREATE_INFO)
		.queueFamilyIndex = q_ind,
		.queueCount = 1,
		.pQueuePriorities = &pri,
		.pNext = NULL,
	};

	const char *dev_ext_names[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	/* TODO: validate device extensions */

	VkDeviceCreateInfo dev_create_info = {
	STYPE(DEVICE_CREATE_INFO)
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &q_create_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = dev_ext_names,
		.pEnabledFeatures = NULL,
		.pNext = NULL,
	};

	VkDevice dev;
	VkResult err = vkCreateDevice(hard_dev, &dev_create_info, NULL, &dev);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create logical device");
	}

	printf("Created logical device\n");

	VkQueue q;
	vkGetDeviceQueue(dev, q_ind, 0, &q);
	printf("Acquired queue [%lu]\n", q_ind);

	return (struct DevData) {
		hard_dev,
		dev,
		q_ind,
		q,
	};
}

struct SwapData mk_swap(struct DevData dev, VkSurfaceKHR surf)
{
	VkSurfaceCapabilitiesKHR cap;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev.hard, surf, &cap);
	printf(
		"Current extent: %ux%u\n",
		cap.currentExtent.width,
		cap.currentExtent.height
	);

	printf("Image count range: %u-", cap.minImageCount);
	if (cap.maxImageCount) {
	       printf("%u\n", cap.maxImageCount);
	       assert(cap.maxImageCount >= SWAP_IMG_COUNT);
	} else printf("inf\n");
	assert(SWAP_IMG_COUNT >= cap.minImageCount);

	unsigned int img_count = SWAP_IMG_COUNT;
	VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
	VkSwapchainCreateInfoKHR swap_create_info = {
	STYPE(SWAPCHAIN_CREATE_INFO_KHR)
		.flags = 0,
		.surface = surf,
		.minImageCount = img_count,
		.imageFormat = format,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = { WIDTH, HEIGHT },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = cap.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	/* TODO: validate formats, presentation modes */

	VkSwapchainKHR swapchain;
	VkResult err = vkCreateSwapchainKHR(
		dev.log,
		&swap_create_info,
		NULL,
		&swapchain
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create swapchain");
	}

	// TODO
	VkImage *img = malloc(sizeof(VkImage) * SWAP_IMG_COUNT);
	vkGetSwapchainImagesKHR(dev.log, swapchain, &img_count, img);
	printf("Created swapchain with %u images\n", img_count);
	assert(img_count == SWAP_IMG_COUNT);

	return (struct SwapData) {
		swapchain,
		format,
		img,
	};
}

VkCommandPool mk_pool(struct DevData dev)
{
	VkResult err;
	VkCommandPoolCreateInfo pool_create_info = {
	STYPE(COMMAND_POOL_CREATE_INFO)
		.flags = 0,
		.queueFamilyIndex = dev.q_ind,
		.pNext = NULL,
	};

	VkCommandPool pool;
	err = vkCreateCommandPool(dev.log, &pool_create_info, NULL, &pool);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create command pool");
	}

	printf("Created command pool\n");
	return pool;
}

static const char *mem_prop_flag_str(int flag)
{
	switch (flag) {
	case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT:
		return "device_local";
	case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT:
		return "host_visible";
	case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
		return "host_coherent";
	case VK_MEMORY_PROPERTY_HOST_CACHED_BIT:
		return "host_cached";
	case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT:
		return "lazily_allocated";
	case VK_MEMORY_PROPERTY_PROTECTED_BIT:
		return "protected";
	default:
		return "unknown_flag";
	}
}

unsigned char *read_font()
{
	errno = 0;
	const char *path = "./font.pbm";
	FILE *file = fopen(path, "r");
	if (errno) {
		fprintf(stderr, "Error opening file at path \"%s\"\n", path);
		panic();
	}

	/* Header */

	#define HEAD_LEN 11
	char head[HEAD_LEN], *ptr = head;

	clearerr(file);
	fread(head, 1, HEAD_LEN, file);
	if (ferror(file)) {
		fprintf(
			stderr,
			"Error reading file \"%s\" header\n",
			path
		);
		panic();
	}

	if (strncmp(ptr, "P4", 2)) {
		fprintf(
			stderr,
			"Invalid header in file \"%s\": "
			"not a PBM file (\"%.2s\")\n",
			path,
			head
		);
		panic();
	}

	ptr += 3;
	if (strncmp(ptr, "128 128", 7)) {
		fprintf(
			stderr,
			"Invalid header in file \"%s\": "
			"not a 128-pixel square image\n",
			path
		);
		panic();
	}

	ptr += 7;
	if (*ptr != '\n') {
		fprintf(stderr, "Invalid header in file \"%s\"\n", path);
		panic();
	}

	#undef HEAD_LEN

	/* Data */

	unsigned char *raw = malloc(FONT_SIZE / 8);
	unsigned char *exp = malloc(FONT_SIZE);

	clearerr(file);
	fread(raw, 1, FONT_SIZE / 8, file);
	if (ferror(file)) {
		fprintf(
			stderr,
			"Error reading file \"%s\" data\n",
			path
		);
		panic();
	}

	for (size_t i = 0; i < FONT_SIZE / 8; ++i) {
		size_t j = 8;
		while (j --> 0) {
			exp[8 * i + j] = 255 * (0 != (raw[i] & (1 << (7 - j))));
		}
	}

	fclose(file);
	free(raw);

	printf("Read %u bytes from \"%s\"\n", FONT_SIZE / 8, path);
	return exp;
}

// FIXME
struct BindingsData load_font(struct DevData dev, VkCommandPool pool)
{
	/* Staging buffer */

	VkResult err;
	VkBufferCreateInfo buf_create_info = {
	STYPE(BUFFER_CREATE_INFO)
		.flags = 0,
		.size = FONT_SIZE,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.pNext = NULL,
	};

	VkBuffer staging;
	err = vkCreateBuffer(dev.log, &buf_create_info, NULL, &staging);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create staging buffer");
	}

	printf("Created font staging buffer with size %d\n", FONT_SIZE);

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(dev.hard, &mem_props);

	for (size_t i = 0; i < mem_props.memoryTypeCount; ++i) {
		VkMemoryType t = mem_props.memoryTypes[i];
		printf("Found memory type %lu:\n", i);
		VkMemoryPropertyFlags flags = t.propertyFlags;

		size_t j = 0;
		while (flags) {
			if (flags & 1) {
				printf(
					"\t%s\n",
					mem_prop_flag_str(1 << j)
				);
			}

			flags >>= 1;
			++j;
		}
	}

	/* TODO: validate hardware memory and select best heap */
	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(dev.log, staging, &mem_requirements);
	assert(mem_requirements.size == FONT_SIZE);
	VkMemoryAllocateInfo staging_alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = FONT_SIZE,
		.memoryTypeIndex = 0,
		.pNext = NULL,
	};

	VkDeviceMemory staging_mem;
	err = vkAllocateMemory(
		dev.log,
		&staging_alloc_info,
		NULL,
		&staging_mem
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate memory for staging buffer");
	}

	err = vkBindBufferMemory(dev.log, staging, staging_mem, 0);
	if (err != VK_SUCCESS) {
		panic_msg("unable to bind staging buffer memory");
	}

	unsigned char *font = read_font();
	void *src;

	err = vkMapMemory(dev.log, staging_mem, 0, VK_WHOLE_SIZE, 0, &src);
	if (err != VK_SUCCESS) {
		panic_msg("unable to map device memory to host");
	}

	printf("Backed staging buffer\n");
	memcpy(src, font, FONT_SIZE);
	printf("Copied font to device\n");

	/* Texture */

	VkFormat format = VK_FORMAT_R8_UNORM;
	VkImageCreateInfo tex_create_info = {
	STYPE(IMAGE_CREATE_INFO)
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { 128, 128, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_SAMPLED_BIT
		       | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.pNext = NULL,
	};

	VkImage tex;

	err = vkCreateImage(dev.log, &tex_create_info, NULL, &tex);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create font texture");
	}

	printf("Created font texture\n");

	VkDeviceSize align_size = (mem_requirements.size + 0x1000 - 1)
		& ~(0x1000 - 1);

	VkMemoryAllocateInfo tex_alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = align_size,
		.memoryTypeIndex = 0,
		.pNext = NULL,
	};

	VkDeviceMemory tex_mem;
	err = vkAllocateMemory(dev.log, &tex_alloc_info, NULL, &tex_mem);
	if (err != VK_SUCCESS) {
		panic_msg(
			"unable to allocate memory for font texture"
		);
	}

	vkBindImageMemory(dev.log, tex, tex_mem, 0);
	printf("Backed font texture\n");

	VkImageView tex_view;
	VkImageViewCreateInfo view_create_info = {
	STYPE(IMAGE_VIEW_CREATE_INFO)
		.flags = 0,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.pNext = NULL,
	};

	view_create_info.image = tex;
	err = vkCreateImageView(dev.log, &view_create_info, NULL, &tex_view);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create image view");
	}

	printf("Created font texture image view\n");

	/* Bindings */

	VkDescriptorSetLayoutBinding bindings[2] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		}, {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		}
	};

	VkDescriptorSetLayoutCreateInfo desc_create_info = {
	STYPE(DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
		.flags = 0,
		.bindingCount = 2,
		.pBindings = bindings,
		.pNext = NULL,
	};

	VkDescriptorSetLayout desc_layout;
	err = vkCreateDescriptorSetLayout(
		dev.log,
		&desc_create_info,
		NULL,
		&desc_layout
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create texture descriptor set layout");
	}

	printf("Created texture descriptor set layout\n");

	VkDescriptorPoolSize pool_sizes[2] = {
		{
			.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
		}, {
			.type = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
		}
	};

	VkDescriptorPoolCreateInfo desc_pool_create_info = {
	STYPE(DESCRIPTOR_POOL_CREATE_INFO)
		.flags = 0,
		.maxSets = 1,
		.poolSizeCount = 2,
		.pPoolSizes = pool_sizes,
		.pNext = NULL,
	};

	VkDescriptorPool desc_pool;
	err = vkCreateDescriptorPool(
		dev.log,
		&desc_pool_create_info,
		NULL,
		&desc_pool
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create texture descriptor pool");
	}

	printf("Created texture descriptor pool\n");

	VkDescriptorSetAllocateInfo desc_alloc_info = {
	STYPE(DESCRIPTOR_SET_ALLOCATE_INFO)
		.descriptorPool = desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_layout,
		.pNext = NULL,
	};

	VkDescriptorSet set;
	err = vkAllocateDescriptorSets(dev.log, &desc_alloc_info, &set);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate texture descriptor sets\n");
	}

	printf("Backed texture descriptor sets\n");

	VkSamplerCreateInfo sampler_create_info = {
	STYPE(SAMPLER_CREATE_INFO)
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.mipLodBias = 0.f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 0.f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_NEVER,
		.minLod = 0.f,
		.maxLod = 0.f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.pNext = NULL,
	};

	VkSampler sampler;
	err = vkCreateSampler(dev.log, &sampler_create_info, NULL, &sampler);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create unfiltered texture sampler\n");
	}

	printf("Created unfiltered texture sampler\n");

	VkDescriptorImageInfo img_info = {
		.sampler = sampler,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	img_info.imageView = tex_view;

	VkWriteDescriptorSet tex_set = {
	STYPE(WRITE_DESCRIPTOR_SET)
		.dstSet = set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = &img_info,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL,
		.pNext = NULL,
	};

	VkWriteDescriptorSet sampler_set = {
	STYPE(WRITE_DESCRIPTOR_SET)
		.dstSet = set,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo = &img_info,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL,
		.pNext = NULL,
	};

	VkWriteDescriptorSet writes[2] = { tex_set, sampler_set };
	vkUpdateDescriptorSets(dev.log, 2, writes, 0, NULL);

	printf("Updated texture descriptor sets\n");

	/* Transfer */

	VkCommandBufferAllocateInfo cmd_alloc_info = {
	STYPE(COMMAND_BUFFER_ALLOCATE_INFO)
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
		.pNext = NULL,
	};

	VkCommandBuffer cmd;
	err = vkAllocateCommandBuffers(dev.log, &cmd_alloc_info, &cmd);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate command buffer\n");
	}

	// TODO: fixall %d
	printf("Allocated transfer command buffer\n");

	VkCommandBufferBeginInfo begin_info = {
	STYPE(COMMAND_BUFFER_BEGIN_INFO)
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL,
		.pNext = NULL,
	};

	err = vkBeginCommandBuffer(cmd, &begin_info);
	if (err != VK_SUCCESS) {
		panic_msg("unable to begin command buffer recording");
	}

	VkImageMemoryBarrier tex_prep_barrier = {
	STYPE(IMAGE_MEMORY_BARRIER)
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = tex,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.pNext = NULL,
	};

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, // TODO: specify
		0, NULL,
		0, NULL,
		1, &tex_prep_barrier
	);

	VkBufferImageCopy dev_region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { 128, 128, 1 },
	};

	vkCmdCopyBufferToImage(
		cmd,
		staging,
		tex,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&dev_region
	);

	VkImageMemoryBarrier tex_swap_barrier = {
	STYPE(IMAGE_MEMORY_BARRIER)
		.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = tex,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.pNext = NULL,
	};

	vkCmdPipelineBarrier(
		cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, // TODO: specify
		0, NULL,
		0, NULL,
		1, &tex_swap_barrier
	);

	err = vkEndCommandBuffer(cmd);
	if (err != VK_SUCCESS) {
		panic_msg("unable to end command buffer recording");
	}

	VkSubmitInfo submit_info = {
	STYPE(SUBMIT_INFO)
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL,
		.pNext = NULL,
	};

	vkQueueSubmit(dev.q, 1, &submit_info, NULL);
	vkQueueWaitIdle(dev.q);
	vkFreeCommandBuffers(dev.log, pool, 1, &cmd);

	// FIXME
	return (struct BindingsData) {
		desc_layout,
		set,
	};
}

static void read_shader(const char *name, uint32_t **out, size_t *out_len)
{
	uint32_t *buffer;
	size_t len;

	errno = 0;
	FILE *file = fopen(name, "rb");
	if (errno) {
		fprintf(stderr, "Error opening file at path \"%s\"\n", name);
		panic();
	}

	if (fseek(file, 0, SEEK_END)) {
		perror("Error seeking file");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	len = ftell(file);
	if (errno) {
		perror("Error acquiring length of file");
		exit(EXIT_FAILURE);
	}

	if (fseek(file, 0, SEEK_SET)) {
		perror("Error seeking file");
		exit(EXIT_FAILURE);
	}

	buffer = malloc(len);
	assert(buffer);

	clearerr(file);
	fread(buffer, 1, len, file);
	if (ferror(file)) {
		fprintf(
			stderr,
			"Error reading file \"%s\"\n",
			name
		);
		exit(EXIT_FAILURE);
	}

	fclose(file);
	assert(buffer[0] == 0x07230203);

	*out = buffer;
	*out_len = len;

	printf("Read %lu words from \"%s\"\n", len, name);
}

struct GraphicsData mk_graphics(
	VkDevice dev,
	struct SwapData swap,
	VkDescriptorSetLayout desc_layout
) {
	/* Shader modules */

	VkResult err;
	size_t spv_size;
	uint32_t *spv;
	read_shader("vert.spv", &spv, &spv_size);

	VkShaderModuleCreateInfo mod_create_info = {
	STYPE(SHADER_MODULE_CREATE_INFO)
		.flags = 0,
		.codeSize = spv_size,
		.pCode = spv,
		.pNext = NULL,
	};

	VkShaderModule vert_mod, frag_mod;

	err = vkCreateShaderModule(dev, &mod_create_info, NULL, &vert_mod);
	if (err != VK_SUCCESS) {
		panic_msg(
			"unable to create vertex "
			"shader module\n"
		);
	}

	read_shader("frag.spv", &spv, &spv_size);
	mod_create_info.codeSize = spv_size;
	mod_create_info.pCode = spv;

	err = vkCreateShaderModule(dev, &mod_create_info, NULL, &frag_mod);
	if (err != VK_SUCCESS) {
		panic_msg(
			"unable to create fragment "
			"shader module\n"
		);
	}

	printf("Created shader modules (2)\n");

	/* Pipeline layout */

	VkPipelineShaderStageCreateInfo vert_stage_create_info = {
	STYPE(PIPELINE_SHADER_STAGE_CREATE_INFO)
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_mod,
		.pName = "main",
		.pSpecializationInfo = NULL,
		.pNext = NULL,
	};

	VkPipelineShaderStageCreateInfo frag_stage_create_info = {
	STYPE(PIPELINE_SHADER_STAGE_CREATE_INFO)
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_mod,
		.pName = "main",
		.pSpecializationInfo = NULL,
		.pNext = NULL,
	};

	VkPipelineShaderStageCreateInfo shader_create_infos[2] = {
		vert_stage_create_info,
		frag_stage_create_info,
	};

	VkPipelineVertexInputStateCreateInfo null_vert_state_create_info = {
	STYPE(PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
		.pNext = NULL,
	};

	VkPipelineInputAssemblyStateCreateInfo asm_state_create_info = {
	STYPE(PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // Quads
		.primitiveRestartEnable = VK_FALSE,
		.pNext = NULL,
	};

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = WIDTH,
		.height = HEIGHT,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { WIDTH, HEIGHT },
	};

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {
	STYPE(PIPELINE_VIEWPORT_STATE_CREATE_INFO)
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
		.pNext = NULL,
	};

	VkPipelineRasterizationStateCreateInfo raster_state_create_info = {
	STYPE(PIPELINE_RASTERIZATION_STATE_CREATE_INFO)
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = 0,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.f,
		.depthBiasClamp = 0.f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.f,
		.pNext = NULL,
	};

	VkPipelineMultisampleStateCreateInfo null_multi_state_create_info = {
	STYPE(PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
		.flags = 0,
		.rasterizationSamples = 1,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0.f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
		.pNext = NULL,
	};

	VkPipelineColorBlendAttachmentState null_blend_attach = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = 0,
		.dstColorBlendFactor = 0,
		.colorBlendOp = 0,
		.srcAlphaBlendFactor = 0,
		.dstAlphaBlendFactor = 0,
		.alphaBlendOp = 0,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
		                | VK_COLOR_COMPONENT_G_BIT
		                | VK_COLOR_COMPONENT_B_BIT
		                | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo null_blend_state_create_info = {
	STYPE(PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = 0,
		.attachmentCount = 1,
		.pAttachments = &null_blend_attach,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f },
		.pNext = NULL,
	};

	VkPipelineLayoutCreateInfo pipe_layout_create_info = {
	STYPE(PIPELINE_LAYOUT_CREATE_INFO)
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &desc_layout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL,
		.pNext = NULL,
	};

	VkPipelineLayout null_pipe_layout;
	err = vkCreatePipelineLayout(
		dev,
		&pipe_layout_create_info,
		NULL,
		&null_pipe_layout
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create pipeline layout");
	}

	printf("Created pipeline layout\n");

	VkAttachmentDescription color_attach = {
		.format = swap.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference attach_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attach_ref,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};

	VkRenderPassCreateInfo pass_create_info = {
	STYPE(RENDER_PASS_CREATE_INFO)
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &color_attach,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0,
		.pDependencies = NULL,
		.pNext = NULL,
	};

	VkRenderPass pass;
	err = vkCreateRenderPass(dev, &pass_create_info, NULL, &pass);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create post-processing render pass");
	}

	printf("Created render pass\n");

	VkGraphicsPipelineCreateInfo pipe_create_info = {
	STYPE(GRAPHICS_PIPELINE_CREATE_INFO)
		.flags = 0,
		.stageCount = 2,
		.pStages = shader_create_infos,
		.pVertexInputState = &null_vert_state_create_info,
		.pInputAssemblyState = &asm_state_create_info,
		.pTessellationState = NULL,
		.pViewportState = &viewport_state_create_info,
		.pRasterizationState = &raster_state_create_info,
		.pMultisampleState = &null_multi_state_create_info,
		.pDepthStencilState = NULL,
		.pColorBlendState = &null_blend_state_create_info,
		.pDynamicState = NULL,
		.layout = null_pipe_layout,
		.renderPass = pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = NULL,
	};

	VkPipeline pipeline;
	err = vkCreateGraphicsPipelines(
		dev,
		VK_NULL_HANDLE,
		1,
		&pipe_create_info,
		NULL,
		&pipeline
	);

	if (err != VK_SUCCESS) {
		panic_msg(
			"unable to create "
			"graphics pipeline"
		);
	}

	printf("Created graphics pipeline\n");

	VkImageViewCreateInfo view_create_info = {
	STYPE(IMAGE_VIEW_CREATE_INFO)
		.flags = 0,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = swap.format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.pNext = NULL,
	};

	VkImageView views[SWAP_IMG_COUNT];
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		view_create_info.image = swap.img[i];

		err = vkCreateImageView(
			dev,
			&view_create_info,
			NULL,
			&views[i]
		);

		if (err != VK_SUCCESS) {
			panic_msg("unable to create image view");
		}
	}

	printf("Created %d image views\n", SWAP_IMG_COUNT);

	VkFramebufferCreateInfo fbuffer_create_info = {
	STYPE(FRAMEBUFFER_CREATE_INFO)
		.flags = 0,
		.renderPass = pass,
		.attachmentCount = 1,
		.width = WIDTH,
		.height = HEIGHT,
		.layers = 1,
		.pNext = NULL,
	};

	VkFramebuffer *fbuffers = malloc(
		sizeof(VkFramebuffer) * SWAP_IMG_COUNT
	);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		fbuffer_create_info.pAttachments = &views[i];
		err = vkCreateFramebuffer(
			dev,
			&fbuffer_create_info,
			NULL,
			&fbuffers[i]
		);

		if (err != VK_SUCCESS) {
			panic_msg("unable to create framebuffer");
		}
	}

	printf("Created %d post-processing framebuffers\n", SWAP_IMG_COUNT);
	return (struct GraphicsData) {
		pass,
		fbuffers, // FIXME
		null_pipe_layout,
		pipeline,
	};
}

VkCommandBuffer *record_graphics(
	VkDevice dev,
	struct SwapData swap,
	VkDescriptorSet set, // FIXME
	struct GraphicsData graphics,
	VkCommandPool pool
) {
	VkCommandBufferAllocateInfo cmd_alloc_info = {
	STYPE(COMMAND_BUFFER_ALLOCATE_INFO)
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = SWAP_IMG_COUNT,
		.pNext = NULL,
	};

	VkCommandBuffer *cmd = malloc(
		sizeof(VkCommandBuffer) * SWAP_IMG_COUNT
	);

	VkResult err = vkAllocateCommandBuffers(dev, &cmd_alloc_info, cmd);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate command buffers\n");
	}

	printf("Allocated %u command buffers\n", SWAP_IMG_COUNT);

	VkCommandBufferBeginInfo begin_info = {
	STYPE(COMMAND_BUFFER_BEGIN_INFO)
		.flags = 0,
		.pInheritanceInfo = NULL,
		.pNext = NULL,
	};

	// TODO: verify

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		err = vkBeginCommandBuffer(cmd[i], &begin_info);
		if (err != VK_SUCCESS) {
			panic_msg("unable to begin command buffer recording");
		}

		VkRenderPassBeginInfo pass_beg_info = {
		STYPE(RENDER_PASS_BEGIN_INFO)
			.renderPass = graphics.pass,
			.framebuffer = graphics.fbuffers[i],
			.renderArea = {
				.offset = { 0, 0 },
				.extent = { WIDTH, HEIGHT },
			},
			.clearValueCount = 0,
			.pClearValues = NULL,
			.pNext = NULL,
		};

		vkCmdBeginRenderPass(
			cmd[i],
			&pass_beg_info,
			VK_SUBPASS_CONTENTS_INLINE
		);

		vkCmdBindPipeline(
			cmd[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics.pipeline
		);

		vkCmdBindDescriptorSets(
			cmd[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics.layout,
			0,
			1,
			&set,
			0,
			NULL
		);

		vkCmdDraw(cmd[i], 4, 1, 0, 0); // Quad
		vkCmdEndRenderPass(cmd[i]);

		/* Transition swapchain image for rendering */

		VkImageMemoryBarrier img_barrier = {
		STYPE(IMAGE_MEMORY_BARRIER)
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_HOST_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = swap.img[i],
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.pNext = NULL,
		};

		vkCmdPipelineBarrier(
			cmd[i],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_HOST_BIT,
			0, // TODO: specify
			0, NULL,
			0, NULL,
			1, &img_barrier
		);

		err = vkEndCommandBuffer(cmd[i]);
		if (err != VK_SUCCESS) {
			panic_msg("unable to end command buffer recording");
		}

		printf("Recorded command buffer [%lu]\n", i);
	}

	return cmd;
}

struct SyncData mk_sync(VkDevice dev)
{
	VkResult err;
	VkFenceCreateInfo fence_create_info = {
	STYPE(FENCE_CREATE_INFO)
		.flags = 0,
		.pNext = NULL,
	};

	VkFence acq_fence;
	err = vkCreateFence(dev, &fence_create_info, NULL, &acq_fence);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create acquisition fence");
	}

	VkFence *sub_fence = malloc(sizeof(VkFence) * SWAP_IMG_COUNT);
	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		err = vkCreateFence(
			dev,
			&fence_create_info,
			NULL,
			&sub_fence[i]
		);

		if (err != VK_SUCCESS) {
			panic_msg("unable to create submit fence");
		}
	}

	printf(
		"Created acquisition and submit fences (%d)\n",
		SWAP_IMG_COUNT + 1
	);

	VkSemaphoreCreateInfo sem_create_info = {
	STYPE(SEMAPHORE_CREATE_INFO)
		.flags = 0,
		.pNext = NULL,
	};

	VkSemaphore *sem = malloc(sizeof(VkSemaphore) * SWAP_IMG_COUNT);
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
	err = vkCreateSemaphore(dev, &sem_create_info, NULL, &sem[i]);
		if (err != VK_SUCCESS) {
			panic_msg("unable to create semaphore");
		}
	}

	printf("Created %d semaphores\n", SWAP_IMG_COUNT);
	return (struct SyncData) {
		acq_fence,
		sub_fence,
		sem,
	};
}

// FIXME
static int done;
void update(
	GLFWwindow *win,
	struct DevData dev,
	VkSwapchainKHR swapchain,
	VkCommandBuffer *cmd,
	struct SyncData sync
) {
	printf("Initializing update data...\n");
	glfwSetTime(0);

	printf("Entering render loop...\n");
	unsigned int img_i;
	struct Frame data = {
		.i = 0,
	};

	VkResult err;
	while (!done) {
		if (glfwWindowShouldClose(win)) break;

		vkAcquireNextImageKHR(
			dev.log,
			swapchain,
			UINT64_MAX,
			NULL,
			sync.acquire,
			&img_i
		);

		++data.i;
		data.t = glfwGetTime();
		data.dt = data.t - data.last_t;
		data.last_t = data.t;
#ifdef DEBUG
		data.acc += data.dt;
		if (data.acc > 1) {
			printf(
				"FPS=%lu\tdt=%.3fms\n",
				data.i - data.last_i,
				data.dt * 1000
			);

			data.last_i = data.i;
			data.acc = 0;
		}
#endif
		glfwPollEvents();
		// TODO: lib
		// update();

		VkSubmitInfo submit_info = {
		STYPE(SUBMIT_INFO)
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = NULL,
			.commandBufferCount = 1,
			.pCommandBuffers = cmd + img_i,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = sync.sem + img_i,
			.pNext = NULL,
		};

		VkFence fences[2] = { sync.acquire, sync.submit[img_i] };
		vkWaitForFences(dev.log, 2, fences, VK_TRUE, UINT64_MAX);
		vkResetFences(dev.log, 2, fences);

		/* TODO: assuming coherent memory
		err = vkFlushMappedMemoryRanges(dev.log, 1, &range[img_i]);
		if (err != VK_SUCCESS) {
			panic_msg("unable to flush mapped memory");
		}
		*/

		err = vkQueueSubmit(
			dev.q,
			1,
			&submit_info,
			sync.submit[img_i]
		);

		if (err != VK_SUCCESS) {
			panic_msg("unable to submit queue");
		}

		VkPresentInfoKHR present_info = {
		STYPE(PRESENT_INFO_KHR)
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = sync.sem + img_i,
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &img_i,
			.pResults = NULL,
			.pNext = NULL,
		};

		err = vkQueuePresentKHR(dev.q, &present_info);
		if (err != VK_SUCCESS) {
			panic_msg("unable to present");
		}
	}

	vkDeviceWaitIdle(dev.log);
}

void app_free()
{
	/*
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyImage(dev, col_tex[i], NULL);
	}

	vkFreeMemory(dev, col_tex_mem, NULL);
	vkUnmapMemory(dev, staging_mem);

	vkDestroySampler(dev, sampler, NULL);
	vkDestroyDescriptorSetLayout(dev, desc_layout, NULL);
	vkDestroyDescriptorPool(dev, desc_pool, NULL);
	vkDestroyShaderModule(dev, vert_mod, NULL);
	vkDestroyShaderModule(dev, frag_mod, NULL);
	vkDestroyRenderPass(dev, pass, NULL);
	vkDestroyPipelineLayout(dev, null_pipe_layout, NULL);
	vkDestroyPipeline(dev, pipeline, NULL);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyImageView(dev, tex_views[i], NULL);
		vkDestroyImageView(dev, views[i], NULL);
		vkDestroyFramebuffer(dev, fbuffers[i], NULL);
		vkDestroyBuffer(dev, staging[i], NULL);
	}

	vkFreeMemory(dev, staging_mem, NULL);
	vkDestroyCommandPool(dev, pool, NULL);
	vkDestroyFence(dev, acq_fence, NULL);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyFence(dev, sub_fence[i], NULL);
		vkDestroySemaphore(dev, sem[i], NULL);
	}
	*/

	vkDestroySwapchainKHR(app.dev.log, app.swap.chain, NULL);
	vkDestroySurfaceKHR(app.inst, app.surf, NULL);
	vkDestroyDevice(app.dev.log, NULL);
	vkDestroyInstance(app.inst, NULL);

	// TODO: free command buffers

	glfwDestroyWindow(app.win);
	glfwTerminate();

	printf("Cleanup complete\n");
}

int main()
{
	app.win = mk_win();
	app.inst = mk_inst(app.win);
	app.surf = mk_surf(app.win, app.inst);
	app.dev = mk_dev(app.inst, app.surf);
	app.swap = mk_swap(app.dev, app.surf);
	app.pool = mk_pool(app.dev);

	// FIXME: separate descriptor
	//        separate cmd
	app.bindings = load_font(app.dev, app.pool);
	app.graphics = mk_graphics(app.dev.log, app.swap, app.bindings.layout);

	app.cmd = record_graphics(
		app.dev.log,
		app.swap,
		app.bindings.set,
		app.graphics,
		app.pool
	);

	app.sync = mk_sync(app.dev.log);
	// TODO: lib
	update(app.win, app.dev, app.swap.chain, app.cmd, app.sync);

	//app_free();
	printf("Exit success\n");
	exit(0);
}
