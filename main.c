#include <string.h>
#include <math.h>
#include <assert.h>
#include <errno.h>
#include <vulkan/vulkan.h> // Must include before GLFW
#include <GLFW/glfw3.h>
#include "txtquad.h"
#include "inp.h"
#include "vkext.h"

#define NAME "txtquad"
#define SWAP_IMG_COUNT 3
#define FONT_SIZE (128 * 128)
#define FONT_OFF (128 / 8)

static struct Share *share_buf;
static struct RawChar *char_buf;
static struct Text text;

static char *root_path;
static char *filename;

struct RawChar {
	m4 model;
	v4 col;
	v2 off;
	v2 _slop;
};

static v2 char_off(char c)
{
	return (v2) {
		c % FONT_OFF,
		c / FONT_OFF,
	};
}

static void text_update(unsigned int index, struct Frame data)
{
	struct RawChar *buf = char_buf + index * MAX_CHAR;

	for (size_t i = 0; i < text.char_count; ++i) {
		struct Char c = text.chars[i];
		buf[i] = (struct RawChar) {
			.model = m4_model(c.pos, c.rot, c.scale),
			.col = c.col,
			.off = char_off(c.v),
		};
	}

	size_t end = text.char_count;
	for (size_t i = 0; i < text.block_count; ++i) {
		struct Block block = text.blocks[i];
		float line_scale = block.spacing * block.scale;
		float height = line_scale;
		float width = 0.f;

		float k = 0;
		for (size_t j = 0; j < block.str_len; ++j) {
			if (block.str[j] == '\n') {
				height += line_scale;
				if (k > width) width = k;
				k = 0;
				continue;
			}

			k += block.scale;
		}

		if (k > width) width = k;

		m3 rot = qt_to_m3(block.rot);
		v3 right = rot.c0;
		v3 left = v3_neg(right);
		v3 up = rot.c1;
		v3 down = v3_neg(up);

		v2 off2 = v2_mul(block.off, block.scale);
		v3 off3 = v3_add(
			v3_mul(left, width * block.piv.x - off2.x),
			v3_mul(up, height * block.piv.y - off2.y)
		);

		v3 start = v3_add(block.pos, off3);
		v3 pos = start;

		// TODO: for justification,
		// need to know the width of each line.

		size_t line = 0;
		for (size_t j = 0; j < block.str_len; ++j) {
			char c = block.str[j];
			switch (c) {
			case '\n':
				++line;
				float amt = line * line_scale;
				pos = v3_add(start, v3_mul(down, amt));
				continue;
			}

			buf[end + j - line] = (struct RawChar) {
				.model = m4_model(pos, block.rot, block.scale),
				.col = block.col,
				.off = char_off(c),
			};

			pos = v3_add(pos, v3_mul(right, block.scale));
		}

		end += block.str_len - line;
		if (!block.cursor) continue;
		int flag = fmodf(data.t, 1.f) > .5f;
		if (flag) continue;
		buf[end] = (struct RawChar) {
			.model = m4_model(pos, block.rot, block.scale),
			.col = block.col,
			.off = char_off(block.cursor),
		};
		++end;
	}

	size_t clear_size = (MAX_CHAR - end) * sizeof(struct RawChar);
	memset(buf + end, 0, clear_size);
}

static struct App {
	GLFWwindow *win;
	VkInstance inst;
	VkSurfaceKHR surf;
	struct DevData {
		VkPhysicalDevice hard;
		VkDevice log;
		VkPhysicalDeviceProperties props;
		size_t q_ind;
		VkQueue q;
	} dev;
	struct SwapData {
		VkSwapchainKHR chain;
		VkFormat format;
		VkImage *img;
	} swap;
	struct FontData {
		VkImage img;
		VkDeviceMemory img_mem;
		VkImageView view;
		VkSampler sampler;
	} font;
	struct AkBuffer share;
	struct AkBuffer text;
	struct DescData {
		VkDescriptorSetLayout *layouts;
		VkDescriptorSet *sets;
		size_t count;
		VkDescriptorPool pool;
	} desc;
	struct GraphicsData {
		VkShaderModule vert;
		VkShaderModule frag;
		VkRenderPass pass;
		VkImageView *views;
		VkFramebuffer *fbuffers;
		VkPipelineLayout layout;
		VkPipeline pipeline;
	} graphics;
	struct SyncData {
		VkFence acquire;
		VkFence *submit;
		VkSemaphore *sem;
	} sync;
	VkCommandPool pool;
	VkCommandBuffer *cmd;
} app;

