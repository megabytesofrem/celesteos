#include "tar.h"

#include <kernel/memory/kmalloc.h>
#include <string.h>

static size_t octal_to_uint(const char *str, size_t size) {
	size_t value = 0;
	for (size_t i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
		value = (value << 3) + (str[i] - '0');
	}
	return value;
}

kern_return_t tar_read_archive(const tar_archive_t *archive, tar_file_t *out_files[TAR_MAX_FILES],
                               size_t *out_count) {
	if (!archive || !out_files || !out_count) {
		return KERN_INVALID_ARGUMENT;
	}

	uint8_t *file_data = (uint8_t *)archive->base;
	uint8_t *archive_end = file_data + archive->size;

	*out_count = 0;

	while (file_data + TAR_BLOCK_SIZE <= archive_end) {
		tar_header_t *header = (tar_header_t *)file_data;
		if (header->name[0] == '\0') {
			break; // End of archive
		}

		// Parse the file size from the header
		size_t file_size = octal_to_uint(header->size, sizeof(header->size));
		void *file_payload = (void *)(file_data + TAR_BLOCK_SIZE);

		// Add the file to the out_files array if there is space
		if (*out_count < TAR_MAX_FILES) {
			out_files[*out_count] = (tar_file_t *)kmalloc(sizeof(tar_file_t));
			if (out_files[*out_count]) {
				out_files[*out_count]->base = file_payload;
				out_files[*out_count]->size = file_size;
				(*out_count)++;
			}
		}

		// Move to the next file header in the archive
		size_t blocks = (file_size + (TAR_BLOCK_SIZE - 1)) / TAR_BLOCK_SIZE;
		file_data += TAR_BLOCK_SIZE + (blocks * TAR_BLOCK_SIZE);
	}

	return KERN_SUCCESS;
}

kern_return_t tar_get_file_names(const tar_archive_t *archive,
                                 char out_names[TAR_MAX_FILES][TAR_NAME_SIZE], size_t *out_count) {
	if (!archive || !out_names || !out_count) {
		return KERN_INVALID_ARGUMENT;
	}

	uint8_t *file_data = (uint8_t *)archive->base;
	uint8_t *archive_end = file_data + archive->size;

	*out_count = 0;

	while (file_data + TAR_BLOCK_SIZE <= archive_end) {
		tar_header_t *header = (tar_header_t *)file_data;
		if (header->name[0] == '\0') {
			break; // End of archive
		}

		// Record the file name if there is space in the out_names array
		if (*out_count < TAR_MAX_FILES) {
			memcpy(out_names[*out_count], header->name, sizeof(header->name));
			out_names[*out_count][TAR_NAME_SIZE - 1] = '\0';

			// Increment the count of recorded file names
			(*out_count)++;
		}

		size_t file_size = octal_to_uint(header->size, sizeof(header->size));
		size_t blocks = (file_size + (TAR_BLOCK_SIZE - 1)) / TAR_BLOCK_SIZE;
		file_data += TAR_BLOCK_SIZE + (blocks * TAR_BLOCK_SIZE);
	}

	return KERN_SUCCESS;
}

kern_return_t tar_read_file(const tar_archive_t *archive, const char *filename,
                            tar_file_t *out_file) {
	if (!archive || !filename || !out_file) {
		return KERN_INVALID_ARGUMENT;
	}

	return tar_lookup_file(archive, filename, out_file);
}

kern_return_t tar_lookup_file(const tar_archive_t *archive, const char *filename,
                              tar_file_t *out_file) {
	if (!archive || !filename || !out_file) {
		return KERN_INVALID_ARGUMENT;
	}

	const char *rel_path = (filename[0] == '/') ? filename + 1 : filename;

	uint8_t *file_data = (uint8_t *)archive->base;
	uint8_t *archive_end = file_data + archive->size;

	while (file_data + TAR_BLOCK_SIZE <= archive_end) {
		tar_header_t *header = (tar_header_t *)file_data;
		if (header->name[0] == '\0') {
			break; // End of archive
		}

		// Parse the file size from the header
		size_t file_size = octal_to_uint(header->size, sizeof(header->size));
		void *file_payload = (void *)(file_data + TAR_BLOCK_SIZE);

		// tar archives commonly store paths with a leading "./".
		const char *header_path = header->name;
		while (header_path[0] == '.' && header_path[1] == '/') {
			header_path += 2;
		}

		// Check if the current header matches the requested filename
		if (strncmp(header_path, rel_path, sizeof(header->name)) == 0) {
			out_file->base = file_data + TAR_BLOCK_SIZE;
			out_file->size = file_size;

			return KERN_SUCCESS;
		}

		// Move to the next file header in the archive
		size_t blocks = (file_size + (TAR_BLOCK_SIZE - 1)) / TAR_BLOCK_SIZE;

		// Advance the pointer by the size of the current file's data, rounded up to the
		// nearest block, plus the header size
		file_data += TAR_BLOCK_SIZE + (blocks * TAR_BLOCK_SIZE);
	}

	return KERN_NOT_FOUND;
}