#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <errno.h>

#include <vulkan/vulkan.h> // Must include before GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "txtquad.h"
#include "vkext.h"

#if defined(INP_KEYS) || defined(INP_TEXT)
#include "inp.h"
#endif

#define ENG_NAME "txtquad"
#define ASSET_PATH_DEFAULT "./assets/"

#define FONT_SIZE (FONT_WIDTH * FONT_WIDTH)
#define FONT_OFF  (FONT_WIDTH / CHAR_WIDTH)

#if defined __APPLE__
#define PLATFORM_COMPAT_VBO
#endif

static struct txt_buf txt;
static char *root_path;
static char *filename;

struct raw_char {
	m4 trs;
	v4 col;
	v2 off;
	v2 _slop;
};

static ALG_INLINE v2 char_off(u8 c)
{
	return (v2) {
		c % FONT_OFF,
		c / FONT_OFF,
	};
}

static void txt_update(struct raw_char *buf)
{
	for (size_t i = 0; i < txt.count; ++i) {
		struct txt_quad *quad = txt.quads + i;
		buf[i] = (struct raw_char) {
			  .trs = quad->model,
			  .col = quad->color,
			  .off = char_off(quad->value),
			._slop = quad->_extra,
		};
	}

	size_t end = txt.count;
	size_t clear_size = (MAX_QUAD - end) * sizeof(struct raw_char);
	memset(buf + end, 0, clear_size);
}

struct pipeline_template {
	VkPipelineShaderStageCreateInfo shader_create_infos[2];
#ifdef PLATFORM_COMPAT_VBO
	VkVertexInputBindingDescription binding_desc;
	VkVertexInputAttributeDescription attr_desc[2];
	VkPipelineVertexInputStateCreateInfo compat_vert_state_create_info;
#else
	VkPipelineVertexInputStateCreateInfo null_vert_state_create_info;
#endif
	VkPipelineInputAssemblyStateCreateInfo asm_state_create_info;
	VkPipelineRasterizationStateCreateInfo raster_state_create_info;
	VkPipelineMultisampleStateCreateInfo multi_state_create_info;
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
	VkPipelineColorBlendAttachmentState blend_attach;
	VkPipelineColorBlendStateCreateInfo blend_state_create_info;
	VkGraphicsPipelineCreateInfo data;
};

static struct {
	GLFWwindow *win;
	VkInstance inst;
	VkSurfaceKHR surf;
	struct dev {
		VkPhysicalDevice *devices;
		VkPhysicalDevice hard;
		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceMemoryProperties props_mem;
		VkPhysicalDeviceFeatures feats;
		VkDevice log;
		size_t q_ind;
		VkQueue q;
		VkSampleCountFlagBits sample_n;
	} dev;
	struct swap {
		VkSwapchainKHR chain;
		VkFormat format;
		struct extent extent;
		VkImage *img;
		struct ak_img aa;
		struct ak_img depth;
	} swap;
	VkCommandPool pool;
	struct font {
		struct ak_img tex;
		VkSampler sampler;
	} font;
	struct buf {
		struct ak_buf gpu;
		void *mapped;
		u64 align;
		u64 frame_size;
	} share, rchar;
	struct desc {
		VkDescriptorSetLayout *layouts;
		u32 lay_count;
		VkDescriptorSetLayout *layouts_exp;
		VkDescriptorSet *sets;
		u32 set_count;
		VkDescriptorPool pool;
	} desc;
	struct graphics {
		struct ak_shader vert;
		struct ak_shader frag;
#ifdef PLATFORM_COMPAT_VBO
		struct ak_buf quad;
#endif
		VkRenderPass pass;
		struct pipeline_template *template;
	} graphics;
	struct pipeline {
		VkPipelineLayout layout;
		VkPipeline line;
	} pipe;
	struct frame {
		VkImageView *views;
		VkFramebuffer *buffers;
	} frame;
	v3 clear_col;
	VkCommandBuffer *cmd;
	struct sync {
		VkFence acquire;
		VkFence *submit;
		VkSemaphore *sem;
	} sync;
} app;

#ifdef INP_KEYS
struct Input inp_data;

void inp_key_init(const int *key_handles, size_t key_count)
{
	inp_data.key.handles = key_handles;
	inp_data.key.count = key_count;
}

void inp_btn_init(const int *btn_handles, size_t btn_count)
{
	inp_data.btn.handles = btn_handles;
	inp_data.btn.count = btn_count;
}

void inp_pad_init(const int *pad_handles, size_t pad_count)
{
	inp_data.pad.handles = pad_handles;
	inp_data.pad.count = pad_count;
}
#endif

#ifdef INP_TEXT
static void glfw_char_callback(GLFWwindow *win, unsigned int unicode)
{
	inp_ev_text(unicode);
}
#endif