struct Input inp_data;
void inp_init(int *handles, size_t count)
{
	inp_data.handles = handles;
	inp_data.count = count;
}

#ifdef INP_TEXT
static void glfw_char_callback(GLFWwindow *win, unsigned int unicode)
{
	inp_ev_text(unicode);
}
#endif

static GLFWwindow *mk_win()
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

#ifdef INP_TEXT
	glfwSetCharCallback(win, glfw_char_callback);
#endif
	return win;
}

static VkInstance mk_inst(GLFWwindow *win)
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

static VkSurfaceKHR mk_surf(GLFWwindow *win, VkInstance inst)
{
	VkSurfaceKHR surf;
	VkResult err = glfwCreateWindowSurface(inst, win, NULL, &surf);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create window surface");
	}

	printf("Created window surface\n");
	return surf;
}

static struct DevData mk_dev(VkInstance inst, VkSurfaceKHR surf)
{
	unsigned int dev_count = 1;
	VkPhysicalDevice hard_dev;
	vkEnumeratePhysicalDevices(inst, &dev_count, &hard_dev);

	if (!dev_count) {
		panic_msg("no physical devices available");
	}

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

	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(hard_dev, &mem_props);

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

	return (struct DevData) {
		hard_dev,
		dev,
		dev_prop,
		q_ind,
		q,
	};
}

static struct SwapData mk_swap(struct DevData dev, VkSurfaceKHR surf)
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
		.presentMode = VK_PRESENT_MODE_FIFO_KHR, // TODO
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

	VkImage *img = NULL;
	img_count = 0;
	vkGetSwapchainImagesKHR(dev.log, swapchain, &img_count, img);
	assert(img_count == SWAP_IMG_COUNT);

	assert(!img);
	img = malloc(sizeof(VkImage) * img_count);
	assert( img);
	vkGetSwapchainImagesKHR(dev.log, swapchain, &img_count, img);

	printf("Created swapchain with %u images\n", img_count);
	return (struct SwapData) {
		swapchain,
		format,
		img,
	};
}

static VkCommandPool mk_pool(struct DevData dev)
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

static unsigned char *read_font() // Read custom PBM file
{
	errno = 0;
	strncpy(filename, "font.pbm", 8 + 1);
	FILE *file = fopen(root_path, "r");
	if (errno) {
		fprintf(
			stderr,
			"Error opening file at path \"%s\"\n",
			root_path
		);

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
			root_path
		);
		panic();
	}

	if (strncmp(ptr, "P4", 2)) {
		fprintf(
			stderr,
			"Invalid header in file \"%s\": "
			"not a PBM file (\"%.2s\")\n",
			root_path,
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
			root_path
		);
		panic();
	}

	ptr += 7;
	if (*ptr != '\n') {
		fprintf(stderr, "Invalid header in file \"%s\"\n", root_path);
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
			root_path
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

	printf("Read %u bytes from \"%s\"\n", FONT_SIZE / 8, root_path);
	return exp;
}

static struct FontData load_font(struct DevData dev, VkCommandPool pool)
{
	/* Staging buffer */

	VkResult err;
	struct AkBuffer staging;
	void *src;

	MK_BUF_AND_MAP(
		dev.log,
		"font staging",
		FONT_SIZE,
		TRANSFER_SRC,
		&staging,
		&src
	);

	unsigned char *font = read_font();
	memcpy(src, font, FONT_SIZE);
	printf("Copied font to device\n");
	free(font);

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

	VkDeviceSize align_size = (staging.req.size + 0x1000 - 1)
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
		staging.buf,
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
	free_buf(dev.log, staging);

