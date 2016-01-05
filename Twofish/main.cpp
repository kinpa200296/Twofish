#include "main.h"

FILE *report_file, *log_file;
long double total_duration;

size_t validate_key_size(size_t key_size)
{
	if (key_size <= to_bytes(KEY_SIZE_SMALL))
	{
		return KEY_SIZE_SMALL;
	}
	if (key_size <= to_bytes(KEY_SIZE_MEDIUM))
	{
		return KEY_SIZE_MEDIUM;
	}
	if (key_size <= to_bytes(KEY_SIZE_LARGE))
	{
		return KEY_SIZE_LARGE;
	}
	return KEY_SIZE_INVALID;
}

size_t get_file_size(FILE* file)
{
	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return file_size;
}

byte* load_key(const char *filename, size_t &key_size)
{
	FILE* file;
	fopen_s(&file, filename, "r");

	size_t file_size = get_file_size(file);
	key_size = validate_key_size(file_size);

	byte *buffer;

	if (key_size != KEY_SIZE_INVALID)
	{
		size_t buffer_size = key_size + 1;
		buffer = (byte*)malloc(buffer_size);
		memset(buffer, 0, buffer_size);

		fread(buffer, 1, buffer_size, file);

		fprintf(log_file, "Loaded key - %d bytes.\n", file_size);
		if (to_bytes(key_size) != file_size)
		{
			fprintf(log_file, "Expanding key to %d bytes.\n", to_bytes(key_size));
		}
	}
	else
	{
		buffer = (byte*)malloc(4);

		printf("Not supported key - %d bytes.\n", file_size);
		fprintf(log_file, "Not supported key - %d bytes.\n", file_size);
	}

	fclose(file);
	
	return buffer;
}

int process(byte* buffer, size_t buffer_size, int action, byte* key, size_t key_size, FILE* input, FILE* output)
{
	if (key_size == KEY_SIZE_INVALID)
	{
		fprintf(log_file, "Invalid key size. Aborting...\n");
		printf("Invalid key size. Aborting...\n");
		return 1;
	}
	fprintf(log_file, "Valid key size - %d bits\n", key_size);

	size_t data_size = get_file_size(input);

	size_t iter_count = data_size / buffer_size + (data_size % buffer_size != 0);
	for (size_t i = 0; i < iter_count; i++)
	{
		memset(buffer, 0, buffer_size);
		size_t bytes_read = fread(buffer, 1, buffer_size, input);
		fprintf(log_file, "iteration %d: read %d bytes\n", i, bytes_read);
		high_resolution_clock::time_point start = high_resolution_clock::now();
		switch (action)
		{
		case ACTION_ENCRYPT:
			encrypt(buffer, MAX_BUFFER, key, key_size);
			break;
		case ACTION_DECRYPT:
			decrypt(buffer, MAX_BUFFER, key, key_size);
			break;
		}
		high_resolution_clock::time_point end = high_resolution_clock::now();
		long double iteration_duration = duration_cast<nanoseconds>(end - start).count() / 1e9;
		total_duration += iteration_duration;
		fprintf(report_file, "%d,%d,%d,%lld,%.9Lf\n", i, bytes_read, buffer_size, duration_cast<nanoseconds>(end - start).count(), iteration_duration);
		fprintf(log_file, "iteration %d: processed %d bytes in %.9Lf s\n", i, bytes_read, iteration_duration);
		printf("iteration %d: processed %d bytes in %.9Lf s\n", i, bytes_read, iteration_duration);
		fwrite(buffer, 1, bytes_read, output);
	}

	return 0;
}

int start_process(const char* action, const char* key_filename, const char* input_filename,
	const char* output_filename, const char* report_filename, const char* log_filename)
{
	FILE *input, *output;

	fopen_s(&input, input_filename, "r");
	fopen_s(&output, output_filename, "w");
	fopen_s(&report_file, report_filename, "w");
	fopen_s(&log_file, log_filename, "w");

	fprintf(report_file, "Executing action \"%s\" on file %s with key in file %s.\nWriting result to file %s.\nWriting log to file %s.\n",
		action, input_filename, key_filename, output_filename, log_filename);
	fprintf(report_file, "iteration,data size(bytes),buffer size(bytes),time(ns),time(s)\n");
	fprintf(log_file, "Executing action \"%s\" on file %s with key in file %s.\nWriting result to file %s.\nWriting report to file %s.\n",
		action, input_filename, key_filename, output_filename, report_filename);

	size_t buffer_size = MAX_BUFFER*to_bytes(BLOCK_SIZE);
	byte *buffer = (byte*)malloc(buffer_size);

	size_t key_size;
	byte *key = load_key(key_filename, key_size);

	int res;
	switch (strcmp(action, "encrypt"))
	{
	case 0:
		res = process(buffer, buffer_size, ACTION_ENCRYPT, key, key_size, input, output);
		break;
	default:
		res = process(buffer, buffer_size, ACTION_DECRYPT, key, key_size, input, output);
		break;
	}

	if (!res)
	{
		fprintf(log_file, "total duration: %.9Lf s\n", total_duration);
		printf("total duration: %.9Lf s\n", total_duration);
	}
	else
	{
		fprintf(log_file, "Process aborted.\n");
		printf("Process aborted.\n");
	}

	free(buffer);
	free(key);

	fclose(input);
	fclose(output);
	fclose(report_file);
	fclose(log_file);

	return res;
}

int main(int argc, char **argv)
{
	char *action = "encrypt", *key_filename = "default.key", *input_filename = "input.txt",
		*output_filename = "output.txt", *report_filename = "report.csv", *log_filename = "recent.log";

	switch (argc)
	{
	case 7:
		log_filename = argv[6];
	case 6:
		report_filename = argv[5];
	case 5:
		output_filename = argv[4];
	case 4:
		input_filename = argv[3];
	case 3:
		key_filename = argv[2];
	case 2:
		if (strcmp(argv[1], "encrypt") && strcmp(argv[1], "decrypt"))
		{
			printf("%s is not a valid action. Valid are \"encrypt\" or \"decrypt\".\n", argv[1]);
			return -1;
		}
		action = argv[1];
		break;
	}

	int res = start_process(action, key_filename, input_filename, output_filename, report_filename, log_filename);

	if (res)
	{
		printf("Failure...\n");
	}
	else
	{
		printf("Success...\n");
	}

	return 0;
}