static GLFWwindow *mk_win(
	const char *name,
	int type,
	struct extent *extent,
	int resizable,
	int cursor
) {
	if (!glfwInit()) {
		panic_msg("unable to initialize GLFW");
	}

	printf("Initialized GLFW\n");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // GLFW Vulkan support
	glfwWindowHint(GLFW_RESIZABLE, resizable);

	int mon_count;
	GLFWmonitor **mons = glfwGetMonitors(&mon_count);

	if (NULL == mons) {
		panic_msg("no monitors found");
	}

	printf("Found %d monitor(s)\n", mon_count);

	int mon_ind = 0;
	GLFWmonitor *mon = *(mons + mon_ind);
	const GLFWvidmode *mode = glfwGetVideoMode(mon);
	const char *mon_name = glfwGetMonitorName(mon);
	printf("\tUsing \"%s\"\n", mon_name);

	int modes_len;
	const GLFWvidmode *modes = glfwGetVideoModes(mon, &modes_len);
	assert(modes_len);

	GLFWvidmode native = modes[modes_len - 1];
	printf("Found %d display modes:\n", modes_len);

	int exists = 0;
	for (int i = 0; i < modes_len; ++i) {
		GLFWvidmode m = modes[i];
		int active = m.width == mode->width
			&& m.height == mode->height
			&& m.refreshRate == mode->refreshRate;
		exists |= active;
		printf(
			"\t%s%5d x %5d (%.2f) @%4dHz\n",
			active ? "* " : "  ",
			m.width,
			m.height,
			m.width / (float)m.height,
			m.refreshRate
		);
	}

	// Render at monitor resolution if requested
	if (0 == *(u32*)extent) {
		if (exists) {
			extent->w = mode->width;
			extent->h = mode->height;
			printf("Creating borderless window at active resolution\n");
		} else {
			// A GLFW bug on high dpi displays.
			// Will force a swapchain recreation immediately
			// on attempting to present for the first time.
			extent->w = native.width;
			extent->h = native.height;
			printf("Warning: no valid display mode in use\n");
			printf("Creating borderless window at native resolution\n");
		}
	} else if (type == MODE_FULLSCREEN) {
		// Unsupported aspect ratios will panic
		printf("Creating fullscreen window at requested resolution\n");
	} else {
		mon = NULL;
	}

	GLFWwindow *win = glfwCreateWindow(
		extent->w,
		extent->h,
		name,
		mon,
		NULL
	);

	printf(
		"Created window and requested extent %ux%u\n",
		extent->w, extent->h
	);

#ifdef INP_TEXT
	glfwSetCharCallback(win, glfw_char_callback);
#endif
	glfwSetInputMode(win, GLFW_CURSOR, cursor);

	return win;
}