	return (struct FontData) {
		tex,
		tex_mem,
		tex_view,
		sampler,
	};
}

static struct AkBuffer prep_share(VkDevice dev, struct Share **data)
{
	/* Could combine with the text buffer
	 * as they are updated at the same rate;
	 * however, this structure may
	 * become larger in the future
	 */

	struct AkBuffer buf;
	size_t size = SWAP_IMG_COUNT * sizeof(struct Share);
	MK_BUF_AND_MAP(dev, "share", size, UNIFORM_BUFFER, &buf, (void**)data);
	(*data)->vp = m4_id();
	return buf;
}

static struct AkBuffer prep_text(VkDevice dev, struct RawChar **data)
{
	struct AkBuffer buf;
	size_t size = SWAP_IMG_COUNT * MAX_CHAR * sizeof(struct RawChar);
	MK_BUF_AND_MAP(dev, "char", size, UNIFORM_BUFFER, &buf, (void**)data);
	memset(*data, 0, size);
	return buf;
}

static struct DescData mk_desc_sets(VkDevice dev)
{
	VkResult err;
	#define POOL_SIZE_COUNT 3
	VkDescriptorPoolSize pool_sizes[POOL_SIZE_COUNT] = {
		{
			.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
		}, {
			.type = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
		}, {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 2 * SWAP_IMG_COUNT,
		}
	};

	// Two UBOs per independent swap chain image
	size_t set_count = 1 + 2 * SWAP_IMG_COUNT;
	VkDescriptorPoolCreateInfo desc_pool_create_info = {
	STYPE(DESCRIPTOR_POOL_CREATE_INFO)
		.flags = 0,
		.maxSets = set_count,
		.poolSizeCount = POOL_SIZE_COUNT,
		.pPoolSizes = pool_sizes,
		.pNext = NULL,
	};
	#undef POOL_SIZE_COUNT

	VkDescriptorPool pool;
	err = vkCreateDescriptorPool(dev, &desc_pool_create_info, NULL, &pool);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create descriptor pool");
	}

	printf("Created descriptor pool\n");

	VkDescriptorSetLayoutBinding bindings[3] = {
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
		}, {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}
	};

	VkDescriptorSetLayout *layouts = malloc(
		set_count * sizeof(VkDescriptorSetLayout)
	);

	MK_SET_LAYOUT(dev, "font", bindings + 0, 2, layouts + 0);
	MK_SET_LAYOUT(dev, "vert", bindings + 2, 1, layouts + 1);
	for (size_t i = 1; i < set_count - 1; ++i)
		layouts[i + 1] = layouts[i];

	VkDescriptorSetAllocateInfo desc_alloc_info = {
	STYPE(DESCRIPTOR_SET_ALLOCATE_INFO)
		.descriptorPool = pool,
		.descriptorSetCount = set_count,
		.pSetLayouts = layouts,
		.pNext = NULL,
	};

	VkDescriptorSet *sets = malloc(set_count * sizeof(VkDescriptorSet));
	err = vkAllocateDescriptorSets(dev, &desc_alloc_info, sets);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate descriptor sets");
	}

	printf("Backed descriptor sets\n");

	return (struct DescData) {
		layouts,
		sets,
		set_count,
		pool,
	};
}

