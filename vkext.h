#ifndef VKEXT_H
#define VKEXT_H

#include <inttypes.h>
#include <vulkan/vulkan.h>
#include "acg/sys.h"
#include "acg/types.h"

#define STYPE(NAME) .sType = VK_STRUCTURE_TYPE_ ## NAME ,
#define AK_MEM_PROP(STR) VK_MEMORY_PROPERTY_ ## STR ## _BIT

static const char *ak_mem_prop_flag_str(int flag)
{
	switch (flag) {
	case AK_MEM_PROP(DEVICE_LOCAL):
		return "device-local";
	case AK_MEM_PROP(HOST_VISIBLE):
		return "host-visible";
	case AK_MEM_PROP(HOST_COHERENT):
		return "host-coherent";
	case AK_MEM_PROP(HOST_CACHED):
		return "host-cached";
	case AK_MEM_PROP(LAZILY_ALLOCATED):
		return "lazily-allocated";
	case AK_MEM_PROP(PROTECTED):
		return "protected";
	default:
		return "unknown-flag";
	}
}

static const char *ak_dev_type_str(int e)
{
	switch (e) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return "integrated";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return "discrete";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		return "virtual";
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return "cpu";
	default:
		return "unknown-device-type";
	}
}

static void ak_print_props_mem(VkMemoryPropertyFlags prop_mask, const char *format)
{
	u32 j = 0;
	while (prop_mask) {
		if (prop_mask & 1) {
			printf(format, ak_mem_prop_flag_str(1 << j));
		}

		prop_mask >>= 1;
		++j;
	}
}

static u32 ak_mem_type_idx(
	VkPhysicalDeviceMemoryProperties props_mem,
	u32 type_mask,
	VkMemoryPropertyFlags prop_mask
) {
	printf("\t| ");
	ak_print_props_mem(prop_mask, "%s ");
	printf("\n");

	for (u32 i = 0; i < props_mem.memoryTypeCount; ++i) {
		int compat = type_mask & (1 << i);
		if (!compat) continue;

		VkMemoryType t = props_mem.memoryTypes[i];
		VkMemoryPropertyFlags flags = t.propertyFlags;
		compat = prop_mask == (flags & prop_mask);
		if (!compat) continue;

		printf("\t| using memory type %u\n", i);
		return i;
	}

	panic_msg("no compatible memory type found");
	return -1;
}

static inline u64 ak_align_up(u64 size, u64 align)
{
	--align;
	return (size + align) & ~align;
}

struct ak_img {
	VkImage img;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
	VkImageView view;
};

#define AK_IMG_USAGE(STR) VK_IMAGE_USAGE_ ## STR ## _BIT

#define AK_IMG_HEAD(HANDLE) \
	printf("Making " HANDLE " image\n")
#define AK_IMG_MK(DEV, MEM, HANDLE, W, H, S, FORMAT, USAGE, ASPECT, OUT) \
{ \
	AK_IMG_HEAD(HANDLE); \
	ak_img_mk( \
		DEV, \
		MEM, \
		W, H, S, \
		FORMAT, \
		USAGE, \
		VK_IMAGE_ASPECT_ ## ASPECT ## _BIT, \
		OUT \
	); \
}