static VkInstance mk_inst(const char *name)
{
	VkApplicationInfo app_info = {
	STYPE(APPLICATION_INFO)
		.pApplicationName = name,
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = ENG_NAME,
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

	u32 layer_count;
#ifdef  VALIDATION_LAYERS
#define VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define VALIDATION_LAYER_NAME_LEN 27
	vkEnumerateInstanceLayerProperties(&layer_count, NULL);
	VkLayerProperties *layers = malloc(layer_count * sizeof(VkLayerProperties));
	assert(layers);

	vkEnumerateInstanceLayerProperties(&layer_count, layers);
	int found;

	for (size_t i = 0; i < layer_count; ++i) {
		found = !strncmp(
			layers[i].layerName,
			VALIDATION_LAYER_NAME,
			VALIDATION_LAYER_NAME_LEN
		);

		if (found) break;
	}

	assert(found);
	printf("Enabled validation layers at app level\n");
	const char *layer_names[] = { VALIDATION_LAYER_NAME };
#undef VALIDATION_LAYER_NAME
#undef VALIDATION_LAYER_NAME_LEN
	layer_count = 1;
#else
	const char **layer_names = NULL;
	layer_count = 0;
#endif
	VkInstanceCreateInfo inst_create_info = {
	STYPE(INSTANCE_CREATE_INFO)
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = inst_ext_count,
		.ppEnabledExtensionNames = inst_ext_names,
		.enabledLayerCount = layer_count,
		.ppEnabledLayerNames = layer_names,
		.pNext = NULL,
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

static struct dev mk_dev(VkInstance inst, VkSurfaceKHR surf)
{
	unsigned int dev_count;
	VkPhysicalDevice *hard_devs, hard_dev;

	vkEnumeratePhysicalDevices(inst, &dev_count, NULL);
	printf("Found %u device(s)\n", dev_count);

	if (!dev_count) {
		panic_msg("no physical devices available");
	}

	hard_devs = malloc(dev_count * sizeof(VkPhysicalDevice));
	assert(hard_devs);

	vkEnumeratePhysicalDevices(inst, &dev_count, hard_devs);
	assert(GPU_IDX < dev_count);
	hard_dev = hard_devs[GPU_IDX];

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(hard_dev, &props);
	printf(
		"Using device \"%s\" (%s)\n"
		"API version %u.%u.%u\n",
		props.deviceName,
		ak_dev_type_str(props.deviceType),
		VK_VERSION_MAJOR(props.apiVersion),
		VK_VERSION_MINOR(props.apiVersion),
		VK_VERSION_PATCH(props.apiVersion)
	);

	VkSampleCountFlags sample_n =
		  props.limits.framebufferColorSampleCounts
		& props.limits.framebufferDepthSampleCounts;

	     if (sample_n & VK_SAMPLE_COUNT_4_BIT)
	         sample_n = VK_SAMPLE_COUNT_4_BIT;
	else if (sample_n & VK_SAMPLE_COUNT_2_BIT)
	         sample_n = VK_SAMPLE_COUNT_2_BIT;
	else     sample_n = VK_SAMPLE_COUNT_1_BIT;

	VkPhysicalDeviceFeatures feats;
	vkGetPhysicalDeviceFeatures(hard_dev, &feats);

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

	printf("Found queue [%zu]\n", q_ind);

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
		.pEnabledFeatures = &feats,
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
	printf("Acquired queue [%zu]\n", q_ind);

	VkPhysicalDeviceMemoryProperties props_mem;
	vkGetPhysicalDeviceMemoryProperties(hard_dev, &props_mem);

	for (u32 i = 0; i < props_mem.memoryTypeCount; ++i) {
		VkMemoryType t = props_mem.memoryTypes[i];
		printf("Found memory type %u:\n", i);
		VkMemoryPropertyFlags flags = t.propertyFlags;
		ak_print_props_mem(flags, "\t%s\n");
	}

	for (u32 i = 0; i < props_mem.memoryHeapCount; ++i) {
		VkMemoryHeap h = props_mem.memoryHeaps[i];
		float size = (float)h.size / (1024 * 1024);
		printf("Found heap %u with size %.1fMB\n", i, size);
	}

	return (struct dev) {
		hard_devs,
		hard_dev,
		props,
		props_mem,
		feats,
		dev,
		q_ind,
		q,
		sample_n,
	};
}

static struct swap mk_swap(
	struct extent req,
	struct extent fbuffer,
	struct dev dev,
	GLFWwindow *win,
	VkSurfaceKHR surf
) {
	u32 win_w, win_h;
	VkSurfaceCapabilitiesKHR cap;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev.hard, surf, &cap);
	{
		if (cap.currentExtent.width == UINT32_MAX) {
			win_w = fbuffer.w;
			win_h = fbuffer.h;
		} else {
			win_w = cap.currentExtent.width;
			win_h = cap.currentExtent.height;
		}

		assert(win_w * win_h > 0);
		printf("Current extent: %ux%u\n", win_w, win_h);
	}

	if (win_w != req.w || win_h != req.h) {
		printf(
			"Warning: "
			"current extent does not match what was requested\n"
		);
	/* We could throw if resizable is false,
	 * or we could just continue on as I have done here;
	 * the behavior is mostly only relevant for tiling window managers.
		float asp_cur = win_w / (float)win_h;
		float asp_req = req.w / (float)req.h;
		assert(asp_cur == asp_req);
	 */
	}

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
		.imageExtent = { win_w, win_h },
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

	struct ak_img aa;
	AK_IMG_MK(
		dev.log,
		dev.props_mem,
		"aa buffer",
		win_w, win_h, dev.sample_n,
		format,
		  AK_IMG_USAGE(TRANSIENT_ATTACHMENT)
		| AK_IMG_USAGE(COLOR_ATTACHMENT),
		COLOR,
		&aa
	);

	struct ak_img depth;
	AK_IMG_MK(
		dev.log,
		dev.props_mem,
		"depth texture",
		win_w, win_h, dev.sample_n,
		VK_FORMAT_D32_SFLOAT,
		AK_IMG_USAGE(DEPTH_STENCIL_ATTACHMENT),
		DEPTH,
		&depth
	);

	return (struct swap) {
		swapchain,
		format,
		{ win_w, win_h },
		img,
		aa,
		depth,
	};
}

static VkCommandPool mk_pool(struct dev dev)
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
	FILE *file = fopen(root_path, "rb");
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
	assert(raw);
	assert(exp);

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

static struct font load_font(struct dev dev, VkCommandPool pool)
{
	/* Staging buffer */

	VkResult err;
	struct ak_buf staging;
	void *src;

	AK_BUF_MK_AND_MAP(
		dev.log,
		dev.props_mem,
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

	struct ak_img tex;
	AK_IMG_MK(
		dev.log,
		dev.props_mem,
		"font texture",
		128, 128, 0,
		VK_FORMAT_R8_UNORM,
		AK_IMG_USAGE(SAMPLED) | AK_IMG_USAGE(TRANSFER_DST),
		COLOR,
		&tex
	);

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
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = tex.img,
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
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
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
		tex.img,
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
		.image = tex.img,
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
		0,
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
	ak_buf_free(dev.log, staging);

	return (struct font) {
		tex,
		sampler,
	};
}

static void prep_share(struct dev dev, struct buf *out)
{
	/* Could combine with the rchar buffer
	 * as they are updated at the same rate;
	 * however, this structure may
	 * become larger in the future
	 */

	struct ak_buf buf;
	u64 align = dev.props.limits.minUniformBufferOffsetAlignment;
	u64 frame_size = ak_align_up(sizeof(struct txt_share), align);
	u64 size = frame_size * SWAP_IMG_COUNT;

	AK_BUF_MK_AND_MAP(
		dev.log,
		dev.props_mem,
		"share",
		size,
		UNIFORM_BUFFER,
		&buf,
		&out->mapped
	);

	out->gpu = buf;
	memset(out->mapped, 0, sizeof(struct txt_share));
	((struct txt_share*)out->mapped)->vp = M4_ID;
	out->align = align;
	out->frame_size = frame_size;
}

static void prep_rchar(struct dev dev, struct buf *out)
{
	struct ak_buf buf;
	u64 align = dev.props.limits.minStorageBufferOffsetAlignment;
	u64 frame_size = ak_align_up(MAX_QUAD * sizeof(struct raw_char), align);
	u64 size = frame_size * SWAP_IMG_COUNT;

	AK_BUF_MK_AND_MAP(
		dev.log,
		dev.props_mem,
		"char",
		size,
		STORAGE_BUFFER,
		&buf,
		&out->mapped
	);

	out->gpu = buf;
	memset(out->mapped, 0, size);
	out->align = align;
	out->frame_size = frame_size;
}

static struct desc mk_desc_sets(VkDevice dev)
{
	VkResult err;

	// One UBO/SSBO per independent swap chain image
	u32 set_count = 1 + 2 * SWAP_IMG_COUNT;
	u32 lay_count = 3;

	/* Pool */

	#define POOL_SIZE_COUNT 4
	VkDescriptorPoolSize pool_sizes[POOL_SIZE_COUNT] = {
		{
			.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
		}, {
			.type = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
		}, {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = SWAP_IMG_COUNT,
		}, {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = SWAP_IMG_COUNT,
		}
	};

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

	/* Bindings */

	VkDescriptorSetLayoutBinding bindings[4] = {
		{ // Set 0 //
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
		},

		{ // Set 1 //
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
			            | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},

		{ // Set 2 //
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}
	};

	/* Sets */

	VkDescriptorSetLayout *layouts;
	layouts = malloc(lay_count * sizeof(VkDescriptorSetLayout));
	assert(layouts);

	AK_MK_SET_LAYOUT(dev, "font",  bindings + 0, 2, layouts + 0);
	AK_MK_SET_LAYOUT(dev, "share", bindings + 2, 1, layouts + 1);
	AK_MK_SET_LAYOUT(dev, "text",  bindings + 3, 1, layouts + 2);

	VkDescriptorSetLayout *layouts_exp; // Expand layouts for the alloc call
	layouts_exp = malloc(set_count * sizeof(VkDescriptorSetLayout));
	assert(layouts_exp);
	layouts_exp[0] = layouts[0];

	layouts_exp[1] = layouts[1];
	for (size_t i = 1; i < SWAP_IMG_COUNT; ++i)
		layouts_exp[i + 1] = layouts_exp[i];

	layouts_exp[1 + SWAP_IMG_COUNT] = layouts[2];
	for (size_t i = 1 + SWAP_IMG_COUNT; i < set_count - 1; ++i)
		layouts_exp[i + 1] = layouts_exp[i];

	VkDescriptorSetAllocateInfo desc_alloc_info = {
	STYPE(DESCRIPTOR_SET_ALLOCATE_INFO)
		.descriptorPool = pool,
		.descriptorSetCount = set_count,
		.pSetLayouts = layouts_exp,
		.pNext = NULL,
	};

	VkDescriptorSet *sets = malloc(set_count * sizeof(VkDescriptorSet));
	assert(sets);

	err = vkAllocateDescriptorSets(dev, &desc_alloc_info, sets);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate descriptor sets");
	}

	printf("Backed descriptor sets\n");

	return (struct desc) {
		layouts,
		lay_count,
		layouts_exp,
		sets,
		set_count,
		pool,
	};
}

static void mk_bindings(
	VkDevice dev,
	struct desc desc,
	struct font font,
	struct buf share,
	struct buf rchar
) {
	size_t write_count = 2 + 2 * SWAP_IMG_COUNT;
	VkWriteDescriptorSet writes[write_count];

	VkDescriptorImageInfo img_info = {
		.sampler = font.sampler,
		.imageView = font.tex.view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	writes[0] = (VkWriteDescriptorSet) {
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

	writes[1] = (VkWriteDescriptorSet) {
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

	VkDescriptorBufferInfo buf_infos[2 * SWAP_IMG_COUNT];
	size_t range = share.gpu.size / SWAP_IMG_COUNT;

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		buf_infos[i] = (VkDescriptorBufferInfo) {
			.buffer = share.gpu.buf,
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

	range = rchar.gpu.size / SWAP_IMG_COUNT;
	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		buf_infos[SWAP_IMG_COUNT + i] = (VkDescriptorBufferInfo) {
			.buffer = rchar.gpu.buf,
			.offset = i * range,
			.range = range,
		};

		writes[2 + SWAP_IMG_COUNT + i] = (VkWriteDescriptorSet) {
		STYPE(WRITE_DESCRIPTOR_SET)
			.dstSet = desc.sets[1 + SWAP_IMG_COUNT + i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = NULL,
			.pBufferInfo = buf_infos + SWAP_IMG_COUNT + i,
			.pTexelBufferView = NULL,
			.pNext = NULL,
		};
	}

	vkUpdateDescriptorSets(dev, write_count, writes, 0, NULL);
	printf("Updated descriptor sets (%zu writes)\n", write_count);
}

static struct graphics mk_graphics(
	struct dev dev,
	struct swap swap
) {
	VkResult err;

	struct pipeline_template *template = malloc(
		sizeof(struct pipeline_template)
	);

	assert(template);

	/* Shader modules */

#ifdef PLATFORM_COMPAT_VBO
	strncpy(filename, "vert_compat.spv", 15 + 1);
#else
	strncpy(filename, "vert.spv", 8 + 1);
#endif
	struct ak_shader vert = ak_shader_mk(dev.log, root_path);

	strncpy(filename, "frag.spv", 8 + 1);
	struct ak_shader frag = ak_shader_mk(dev.log, root_path);

	printf("Created shader modules (2)\n");

	template->shader_create_infos[0] = (VkPipelineShaderStageCreateInfo) {
	STYPE(PIPELINE_SHADER_STAGE_CREATE_INFO)
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert.mod,
		.pName = "main",
		.pSpecializationInfo = NULL,
		.pNext = NULL,
	};

	template->shader_create_infos[1] = (VkPipelineShaderStageCreateInfo) {
	STYPE(PIPELINE_SHADER_STAGE_CREATE_INFO)
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag.mod,
		.pName = "main",
		.pSpecializationInfo = NULL,
		.pNext = NULL,
	};

#ifdef PLATFORM_COMPAT_VBO
	struct ak_buf quad;
	{
		float *verts;
		AK_BUF_MK_AND_MAP(
			dev.log,
			dev.props_mem,
			"compat vertex",
			4 * sizeof(float) * 4,
			VERTEX_BUFFER,
			&quad,
			(void**)&verts
		);

		float raw[] = {
			0.f - PADDING, 1.f + PADDING, MIN_BIAS - PADDING, MIN_BIAS - PADDING,
			1.f + PADDING, 1.f + PADDING, MAX_BIAS + PADDING, MIN_BIAS - PADDING,
			0.f - PADDING, 0.f - PADDING, MIN_BIAS - PADDING, MAX_BIAS + PADDING,
			1.f + PADDING, 0.f - PADDING, MAX_BIAS + PADDING, MAX_BIAS + PADDING,
		};

		memcpy(verts, raw, 4 * sizeof(float) * 4);
	}

	template->binding_desc = (VkVertexInputBindingDescription) {
		.binding = 0,
		.stride = sizeof(float) * 4,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	template->attr_desc[0] = (VkVertexInputAttributeDescription) {
		.binding = 0,
		.location = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = 0,
	};

	template->attr_desc[1] = (VkVertexInputAttributeDescription) {
		.binding = 0,
		.location = 1,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = sizeof(float) * 2,
	};

	template->compat_vert_state_create_info
	= (VkPipelineVertexInputStateCreateInfo) {
	STYPE(PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &template->binding_desc,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = template->attr_desc,
		.pNext = NULL,
	};
#else
	template->null_vert_state_create_info
	= (VkPipelineVertexInputStateCreateInfo) {
	STYPE(PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO)
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL,
		.pNext = NULL,
	};
#endif

	template->asm_state_create_info
	= (VkPipelineInputAssemblyStateCreateInfo) {
	STYPE(PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO)
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // Quads
		.primitiveRestartEnable = VK_FALSE,
		.pNext = NULL,
	};

	template->raster_state_create_info
	= (VkPipelineRasterizationStateCreateInfo) {
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

	template->multi_state_create_info
	= (VkPipelineMultisampleStateCreateInfo) {
	STYPE(PIPELINE_MULTISAMPLE_STATE_CREATE_INFO)
		.flags = 0,
		.rasterizationSamples = dev.sample_n,
		.sampleShadingEnable = dev.feats.sampleRateShading,
		.minSampleShading = 1.f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
		.pNext = NULL,
	};

	template->depth_stencil_state_create_info
	= (VkPipelineDepthStencilStateCreateInfo) {
	STYPE(PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO)
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_GREATER,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = { },
		.back = { },
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f,
		.pNext = NULL,
	};

	template->blend_attach = (VkPipelineColorBlendAttachmentState) {
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

	template->blend_state_create_info
	= (VkPipelineColorBlendStateCreateInfo) {
	STYPE(PIPELINE_COLOR_BLEND_STATE_CREATE_INFO)
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = 0,
		.attachmentCount = 1,
		.pAttachments = &template->blend_attach,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f },
		.pNext = NULL,
	};

	VkAttachmentDescription attach_col = {
		.format = swap.format,
		.samples = dev.sample_n,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription attach_depth = {
		.format = VK_FORMAT_D32_SFLOAT,
		.samples = dev.sample_n,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentDescription attach_resolve = {
		.format = swap.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentReference attach_col_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference attach_depth_ref = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference attach_resolve_ref = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attach_col_ref,
		.pResolveAttachments = &attach_resolve_ref,
		.pDepthStencilAttachment = &attach_depth_ref,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};

	VkAttachmentDescription attach[] = {
		attach_col,
		attach_depth,
		attach_resolve,
	};

	VkRenderPassCreateInfo pass_create_info = {
	STYPE(RENDER_PASS_CREATE_INFO)
		.flags = 0,
		.attachmentCount = 3,
		.pAttachments = attach,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0,
		.pDependencies = NULL,
		.pNext = NULL,
	};

	VkRenderPass pass;
	err = vkCreateRenderPass(dev.log, &pass_create_info, NULL, &pass);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create post-processing render pass");
	}

	printf("Created render pass\n");

	template->data = (VkGraphicsPipelineCreateInfo) {
	STYPE(GRAPHICS_PIPELINE_CREATE_INFO)
		.flags = 0,
		.stageCount = 2,
		.pStages = template->shader_create_infos,
#ifdef PLATFORM_COMPAT_VBO
		.pVertexInputState = &template->compat_vert_state_create_info,
#else
		.pVertexInputState = &template->null_vert_state_create_info,
#endif
		.pInputAssemblyState = &template->asm_state_create_info,
		.pTessellationState = NULL,
		/* .pViewportState */
		.pRasterizationState = &template->raster_state_create_info,
		.pMultisampleState = &template->multi_state_create_info,
		.pDepthStencilState = &template->depth_stencil_state_create_info,
		.pColorBlendState = &template->blend_state_create_info,
		.pDynamicState = NULL,
		/* .layout */
		.renderPass = pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
		.pNext = NULL,
	};

	printf("Created pipeline template\n");

	return (struct graphics) {
		vert,
		frag,
#ifdef PLATFORM_COMPAT_VBO
		quad,
#endif
		pass,
		template,
	};
}

static struct pipeline mk_pipe(
	VkDevice dev,
	struct extent extent,
	struct desc desc,
	struct pipeline_template *template
) {
	VkResult err;

	VkViewport viewport = {
		.x = 0.f,
		.y = 0.f,
		.width = extent.w,
		.height = extent.h,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = { extent.w, extent.h },
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

	VkPipelineLayoutCreateInfo pipe_layout_create_info = {
	STYPE(PIPELINE_LAYOUT_CREATE_INFO)
		.flags = 0,
		.setLayoutCount = desc.lay_count,
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

	// Fill template
	template->data.pViewportState = &viewport_state_create_info;
	template->data.layout = null_pipe_layout;

	VkPipeline pipeline;
	err = vkCreateGraphicsPipelines(
		dev,
		VK_NULL_HANDLE,
		1,
		&template->data,
		NULL,
		&pipeline
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create graphics pipeline");
	}

	printf("Created graphics pipeline\n");
	return (struct pipeline) {
		null_pipe_layout,
		pipeline,
	};
}

static struct frame mk_fbuffers(
	VkDevice dev,
	struct swap swap,
	VkRenderPass pass
) {
	VkResult err;

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

	VkImageView *views = malloc(3 * sizeof(VkImageView) * SWAP_IMG_COUNT);
	assert(views);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		views[3 * i + 0] = swap.aa.view;
		views[3 * i + 1] = swap.depth.view;

		view_create_info.image = swap.img[i];
		err = vkCreateImageView(
			dev,
			&view_create_info,
			NULL,
			&views[3 * i + 2]
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
		.attachmentCount = 3,
		.width = swap.extent.w,
		.height = swap.extent.h,
		.layers = 1,
		.pNext = NULL,
	};

	VkFramebuffer *fbuffers = malloc(
		sizeof(VkFramebuffer) * SWAP_IMG_COUNT
	);

	assert(fbuffers);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		fbuffer_create_info.pAttachments = views + 3 * i;
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
	return (struct frame) {
		views,
		fbuffers,
	};
}

static VkCommandBuffer *record_graphics(
	VkDevice dev,
	struct swap swap,
	VkDescriptorSet *sets,
	struct graphics graphics,
	struct pipeline pipe,
	struct frame frame,
	VkCommandPool pool,
	v3 clear_col
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

	assert(cmd);

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

	VkClearValue clears[] = {
		{ clear_col.x, clear_col.y, clear_col.z, 1 },
		{ 0, 0 },
	};

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		err = vkBeginCommandBuffer(cmd[i], &begin_info);
		if (err != VK_SUCCESS) {
			panic_msg("unable to begin command buffer recording");
		}

		VkRenderPassBeginInfo pass_beg_info = {
		STYPE(RENDER_PASS_BEGIN_INFO)
			.renderPass = graphics.pass,
			.framebuffer = frame.buffers[i],
			.renderArea = {
				.offset = { 0, 0 },
				.extent = { swap.extent.w, swap.extent.h },
			},
			.clearValueCount = 2,
			.pClearValues = clears,
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
			pipe.line
		);

#ifdef PLATFORM_COMPAT_VBO
		VkDeviceSize off = 0;
		vkCmdBindVertexBuffers(cmd[i], 0, 1, &graphics.quad.buf, &off);
#endif
		VkDescriptorSet frame_sets[3] = {
			sets[0],
			sets[1 + i],
			sets[1 + SWAP_IMG_COUNT + i],
		};

		vkCmdBindDescriptorSets(
			cmd[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipe.layout,
			0,
			3,
			frame_sets,
			0,
			NULL
		);

		vkCmdDraw(cmd[i], 4, MAX_QUAD, 0, 0); // Quad
		vkCmdEndRenderPass(cmd[i]);

		err = vkEndCommandBuffer(cmd[i]);
		if (err != VK_SUCCESS) {
			panic_msg("unable to end command buffer recording");
		}

		printf("Recorded command buffer [%zu]\n", i);
	}

	return cmd;
}

static struct sync mk_sync(VkDevice dev)
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

	fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VkFence *sub_fence = malloc(sizeof(VkFence) * SWAP_IMG_COUNT);
	assert(sub_fence);

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
	assert(sem);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		err = vkCreateSemaphore(dev, &sem_create_info, NULL, &sem[i]);
		if (err != VK_SUCCESS) {
			panic_msg("unable to create semaphore");
		}
	}

	printf("Created %u semaphores\n", SWAP_IMG_COUNT);
	return (struct sync) {
		acq_fence,
		sub_fence,
		sem,
	};
}

static void swap_free(
	VkDevice dev,
	struct swap swap,
	struct pipeline pipe,
	struct frame frame,
	VkCommandPool pool,
	VkCommandBuffer *cmd
) {
	vkFreeCommandBuffers(dev, pool, SWAP_IMG_COUNT, cmd);
	free(cmd);

	for (size_t i = 0; i < SWAP_IMG_COUNT; ++i) {
		vkDestroyImageView(dev, frame.views[3 * i + 2], NULL);
		vkDestroyFramebuffer(dev, frame.buffers[i], NULL);
	}

	free(frame.views);
	free(frame.buffers);

	vkDestroyPipelineLayout(dev, pipe.layout, NULL);
	vkDestroyPipeline(dev, pipe.line, NULL);

	vkDestroySwapchainKHR(dev, swap.chain, NULL);
	free(swap.img);
	ak_img_free(dev, swap.aa);
	ak_img_free(dev, swap.depth);
}

struct reswap_data {
	struct swap *swap;
	struct pipeline *pipe;
	struct frame *frame;
	VkCommandBuffer **cmd;
	v3 clear_col;
};

static int reswap(
	GLFWwindow *win,
	VkSurfaceKHR surf,
	struct dev dev,
	struct graphics graphics,
	struct desc desc,
	VkCommandPool pool,
	struct reswap_data in
) {
	int fbw, fbh;
	glfwGetFramebufferSize(win, &fbw, &fbh);

	// Block while iconified
	while (0 == fbw * fbh) {
		printf("Minimized...\n");
		glfwWaitEvents();
		if (glfwWindowShouldClose(win)) return 1;
		glfwGetFramebufferSize(win, &fbw, &fbh);
	}

	struct extent fbs = { fbw, fbh };
	printf("Recreating swapchain\n");

	swap_free(
		dev.log,
		*(in.swap),
		*(in.pipe),
		*(in.frame),
		pool,
		*(in.cmd)
	);

	*(in.swap) = mk_swap(in.swap->extent, fbs, dev, win, surf);
	*(in.pipe) = mk_pipe(dev.log, in.swap->extent, desc, graphics.template);
	*(in.frame) = mk_fbuffers(dev.log, *(in.swap), graphics.pass);
	*(in.cmd) = record_graphics(
		dev.log,
		*(in.swap),
		desc.sets,
		graphics,
		*(in.pipe),
		*(in.frame),
		pool,
		in.clear_col
	);

	return 0;
}

static int done;
static void run(
	GLFWwindow *win,
	VkSurfaceKHR surf,
	struct dev dev,
	struct graphics graphics,
	struct desc desc,
	VkCommandPool pool,
	struct sync sync,
	struct buf share,
	struct buf rchar,
	struct reswap_data vol
) {
	printf("Initializing update data...\n");
	glfwSetTime(0);

	printf("Entering render loop...\n");
	unsigned int img_i;
	struct txt_frame frame = {
		.size = vol.swap->extent,
		.i = 0,
		.t = 0.f,
	};

	VkResult err;
	while (!done) {
		if (glfwWindowShouldClose(win)) break;

		err = vkAcquireNextImageKHR(
			dev.log,
			vol.swap->chain,
			UINT64_MAX,
			NULL,
			sync.acquire,
			&img_i
		);

		switch (err) {
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			printf("Swapchain unsuitable for image acquisition\n");
			vkDeviceWaitIdle(dev.log);
			done = reswap(win, surf, dev, graphics, desc, pool, vol);
			frame.size = vol.swap->extent;
			continue;
		default:
			fprintf(
				stderr,
				"Error: Unable to acquire image (%d)\n",
				err
			);

			panic();
		}

		++frame.i;
		float t = glfwGetTime();
		frame.dt = minf(t - frame.t_prev, MAX_DT);
		frame.t_prev = t;
		frame.t += frame.dt;
#ifdef TXT_DEBUG
		frame.acc += frame.dt;
		if (frame.acc > 1) {
			printf(
				"FPS=%zu\tdt=%.3fms\n",
				frame.i - frame.i_last,
				frame.dt * 1000
			);

			frame.i_last = frame.i;
			frame.acc = 0;
		}
#endif
		glfwPollEvents();
#ifdef INP_KEYS
		inp_update(win);
#endif
		void *share_buf = share.mapped + img_i * share.frame_size;
		*((struct txt_share*)share_buf) = txtquad_update(frame, &txt);
		assert(txt.count <= MAX_QUAD);

		void *rchar_buf = rchar.mapped + img_i * rchar.frame_size;
		txt_update((struct raw_char*)rchar_buf);

		VkSubmitInfo submit_info = {
		STYPE(SUBMIT_INFO)
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = NULL,
			.commandBufferCount = 1,
			.pCommandBuffers = *vol.cmd + img_i,
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
			.pSwapchains = &vol.swap->chain,
			.pImageIndices = &img_i,
			.pResults = NULL,
			.pNext = NULL,
		};

		err = vkQueuePresentKHR(dev.q, &present_info);
		switch (err) {
		case VK_SUCCESS:
			continue;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			printf("Swapchain unsuitable for presentation\n");
			vkDeviceWaitIdle(dev.log);
			done = reswap(win, surf, dev, graphics, desc, pool, vol);
			frame.size = vol.swap->extent;
			continue;
		default:
			fprintf(
				stderr,
				"Error: Unable to present (%d)\n",
				err
			);

			panic();
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

	swap_free(app.dev.log, app.swap, app.pipe, app.frame, app.pool, app.cmd);
	vkDestroyCommandPool(app.dev.log, app.pool, NULL);

	vkDestroyRenderPass(app.dev.log, app.graphics.pass, NULL);
	ak_shader_free(app.dev.log, app.graphics.vert);
	ak_shader_free(app.dev.log, app.graphics.frag);
#ifdef PLATFORM_COMPAT_VBO
	ak_buf_free(app.dev.log, app.graphics.quad);
#endif
	free(app.graphics.template);

	for (size_t i = 0; i < app.desc.lay_count; ++i) {
		vkDestroyDescriptorSetLayout(
			app.dev.log,
			app.desc.layouts[i],
			NULL
		);
	}

	free(app.desc.layouts);
	free(app.desc.layouts_exp);
	free(app.desc.sets);
	vkDestroyDescriptorPool(app.dev.log, app.desc.pool, NULL);

	ak_buf_free(app.dev.log, app.rchar.gpu);
	ak_buf_free(app.dev.log, app.share.gpu);

	// Font
	ak_img_free(app.dev.log, app.font.tex);
	vkDestroySampler(app.dev.log, app.font.sampler, NULL);

	vkDestroyDevice(app.dev.log, NULL);
	free(app.dev.devices);

	vkDestroySurfaceKHR(app.inst, app.surf, NULL);
	vkDestroyInstance(app.inst, NULL);

	glfwDestroyWindow(app.win);
	glfwTerminate();

	printf("Cleanup complete\n");
}

void txtquad_init(struct txt_cfg cfg)
{
	struct extent zero;
	memset(&zero, 0, sizeof(struct extent));

	switch (cfg.mode) {
	case MODE_WINDOWED:
	case MODE_FULLSCREEN:
		assert(cfg.win_size.w > 0);
		assert(cfg.win_size.h > 0);
		break;
	case MODE_BORDERLESS:
		cfg.win_size = zero;
		break;
	default:
		panic();
	}

	int cursor;
	switch (cfg.cursor) {
	case CURSOR_SCREEN:
		cursor = GLFW_CURSOR_HIDDEN;
		break;
	case CURSOR_INF:
		cursor = GLFW_CURSOR_DISABLED;
		break;
	default:
		panic();
	}

	const char *app_name = cfg.app_name ?: ENG_NAME;
	const char *asset_path = cfg.asset_path ?: ASSET_PATH_DEFAULT;

	size_t len = strlen(asset_path);
	root_path = malloc(len + 32);
	assert(root_path);

	strncpy(root_path, asset_path, len + 1);
	filename = root_path + len;

	app.win = mk_win(app_name, cfg.mode, &cfg.win_size, cfg.resizable, cursor);
	app.inst = mk_inst(app_name);
	app.surf = mk_surf(app.win, app.inst);
	app.dev = mk_dev(app.inst, app.surf);
	app.swap = mk_swap(cfg.win_size, zero, app.dev, app.win, app.surf);
	app.pool = mk_pool(app.dev);
	app.font = load_font(app.dev, app.pool);

	prep_share(app.dev, &app.share);
	prep_rchar(app.dev, &app.rchar);
	app.desc = mk_desc_sets(app.dev.log);

	mk_bindings(
		app.dev.log,
		app.desc,
		app.font,
		app.share,
		app.rchar
	);

	app.graphics = mk_graphics(app.dev, app.swap);
	app.pipe = mk_pipe(
		app.dev.log,
		app.swap.extent,
		app.desc,
		app.graphics.template
	);

	app.frame = mk_fbuffers(app.dev.log, app.swap, app.graphics.pass);
	app.clear_col = cfg.clear_col;
	app.cmd = record_graphics(
		app.dev.log,
		app.swap,
		app.desc.sets,
		app.graphics,
		app.pipe,
		app.frame,
		app.pool,
		app.clear_col
	);

	app.sync = mk_sync(app.dev.log);
	printf("Init success\n");
}

void txtquad_start()
{
#ifdef TXT_DEBUG
	printf(
		"Text memory usage: %.2f MB\n",
		// Note: does not include mapped memory
		(float)sizeof(struct txt_buf) / (1000 * 1000)
	);
#endif
	run(
		app.win,
		app.surf,
		app.dev,
		app.graphics,
		app.desc,
		app.pool,
		app.sync,
		app.share,
		app.rchar,
		(struct reswap_data) {
			.swap = &app.swap,
			.pipe = &app.pipe,
			.frame = &app.frame,
			.cmd = &app.cmd,
			.clear_col = app.clear_col,
		}
	);

	free(root_path);
	app_free();
	printf("Exit success\n");
}
