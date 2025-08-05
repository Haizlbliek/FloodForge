#include "Backup.hpp"

#include <vector>
#include <algorithm>
#include "../Constants.hpp"
#include "../Utils.hpp"

void Backup::backup(std::filesystem::path file) {
	if (!std::filesystem::exists(file))
		return;
	
	// Identify file
	std::filesystem::path backupDir = BASE_PATH / "backups";
	std::filesystem::create_directories(backupDir);

	std::string stem = file.stem().generic_u8string();
	std::string ext = file.extension().generic_u8string();


	// Delete old backups
	std::vector<std::filesystem::directory_entry> matchingFiles;
	for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
		if (!entry.is_regular_file())
			continue;

		std::string name = entry.path().filename().generic_u8string();
		if (startsWith(name, stem + "-") && endsWith(name, ext))
			matchingFiles.push_back(entry);
	}
	std::sort(matchingFiles.begin(), matchingFiles.end(), [](const auto& a, const auto& b) {
		return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
	});
	while (matchingFiles.size() >= 5) {
		std::filesystem::remove(matchingFiles.front().path());
		matchingFiles.erase(matchingFiles.begin());
	}
	


	// Create backup
	auto now = std::chrono::system_clock::now();
	std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
	std::ostringstream timeStream;
	timeStream << std::put_time(std::localtime(&nowTime), "-%Y%m%d-%H%M%S");

	std::string fileName = stem + timeStream.str() + ext;
	std::filesystem::path newFile = backupDir / fileName;

	std::filesystem::copy_file(file, newFile, std::filesystem::copy_options::overwrite_existing);
}