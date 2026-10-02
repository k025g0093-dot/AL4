#include "stageManager.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

void stageManager::LoadStageDataCsv() {
	const std::string& filePath = "Resources/stageData.csv";

	std::ifstream file(filePath); // コンストラクタで直接開く
	assert(file.is_open() && "ステージデータCSVが開けませんでした");

	// stringstreamに一気に流し込まず、ファイルから直接行単位で読み込む
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty())
			continue; // 空行があればスキップ

		std::stringstream lineStream(line);
		StageData data;

		// 名前
		std::getline(lineStream, data.name, ',');

		// 制限時間
		std::string timeStr;
		std::getline(lineStream, timeStr, ',');
		if (!timeStr.empty()) {
			data.timeLimit = std::stoi(timeStr);
		}

		stageData_.push_back(data);
	}
}


void stageManager::SetCurrentStageIndexByName(
	const std::string& name
) {

	for (size_t i = 0; i < stageData_.size(); i++) {
		if (stageData_[i].name == name) {
			currentStageIndex_ = static_cast<int32_t>(i);
			return;
		}
	}
	assert(false && "指定されたステージ名はないよー");
}
