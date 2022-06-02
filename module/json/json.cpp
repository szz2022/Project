#include "json.h"

using namespace rapidjson;

bool json::read(const char* file_name, Document& dom_tree) {
	try {
		FILE* json_file = fopen(file_name, "rb");

		if (json_file == nullptr)
            throw std::runtime_error("Json::Open file error");

		char readBuffer[65536];
		FileReadStream is(json_file, readBuffer, sizeof(readBuffer));

		dom_tree.ParseStream(is);

		fclose(json_file);
		return true;
	}
	catch (const std::exception& error) {
		printf("%s\n", error.what());
		return false;
	}
}

bool json::write(const char* file_name, Document& json) {
	try {
		FILE* json_file = fopen(file_name, "wb");

		if (json_file == nullptr)
			throw std::runtime_error("Json::Open file error");

		char writeBuffer[65536];
		FileWriteStream os(json_file, writeBuffer, sizeof(writeBuffer));

		Writer<FileWriteStream> writer(os);
		json.Accept(writer);

		fclose(json_file);
		return true;
	}
	catch (const std::exception& error) {
		printf("%s\n", error.what());
		return false;
	}
}