static void mk_bindings(
	VkDevice dev,
	struct DescData desc,
	struct FontData font,
	struct AkBuffer share,
	struct AkBuffer text
) {
	VkDescriptorImageInfo img_info = {
		.sampler = font.sampler,
		.imageView = font.view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkWriteDescriptorSet tex = {
	STYPE(WRITE_DESCRIPTOR_SET)
		.dstSet = desc.sets[0],
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		.pImageInfo = &img_info,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL,
		.pNext = NULL,
	};

	VkWriteDescriptorSet sampler = {
	STYPE(WRITE_DESCRIPTOR_SET)
		.dstSet = desc.sets[0],
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
		.pImageInfo = &img_info,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL,
		.pNext = NULL,
	};

	size_t write_count = 2 + 2 * SWAP_IMG_COUNT;
	VkWriteDescriptorSet writes[write_count];
	writes[0] = tex;
	writes[1] = sampler;

	VkDescriptorBufferInfo buf_infos[2 * SWAP_IMG_COUNT];
	size_t range = share.size / SWAP_IMG_COUNT;

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		buf_infos[i] = (VkDescriptorBufferInfo) {
			.buffer = share.buf,
			.offset = i * range,
			.range = range,
		};

		writes[2 + i] = (VkWriteDescriptorSet) {
		STYPE(WRITE_DESCRIPTOR_SET)
			.dstSet = desc.sets[1 + i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = NULL,
			.pBufferInfo = buf_infos + i,
			.pTexelBufferView = NULL,
			.pNext = NULL,
		};
	}

	range = text.size / SWAP_IMG_COUNT;
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		buf_infos[SWAP_IMG_COUNT + i] = (VkDescriptorBufferInfo) {
			.buffer = text.buf,
			.offset = i * range,
			.range = range,
		};

		writes[2 + SWAP_IMG_COUNT + i] = (VkWriteDescriptorSet) {
		STYPE(WRITE_DESCRIPTOR_SET)
			.dstSet = desc.sets[1 + SWAP_IMG_COUNT + i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = NULL,
			.pBufferInfo = buf_infos + SWAP_IMG_COUNT + i,
			.pTexelBufferView = NULL,
			.pNext = NULL,
		};
	}

	vkUpdateDescriptorSets(dev, write_count, writes, 0, NULL);
	printf("Updated descriptor sets (%lu writes)\n", write_count);
}

static struct GraphicsData mk_graphics(
	VkDevice dev,
	struct SwapData swap,
	struct DescData desc
) {
	/* Shader modules */

	VkResult err;
	size_t spv_size;
	uint32_t *spv;
	strncpy(filename, "vert.spv", 8 + 1);
	read_shader(root_path, &spv, &spv_size);

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

	strncpy(filename, "frag.spv", 8 + 1);
	read_shader(root_path, &spv, &spv_size);
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

	VkPipelineColorBlendAttachmentState blend_attach = {
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

	VkPipelineColorBlendStateCreateInfo blend_state_create_info = {
	STYPE(PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = 0,
		.attachmentCount = 1,
		.pAttachments = &blend_attach,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f },
		.pNext = NULL,
	};

	VkPipelineLayoutCreateInfo pipe_layout_create_info = {
	STYPE(PIPELINE_LAYOUT_CREATE_INFO)
		.flags = 0,
		.setLayoutCount = desc.count,
		.pSetLayouts = desc.layouts,
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
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
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
		.pColorBlendState = &blend_state_create_info,
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

	VkImageView *views = malloc(
		sizeof(VkImageView) * SWAP_IMG_COUNT
	);

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

	printf("Created %u image views\n", SWAP_IMG_COUNT);

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

	printf("Created %u framebuffers\n", SWAP_IMG_COUNT);
	return (struct GraphicsData) {
		vert_mod,
		frag_mod,
		pass,
		views,
		fbuffers,
		null_pipe_layout,
		pipeline,
	};
}

static VkCommandBuffer *record_graphics(
	VkDevice dev,
	struct SwapData swap,
	VkDescriptorSet *sets,
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

	VkClearValue clear = { 0, 0, 0, 1 };

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
			.clearValueCount = 1,
			.pClearValues = &clear,
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

		VkDescriptorSet frame_sets[3] = {
			sets[0],
			sets[1 + i],
			sets[1 + SWAP_IMG_COUNT + i],
		};

		vkCmdBindDescriptorSets(
			cmd[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphics.layout,
			0,
			3,
			frame_sets,
			0,
			NULL
		);

		vkCmdDraw(cmd[i], 4, MAX_CHAR, 0, 0); // Quad
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

static struct SyncData mk_sync(VkDevice dev)
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
		"Created acquisition and submit fences (%u)\n",
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

	printf("Created %u semaphores\n", SWAP_IMG_COUNT);
	return (struct SyncData) {
		acq_fence,
		sub_fence,
		sem,
	};
}

static int done;
static void run(
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

	// TODO: review loop ordering

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
		inp_update(win);
		*(share_buf + img_i) = txtquad_update(data, &text);
		text_update(img_i, data);

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

static void app_free()
{
	vkDestroyFence(app.dev.log, app.sync.acquire, NULL);
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyFence(app.dev.log, app.sync.submit[i], NULL);
		vkDestroySemaphore(app.dev.log, app.sync.sem[i], NULL);
	}
	free(app.sync.submit);
	free(app.sync.sem);

	vkDestroyShaderModule(app.dev.log, app.graphics.vert, NULL);
	vkDestroyShaderModule(app.dev.log, app.graphics.frag, NULL);
	vkDestroyRenderPass(app.dev.log, app.graphics.pass, NULL);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyImageView(
			app.dev.log,
			app.graphics.views[i],
			NULL
		);

		vkDestroyFramebuffer(
			app.dev.log,
			app.graphics.fbuffers[i],
			NULL
		);
	}
	free(app.graphics.views);
	free(app.graphics.fbuffers);

	vkDestroyPipelineLayout(app.dev.log, app.graphics.layout, NULL);
	vkDestroyPipeline(app.dev.log, app.graphics.pipeline, NULL);

	for (size_t i = 0; i < 2; ++i) {
		vkDestroyDescriptorSetLayout(
			app.dev.log,
			app.desc.layouts[i],
			NULL
		);
	}
	free(app.desc.layouts);
	free(app.desc.sets);

	vkDestroyDescriptorPool(app.dev.log, app.desc.pool, NULL);

	free_buf(app.dev.log, app.text);
	free_buf(app.dev.log, app.share);

	// Font
	vkDestroyImage(app.dev.log, app.font.img, NULL);
	vkDestroyImageView(app.dev.log, app.font.view, NULL);
	vkFreeMemory(app.dev.log, app.font.img_mem, NULL);
	vkDestroySampler(app.dev.log, app.font.sampler, NULL);

	vkFreeCommandBuffers(app.dev.log, app.pool, SWAP_IMG_COUNT, app.cmd);
	free(app.cmd);
	vkDestroyCommandPool(app.dev.log, app.pool, NULL);

	vkDestroySwapchainKHR(app.dev.log, app.swap.chain, NULL);
	free(app.swap.img);

	vkDestroyDevice(app.dev.log, NULL);
	vkDestroySurfaceKHR(app.inst, app.surf, NULL);
	vkDestroyInstance(app.inst, NULL);

	glfwDestroyWindow(app.win);
	glfwTerminate();

	printf("Cleanup complete\n");
}

void txtquad_init(const char *asset_path)
{
	size_t len = strlen(asset_path);
	root_path = malloc(len + 32);
	strncpy(root_path, asset_path, len + 1);
	filename = root_path + len;

	app.win = mk_win();
	app.inst = mk_inst(app.win);
	app.surf = mk_surf(app.win, app.inst);
	app.dev = mk_dev(app.inst, app.surf);
	app.swap = mk_swap(app.dev, app.surf);
	app.pool = mk_pool(app.dev);
	app.font = load_font(app.dev, app.pool);
	app.share = prep_share(app.dev.log, &share_buf);
	app.text = prep_text(app.dev.log, &char_buf);
	app.desc = mk_desc_sets(app.dev.log);

	mk_bindings(
		app.dev.log,
		app.desc,
		app.font,
		app.share,
		app.text
	);

	app.graphics = mk_graphics(app.dev.log, app.swap, app.desc);
	app.cmd = record_graphics(
		app.dev.log,
		app.swap,
		app.desc.sets,
		app.graphics,
		app.pool
	);

	app.sync = mk_sync(app.dev.log);
	printf("Init success\n");
}

void txtquad_start()
{
	run(app.win, app.dev, app.swap.chain, app.cmd, app.sync);
	free(root_path);
	app_free();
	printf("Exit success\n");
}