static void ak_img_mk(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties mem_info,
	u32 width, u32 height,
	VkSampleCountFlagBits sample_n,
	VkFormat format,
	VkImageUsageFlags usage,
	VkImageAspectFlags aspect,
	struct ak_img *const out
) {
	VkResult err;
	VkImageCreateInfo img_create_info = {
	STYPE(IMAGE_CREATE_INFO)
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { width, height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = sample_n ?: VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.pNext = NULL,
	};

	VkImage img;
	err = vkCreateImage(dev, &img_create_info, NULL, &img);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create image");
	}

	printf("\t. created\n");

	VkMemoryRequirements req;
	vkGetImageMemoryRequirements(dev, img, &req);

	VkMemoryAllocateInfo alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = req.size,
		.memoryTypeIndex = ak_mem_type_idx(
			mem_info,
			req.memoryTypeBits,
			// All images are currently host-inaccessible
			AK_MEM_PROP(DEVICE_LOCAL)
		),
		.pNext = NULL,
	};

	VkDeviceMemory mem;
	err = vkAllocateMemory(dev, &alloc_info, NULL, &mem);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate memory for image");
	}

	vkBindImageMemory(dev, img, mem, 0);
	if (err != VK_SUCCESS) {
		panic_msg("unable to bind image memory");
	}

	printf("\t. allocated\n");

	VkImageView view;
	VkImageViewCreateInfo view_create_info = {
	STYPE(IMAGE_VIEW_CREATE_INFO)
		.flags = 0,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = aspect,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.pNext = NULL,
	};

	view_create_info.image = img;
	err = vkCreateImageView(dev, &view_create_info, NULL, &view);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create image view");
	}

	out->img = img;
	out->mem = mem;
	out->req = req;
	out->view = view;
}

static void ak_img_free(VkDevice dev, struct ak_img ak)
{
	vkDestroyImage(dev, ak.img, NULL);
	vkDestroyImageView(dev, ak.view, NULL);
	vkFreeMemory(dev, ak.mem, NULL);
}

struct ak_buf {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDeviceSize size;
	VkMemoryRequirements alloc_info;
};

#define AK_BUF_HEAD(HANDLE, SZ) \
	printf("Making " HANDLE " buffer with size %zu\n", (size_t)SZ)
#define AK_BUF_USAGE(STR) VK_BUFFER_USAGE_ ## STR ## _BIT

#define AK_BUF_MK(DEV, MEM, HANDLE, SZ, USAGE, PROPS, OUT) \
{ \
	AK_BUF_HEAD(HANDLE, SZ); \
	ak_buf_mk(DEV, MEM, SZ, AK_BUF_USAGE(USAGE), PROPS, OUT); \
}

static void ak_buf_mk(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties mem_info,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags props_mem,
	struct ak_buf *out
) {
	VkResult err;
	VkBufferCreateInfo buf_create_info = {
	STYPE(BUFFER_CREATE_INFO)
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.pNext = NULL,
	};

	VkBuffer buf;
	err = vkCreateBuffer(dev, &buf_create_info, NULL, &buf);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create buffer");
	}

	printf("\t. created\n");

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buf, &req);

	if (size != req.size) {
		assert(req.size > size);
		printf("\t| aligned up to %" PRIu64 "\n", req.size);
	}

	VkMemoryAllocateInfo alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = req.size,
		.memoryTypeIndex = ak_mem_type_idx(
			mem_info,
			req.memoryTypeBits,
			props_mem
		),
		.pNext = NULL,
	};

	VkDeviceMemory mem;
	err = vkAllocateMemory(dev, &alloc_info, NULL, &mem);
	if (err != VK_SUCCESS) {
		panic_msg("unable to allocate memory for buffer");
	}

	err = vkBindBufferMemory(dev, buf, mem, 0);
	if (err != VK_SUCCESS) {
		panic_msg("unable to bind buffer memory");
	}

	printf("\t. allocated\n");

	out->buf = buf;
	out->mem = mem;
	out->size = size;
	out->alloc_info = req;
}

#define AK_BUF_MK_AND_MAP(DEV, MEM, HANDLE, SZ, USAGE, OUT, SRC) \
{ \
	AK_BUF_HEAD(HANDLE, SZ); \
	ak_buf_mk_and_map(DEV, MEM, SZ, AK_BUF_USAGE(USAGE), OUT, SRC); \
}

static void ak_buf_mk_and_map(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties props_mem,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	struct ak_buf *out,
	void **src
) {
	ak_buf_mk(
		dev,
		props_mem,
		size,
		usage,
		// TODO: support uncached and/or non host-coherent heaps
		AK_MEM_PROP(HOST_VISIBLE) | AK_MEM_PROP(HOST_COHERENT),
		out
	);

	VkResult err;
	err = vkMapMemory(dev, out->mem, 0, VK_WHOLE_SIZE, 0, src);
	if (err != VK_SUCCESS) {
		panic_msg("unable to map device memory to host");
	}

	printf("\t. backed\n");
}

