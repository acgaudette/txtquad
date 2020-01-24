#ifndef VKEXT_H
#define VKEXT_H

#include <vulkan/vulkan.h>
#include "sys.h"

#define STYPE(NAME) .sType = VK_STRUCTURE_TYPE_ ## NAME ,

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

struct AkBuffer {
	VkBuffer buf;
	VkDeviceMemory mem;
	VkMemoryRequirements req;
	VkDeviceSize size;
};

#define BUF_HEAD(HANDLE, SZ) \
	printf("Making " HANDLE " buffer with size %lu\n", (size_t)SZ)
#define BUF_USAGE(STR) VK_BUFFER_USAGE_ ## STR ## _BIT

#define MK_BUF(DEV, HANDLE, SZ, USAGE, OUT) \
{ \
	BUF_HEAD(HANDLE, SZ); \
	mk_buf(DEV, SZ, BUF_USAGE(USAGE), OUT); \
}

static void mk_buf(
	VkDevice dev,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	struct AkBuffer *out
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

	/* TODO: validate hardware memory and select best heap */

	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(dev, buf, &req);
	assert(req.size == size);

	VkMemoryAllocateInfo alloc_info = {
	STYPE(MEMORY_ALLOCATE_INFO)
		.allocationSize = size,
		.memoryTypeIndex = 0,
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
	out->size = size;
}

#define MK_BUF_AND_MAP(DEV, HANDLE, SZ, USAGE, OUT, SRC) \
{ \
	BUF_HEAD(HANDLE, SZ); \
	mk_buf_and_map(DEV, SZ, BUF_USAGE(USAGE), OUT, SRC); \
}

static void mk_buf_and_map(
	VkDevice dev,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	struct AkBuffer *out,
	void **src
) {
	mk_buf(dev, size, usage, out);

	VkResult err;
	err = vkMapMemory(dev, out->mem, 0, VK_WHOLE_SIZE, 0, src);
	if (err != VK_SUCCESS) {
		panic_msg("unable to map device memory to host");
	}

	printf("\t. backed\n");
}

static void free_buf(VkDevice dev, struct AkBuffer ak)
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
