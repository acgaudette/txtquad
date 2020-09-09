#ifndef VKEXT_H
#define VKEXT_H

#include <vulkan/vulkan.h>
#include "sys.h"
#include "types.h"

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

static void ak_print_mem_props(VkMemoryPropertyFlags prop_mask, const char *format)
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
	VkPhysicalDeviceMemoryProperties mem_props,
	u32 type_mask,
	VkMemoryPropertyFlags prop_mask
) {
	printf("\t| ");
	ak_print_mem_props(prop_mask, "%s ");
	printf("\n");

	for (u32 i = 0; i < mem_props.memoryTypeCount; ++i) {
		int compat = type_mask & (1 << i);
		if (!compat) continue;

		VkMemoryType t = mem_props.memoryTypes[i];
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
#define AK_IMG_MK(DEV, MEM, HANDLE, W, H, FORMAT, USAGE, ASPECT, OUT) \
{ \
	AK_IMG_HEAD(HANDLE); \
	ak_img_mk( \
		DEV, \
		MEM, \
		W, H, \
		VK_FORMAT_ ## FORMAT, \
		USAGE, \
		VK_IMAGE_ASPECT_ ## ASPECT ## _BIT, \
		OUT \
	); \
}

static void ak_img_mk(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties mem_info,
	u32 width, u32 height,
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
		.samples = VK_SAMPLE_COUNT_1_BIT,
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
	VkMemoryRequirements req;
};

#define AK_BUF_HEAD(HANDLE, SZ) \
	printf("Making " HANDLE " buffer with size %lu\n", (size_t)SZ)
#define AK_BUF_USAGE(STR) VK_BUFFER_USAGE_ ## STR ## _BIT

#define AK_BUF_MK(DEV, MEM, HANDLE, SZ, ALIGN, USAGE, PROPS, OUT) \
{ \
	AK_BUF_HEAD(HANDLE, SZ); \
	ak_buf_mk(DEV, MEM, SZ, ALIGN, AK_BUF_USAGE(USAGE), PROPS, OUT); \
}

static void ak_buf_mk(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties mem_info,
	VkDeviceSize size,
	VkDeviceSize align, // Pass zero to auto-align
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags mem_props,
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

	if (align > 0) {
		assert(req.size == size);
		assert(req.alignment == align);
	} else if (size != req.size) {
		assert(req.size > size);
		size = req.size;
		printf("\t| resized to %lu\n", size);
	}

	printf("\t| alignment %lu\n", req.alignment);

	VkMemoryAllocateInfo alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = size,
		.memoryTypeIndex = ak_mem_type_idx(
			mem_info,
			req.memoryTypeBits,
			mem_props
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
	out->req = req;
}

#define AK_BUF_MK_AND_MAP(DEV, MEM, HANDLE, SZ, ALIGN, USAGE, OUT, SRC) \
{ \
	AK_BUF_HEAD(HANDLE, SZ); \
	ak_buf_mk_and_map(DEV, MEM, SZ, ALIGN, AK_BUF_USAGE(USAGE), OUT, SRC); \
}

static void ak_buf_mk_and_map(
	VkDevice dev,
	VkPhysicalDeviceMemoryProperties mem_props,
	VkDeviceSize size,
	VkDeviceSize align,
	VkBufferUsageFlags usage,
	struct ak_buf *out,
	void **src
) {
	ak_buf_mk(
		dev,
		mem_props,
		size,
		align,
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

#define MK_SET_LAYOUT(DEV, HANDLE, BINDINGS, COUNT, OUT) \
{ \
	printf( \
		"Making " HANDLE " descriptor set " \
		"with %lu binding(s)\n", \
		(size_t)COUNT \
	); \
	mk_set_layout(DEV, BINDINGS, COUNT, OUT); \
}

static void mk_set_layout(
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

#endif