static void ak_buf_free(VkDevice dev, struct ak_buf ak)
{
	vkDestroyBuffer(dev, ak.buf, NULL);
	vkUnmapMemory(dev, ak.mem); // TODO: check if mapped
	vkFreeMemory(dev, ak.mem, NULL);
}

#define AK_MK_SET_LAYOUT(DEV, HANDLE, BINDINGS, COUNT, OUT) \
{ \
	printf( \
		"Making " HANDLE " descriptor set " \
		"with %u binding(s)\n", \
		COUNT \
	); \
	ak_mk_set_layout(DEV, BINDINGS, COUNT, OUT); \
}

static void ak_mk_set_layout(
	VkDevice dev,
	VkDescriptorSetLayoutBinding *bindings,
	size_t count,
	VkDescriptorSetLayout *out
) {
	VkResult err;

	VkDescriptorSetLayoutCreateInfo desc_create_info = {
	STYPE(DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
		.flags = 0,
		.bindingCount = count,
		.pBindings = bindings,
		.pNext = NULL,
	};

	VkDescriptorSetLayout desc_layout;
	err = vkCreateDescriptorSetLayout(
		dev,
		&desc_create_info,
		NULL,
		&desc_layout
	);

	if (err != VK_SUCCESS) {
		panic_msg("unable to create descriptor set layout");
	}

	printf("\t. done\n");
	*out = desc_layout;
}

// Note: memory owned by caller
static u32 *ak_read_shader(const char *filename, size_t *out_size)
{
	char *buf;
	long size;

	errno = 0;
	FILE *file = fopen(filename, "rb");
	if (errno) {
		fprintf(stderr, "Error opening file at path \"%s\"\n", filename);
		panic();
	}

	if (fseek(file, 0, SEEK_END)) {
		perror("Error seeking file");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	size = ftell(file);
	if (errno) {
		perror("Error acquiring length of file");
		exit(EXIT_FAILURE);
	}

	if (fseek(file, 0, SEEK_SET)) {
		perror("Error seeking file");
		exit(EXIT_FAILURE);
	}

	buf = malloc(size);
	assert(buf);

	clearerr(file);
	assert(size == fread(buf, 1, size, file));
	if (ferror(file)) {
		fprintf(
			stderr,
			"Error reading file \"%s\"\n",
			filename
		);
		exit(EXIT_FAILURE);
	}

	fclose(file);

	u32 *out = (u32*)buf;
	assert(out[0] == 0x07230203);
	assert(!(size % 4));

	printf("Read %lu words from \"%s\"\n", size / 4, filename);
	*out_size = size;
	return out;
}

struct ak_shader {
	VkShaderModule mod;
	u32 *words;
	size_t size;
	size_t len;
};

static struct ak_shader ak_shader_mk(VkDevice dev, const char *filename)
{
	VkResult err;
	size_t size;
	u32 *spv = ak_read_shader(filename, &size);

	VkShaderModuleCreateInfo mod_create_info = {
	STYPE(SHADER_MODULE_CREATE_INFO)
		.flags = 0,
		.codeSize = size,
		.pCode = spv,
		.pNext = NULL,
	};

	VkShaderModule mod;
	err = vkCreateShaderModule(dev, &mod_create_info, NULL, &mod);
	if (err != VK_SUCCESS) {
		panic_msg("unable to create shader module\n");
	}

	return (struct ak_shader) {
		mod,
		.words = spv,
		size,
		.len = size / 4,
	};
}

static void ak_shader_free(VkDevice dev, struct ak_shader shader)
{
	vkDestroyShaderModule(dev, shader.mod, NULL);
	free(shader.words);
}

#endif
